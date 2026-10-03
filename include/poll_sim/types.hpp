#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace poll_sim {

// Enumerators are numbered contiguously from 0 so they can index parameter tables directly.
// The underlying type is std::uint8_t to keep Voter small.

enum class AgeGroup : std::uint8_t {
    Young,   // 20-39
    Middle,  // 40-59
    Senior,  // 60+
};

enum class Region : std::uint8_t {
    North,
    Central,
    South,
    East,
};

enum class Gender : std::uint8_t {
    Male,
    Female,
};

enum class Candidate : std::uint8_t {
    DPP,
    KMT,
    None,
};

inline constexpr std::size_t kAgeGroupCount = 3;
inline constexpr std::size_t kRegionCount = 4;
inline constexpr std::size_t kGenderCount = 2;
inline constexpr std::size_t kCandidateCount = 3;

constexpr std::string_view to_string(AgeGroup v) noexcept {
    switch (v) {
        case AgeGroup::Young: return "Young";
        case AgeGroup::Middle: return "Middle";
        case AgeGroup::Senior: return "Senior";
    }
    return "Unknown";
}

constexpr std::string_view to_string(Region v) noexcept {
    switch (v) {
        case Region::North: return "North";
        case Region::Central: return "Central";
        case Region::South: return "South";
        case Region::East: return "East";
    }
    return "Unknown";
}

constexpr std::string_view to_string(Gender v) noexcept {
    switch (v) {
        case Gender::Male: return "Male";
        case Gender::Female: return "Female";
    }
    return "Unknown";
}

constexpr std::string_view to_string(Candidate v) noexcept {
    switch (v) {
        case Candidate::DPP: return "DPP";
        case Candidate::KMT: return "KMT";
        case Candidate::None: return "None";
    }
    return "Unknown";
}

}  // namespace poll_sim
