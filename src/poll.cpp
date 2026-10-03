#include "poll_sim/poll.hpp"

namespace poll_sim {

double PollResult::share(Candidate candidate) const {
    return static_cast<double>(counts[static_cast<std::size_t>(candidate)]) / sample_size;
}

double PollResult::share(AgeGroup age) const {
    return static_cast<double>(age_counts[static_cast<std::size_t>(age)]) / sample_size;
}

double PollResult::response_rate() const {
    return static_cast<double>(sample_size) / contacts;
}

PollResult tally(const std::vector<Voter>& population, const Sample& sample) {
    PollResult result;
    for (std::size_t index : sample.respondents) {
        const Voter& voter = population[index];
        ++result.counts[static_cast<std::size_t>(voter.candidate)];
        ++result.age_counts[static_cast<std::size_t>(voter.age)];
        ++result.cell_counts[static_cast<std::size_t>(voter.age)]
                            [static_cast<std::size_t>(voter.candidate)];
    }
    result.sample_size = sample.respondents.size();
    result.contacts = sample.contacts;
    return result;
}

Arr<double, kCandidateCount> population_vote_share(const std::vector<Voter>& population) {
    Arr<std::size_t, kCandidateCount> counts{};
    for (const Voter& voter : population) {
        ++counts[static_cast<std::size_t>(voter.candidate)];
    }

    Arr<double, kCandidateCount> shares{};
    for (std::size_t c = 0; c < kCandidateCount; ++c) {
        shares[c] = static_cast<double>(counts[c]) / population.size();
    }
    return shares;
}

Arr<double, kAgeGroupCount> population_age_share(const std::vector<Voter>& population) {
    Arr<std::size_t, kAgeGroupCount> counts{};
    for (const Voter& voter : population) {
        ++counts[static_cast<std::size_t>(voter.age)];
    }

    Arr<double, kAgeGroupCount> shares{};
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        shares[a] = static_cast<double>(counts[a]) / population.size();
    }
    return shares;
}

}  // namespace poll_sim
