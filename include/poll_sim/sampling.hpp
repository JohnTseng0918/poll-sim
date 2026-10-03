#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>
#include <vector>

#include "poll_sim/params.hpp"
#include "poll_sim/voter.hpp"

namespace poll_sim {

enum class SamplingMethod {
    SimpleRandom,  // everyone drawn answers
    Landline,      // landline owners drawn answer with probability response_rate[age][candidate]
    LandlineMNAR,  // same as Landline but with landline_mnar_response_rate[age][candidate]
    Mobile,        // phone numbers drawn; owners answer with mobile_response_rate[age][candidate]
};

constexpr std::string_view to_string(SamplingMethod v) noexcept {
    switch (v) {
        case SamplingMethod::SimpleRandom: return "SimpleRandom";
        case SamplingMethod::Landline: return "Landline";
        case SamplingMethod::LandlineMNAR: return "LandlineMNAR";
        case SamplingMethod::Mobile: return "Mobile";
    }
    return "Unknown";
}

// Voters who completed the poll, plus how many voters were contacted to get them.
struct Sample {
    std::vector<std::size_t> respondents;  // indices into the population
    // Design weight of each respondent, parallel to respondents:
    // 1 for SimpleRandom and Landline, 1 / phones for Mobile (multiplicity weight).
    std::vector<double> weights;
    std::size_t contacts = 0;
    // Responses from a voter who had already responded through another number.
    // Such voters are counted once per response.
    std::size_t duplicates = 0;
};

// Draws a simple random sample of n distinct voters (without replacement).
// Everyone drawn responds, so contacts == n. Every weight is 1.
// Throws std::invalid_argument if n exceeds the population size.
Sample simple_random_sample(const std::vector<Voter>& population, std::size_t n,
                            std::mt19937_64& rng);

// A sampling frame lists phone numbers; frame[k] is the index of the voter owning number k.
// Indices are stored as 32 bits to halve the frame size.
using Frame = std::vector<std::uint32_t>;

// Landline frame: one number per voter with a landline.
// Simplification: real landlines are shared by a household; here each owner has their own.
// Throws std::invalid_argument if a voter index does not fit in 32 bits.
Frame build_landline_frame(const std::vector<Voter>& population);

// Mobile frame: one number per mobile phone, so a voter with k phones appears k times.
// Throws std::invalid_argument if a voter index does not fit in 32 bits.
Frame build_mobile_frame(const std::vector<Voter>& population);

// Simulates a landline poll: keeps calling distinct random numbers from the landline frame
// until n voters respond. Each voter called responds with probability
// response_rate[age][candidate]; numbers that do not respond are never called again.
// Every weight is 1.
// Throws std::invalid_argument if the frame runs out before n voters respond.
Sample landline_sample(const std::vector<Voter>& population, const Frame& frame,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, std::mt19937_64& rng);

// Simulates a mobile poll: same as landline_sample but calls numbers from the mobile frame.
// A voter with k phones is k times as likely to be called and may respond more than once
// through different numbers; each respondent's weight is 1 / k.
// Throws std::invalid_argument if the frame runs out before n voters respond.
Sample mobile_sample(const std::vector<Voter>& population, const Frame& frame,
                     const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                     std::size_t n, std::mt19937_64& rng);

}  // namespace poll_sim
