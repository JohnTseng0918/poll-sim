#pragma once

#include <cstddef>
#include <vector>

#include "poll_sim/params.hpp"
#include "poll_sim/sampling.hpp"
#include "poll_sim/voter.hpp"

namespace poll_sim {

// Results of one poll.
struct PollResult {
    Arr<std::size_t, kCandidateCount> counts{};    // votes per candidate
    Arr<std::size_t, kAgeGroupCount> age_counts{};  // respondents per age group
    // respondents per [age][candidate]
    Arr<Arr<std::size_t, kCandidateCount>, kAgeGroupCount> cell_counts{};
    std::size_t sample_size = 0;                    // number of respondents
    std::size_t contacts = 0;                       // number of voters contacted

    // Fraction of the sample supporting the given candidate.
    double share(Candidate candidate) const;
    // Fraction of the sample in the given age group.
    double share(AgeGroup age) const;
    // Fraction of contacted voters who responded.
    double response_rate() const;
};

// Counts the votes and age groups of the respondents.
PollResult tally(const std::vector<Voter>& population, const Sample& sample);

// Actual vote share of each candidate across the whole population.
Arr<double, kCandidateCount> population_vote_share(const std::vector<Voter>& population);

// Actual share of each age group across the whole population.
Arr<double, kAgeGroupCount> population_age_share(const std::vector<Voter>& population);

}  // namespace poll_sim
