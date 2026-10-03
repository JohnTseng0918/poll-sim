#include "poll_sim/experiment.hpp"

#include <random>
#include <stdexcept>

namespace poll_sim {

std::vector<PollResult> run_experiment(const std::vector<Voter>& population,
                                       const PopulationParams& params, SamplingMethod method,
                                       std::size_t sample_size, const SimParams& sim) {
    if (sample_size == 0) {
        throw std::invalid_argument("sample_size must be positive");
    }
    if (sim.replications == 0) {
        throw std::invalid_argument("replications must be positive");
    }

    std::mt19937_64 rng(sim.seed);

    // Built once per experiment and shared by every replication.
    std::vector<std::size_t> frame;
    if (method == SamplingMethod::Landline || method == SamplingMethod::LandlineMNAR) {
        frame = build_landline_frame(population);
    } else if (method == SamplingMethod::Mobile) {
        frame = build_mobile_frame(population);
    }

    std::vector<PollResult> results;
    for (std::size_t r = 0; r < sim.replications; ++r) {
        Sample sample;
        switch (method) {
            case SamplingMethod::SimpleRandom:
                sample = simple_random_sample(population, sample_size, rng);
                break;
            case SamplingMethod::Landline:
                sample = landline_sample(population, frame, params.response_rate, sample_size, rng);
                break;
            case SamplingMethod::LandlineMNAR:
                sample = landline_sample(population, frame, params.landline_mnar_response_rate,
                                         sample_size, rng);
                break;
            case SamplingMethod::Mobile:
                sample = mobile_sample(population, frame, params.mobile_response_rate,
                                       sample_size, rng);
                break;
        }
        results.push_back(tally(population, sample));
    }
    return results;
}

}  // namespace poll_sim
