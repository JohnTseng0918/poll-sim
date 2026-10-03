#include "poll_sim/stats.hpp"

#include <cmath>
#include <stdexcept>

namespace poll_sim {

namespace {

constexpr double kZ95 = 1.96;
constexpr double kThreePoints = 0.03;

}  // namespace

Summary summarize(const std::vector<Estimate>& estimates, double true_share) {
    if (estimates.empty()) {
        throw std::invalid_argument("estimates must not be empty");
    }

    const double replications = static_cast<double>(estimates.size());

    double sum = 0.0;
    double se_sum = 0.0;
    std::size_t covered = 0;
    std::size_t within = 0;
    for (const Estimate& estimate : estimates) {
        sum += estimate.share;
        se_sum += estimate.se;
        if (std::abs(estimate.share - true_share) <= kZ95 * estimate.se) {
            ++covered;
        }
        if (std::abs(estimate.share - true_share) <= kThreePoints) {
            ++within;
        }
    }

    Summary summary;
    summary.true_share = true_share;
    summary.mean_estimate = sum / replications;
    summary.bias = summary.mean_estimate - true_share;

    double squared_deviations = 0.0;
    for (const Estimate& estimate : estimates) {
        const double deviation = estimate.share - summary.mean_estimate;
        squared_deviations += deviation * deviation;
    }
    summary.empirical_sd =
        estimates.size() > 1 ? std::sqrt(squared_deviations / (replications - 1.0)) : 0.0;

    summary.mean_se = se_sum / replications;
    summary.margin_of_error = kZ95 * summary.empirical_sd;
    summary.coverage = covered / replications;
    summary.within_3pct = within / replications;
    return summary;
}

}  // namespace poll_sim
