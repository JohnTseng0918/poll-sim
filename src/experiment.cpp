#include "poll_sim/experiment.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>

namespace poll_sim {

Frame build_frame(const std::vector<Voter>& population, SamplingMethod method,
                  const SimParams& sim) {
    Frame frame;
    if (method == SamplingMethod::Landline || method == SamplingMethod::LandlineMNAR) {
        frame = build_landline_frame(population);
    } else if (method == SamplingMethod::Mobile) {
        frame = build_mobile_frame(population);
        if (sim.order == DrawOrder::Sequential) {
            // A voter's numbers are adjacent in the frame; shuffle so consecutive calls behave
            // like random calls. Uses its own stream so the polls' stream is unchanged.
            std::mt19937_64 shuffle_rng(sim.seed ^ 0xD1B54A32D192ED03ULL);
            std::shuffle(frame.begin(), frame.end(), shuffle_rng);
        }
    }
    return frame;
}

std::vector<PollResult> run_experiment(const std::vector<Voter>& population, const Frame& frame,
                                       const PopulationParams& params, SamplingMethod method,
                                       std::size_t sample_size, const SimParams& sim) {
    if (sample_size == 0) {
        throw std::invalid_argument("sample_size must be positive");
    }
    if (sim.replications == 0) {
        throw std::invalid_argument("replications must be positive");
    }

    std::mt19937_64 rng(sim.seed);

    // Records the positions used by random draws; allocated once and reused by every poll.
    UsedPositions used(method == SamplingMethod::SimpleRandom ? population.size()
                                                              : frame.size());

    std::vector<PollResult> results;
    for (std::size_t r = 0; r < sim.replications; ++r) {
        Sample sample;
        switch (method) {
            case SamplingMethod::SimpleRandom:
                sample = simple_random_sample(population, sample_size, sim.order, used, rng);
                break;
            case SamplingMethod::Landline:
                sample = landline_sample(frame, params.response_rate, sample_size, sim.order,
                                         used, rng);
                break;
            case SamplingMethod::LandlineMNAR:
                sample = landline_sample(frame, params.landline_mnar_response_rate, sample_size,
                                         sim.order, used, rng);
                break;
            case SamplingMethod::Mobile:
                sample = mobile_sample(frame, params.mobile_response_rate, sample_size,
                                       sim.order, used, rng);
                break;
        }
        results.push_back(tally(sample));
    }
    return results;
}

}  // namespace poll_sim
