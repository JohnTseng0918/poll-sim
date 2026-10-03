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

// Records which positions (voter indices or frame numbers) a poll has used, so that random
// draws are made without replacement. One bit per position. Meant to be reused across polls:
// clear() resets only the positions marked since the last clear, so its cost grows with the
// number of draws rather than with the size.
class UsedPositions {
public:
    explicit UsedPositions(std::size_t size);

    // Marks a position as used. Returns false if it was already used.
    bool insert(std::size_t position);
    // Number of positions used since the last clear.
    std::size_t count() const { return marked_.size(); }
    // Number of positions that can be recorded.
    std::size_t size() const { return used_.size(); }
    void clear();

private:
    std::vector<bool> used_;
    std::vector<std::size_t> marked_;
};

// Draws a simple random sample of n distinct voters (without replacement).
// Random: draws random indices, redrawing any already drawn; records them in used, which must
// cover the population and is cleared first.
// Sequential: takes n consecutive voters from a random start, wrapping around to the first
// voter; equivalent to Random only because the population is in random order.
// used is not touched by Sequential.
// Everyone drawn responds, so contacts == n. Every weight is 1.
// Throws std::invalid_argument if n exceeds the population size, or used is too small.
Sample simple_random_sample(const std::vector<Voter>& population, std::size_t n,
                            DrawOrder order, UsedPositions& used, std::mt19937_64& rng);

// One phone number in a sampling frame: the owner's population index packed together with the
// attributes a call needs, so calling a number does not read the population.
// Bits 0-23: voter index, 24-25: age group, 26-27: candidate, 28-29: phone count.
class FrameEntry {
public:
    static constexpr std::size_t kMaxVoters = std::size_t{1} << 24;

    // index must be below kMaxVoters.
    FrameEntry(std::size_t index, const Voter& voter);

    std::size_t voter_index() const { return bits_ & kIndexMask; }
    AgeGroup age() const { return static_cast<AgeGroup>((bits_ >> kAgeShift) & kFieldMask); }
    Candidate candidate() const {
        return static_cast<Candidate>((bits_ >> kCandidateShift) & kFieldMask);
    }
    int phones() const { return static_cast<int>((bits_ >> kPhonesShift) & kFieldMask); }

private:
    static constexpr std::uint32_t kIndexMask = (std::uint32_t{1} << 24) - 1;
    static constexpr std::uint32_t kFieldMask = 0x3;
    static constexpr unsigned kAgeShift = 24;
    static constexpr unsigned kCandidateShift = 26;
    static constexpr unsigned kPhonesShift = 28;

    std::uint32_t bits_;
};

static_assert(sizeof(FrameEntry) == 4);
static_assert(kAgeGroupCount <= 4 && kCandidateCount <= 4 && kMaxPhones <= 3,
              "FrameEntry stores age group, candidate and phone count in 2 bits each");

// A sampling frame lists phone numbers; frame[k] describes the owner of number k.
using Frame = std::vector<FrameEntry>;

// Landline frame: one number per voter with a landline.
// Simplification: real landlines are shared by a household; here each owner has their own.
// Throws std::invalid_argument if the population has more than FrameEntry::kMaxVoters voters.
Frame build_landline_frame(const std::vector<Voter>& population);

// Mobile frame: one number per mobile phone, so a voter with k phones appears k times.
// Throws std::invalid_argument if the population has more than FrameEntry::kMaxVoters voters.
Frame build_mobile_frame(const std::vector<Voter>& population);

// Simulates a landline poll: keeps calling distinct numbers from the landline frame
// until n voters respond. Each voter called responds with probability
// response_rate[age][candidate]; numbers that do not respond are never called again.
// Random: calls random numbers, recording them in used, which must cover the frame and is
// cleared first. Sequential: calls consecutive numbers from a random position, wrapping
// around; the frame must be in random order, and used is not touched. Every weight is 1.
// Throws std::invalid_argument if the frame runs out before n voters respond,
// or used is too small.
Sample landline_sample(const Frame& frame,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, DrawOrder order, UsedPositions& used,
                       std::mt19937_64& rng);

// Simulates a mobile poll: same as landline_sample but calls numbers from the mobile frame.
// A voter with k phones is k times as likely to be called and may respond more than once
// through different numbers; each respondent's weight is 1 / k, read from the population.
// For Sequential, shuffle the frame first: build_mobile_frame puts a voter's numbers next
// to each other.
// Throws std::invalid_argument if the frame runs out before n voters respond,
// or used is too small.
Sample mobile_sample(const std::vector<Voter>& population, const Frame& frame,
                     const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                     std::size_t n, DrawOrder order, UsedPositions& used,
                     std::mt19937_64& rng);

}  // namespace poll_sim
