#include <array>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "poll_sim/estimate.hpp"
#include "poll_sim/experiment.hpp"
#include "poll_sim/params.hpp"
#include "poll_sim/poll.hpp"
#include "poll_sim/population.hpp"
#include "poll_sim/sampling.hpp"
#include "poll_sim/stats.hpp"

namespace {

using namespace poll_sim;

// Prints the actual vs. expected share of each category in one dimension.
template <class Enum, std::size_t N, class Getter>
void print_share(std::string_view title, const std::vector<Voter>& population,
                 const Arr<double, N>& expected, Getter get) {
    std::array<std::size_t, N> counts{};
    for (const Voter& voter : population) {
        ++counts[static_cast<std::size_t>(get(voter))];
    }

    std::cout << title << '\n';
    for (std::size_t i = 0; i < N; ++i) {
        const double actual = static_cast<double>(counts[i]) / population.size();
        std::cout << "  " << std::left << std::setw(10) << to_string(static_cast<Enum>(i))
                  << std::right << std::fixed << std::setprecision(4) << actual
                  << "  (expected " << expected[i] << ")\n";
    }
}

// Theoretical population-wide vote share: per-age vote shares weighted by age share.
Arr<double, kCandidateCount> expected_vote_share(const PopulationParams& params) {
    Arr<double, kCandidateCount> result{};
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        for (std::size_t c = 0; c < kCandidateCount; ++c) {
            result[c] += params.age_share[a] * params.vote_share[a][c];
        }
    }
    return result;
}

// Prints landline ownership and the mobile phone count distribution against their
// theoretical values, followed by the size of each sampling frame (counted from the population,
// without building the frames).
void print_coverage(const std::vector<Voter>& population, const PopulationParams& params) {
    double expected_landline = 0.0;
    Arr<double, kMaxPhones + 1> expected_phones{};
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        for (std::size_t c = 0; c < kCandidateCount; ++c) {
            const double cell = params.age_share[a] * params.vote_share[a][c];
            expected_landline += cell * params.landline_share[a][c];
            for (std::size_t k = 0; k <= kMaxPhones; ++k) {
                expected_phones[k] += cell * params.phone_share[a][c][k];
            }
        }
    }

    std::size_t landline_count = 0;
    std::size_t mobile_numbers = 0;
    Arr<std::size_t, kMaxPhones + 1> phone_counts{};
    for (const Voter& voter : population) {
        if (voter.has_landline) {
            ++landline_count;
        }
        mobile_numbers += voter.phones;
        ++phone_counts[static_cast<std::size_t>(voter.phones)];
    }

    const auto size = static_cast<double>(population.size());
    std::cout << "Coverage\n";
    std::cout << "  " << std::left << std::setw(10) << "Landline" << std::right << std::fixed
              << std::setprecision(4) << landline_count / size << "  (expected "
              << expected_landline << ")\n";
    for (std::size_t k = 0; k <= kMaxPhones; ++k) {
        std::cout << "  " << std::left << std::setw(10)
                  << (std::to_string(k) + (k == kMaxPhones ? "+" : "") + " phones") << std::right
                  << phone_counts[k] / size << "  (expected " << expected_phones[k] << ")\n";
    }
    std::cout << "  Frames: landline " << landline_count << " numbers, mobile " << mobile_numbers
              << " numbers\n";
}

enum class Estimator {
    Unweighted,
    AgeWeighted,
    MultiplicityWeighted,
    MultiplicityAgeWeighted,
};

// A sampling method combined with an estimator.
struct Approach {
    std::string_view name;
    SamplingMethod sampling;
    Estimator estimator;
};

inline constexpr Arr<Approach, 8> kApproaches{{
    {"SimpleRandom", SamplingMethod::SimpleRandom, Estimator::Unweighted},
    {"Landline", SamplingMethod::Landline, Estimator::Unweighted},
    {"Landline+AgeWt", SamplingMethod::Landline, Estimator::AgeWeighted},
    {"LandlineMNAR", SamplingMethod::LandlineMNAR, Estimator::Unweighted},
    {"LandlineMNAR+AgeWt", SamplingMethod::LandlineMNAR, Estimator::AgeWeighted},
    {"Mobile", SamplingMethod::Mobile, Estimator::Unweighted},
    {"Mobile+MultWt", SamplingMethod::Mobile, Estimator::MultiplicityWeighted},
    {"Mobile+MultWt+AgeWt", SamplingMethod::Mobile, Estimator::MultiplicityAgeWeighted},
}};

inline constexpr Arr<SamplingMethod, 4> kSamplingMethods{
    SamplingMethod::SimpleRandom, SamplingMethod::Landline, SamplingMethod::LandlineMNAR,
    SamplingMethod::Mobile};

