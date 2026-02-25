#include "track_selector.h"

#include <iostream>
#include <set>
#include <sstream>

namespace marec {

std::vector<TrackInfo> TrackSelector::selectInteractively(
    const std::vector<TrackInfo>& audioTracks, bool allTracks)
{
    if (audioTracks.empty()) {
        std::cerr << "No audio tracks found in session.\n";
        return {};
    }

    if (allTracks) {
        return audioTracks;
    }

    std::cout << "\n=== MAREC -- Track Selection ===\n\n";
    std::cout << "Available audio tracks:\n";

    for (size_t i = 0; i < audioTracks.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << audioTracks[i].name << "\n";
    }

    std::cout << "\nSelection (e.g. 1,2,3 | all | q): ";
    std::string input;
    std::getline(std::cin, input);

    if (input == "q" || input == "Q") {
        return {};
    }

    if (input == "all" || input == "ALL") {
        return audioTracks;
    }

    // Parse comma-separated indices
    std::vector<TrackInfo> selected;
    std::set<int> seen;
    std::istringstream iss(input);
    std::string token;

    while (std::getline(iss, token, ',')) {
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        token = token.substr(start);
        size_t end = token.find_last_not_of(" \t");
        if (end != std::string::npos) token = token.substr(0, end + 1);

        try {
            int idx = std::stoi(token);
            if (idx >= 1 && idx <= static_cast<int>(audioTracks.size()) && seen.find(idx) == seen.end()) {
                selected.push_back(audioTracks[static_cast<size_t>(idx - 1)]);
                seen.insert(idx);
            } else if (seen.find(idx) == seen.end()) {
                std::cerr << "  Warning: index " << idx << " out of range, skipping.\n";
            }
        } catch (...) {
            std::cerr << "  Warning: '" << token << "' is not a valid number, skipping.\n";
        }
    }

    if (selected.empty()) {
        std::cerr << "No valid tracks selected.\n";
    }

    return selected;
}

} // namespace marec
