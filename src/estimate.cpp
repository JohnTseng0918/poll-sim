#include "poll_sim/estimate.hpp"

#include <cmath>

namespace poll_sim {

namespace {

// Population age shares renormalized over the age groups that have respondents.
Arr<double, kAgeGroupCount> stratum_weights(
    const PollResult& result, const Arr<double, kAgeGroupCount>& population_age_share) {
    double covered = 0.0;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        if (result.age_counts[a] > 0) {
            covered += population_age_share[a];
        }
    }

    Arr<double, kAgeGroupCount> weights{};
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        if (result.age_counts[a] > 0) {
            weights[a] = population_age_share[a] / covered;
        }
    }
    return weights;
}

// Sum of design weights of all respondents in age group a.
double age_weight_sum(const PollResult& result, std::size_t a) {
    double sum = 0.0;
    for (double w : result.cell_weights[a]) {
        sum += w;
    }
    return sum;
}

// Kish effective sample size within age group a: (sum w)^2 / sum w^2.
double age_effective_size(const PollResult& result, std::size_t a) {
    const double sum = age_weight_sum(result, a);
    return sum * sum / result.age_weight_squares[a];
}

}  // namespace

Estimate unweighted_estimate(const PollResult& result, Candidate candidate) {
    Estimate estimate;
    estimate.share = result.share(candidate);
    estimate.se = std::sqrt(estimate.share * (1.0 - estimate.share) / result.sample_size);
    return estimate;
}

Estimate age_weighted_estimate(const PollResult& result, Candidate candidate,
                               const Arr<double, kAgeGroupCount>& population_age_share) {
    const Arr<double, kAgeGroupCount> weights = stratum_weights(result, population_age_share);
    const auto c = static_cast<std::size_t>(candidate);

    Estimate estimate;
    double variance = 0.0;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        const std::size_t n_a = result.age_counts[a];
        if (n_a == 0) {
            continue;
        }
        const double p_a = static_cast<double>(result.cell_counts[a][c]) / n_a;
        estimate.share += weights[a] * p_a;
        variance += weights[a] * weights[a] * p_a * (1.0 - p_a) / n_a;
    }
    estimate.se = std::sqrt(variance);
    return estimate;
}

double age_weighted_effective_size(const PollResult& result,
                                   const Arr<double, kAgeGroupCount>& population_age_share) {
    const Arr<double, kAgeGroupCount> weights = stratum_weights(result, population_age_share);

    // Each respondent in age group a gets weight W_a / n_a, so
    // (sum w)^2 / sum w^2 = 1 / sum(W_a^2 / n_a).
    double sum_squared = 0.0;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        if (result.age_counts[a] > 0) {
            sum_squared += weights[a] * weights[a] / result.age_counts[a];
        }
    }
    return 1.0 / sum_squared;
}

Estimate multiplicity_weighted_estimate(const PollResult& result, Candidate candidate) {
    const auto c = static_cast<std::size_t>(candidate);
    double supporters = 0.0;
    double total = 0.0;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        supporters += result.cell_weights[a][c];
        total += age_weight_sum(result, a);
    }

    Estimate estimate;
    estimate.share = supporters / total;
    estimate.se = std::sqrt(estimate.share * (1.0 - estimate.share) /
                            multiplicity_weighted_effective_size(result));
    return estimate;
}

Estimate multiplicity_age_weighted_estimate(
    const PollResult& result, Candidate candidate,
    const Arr<double, kAgeGroupCount>& population_age_share) {
    const Arr<double, kAgeGroupCount> weights = stratum_weights(result, population_age_share);
    const auto c = static_cast<std::size_t>(candidate);

    Estimate estimate;
    double variance = 0.0;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        if (result.age_counts[a] == 0) {
            continue;
        }
        const double p_a = result.cell_weights[a][c] / age_weight_sum(result, a);
        estimate.share += weights[a] * p_a;
        variance += weights[a] * weights[a] * p_a * (1.0 - p_a) / age_effective_size(result, a);
    }
    estimate.se = std::sqrt(variance);
    return estimate;
}

double multiplicity_weighted_effective_size(const PollResult& result) {
    double sum = 0.0;
    double sum_squared = 0.0;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        sum += age_weight_sum(result, a);
        sum_squared += result.age_weight_squares[a];
    }
    return sum * sum / sum_squared;
}

double multiplicity_age_weighted_effective_size(
    const PollResult& result, const Arr<double, kAgeGroupCount>& population_age_share) {
    const Arr<double, kAgeGroupCount> weights = stratum_weights(result, population_age_share);

    // A respondent in age group a with design weight w gets W_a * w / (sum of w in group a),
    // so (sum w)^2 / sum w^2 = 1 / sum(W_a^2 / n_eff_a).
    double sum_squared = 0.0;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        if (result.age_counts[a] > 0) {
            sum_squared += weights[a] * weights[a] / age_effective_size(result, a);
        }
    }
    return 1.0 / sum_squared;
}

}  // namespace poll_sim
