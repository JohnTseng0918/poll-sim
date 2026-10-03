#pragma once

#include <cstddef>
#include <vector>

#include "poll_sim/params.hpp"
#include "poll_sim/poll.hpp"
#include "poll_sim/sampling.hpp"
#include "poll_sim/voter.hpp"

namespace poll_sim {

// Builds the sampling frame the given method calls from: the landline frame for Landline and
// LandlineMNAR, the mobile frame for Mobile, and an empty frame for SimpleRandom.
// For sequential mobile polls the frame is shuffled, with a stream derived from sim.seed,
// since a voter's numbers are adjacent. Build it once and share it across sample sizes.
// Throws std::invalid_argument if the population is too large for a frame.
Frame build_frame(const std::vector<Voter>& population, SamplingMethod method,
                  const SimParams& sim);

// Runs sim.replications independent polls of sample_size respondents using the given method,
// seeded with sim.seed. frame must come from build_frame for the same method and sim.
// Landline polls use params.response_rate (params.landline_mnar_response_rate for
// LandlineMNAR); mobile polls use params.mobile_response_rate.
// sim.order selects random or sequential draws.
// Returns one PollResult per replication.
// Throws std::invalid_argument if sample_size or replications is 0,
// or the population cannot supply sample_size respondents.
std::vector<PollResult> run_experiment(const std::vector<Voter>& population, const Frame& frame,
                                       const PopulationParams& params, SamplingMethod method,
                                       std::size_t sample_size, const SimParams& sim);

}  // namespace poll_sim
