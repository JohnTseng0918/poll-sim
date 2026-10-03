#pragma once

#include <vector>

#include "poll_sim/estimate.hpp"

namespace poll_sim {

// Summary of one candidate's estimates across all replications.
// All values are proportions (0.45 = 45%).
struct Summary {
    double true_share = 0.0;      // actual share in the population
    double mean_estimate = 0.0;   // mean of the per-replication estimates
    double bias = 0.0;            // mean_estimate - true_share
    double empirical_sd = 0.0;    // standard deviation of the estimates
    double mean_se = 0.0;         // mean of the per-replication standard errors
    double margin_of_error = 0.0; // 1.96 * empirical_sd
    double coverage = 0.0;        // fraction of 95% CIs (share ± 1.96 se) containing true_share
    double within_3pct = 0.0;     // fraction of estimates within ±3 points of true_share
};

// Throws std::invalid_argument if estimates is empty.
Summary summarize(const std::vector<Estimate>& estimates, double true_share);

}  // namespace poll_sim
