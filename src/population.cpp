#include "poll_sim/population.hpp"

#include <cstdint>
#include <random>

namespace poll_sim {

std::vector<Voter> generate_population(const PopulationParams& params) {
    params.validate();

    std::mt19937_64 rng(params.seed);
    // Separate stream for coverage attributes, so changing them (or adding new ones)
    // does not shift the draws of the demographic attributes and vote intention.
    std::mt19937_64 coverage_rng(params.seed ^ 0x9E3779B97F4A7C15ULL);

    std::discrete_distribution<int> age_dist(params.age_share.begin(), params.age_share.end());
    std::discrete_distribution<int> region_dist(params.region_share.begin(),
                                                params.region_share.end());
    std::discrete_distribution<int> gender_dist(params.gender_share.begin(),
                                                params.gender_share.end());

    // Vote intention depends on age group: one distribution per age group.
    std::vector<std::discrete_distribution<int>> vote_dists;
    for (const auto& row : params.vote_share) {
        vote_dists.emplace_back(row.begin(), row.end());
    }

    // Landline ownership and phone count depend on [age][candidate].
    Arr<Arr<std::bernoulli_distribution, kCandidateCount>, kAgeGroupCount> landline_dists;
    Arr<Arr<std::discrete_distribution<int>, kCandidateCount>, kAgeGroupCount> phone_dists;
    for (std::size_t a = 0; a < kAgeGroupCount; ++a) {
        for (std::size_t c = 0; c < kCandidateCount; ++c) {
            landline_dists[a][c] = std::bernoulli_distribution(params.landline_share[a][c]);
            const auto& dist = params.phone_share[a][c];
            phone_dists[a][c] = std::discrete_distribution<int>(dist.begin(), dist.end());
        }
    }

    std::vector<Voter> population;
    for (std::size_t i = 0; i < params.size; ++i) {
        Voter voter;
        voter.age = static_cast<AgeGroup>(age_dist(rng));
        voter.region = static_cast<Region>(region_dist(rng));
        voter.gender = static_cast<Gender>(gender_dist(rng));
        voter.candidate = static_cast<Candidate>(vote_dists[static_cast<int>(voter.age)](rng));

        const auto a = static_cast<std::size_t>(voter.age);
        const auto c = static_cast<std::size_t>(voter.candidate);
        voter.has_landline = landline_dists[a][c](coverage_rng);
        voter.phones = static_cast<std::uint8_t>(phone_dists[a][c](coverage_rng));
        population.push_back(voter);
    }
    return population;
}

}  // namespace poll_sim
