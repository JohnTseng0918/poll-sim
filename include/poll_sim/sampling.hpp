#pragma once

#include <cstddef>
#include <random>
#include <string_view>
#include <vector>

#include "poll_sim/params.hpp"
#include "poll_sim/voter.hpp"

namespace poll_sim {

enum class SamplingMethod {
    SimpleRandom,  // everyone drawn answers
    Landline,      // drawn voters answer with probability response_rate[age][candidate]
};

constexpr std::string_view to_string(SamplingMethod v) noexcept {
    switch (v) {
        case SamplingMethod::SimpleRandom: return "SimpleRandom";
        case SamplingMethod::Landline: return "Landline";
    }
    return "Unknown";
}

// Voters who completed the poll, plus how many voters were contacted to get them.
struct Sample {
    std::vector<std::size_t> respondents;  // indices into the population
    std::size_t contacts = 0;
};

// Draws a simple random sample of n distinct voters (without replacement).
// Everyone drawn responds, so contacts == n.
// Throws std::invalid_argument if n exceeds the population size.
Sample simple_random_sample(const std::vector<Voter>& population, std::size_t n,
                            std::mt19937_64& rng);

// Simulates a landline poll: keeps contacting distinct random voters until n of them respond.
// Each contacted voter responds with probability response_rate[age][candidate];
// voters who do not respond are never contacted again.
// Throws std::invalid_argument if the population runs out before n voters respond.
Sample landline_sample(const std::vector<Voter>& population,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, std::mt19937_64& rng);

}  // namespace poll_sim
