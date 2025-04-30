#include <iostream>
#include <chrono>
#include <unordered_map>
#include <string>
#include <format> // Requires C++20

// Global map definition (assuming it's needed)
struct TimerResult {
    float allTimes = 0;
    float sampleCount = 0;
};
// Use 'inline' if defined in a header to prevent multiple definition errors
inline std::unordered_map<std::string, TimerResult> g_timerResults;

// Original Timer class structure
struct Timer {

    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
    std::string m_name;

    Timer(const std::string& name) : m_name(name) { // Use initializer list
        m_startTime = std::chrono::steady_clock::now();
    }

    ~Timer() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<float, std::milli> duration = now - m_startTime;
        // This 'time' variable should now be correct if no copies were made
        float time = duration.count();

        // Calculate spacing (ensure extraSpaces isn't negative)
        std::string spacing;
        int nameLength = static_cast<int>(m_name.length());
        int extraSpaces = 50 - nameLength;
        if (extraSpaces < 0) extraSpaces = 0;
        for (int i = 0; i < extraSpaces; i++) {
            spacing += " ";
        }

        // Update global map and calculate average
        // Note: Accessing map creates entry if it doesn't exist
        TimerResult& result = g_timerResults[m_name]; // Get reference
        result.allTimes += time;
        result.sampleCount++;
        // Avoid division by zero if sampleCount somehow became 0
        float avg = (result.sampleCount > 0) ? (result.allTimes / result.sampleCount) : 0.0f;

        // Print results
        std::cout << m_name << ":" << spacing
            << std::format("{:.4f}", time) << "ms      average: "
            << std::format("{:.4f}", avg) << "ms\n";
        std::cout.flush(); // Good practice to flush
    }

    // --- Add these lines to prevent copying/moving ---
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;
};
