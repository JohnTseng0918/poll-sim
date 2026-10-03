#pragma once

#include "poll_sim/types.hpp"

namespace poll_sim {

struct Voter {
    AgeGroup age;
    Region region;
    Gender gender;
    Candidate candidate;
    bool has_landline;  // reachable by landline
    int phones;         // number of mobile phones owned (0 = unreachable by mobile)
};

}  // namespace poll_sim