Estimate estimate_with(Estimator estimator, const PollResult& result, Candidate candidate,
                       const Arr<double, kAgeGroupCount>& age_share) {
    switch (estimator) {
        case Estimator::Unweighted:
            return unweighted_estimate(result, candidate);
        case Estimator::AgeWeighted:
            return age_weighted_estimate(result, candidate, age_share);
        case Estimator::MultiplicityWeighted:
            return multiplicity_weighted_estimate(result, candidate);
        case Estimator::MultiplicityAgeWeighted:
            return multiplicity_age_weighted_estimate(result, candidate, age_share);
    }
    return {};
}

// Landline poll diagnostics at one sample size, averaged over replications.
struct LandlineDiagnostics {
    double response_rate = 0.0;
    Arr<double, kAgeGroupCount> respondent_age_share{};
    // Vote share within each age group's respondents [age][candidate],
    // pooled over replications (0 if an age group never responded).
    Arr<Arr<double, kCandidateCount>, kAgeGroupCount> respondent_vote_share{};
    double young_respondents = 0.0;
    double effective_size = 0.0;     // Kish effective sample size after age weighting
    double empty_age_fraction = 0.0; // fraction of polls with an age group lacking respondents
};

// Mobile poll diagnostics at one sample size, averaged over replications.
struct MobileDiagnostics {
    double response_rate = 0.0;
    Arr<double, kAgeGroupCount> respondent_age_share{};
    double multiplicity_effective_size = 0.0;  // Kish n_eff with multiplicity weights
    double combined_effective_size = 0.0;      // Kish n_eff with multiplicity and age weights
    double duplicates = 0.0;                   // repeat responses per poll
};

// Results of every approach at one sample size.
struct SampleSizeResult {
    std::size_t sample_size = 0;
    // summaries[approach][candidate]
    Arr<Arr<Summary, kCandidateCount>, kApproaches.size()> summaries{};
    LandlineDiagnostics landline;
    LandlineDiagnostics landline_mnar;
    MobileDiagnostics mobile;
};

LandlineDiagnostics diagnose_landline(const std::vector<PollResult>& results,
                                      const Arr<double, kAgeGroupCount>& age_share) {
    LandlineDiagnostics d;
    std::size_t empty = 0;
    Arr<Arr<std::size_t, kCandidateCount>, kAgeGroupCount> cell_totals{};
    Arr<std::size_t, kAgeGroupCount> age_totals{};
    for (const PollResult& result : results) {
        d.response_rate += result.response_rate();
        for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
            d.respondent_age_share[a] += result.share(static_cast<AgeGroup>(a));
            age_totals[a] += result.age_counts[a];
            for (std::size_t c = 0; c < kCandidateCount; ++c) {
                cell_totals[a][c] += result.cell_counts[a][c];
            }
        }
        d.young_respondents +=
            static_cast<double>(result.age_counts[static_cast<std::size_t>(AgeGroup::Young)]);
        d.effective_size += age_weighted_effective_size(result, age_share);
        for (std::size_t count : result.age_counts) {
            if (count == 0) {
                ++empty;
                break;
            }
        }
    }

    const double replications = static_cast<double>(results.size());
    d.response_rate /= replications;
    for (double& share : d.respondent_age_share) {
        share /= replications;
    }
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        for (std::size_t c = 0; c < kCandidateCount; ++c) {
            if (age_totals[a] > 0) {
                d.respondent_vote_share[a][c] =
                    static_cast<double>(cell_totals[a][c]) / age_totals[a];
            }
        }
    }
    d.young_respondents /= replications;
    d.effective_size /= replications;
    d.empty_age_fraction = empty / replications;
    return d;
}

MobileDiagnostics diagnose_mobile(const std::vector<PollResult>& results,
                                  const Arr<double, kAgeGroupCount>& age_share) {
    MobileDiagnostics d;
    for (const PollResult& result : results) {
        d.response_rate += result.response_rate();
        for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
            d.respondent_age_share[a] += result.share(static_cast<AgeGroup>(a));
        }
        d.multiplicity_effective_size += multiplicity_weighted_effective_size(result);
        d.combined_effective_size += multiplicity_age_weighted_effective_size(result, age_share);
        d.duplicates += static_cast<double>(result.duplicates);
    }

    const double replications = static_cast<double>(results.size());
    d.response_rate /= replications;
    for (double& share : d.respondent_age_share) {
        share /= replications;
    }
    d.multiplicity_effective_size /= replications;
    d.combined_effective_size /= replications;
    d.duplicates /= replications;
    return d;
}

