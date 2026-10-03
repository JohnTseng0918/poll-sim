#include "poll_sim/experiment.hpp"

#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>

namespace poll_sim {

namespace {

// Response rates [age][candidate] used by the method's phone polls.
const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate_for(
    SamplingMethod method, const PopulationParams& params) {
    switch (method) {
        case SamplingMethod::LandlineMNAR: return params.landline_mnar_response_rate;
        case SamplingMethod::Mobile: return params.mobile_response_rate;
        case SamplingMethod::SimpleRandom:
        case SamplingMethod::Landline: break;
    }
    return params.response_rate;
}

}  // namespace

Frame build_frame(const std::vector<Voter>& population, const PopulationParams& params,
                  SamplingMethod method, const SimParams& sim) {
    Frame frame;
    if (method == SamplingMethod::Landline || method == SamplingMethod::LandlineMNAR) {
        frame = build_landline_frame(population);
    } else if (method == SamplingMethod::Mobile) {
        frame = build_mobile_frame(population);
    }

    if (sim.response == ResponseModel::Fixed) {
        // Drawn in frame order, before any shuffle, so both draw orders see the same answering
        // numbers. Each method has its own stream, separate from the polls' stream.
        const auto& response_rate = response_rate_for(method, params);
        std::mt19937_64 response_rng(sim.seed ^ 0x9E6C63D0676A9A99ULL ^
                                     static_cast<std::uint64_t>(method));
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        for (FrameEntry& entry : frame) {
            const double p = response_rate[static_cast<std::size_t>(entry.age())]
                                          [static_cast<std::size_t>(entry.candidate())];
            entry.set_answers(unit(response_rng) < p);
        }
    }

    if (method == SamplingMethod::Mobile && sim.order == DrawOrder::Sequential) {
        // A voter's numbers are adjacent in the frame; shuffle so consecutive calls behave
        // like random calls. Uses its own stream so the polls' stream is unchanged.
        std::mt19937_64 shuffle_rng(sim.seed ^ 0xD1B54A32D192ED03ULL);
        std::shuffle(frame.begin(), frame.end(), shuffle_rng);
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
            case SamplingMethod::LandlineMNAR:
                sample = landline_sample(frame, response_rate_for(method, params), sample_size,
                                         sim.order, sim.response, used, rng);
                break;
            case SamplingMethod::Mobile:
                sample = mobile_sample(frame, response_rate_for(method, params), sample_size,
                                       sim.order, sim.response, used, rng);
                break;
        }
        results.push_back(tally(sample));
    }
    return results;
}

}  // namespace poll_sim
