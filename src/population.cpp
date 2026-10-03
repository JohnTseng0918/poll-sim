#include "poll_sim/population.hpp"

#include <random>

namespace poll_sim {

std::vector<Voter> generate_population(const PopulationParams& params) {
    params.validate();

    std::mt19937_64 rng(params.seed);

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

    std::vector<Voter> population;
    for (std::size_t i = 0; i < params.size; ++i) {
        Voter voter;
        voter.age = static_cast<AgeGroup>(age_dist(rng));
        voter.region = static_cast<Region>(region_dist(rng));
        voter.gender = static_cast<Gender>(gender_dist(rng));
        voter.candidate = static_cast<Candidate>(vote_dists[static_cast<int>(voter.age)](rng));
        population.push_back(voter);
    }
    return population;
}

}  // namespace poll_sim
