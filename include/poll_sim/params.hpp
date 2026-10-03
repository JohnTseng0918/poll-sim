#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include "poll_sim/types.hpp"

namespace poll_sim {

template <class T, std::size_t N>
using Arr = std::array<T, N>;

// Upper bound on mobile phones per voter; "kMaxPhones or more" is folded into kMaxPhones.
inline constexpr std::size_t kMaxPhones = 3;

// Population parameters: composition and behavior of the synthetic voter population.
struct PopulationParams {
    std::size_t size = 10'000'000;
    std::uint64_t seed = 1;  // used only for population generation

    // Demographics: Phase 1 assumes the three dimensions are independent,
    // so each is given as a marginal distribution.
    Arr<double, kAgeGroupCount> age_share{0.30, 0.35, 0.35};
    Arr<double, kRegionCount> region_share{0.45, 0.25, 0.27, 0.03};
    Arr<double, kGenderCount> gender_share{0.49, 0.51};

    // Vote share [age][candidate]; each row sums to 1.
    // Column order is DPP, KMT, None. KMT / None values are placeholders.
    Arr<Arr<double, kCandidateCount>, kAgeGroupCount> vote_share{{
        {0.65, 0.25, 0.10},  // Young
        {0.45, 0.40, 0.15},  // Middle
        {0.30, 0.55, 0.15},  // Senior
    }};

    // Landline response rate [age][candidate].
    // Equal values within a row: response depends on age only (MAR).
    // Different values within a row: response depends on vote intention (MNAR),
    // which weighting cannot fully correct.
    Arr<Arr<double, kCandidateCount>, kAgeGroupCount> response_rate{{
        {0.05, 0.05, 0.05},  // Young
        {0.30, 0.30, 0.30},  // Middle
        {0.60, 0.60, 0.60},  // Senior
    }};

    // Coverage attributes are drawn from a separate random stream derived from seed,
    // so changing them leaves age, region, gender and vote intention unchanged.

    // Probability of having a landline [age][candidate].
    // 1.0 everywhere: everyone is reachable by landline, and response_rate alone
    // decides who answers.
    Arr<Arr<double, kCandidateCount>, kAgeGroupCount> landline_share{{
        {1.0, 1.0, 1.0},  // Young
        {1.0, 1.0, 1.0},  // Middle
        {1.0, 1.0, 1.0},  // Senior
    }};

    // Distribution of mobile phone count [age][candidate][k], k = 0..kMaxPhones;
    // each row sums to 1. Equal rows within an age group: phone count depends on age only.
    // Values are placeholders.
    using PhoneDist = Arr<double, kMaxPhones + 1>;
    Arr<Arr<PhoneDist, kCandidateCount>, kAgeGroupCount> phone_share{{
        {{{0.01, 0.80, 0.15, 0.04}, {0.01, 0.80, 0.15, 0.04}, {0.01, 0.80, 0.15, 0.04}}},
        {{{0.02, 0.83, 0.12, 0.03}, {0.02, 0.83, 0.12, 0.03}, {0.02, 0.83, 0.12, 0.03}}},
        {{{0.10, 0.80, 0.08, 0.02}, {0.10, 0.80, 0.08, 0.02}, {0.10, 0.80, 0.08, 0.02}}},
    }};

    // Throws std::invalid_argument if any parameter is invalid.
    void validate() const;
};

// Simulation parameters: how sampling is run.
struct SimParams {
    // Each sample size is run as a separate experiment.
    // 1067 gives a ±3% margin of error at 95% confidence.
    std::vector<std::size_t> sample_sizes{50, 100, 400, 1067, 4000, 10000};
    std::size_t replications = 1000;
    std::uint64_t seed = 42;  // used only for sampling
    unsigned threads = 0;  // 0 = auto-detect
};

namespace detail {

inline constexpr double kSumTolerance = 1e-9;

inline void check_probability(double p, const std::string& name) {
    if (!(p >= 0.0 && p <= 1.0)) {
        throw std::invalid_argument(name + " must be in [0, 1], got " + std::to_string(p));
    }
}

template <std::size_t N>
void check_distribution(const Arr<double, N>& dist, const std::string& name) {
    double sum = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        check_probability(dist[i], name + "[" + std::to_string(i) + "]");
        sum += dist[i];
    }
    if (std::abs(sum - 1.0) > kSumTolerance) {
        throw std::invalid_argument(name + " must sum to 1, got " + std::to_string(sum));
    }
}

}  // namespace detail

inline void PopulationParams::validate() const {
    if (size == 0) {
        throw std::invalid_argument("size must be positive");
    }
    detail::check_distribution(age_share, "age_share");
    detail::check_distribution(region_share, "region_share");
    detail::check_distribution(gender_share, "gender_share");
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        const std::string row = "[" + std::to_string(a) + "]";
        detail::check_distribution(vote_share[a], "vote_share" + row);
        for (std::size_t c = 0; c < kCandidateCount; ++c) {
            const std::string cell = row + "[" + std::to_string(c) + "]";
            detail::check_probability(response_rate[a][c], "response_rate" + cell);
            detail::check_probability(landline_share[a][c], "landline_share" + cell);
            detail::check_distribution(phone_share[a][c], "phone_share" + cell);
        }
    }
}

}  // namespace poll_sim
