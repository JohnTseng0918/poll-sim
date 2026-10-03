#pragma once

#include <vector>

#include "poll_sim/params.hpp"
#include "poll_sim/voter.hpp"

namespace poll_sim {

// Generates a synthetic voter population from the given parameters.
// The same params (including params.seed) always produce the same population.
// Throws std::invalid_argument if the parameters are invalid.
std::vector<Voter> generate_population(const PopulationParams& params);

}  // namespace poll_sim