// Prints the response rate, respondent age mix, and age weights of one landline scenario,
// followed by how age weighting shrinks the effective sample size at each n.
void print_landline_diagnostics(std::string_view name, const std::vector<SampleSizeResult>& rows,
                                LandlineDiagnostics SampleSizeResult::*scenario,
                                const Arr<double, kAgeGroupCount>& age_share) {
    const LandlineDiagnostics& last = rows.back().*scenario;
    std::cout << name << " respondents (n = " << rows.back().sample_size
              << ", mean over replications)\n";
    std::cout << "  Response rate " << std::fixed << std::setprecision(2)
              << last.response_rate * 100.0 << "%\n";
    std::cout << "  " << std::left << std::setw(10) << "Age" << std::right << std::setw(12)
              << "Population" << std::setw(13) << "Respondents" << std::setw(9) << "Weight"
              << '\n';
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        std::cout << "  " << std::left << std::setw(10) << to_string(static_cast<AgeGroup>(a))
                  << std::right << std::setw(12) << age_share[a] * 100.0 << std::setw(13)
                  << last.respondent_age_share[a] * 100.0 << std::setw(9)
                  << age_share[a] / last.respondent_age_share[a] << '\n';
    }
    std::cout << '\n';

    std::cout << name << " age weighting cost (mean over replications)\n";
    std::cout << std::setw(10) << "n" << std::setw(10) << "Young" << std::setw(10) << "Eff. n"
              << std::setw(8) << "Deff" << std::setw(10) << "EmptyAge" << '\n';
    for (const SampleSizeResult& row : rows) {
        const LandlineDiagnostics& d = row.*scenario;
        std::cout << std::setw(10) << row.sample_size << std::setw(10) << d.young_respondents
                  << std::setw(10) << d.effective_size << std::setw(8)
                  << row.sample_size / d.effective_size << std::setw(9)
                  << d.empty_age_fraction * 100.0 << "%\n";
    }
    std::cout << "  Young = Young respondents, Eff. n = Kish effective sample size,\n"
              << "  Deff = n / Eff. n, EmptyAge = polls with an age group lacking respondents\n\n";
}

// Prints the vote share within each age group for the population and for the respondents
// of each landline scenario. Age weighting only rescales age groups, so it cannot fix
// a gap between the population and the respondents within an age group.
void print_respondent_vote_share(
    const std::vector<SampleSizeResult>& rows,
    const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& age_vote_share) {
    const SampleSizeResult& last = rows.back();
    std::cout << "Vote share within each age group (n = " << last.sample_size
              << ", respondents pooled over replications)\n";
    std::cout << "  " << std::left << std::setw(10) << "Age" << std::setw(24) << "     Population"
              << std::setw(24) << "     Landline" << "     LandlineMNAR" << '\n';
    std::cout << "  " << std::setw(10) << "" << std::right;
    for (int group = 0; group < 3; ++group) {
        for (std::size_t c = 0; c < kCandidateCount; ++c) {
            std::cout << std::setw(8) << to_string(static_cast<Candidate>(c));
        }
    }
    std::cout << '\n';

    const auto print_group = [](const Arr<double, kCandidateCount>& shares) {
        for (double share : shares) {
            std::cout << std::setw(8) << share * 100.0;
        }
    };
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        std::cout << "  " << std::left << std::setw(10) << to_string(static_cast<AgeGroup>(a))
                  << std::right << std::fixed << std::setprecision(2);
        print_group(age_vote_share[a]);
        print_group(last.landline.respondent_vote_share[a]);
        print_group(last.landline_mnar.respondent_vote_share[a]);
        std::cout << '\n';
    }
    std::cout << '\n';
}

