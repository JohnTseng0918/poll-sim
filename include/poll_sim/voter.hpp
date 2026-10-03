#pragma once

#include <cstdint>

#include "poll_sim/types.hpp"

namespace poll_sim {

// Every member is one byte, so sizeof(Voter) is 6 with no padding.
struct Voter {
    AgeGroup age;
    Region region;
    Gender gender;
    Candidate candidate;
    bool has_landline;    // reachable by landline
    std::uint8_t phones;  // number of mobile phones owned (0 = unreachable by mobile)
};

}  // namespace poll_sim
