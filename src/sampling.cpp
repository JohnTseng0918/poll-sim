#include "poll_sim/sampling.hpp"

#include <stdexcept>

namespace poll_sim {

namespace {

// Whether the owner of a number answers this call: drawn now for PerCall,
// read from the frame for Fixed.
bool answers_call(const FrameEntry& entry,
                  const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                  ResponseModel response, std::uniform_real_distribution<double>& unit,
                  std::mt19937_64& rng) {
    if (response == ResponseModel::Fixed) {
        return entry.answers();
    }
    const double p = response_rate[static_cast<std::size_t>(entry.age())]
                                  [static_cast<std::size_t>(entry.candidate())];
    return unit(rng) < p;
}

// Keeps calling distinct random numbers from the frame until n voters respond, recording the
// called numbers in used. Fills respondents and contacts; the caller sets the rest.
Sample random_phone_sample(const Frame& frame,
                           const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                           std::size_t n, ResponseModel response, UsedPositions& used,
                           std::mt19937_64& rng) {
    if (used.size() < frame.size()) {
        throw std::invalid_argument("UsedPositions is smaller than the frame");
    }

    std::uniform_int_distribution<std::size_t> number_dist(0, frame.size() - 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    used.clear();
    Sample sample;

    while (sample.respondents.size() < n) {
        if (used.count() == frame.size()) {
            throw std::invalid_argument("frame exhausted before reaching the sample size");
        }

        // Redraw whenever a number has already been called.
        const std::size_t number = number_dist(rng);
        if (!used.insert(number)) {
            continue;
        }

        const FrameEntry entry = frame[number];
        if (answers_call(entry, response_rate, response, unit, rng)) {
            sample.respondents.push_back(entry);
        }
    }
    sample.contacts = used.count();
    return sample;
}

// Keeps calling consecutive numbers from a random position in the frame, wrapping around,
// until n voters respond. Consecutive numbers are distinct until the frame wraps back to the
// start, so no record of called numbers is needed.
// Fills respondents and contacts; the caller sets the rest.
Sample sequential_phone_sample(
    const Frame& frame, const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
    std::size_t n, ResponseModel response, std::mt19937_64& rng) {
    std::uniform_int_distribution<std::size_t> start_dist(0, frame.size() - 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::size_t number = start_dist(rng);
    Sample sample;

    while (sample.respondents.size() < n) {
        if (sample.contacts == frame.size()) {
            throw std::invalid_argument("frame exhausted before reaching the sample size");
        }

        const FrameEntry entry = frame[number];
        if (answers_call(entry, response_rate, response, unit, rng)) {
            sample.respondents.push_back(entry);
        }
        ++sample.contacts;
        number = (number + 1 == frame.size()) ? 0 : number + 1;
    }
    return sample;
}

// Number of responses from a voter who had already responded: a voter who responded through
// k numbers adds k - 1. Counted once after the poll by marking each respondent in responded,
// which must cover the population.
std::size_t count_duplicates(const std::vector<FrameEntry>& respondents,
                             UsedPositions& responded) {
    responded.clear();
    std::size_t duplicates = 0;
    for (const FrameEntry& respondent : respondents) {
        const std::size_t index = respondent.voter_index();
        if (index >= responded.size()) {
            throw std::invalid_argument("UsedPositions is smaller than the population");
        }
        if (!responded.insert(index)) {
            ++duplicates;
        }
    }
    return duplicates;
}

// Fills respondents and contacts; the caller sets the weights and duplicates.
Sample phone_sample(const Frame& frame,
                    const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                    std::size_t n, DrawOrder order, ResponseModel response, UsedPositions& used,
                    std::mt19937_64& rng) {
    if (n > frame.size()) {
        throw std::invalid_argument("sample size exceeds frame size");
    }

    Sample sample;
    switch (order) {
        case DrawOrder::Random:
            sample = random_phone_sample(frame, response_rate, n, response, used, rng);
            break;
        case DrawOrder::Sequential:
            sample = sequential_phone_sample(frame, response_rate, n, response, rng);
            break;
    }
    return sample;
}

void check_frame_index_range(const std::vector<Voter>& population) {
    if (population.size() > FrameEntry::kMaxVoters) {
        throw std::invalid_argument("population too large for 24-bit frame indices");
    }
}

}  // namespace

FrameEntry::FrameEntry(std::size_t index, const Voter& voter)
    : bits_(static_cast<std::uint32_t>(index) |
            (static_cast<std::uint32_t>(voter.age) << kAgeShift) |
            (static_cast<std::uint32_t>(voter.candidate) << kCandidateShift) |
            (static_cast<std::uint32_t>(voter.phones) << kPhonesShift)) {}

UsedPositions::UsedPositions(std::size_t size) : used_(size) {}

bool UsedPositions::insert(std::size_t position) {
    if (used_[position]) {
        return false;
    }
    used_[position] = true;
    marked_.push_back(position);
    return true;
}

void UsedPositions::clear() {
    for (std::size_t position : marked_) {
        used_[position] = false;
    }
    marked_.clear();
}

Sample simple_random_sample(const std::vector<Voter>& population, std::size_t n,
                            DrawOrder order, UsedPositions& used, std::mt19937_64& rng) {
    if (n > population.size()) {
        throw std::invalid_argument("sample size exceeds population size");
    }
    if (population.size() > FrameEntry::kMaxVoters) {
        throw std::invalid_argument("population too large for 24-bit voter indices");
    }

    std::uniform_int_distribution<std::size_t> index_dist(0, population.size() - 1);
    Sample sample;

    if (order == DrawOrder::Sequential) {
        std::size_t index = index_dist(rng);
        for (std::size_t i = 0; i < n; ++i) {
            sample.respondents.push_back(FrameEntry(index, population[index]));
            index = (index + 1 == population.size()) ? 0 : index + 1;
        }
    } else {
        if (used.size() < population.size()) {
            throw std::invalid_argument("UsedPositions is smaller than the population");
        }

        // Redraw whenever an index has already been drawn.
        used.clear();
        while (sample.respondents.size() < n) {
            const std::size_t index = index_dist(rng);
            if (used.insert(index)) {
                sample.respondents.push_back(FrameEntry(index, population[index]));
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
            frame.push_back(FrameEntry(i, population[i]));
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
            frame.push_back(FrameEntry(i, population[i]));
        }
    }
    return frame;
}

Sample landline_sample(const Frame& frame,
                       const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                       std::size_t n, DrawOrder order, ResponseModel response,
                       UsedPositions& used, std::mt19937_64& rng) {
    Sample sample = phone_sample(frame, response_rate, n, order, response, used, rng);
    sample.weights.assign(sample.respondents.size(), 1.0);
    // Each voter has a single landline number, so nobody can respond twice: duplicates stays 0.
    return sample;
}

Sample mobile_sample(const Frame& frame,
                     const Arr<Arr<double, kCandidateCount>, kAgeGroupCount>& response_rate,
                     std::size_t n, DrawOrder order, ResponseModel response,
                     UsedPositions& used, UsedPositions& responded, std::mt19937_64& rng) {
    Sample sample = phone_sample(frame, response_rate, n, order, response, used, rng);
    for (const FrameEntry& respondent : sample.respondents) {
        sample.weights.push_back(1.0 / respondent.phones());
    }
    sample.duplicates = count_duplicates(sample.respondents, responded);
    return sample;
}

}  // namespace poll_sim