// Prints the mobile response rate and respondent age mix,
// followed by how multiplicity and age weighting shrink the effective sample size at each n.
void print_mobile_diagnostics(const std::vector<SampleSizeResult>& rows,
                              const Arr<double, kAgeGroupCount>& age_share) {
    const MobileDiagnostics& last = rows.back().mobile;
    std::cout << "Mobile respondents (n = " << rows.back().sample_size
              << ", mean over replications)\n";
    std::cout << "  Response rate " << std::fixed << std::setprecision(2)
              << last.response_rate * 100.0 << "%\n";
    std::cout << "  " << std::left << std::setw(10) << "Age" << std::right << std::setw(12)
              << "Population" << std::setw(13) << "Respondents" << '\n';
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        std::cout << "  " << std::left << std::setw(10) << to_string(static_cast<AgeGroup>(a))
                  << std::right << std::setw(12) << age_share[a] * 100.0 << std::setw(13)
                  << last.respondent_age_share[a] * 100.0 << '\n';
    }
    std::cout << '\n';

    std::cout << "Mobile weighting cost (mean over replications)\n";
    std::cout << std::setw(10) << "n" << std::setw(12) << "Mult Eff. n" << std::setw(8) << "Deff"
              << std::setw(12) << "+Age Eff. n" << std::setw(8) << "Deff" << std::setw(8)
              << "Dup" << '\n';
    for (const SampleSizeResult& row : rows) {
        const MobileDiagnostics& d = row.mobile;
        std::cout << std::setw(10) << row.sample_size << std::setw(12)
                  << d.multiplicity_effective_size << std::setw(8)
                  << row.sample_size / d.multiplicity_effective_size << std::setw(12)
                  << d.combined_effective_size << std::setw(8)
                  << row.sample_size / d.combined_effective_size << std::setw(8) << d.duplicates
                  << '\n';
    }
    std::cout << "  Mult = multiplicity weights (1 / phones), +Age = then post-stratified by age,\n"
              << "  Dup = repeat responses from the same voter per poll\n\n";
}

// Prints one table per candidate comparing the approaches across sample sizes.
void print_sample_size_tables(const std::vector<SampleSizeResult>& rows) {
    const auto pct = [](double v) { return v * 100.0; };

    for (std::size_t c = 0; c < kCandidateCount; ++c) {
        const auto candidate = static_cast<Candidate>(c);
        std::cout << "  " << to_string(candidate) << " (true " << std::fixed
                  << std::setprecision(2) << pct(rows.front().summaries[0][c].true_share)
                  << "%)\n";
        std::cout << std::right << std::setw(10) << "n" << "  " << std::left << std::setw(20)
                  << "Approach" << std::right << std::setw(8) << "Mean" << std::setw(8) << "Bias"
                  << std::setw(8) << "MoE" << std::setw(10) << "Coverage" << std::setw(9)
                  << "Within3" << '\n';

        for (const SampleSizeResult& row : rows) {
            for (std::size_t m = 0; m < kApproaches.size(); ++m) {
                const Summary& s = row.summaries[m][c];
                std::cout << std::right << std::setw(10);
                if (m == 0) {
                    std::cout << row.sample_size;
                } else {
                    std::cout << "";
                }
                std::cout << "  " << std::left << std::setw(20) << kApproaches[m].name
                          << std::right << std::setw(8) << pct(s.mean_estimate) << std::setw(8)
                          << pct(s.bias) << std::setw(8) << pct(s.margin_of_error)
                          << std::setw(10) << pct(s.coverage) << std::setw(9)
                          << pct(s.within_3pct) << '\n';
            }
        }
        std::cout << '\n';
    }
    std::cout << "  Mean = mean estimate across replications, Bias = Mean - True,\n"
              << "  MoE = 1.96 * empirical SD of estimates,\n"
              << "  Coverage = 95% CIs (estimate +/- 1.96 SE) containing True,\n"
              << "  Within3 = estimates within +/-3 points of True,\n"
              << "  Landline+AgeWt = same landline polls, post-stratified by age,\n"
              << "  LandlineMNAR = landline polls whose response rate also depends on the\n"
              << "    candidate (MNAR), LandlineMNAR+AgeWt = same polls, post-stratified by age,\n"
              << "  Mobile+MultWt = mobile polls weighted by 1 / phones,\n"
              << "  Mobile+MultWt+AgeWt = multiplicity weights, then post-stratified by age\n";
}

void print_usage(std::string_view program) {
    std::cout << "Usage: " << program
              << " [--order=random|sequential] [--response=per-call|fixed]\n"
              << "  --order     how polls draw voters or phone numbers; overrides\n"
              << "              SimParams::order (default " << to_string(SimParams{}.order)
              << ")\n"
              << "  --response  whether each call is answered by a fresh draw or by a choice\n"
              << "              fixed per number; overrides SimParams::response (default "
              << to_string(SimParams{}.response) << ")\n";
}

