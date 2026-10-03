#include "poll_sim/poll.hpp"

namespace poll_sim {

double PollResult::share(Candidate candidate) const {
    return static_cast<double>(counts[static_cast<std::size_t>(candidate)]) / sample_size;
}

double PollResult::share(AgeGroup age) const {
    return static_cast<double>(age_counts[static_cast<std::size_t>(age)]) / sample_size;
}

double PollResult::response_rate() const {
    return static_cast<double>(sample_size) / contacts;
}

PollResult tally(const Sample& sample) {
    PollResult result;
    for (std::size_t i = 0; i < sample.respondents.size(); ++i) {
        const FrameEntry& respondent = sample.respondents[i];
        const auto a = static_cast<std::size_t>(respondent.age());
        const auto c = static_cast<std::size_t>(respondent.candidate());
        const double w = sample.weights[i];
        ++result.counts[c];
        ++result.age_counts[a];
        ++result.cell_counts[a][c];
        result.cell_weights[a][c] += w;
        result.age_weight_squares[a] += w * w;
    }
    result.sample_size = sample.respondents.size();
    result.contacts = sample.contacts;
    result.duplicates = sample.duplicates;
    return result;
}

Arr<double, kCandidateCount> population_vote_share(const std::vector<Voter>& population) {
    Arr<std::size_t, kCandidateCount> counts{};
    for (const Voter& voter : population) {
        ++counts[static_cast<std::size_t>(voter.candidate)];
    }

    Arr<double, kCandidateCount> shares{};
    for (std::size_t c = 0; c < kCandidateCount; ++c) {
        shares[c] = static_cast<double>(counts[c]) / population.size();
    }
    return shares;
}

Arr<Arr<double, kCandidateCount>, kAgeGroupCount> population_age_vote_share(
    const std::vector<Voter>& population) {
    Arr<Arr<std::size_t, kCandidateCount>, kAgeGroupCount> counts{};
    Arr<std::size_t, kAgeGroupCount> age_counts{};
    for (const Voter& voter : population) {
        const auto a = static_cast<std::size_t>(voter.age);
        ++counts[a][static_cast<std::size_t>(voter.candidate)];
        ++age_counts[a];
    }

    Arr<Arr<double, kCandidateCount>, kAgeGroupCount> shares{};
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        for (std::size_t c = 0; c < kCandidateCount; ++c) {
            shares[a][c] = static_cast<double>(counts[a][c]) / age_counts[a];
        }
    }
    return shares;
}

Arr<double, kAgeGroupCount> population_age_share(const std::vector<Voter>& population) {
    Arr<std::size_t, kAgeGroupCount> counts{};
    for (const Voter& voter : population) {
        ++counts[static_cast<std::size_t>(voter.age)];
    }

    Arr<double, kAgeGroupCount> shares{};
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        shares[a] = static_cast<double>(counts[a]) / population.size();
    }
    return shares;
}

}  // namespace poll_sim
