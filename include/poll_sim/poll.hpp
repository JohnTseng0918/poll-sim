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
    // Sum of design weights per [age][candidate], and sum of squared weights per age group.
    // Equal to the counts when every weight is 1.
    Arr<Arr<double, kCandidateCount>, kAgeGroupCount> cell_weights{};
    Arr<double, kAgeGroupCount> age_weight_squares{};
    std::size_t sample_size = 0;                    // number of respondents
    std::size_t contacts = 0;                       // number of voters contacted
    std::size_t duplicates = 0;                     // repeat responses from the same voter

    // Fraction of the sample supporting the given candidate.
    double share(Candidate candidate) const;
    // Fraction of the sample in the given age group.
    double share(AgeGroup age) const;
    // Fraction of contacted voters who responded.
    double response_rate() const;
};

// Counts the votes and age groups of the respondents and sums their design weights.
// Reads only the sample: each respondent carries its own age group and candidate.
PollResult tally(const Sample& sample);

// Actual vote share of each candidate across the whole population.
Arr<double, kCandidateCount> population_vote_share(const std::vector<Voter>& population);

// Actual vote share of each candidate within each age group, indexed [age][candidate].
Arr<Arr<double, kCandidateCount>, kAgeGroupCount> population_age_vote_share(
    const std::vector<Voter>& population);

// Actual share of each age group across the whole population.
Arr<double, kAgeGroupCount> population_age_share(const std::vector<Voter>& population);

}  // namespace poll_sim