// Applies command-line overrides to sim. Returns false if help was requested.
// Throws std::invalid_argument on an unknown argument or value.
bool parse_args(int argc, char* argv[], SimParams& sim) {
    constexpr std::string_view kOrder = "--order=";
    constexpr std::string_view kResponse = "--response=";
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            return false;
        }
        if (arg.starts_with(kOrder)) {
            const std::string_view value = arg.substr(kOrder.size());
            if (value == "random") {
                sim.order = DrawOrder::Random;
            } else if (value == "sequential") {
                sim.order = DrawOrder::Sequential;
            } else {
                throw std::invalid_argument("unknown --order value: " + std::string(value));
            }
            continue;
        }
        if (arg.starts_with(kResponse)) {
            const std::string_view value = arg.substr(kResponse.size());
            if (value == "per-call") {
                sim.response = ResponseModel::PerCall;
            } else if (value == "fixed") {
                sim.response = ResponseModel::Fixed;
            } else {
                throw std::invalid_argument("unknown --response value: " + std::string(value));
            }
            continue;
        }
        throw std::invalid_argument("unknown argument: " + std::string(arg));
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    SimParams sim;
    try {
        if (!parse_args(argc, argv, sim)) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid arguments: " << e.what() << '\n';
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::cout << "poll-sim v0.1.0\n\n";

    const PopulationParams params;

    try {
        const auto start = std::chrono::steady_clock::now();
        const std::vector<Voter> population = generate_population(params);
        const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start);

        std::cout << "Generated " << population.size() << " voters in " << std::fixed
                  << std::setprecision(2) << elapsed.count() << " s ("
                  << population.size() * sizeof(Voter) / (1024.0 * 1024.0) << " MB)\n\n";

        print_share<AgeGroup>("Age", population, params.age_share,
                              [](const Voter& v) { return v.age; });
        print_share<Region>("Region", population, params.region_share,
                            [](const Voter& v) { return v.region; });
        print_share<Gender>("Gender", population, params.gender_share,
                            [](const Voter& v) { return v.gender; });
        print_share<Candidate>("Candidate", population, expected_vote_share(params),
                               [](const Voter& v) { return v.candidate; });
        print_coverage(population, params);
        std::cout << '\n';

        const Arr<double, kCandidateCount> true_shares = population_vote_share(population);
        const Arr<double, kAgeGroupCount> age_share = population_age_share(population);

        std::cout << "Experiment by sample size: replications = " << sim.replications
                  << ", seed = " << sim.seed << ", order = " << to_string(sim.order)
                  << ", response = " << to_string(sim.response) << '\n';

        std::vector<SampleSizeResult> rows;
        for (std::size_t n : sim.sample_sizes) {
            SampleSizeResult row;
            row.sample_size = n;
            rows.push_back(row);
        }

        // Methods outermost, so each frame is built (and shuffled) once, shared by every
        // sample size, and freed before the next method's frame is built.
        for (SamplingMethod method : kSamplingMethods) {
            const auto frame_start = std::chrono::steady_clock::now();
            const Frame frame = build_frame(population, params, method, sim);
            const auto frame_elapsed =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - frame_start);
            std::cout << "  " << to_string(method) << " frame built in " << std::fixed
                      << std::setprecision(2) << frame_elapsed.count() << " s\n";

            for (SampleSizeResult& row : rows) {
                const std::size_t n = row.sample_size;
                const auto experiment_start = std::chrono::steady_clock::now();
                const std::vector<PollResult> results =
                    run_experiment(population, frame, params, method, n, sim);
                const auto experiment_elapsed = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - experiment_start);
                std::cout << "  " << to_string(method) << " n = " << n << " done in "
                          << std::fixed << std::setprecision(2) << experiment_elapsed.count()
                          << " s\n";

                // Every approach using this sampling method shares the same polls.
                for (std::size_t m = 0; m < kApproaches.size(); ++m) {
                    const Approach& approach = kApproaches[m];
                    if (approach.sampling != method) {
                        continue;
                    }
                    for (std::size_t c = 0; c < kCandidateCount; ++c) {
                        const auto candidate = static_cast<Candidate>(c);
                        std::vector<Estimate> estimates;
                        for (const PollResult& result : results) {
                            estimates.push_back(
                                estimate_with(approach.estimator, result, candidate, age_share));
                        }
                        row.summaries[m][c] = summarize(estimates, true_shares[c]);
                    }
                }
                if (method == SamplingMethod::Landline) {
                    row.landline = diagnose_landline(results, age_share);
                } else if (method == SamplingMethod::LandlineMNAR) {
                    row.landline_mnar = diagnose_landline(results, age_share);
                } else if (method == SamplingMethod::Mobile) {
                    row.mobile = diagnose_mobile(results, age_share);
                }
            }
        }
        std::cout << '\n';

        if (!rows.empty()) {
            print_landline_diagnostics("Landline", rows, &SampleSizeResult::landline, age_share);
            print_landline_diagnostics("LandlineMNAR", rows, &SampleSizeResult::landline_mnar,
                                       age_share);
            print_respondent_vote_share(rows, population_age_vote_share(population));
            print_mobile_diagnostics(rows, age_share);
            std::cout << "(values in %)\n";
            print_sample_size_tables(rows);
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << "Invalid parameters: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
