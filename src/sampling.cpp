#include "poll_sim/sampling.hpp"

#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace poll_sim {

namespace {

// Keeps calling distinct random numbers from the frame until n voters respond.
// Fills respondents, contacts and duplicates; the caller sets the weights.
Sample random_phone_sample(const std::vector<Voter>& population, const Frame& frame,
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

// Keeps calling consecutive numbers from a random position in the frame, wrapping around,
// until n voters respond. Consecutive numbers are distinct until the frame wraps back to the
// start, so no record of called numbers is needed.
// Fills respondents, contacts and duplicates; the caller sets the weights.
Sample sequential_phone_sample(
    const std::vector<Voter>& population, const Frame& frame,
    const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate, std::size_t n,
    std::mt19937_64& rng) {
    if (n > frame.size()) {
        throw std::invalid_argument("sample size exceeds frame size");
    }

    std::uniform_int_distribution<std::size_t> start_dist(0, frame.size() - 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::unordered_set<std::size_t> responded;  // voters who have responded
    std::size_t number = start_dist(rng);
    Sample sample;

    while (sample.respondents.size() < n) {
        if (sample.contacts == frame.size()) {
            throw std::invalid_argument("frame exhausted before reaching the sample size");
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
        ++sample.contacts;
        number = (number + 1 == frame.size()) ? 0 : number + 1;
    }
    return sample;
}

Sample phone_sample(const std::vector<Voter>& population, const Frame& frame,
                    const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                    std::size_t n, DrawOrder order, std::mt19937_64& rng) {
    switch (order) {
        case DrawOrder::Random:
            return random_phone_sample(population, frame, response_rate, n, rng);
        case DrawOrder::Sequential:
            return sequential_phone_sample(population, frame, response_rate, n, rng);
    }
    throw std::invalid_argument("unknown draw order");
}

void check_frame_index_range(const std::vector<Voter>& population) {
    if (population.size() > std::numeric_limits<Frame::value_type>::max()) {
        throw std::invalid_argument("population too large for 32-bit frame indices");
    }
}

}  // namespace

Sample simple_random_sample(const std::vector<Voter>& population, std::size_t n,
                            DrawOrder order, std::mt19937_64& rng) {
    if (n > population.size()) {
        throw std::invalid_argument("sample size exceeds population size");
    }

    std::uniform_int_distribution<std::size_t> index_dist(0, population.size() - 1);
    Sample sample;

    if (order == DrawOrder::Sequential) {
        std::size_t index = index_dist(rng);
        for (std::size_t i = 0; i < n; ++i) {
            sample.respondents.push_back(index);
            index = (index + 1 == population.size()) ? 0 : index + 1;
        }
    } else {
        // Redraw whenever an index has already been drawn.
        std::unordered_set<std::size_t> drawn;
        while (sample.respondents.size() < n) {
            const std::size_t index = index_dist(rng);
            if (drawn.insert(index).second) {
                sample.respondents.push_back(index);
            }
        }
    }
    sample.weights.assign(n, 1.0);
    sample.contacts = n;
    return sample;
}

Frame build_landline_frame(const std::vector<Voter>& population) {
    check_frame_index_range(population);

    // Count first so the frame is allocated once at its final size.
    std::size_t count = 0;
    for (const Voter& voter : population) {
        if (voter.has_landline) {
            ++count;
        }
    }

    Frame frame;
    frame.reserve(count);
    for (std::size_t i = 0; i < population.size(); ++i) {
        if (population[i].has_landline) {
            frame.push_back(static_cast<Frame::value_type>(i));
        }
    }
    return frame;
}

Frame build_mobile_frame(const std::vector<Voter>& population) {
    check_frame_index_range(population);

    // Count first so the frame is allocated once at its final size.
    std::size_t count = 0;
    for (const Voter& voter : population) {
        count += voter.phones;
    }

    Frame frame;
    frame.reserve(count);
    for (std::size_t i = 0; i < population.size(); ++i) {
        for (int k = 0; k < population[i].phones; ++k) {
            frame.push_back(static_cast<Frame::value_type>(i));
        }
    }
    return frame;
}

Sample landline_sample(const std::vector<Voter>& population, const Frame& frame,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, DrawOrder order, std::mt19937_64& rng) {
    Sample sample = phone_sample(population, frame, response_rate, n, order, rng);
    sample.weights.assign(sample.respondents.size(), 1.0);
    return sample;
}

Sample mobile_sample(const std::vector<Voter>& population, const Frame& frame,
                     const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                     std::size_t n, DrawOrder order, std::mt19937_64& rng) {
    Sample sample = phone_sample(population, frame, response_rate, n, order, rng);
    for (std::size_t index : sample.respondents) {
        sample.weights.push_back(1.0 / population[index].phones);
    }
    return sample;
}

}  // namespace poll_sim
