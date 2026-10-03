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
    Landline,      // landline owners drawn answer with probability response_rate[age][candidate]
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

// A sampling frame lists phone numbers; frame[k] is the index of the voter owning number k.

// Landline frame: one number per voter with a landline.
// Simplification: real landlines are shared by a household; here each owner has their own.
std::vector<std::size_t> build_landline_frame(const std::vector<Voter>& population);

// Mobile frame: one number per mobile phone, so a voter with k phones appears k times.
std::vector<std::size_t> build_mobile_frame(const std::vector<Voter>& population);

// Simulates a landline poll: keeps calling distinct random numbers from the landline frame
// until n voters respond. Each voter called responds with probability
// response_rate[age][candidate]; numbers that do not respond are never called again.
// Throws std::invalid_argument if the frame runs out before n voters respond.
Sample landline_sample(const std::vector<Voter>& population,
                       const std::vector<std::size_t>& frame,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, std::mt19937_64& rng);

}  // namespace poll_sim
