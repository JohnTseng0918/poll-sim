#pragma once

#include "poll_sim/params.hpp"
#include "poll_sim/poll.hpp"
#include "poll_sim/types.hpp"

namespace poll_sim {

// A poll's estimate of one candidate's vote share and its standard error.
struct Estimate {
    double share = 0.0;
    double se = 0.0;
};

// Raw sample proportion, with SE = sqrt(p(1-p)/n).
Estimate unweighted_estimate(const PollResult& result, Candidate candidate);

// Post-stratification by age: sum over age groups of
// population_age_share[a] * (sample proportion within age group a),
// with SE = sqrt(sum of W_a^2 * p_a(1-p_a) / n_a).
// Age groups with no respondents are dropped and the remaining shares renormalized.
Estimate age_weighted_estimate(const PollResult& result, Candidate candidate,
                               const Arr<double, kAgeGroupCount>& population_age_share);

// Kish effective sample size of the age-weighted poll: (sum w)^2 / sum w^2.
double age_weighted_effective_size(const PollResult& result,
                                   const Arr<double, kAgeGroupCount>& population_age_share);

// The estimators below use the design weights in PollResult (1 / phones for mobile polls).
// Their SEs use the Kish approximation sqrt(p(1-p) / n_eff), which ignores any correlation
// between the weights and vote intention.

// Weighted sample proportion: sum of weights of the candidate's supporters / sum of weights,
// with n_eff = (sum w)^2 / sum w^2.
Estimate multiplicity_weighted_estimate(const PollResult& result, Candidate candidate);

// Post-stratification by age on top of the design weights: sum over age groups of
// population_age_share[a] * (weighted proportion within age group a),
// with SE = sqrt(sum of W_a^2 * p_a(1-p_a) / n_eff_a).
// Age groups with no respondents are dropped and the remaining shares renormalized.
Estimate multiplicity_age_weighted_estimate(
    const PollResult& result, Candidate candidate,
    const Arr<double, kAgeGroupCount>& population_age_share);

// Kish effective sample size of the multiplicity-weighted poll: (sum w)^2 / sum w^2.
double multiplicity_weighted_effective_size(const PollResult& result);

// Kish effective sample size with both multiplicity and age weights:
// 1 / sum(W_a^2 / n_eff_a).
double multiplicity_age_weighted_effective_size(
    const PollResult& result, const Arr<double, kAgeGroupCount>& population_age_share);

}  // namespace poll_sim
