#include "poll_sim/sampling.hpp"

#include <stdexcept>
#include <unordered_set>

namespace poll_sim {

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
    sample.contacts = n;
    return sample;
}

Sample landline_sample(const std::vector<Voter>& population,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, std::mt19937_64& rng) {
    if (n > population.size()) {
        throw std::invalid_argument("sample size exceeds population size");
    }

    std::uniform_int_distribution<std::size_t> index_dist(0, population.size() - 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::unordered_set<std::size_t> contacted;
    Sample sample;

    while (sample.respondents.size() < n) {
        if (contacted.size() == population.size()) {
            throw std::invalid_argument("population exhausted before reaching the sample size");
        }

        // Redraw whenever a voter has already been contacted.
        const std::size_t index = index_dist(rng);
        if (!contacted.insert(index).second) {
            continue;
        }

        const Voter& voter = population[index];
        const double p = response_rate[static_cast<std::size_t>(voter.age)]
                                      [static_cast<std::size_t>(voter.candidate)];
        if (unit(rng) < p) {
            sample.respondents.push_back(index);
        }
    }
    sample.contacts = contacted.size();
    return sample;
}

}  // namespace poll_sim
