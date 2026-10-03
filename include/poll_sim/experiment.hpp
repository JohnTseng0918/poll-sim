#pragma once

#include <cstddef>
#include <vector>

#include "poll_sim/params.hpp"
#include "poll_sim/poll.hpp"
#include "poll_sim/sampling.hpp"
#include "poll_sim/voter.hpp"

namespace poll_sim {

// Runs sim.replications independent polls of sample_size respondents using the given method,
// seeded with sim.seed. Landline polls draw from the voters with a landline
// and use params.response_rate.
// Returns one PollResult per replication.
// Throws std::invalid_argument if sample_size or replications is 0,
// or the population cannot supply sample_size respondents.
std::vector<PollResult> run_experiment(const std::vector<Voter>& population,
                                       const PopulationParams& params, SamplingMethod method,
                                       std::size_t sample_size, const SimParams& sim);

}  // namespace poll_sim
