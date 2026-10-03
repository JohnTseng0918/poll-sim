#pragma once

#include "poll_sim/types.hpp"

namespace poll_sim {

struct Voter {
    AgeGroup age;
    Region region;
    Gender gender;
    Candidate candidate;
};

}  // namespace poll_sim
