#include "poll_sim/sampling.hpp"

#include <stdexcept>
#include <unordered_set>

namespace poll_sim {

namespace {

// Keeps calling distinct random numbers from the frame until n voters respond.
// Fills respondents, contacts and duplicates; the caller sets the weights.
Sample phone_sample(const std::vector<Voter>& population, const std::vector<std::size_t>& frame,
                    const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                    std::size_t n, std::mt19937_64& rng) {
    if (n > frame.size()) {
        throw std::invalid_argument("sample size exceeds frame size");
    }

    std::uniform_int_distribution<std::size_t> number_dist(0, frame.size() - 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::unordered_set<std::size_t> called;     // positions in the frame
    std::unordered_set<std::size_t> responded;  // voters who have responded
    Sample sample;

    while (sample.respondents.size() < n) {
        if (called.size() == frame.size()) {
            throw std::invalid_argument("frame exhausted before reaching the sample size");
        }

        // Redraw whenever a number has already been called.
        const std::size_t number = number_dist(rng);
        if (!called.insert(number).second) {
            continue;
        }

        const std::size_t index = frame[number];
        const Voter& voter = population[index];
        const double p = response_rate[static_cast<std::size_t>(voter.age)]
                                      [static_cast<std::size_t>(voter.candidate)];
        if (unit(rng) < p) {
            sample.respondents.push_back(index);
            if (!responded.insert(index).second) {
                ++sample.duplicates;
            }
        }
    }
    sample.contacts = called.size();
    return sample;
}

}  // namespace

Sample simple_random_sample(const std::vector<Voter>& population, std::size_t n,
                            std::mt19937_64& rng) {
    if (n > population.size()) {
        throw std::invalid_argument("sample size exceeds population size");
    }

    std::uniform_int_distribution<std::size_t> index_dist(0, population.size() - 1);
    std::unordered_set<std::size_t> drawn;
    Sample sample;

    // Redraw whenever an index has already been drawn.
    while (sample.respondents.size() < n) {
        const std::size_t index = index_dist(rng);
        if (drawn.insert(index).second) {
            sample.respondents.push_back(index);
        }
    }
    sample.weights.assign(n, 1.0);
    sample.contacts = n;
    return sample;
}

std::vector<std::size_t> build_landline_frame(const std::vector<Voter>& population) {
    std::vector<std::size_t> frame;
    for (std::size_t i = 0; i < population.size(); ++i) {
        if (population[i].has_landline) {
            frame.push_back(i);
        }
    }
    return frame;
}

std::vector<std::size_t> build_mobile_frame(const std::vector<Voter>& population) {
    std::vector<std::size_t> frame;
    for (std::size_t i = 0; i < population.size(); ++i) {
        for (int k = 0; k < population[i].phones; ++k) {
            frame.push_back(i);
        }
    }
    return frame;
}

Sample landline_sample(const std::vector<Voter>& population,
                       const std::vector<std::size_t>& frame,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, std::mt19937_64& rng) {
    Sample sample = phone_sample(population, frame, response_rate, n, rng);
    sample.weights.assign(sample.respondents.size(), 1.0);
    return sample;
}

Sample mobile_sample(const std::vector<Voter>& population, const std::vector<std::size_t>& frame,
                     const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                     std::size_t n, std::mt19937_64& rng) {
    Sample sample = phone_sample(population, frame, response_rate, n, rng);
    for (std::size_t index : sample.respondents) {
        sample.weights.push_back(1.0 / population[index].phones);
    }
    return sample;
}

}  // namespace poll_sim
