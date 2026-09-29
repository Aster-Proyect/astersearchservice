#include "../include/search_engine.h"
#include "../include/prefix_tree.h"
#include "../include/inverted_index.h"
#include "../include/aster_branding.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <sys/stat.h>

long get_memory_kb() {
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("VmRSS:", 0) == 0) {
            size_t pos = line.find_first_of("0123456789");
            return std::stol(line.substr(pos));
        }
    }
    return -1;
}

int main(int argc, char* argv[]) {
    std::string dir = (argc >= 2) ? argv[1] : "./benchmark_data";

    // Generate filename with timestamp
    auto now = std::time(nullptr);
    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));
    std::string filename = "benchmark_results/aster_search_benchmark_" + std::string(timestamp) + ".txt";

    mkdir("benchmark_results", 0755);
    std::ofstream out(filename);

    out << "=== " << AsterSearch::DISPLAY_NAME << " v" << AsterSearch::VERSION << " BENCHMARK ===\n";
    out << "Directory: " << dir << "\n";
    out << "Timestamp: " << std::ctime(&now) << "\n";

    // --- Build ---
    long mem_before = get_memory_kb();
    auto start = std::chrono::high_resolution_clock::now();

    SearchEngine engine;
    engine.build(dir);

    auto end = std::chrono::high_resolution_clock::now();
    long mem_after = get_memory_kb();
    auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    out << "Build time: " << build_ms << " ms\n";
    out << "Memory before: " << mem_before << " kB\n";
    out << "Memory after:  " << mem_after << " kB\n";
    out << "Memory used:   " << (mem_after - mem_before) << " kB\n\n";

    // --- Queries ---
    std::vector<std::string> exact_queries = {
        "apple", "banana", "memory", "stack", "queue",
        "algorithm", "database", "pointer", "thread"
    };

    std::vector<std::string> prefix_queries = {
        "ap", "ba", "me", "st", "qu",
        "al", "da", "po", "th", "re"
    };

    long total_us = 0;
    // --- Exact Filename Search (PrefixTree) ---
    const auto& pt = engine.get_prefix_tree();
    out << "--- Exact Filename Search (PrefixTree) ---\n";
    total_us = 0;
    for (const auto& q : exact_queries) {
        start = std::chrono::high_resolution_clock::now();
        auto results = pt.search(q);
        end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        total_us += us;
        out << "  \"" << q << "\": " << us << " µs, " << results.size() << " results\n";
    }
    out << "  Average: " << (total_us / exact_queries.size()) << " µs\n\n";

    // --- Prefix Filename Search (PrefixTree) ---
    out << "--- Prefix Filename Search (PrefixTree) ---\n";
    total_us = 0;
    for (const auto& q : prefix_queries) {
        start = std::chrono::high_resolution_clock::now();
        auto results = pt.search_matching(q);
        end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        total_us += us;
        out << "  \"" << q << "\": " << us << " µs, " << results.size() << " results\n";
    }
    out << "  Average: " << (total_us / prefix_queries.size()) << " µs\n\n";

    // --- Content Search (InvertedIndex) ---
    const auto& idx = engine.get_inverted_index();
    out << "--- Content Search (Inverted Index) ---\n";
    total_us = 0;
    for (const auto& q : exact_queries) {
        start = std::chrono::high_resolution_clock::now();
        auto results = idx.search(q);
        end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        total_us += us;
        out << "  \"" << q << "\": " << us << " µs, " << results.size() << " results\n";
    }
    out << "  Average: " << (total_us / exact_queries.size()) << " µs\n\n";

    // --- Combined Search (SearchEngine) ---
    out << "--- Combined Search (SearchEngine) ---\n";
    total_us = 0;
    for (const auto& q : exact_queries) {
        start = std::chrono::high_resolution_clock::now();
        auto results = engine.search(q);
        end = std::chrono::high_resolution_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        total_us += us;
        out << "  \"" << q << "\": " << us << " µs, " << results.size() << " results\n";
    }
    out << "  Average: " << (total_us / exact_queries.size()) << " µs\n\n";

    // --- Final ---
    out << "Final memory: " << get_memory_kb() << " kB\n";

    out.close();
    std::cout << "Benchmark written to " << filename << "\n";
    return 0;
}
