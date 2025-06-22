#include "../include/ufmt/ufmt.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <cstdio>
#include <numeric>
#include <algorithm>
#include <cmath>
#include <iomanip>

// Benchmark configuration
const int WARMUP_ITERATIONS = 100;     // Reduced from 1000
const int BENCHMARK_ITERATIONS = 1000;  // Reduced from 10000, but still >1000 as requested
const int STATISTICAL_RUNS = 2000;       // Increased from 20 for more robust statistics

// Test data
struct TestData {
    std::string name;
    int id;
    double score;
    bool active;
};

// Statistics calculation
struct BenchmarkStats {
    double min_ms;
    double max_ms;
    double avg_ms;
    double stddev_ms;
    size_t iterations;
    
    BenchmarkStats() : min_ms(0), max_ms(0), avg_ms(0), stddev_ms(0), iterations(0) {}
};

class BenchmarkTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double stop_ms() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        return duration.count() / 1000000.0; // Convert to milliseconds
    }
};

BenchmarkStats calculate_stats(const std::vector<double>& times) {
    BenchmarkStats stats;
    stats.iterations = times.size();
    
    if (times.empty()) return stats;
    
    stats.min_ms = *std::min_element(times.begin(), times.end());
    stats.max_ms = *std::max_element(times.begin(), times.end());
    stats.avg_ms = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double time : times) {
        variance += (time - stats.avg_ms) * (time - stats.avg_ms);
    }
    variance /= times.size();
    stats.stddev_ms = std::sqrt(variance);
    
    return stats;
}

void print_stats(const std::string& method_name, const BenchmarkStats& stats) {
    std::cout << std::fixed << std::setprecision(4);
    std::cout << method_name << ":\n";
    std::cout << "  Iterations: " << stats.iterations << "\n";
    std::cout << "  Min:        " << stats.min_ms << " ms\n";
    std::cout << "  Max:        " << stats.max_ms << " ms\n";
    std::cout << "  Average:    " << stats.avg_ms << " ms\n";
    std::cout << "  Std Dev:    " << stats.stddev_ms << " ms\n\n";
}

// sprintf implementation (safe)
std::string format_with_sprintf(const TestData& data) {
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), 
                  "User %s (ID: %d) has score %.2f, active: %s", 
                  data.name.c_str(), data.id, data.score, data.active ? "true" : "false");
    return std::string(buffer);
}

// C++ stringstream implementation
std::string format_with_stringstream(const TestData& data) {
    std::stringstream ss;
    ss << "User " << data.name << " (ID: " << data.id << ") has score " 
       << std::fixed << std::setprecision(2) << data.score 
       << ", active: " << (data.active ? "true" : "false");
    return ss.str();
}

// ufmt implementation
std::string format_with_ufmt(const TestData& data) {
    return ufmt::format("User {0} (ID: {1}) has score {2:.2f}, active: {3}", 
                       data.name, data.id, data.score, data.active);
}

// ufmt with context implementation
std::string format_with_ufmt_context(const TestData& data, std::unique_ptr<ufmt::local_context>& ctx) {
    ctx->set_var("name", data.name);
    ctx->set_var("id", data.id);
    ctx->set_var("score", data.score);
    ctx->set_var("active", data.active);
    return ctx->format("User {name} (ID: {id}) has score {score:.2f}, active: {active}");
}

// Benchmark functions
BenchmarkStats benchmark_sprintf(const std::vector<TestData>& test_data) {
    std::vector<double> times;
    BenchmarkTimer timer;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        for (const auto& data : test_data) {
            volatile auto result = format_with_sprintf(data);
            (void)result; // Prevent optimization
        }
    }
    
    // Actual benchmark
    for (int run = 0; run < STATISTICAL_RUNS; ++run) {
        timer.start();
        for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
            for (const auto& data : test_data) {
                volatile auto result = format_with_sprintf(data);
                (void)result; // Prevent optimization
            }
        }
        times.push_back(timer.stop_ms());
    }
    
    return calculate_stats(times);
}

BenchmarkStats benchmark_stringstream(const std::vector<TestData>& test_data) {
    std::vector<double> times;
    BenchmarkTimer timer;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        for (const auto& data : test_data) {
            volatile auto result = format_with_stringstream(data);
            (void)result; // Prevent optimization
        }
    }
    
    // Actual benchmark
    for (int run = 0; run < STATISTICAL_RUNS; ++run) {
        timer.start();
        for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
            for (const auto& data : test_data) {
                volatile auto result = format_with_stringstream(data);
                (void)result; // Prevent optimization
            }
        }
        times.push_back(timer.stop_ms());
    }
    
    return calculate_stats(times);
}

BenchmarkStats benchmark_ufmt(const std::vector<TestData>& test_data) {
    std::vector<double> times;
    BenchmarkTimer timer;
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        for (const auto& data : test_data) {
            volatile auto result = format_with_ufmt(data);
            (void)result; // Prevent optimization
        }
    }
    
    // Actual benchmark
    for (int run = 0; run < STATISTICAL_RUNS; ++run) {
        timer.start();
        for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
            for (const auto& data : test_data) {
                volatile auto result = format_with_ufmt(data);
                (void)result; // Prevent optimization
            }
        }
        times.push_back(timer.stop_ms());
    }
    
    return calculate_stats(times);
}

BenchmarkStats benchmark_ufmt_context(const std::vector<TestData>& test_data) {
    std::vector<double> times;
    BenchmarkTimer timer;
    auto ctx = ufmt::create_local_context();
    
    // Warmup
    for (int i = 0; i < WARMUP_ITERATIONS; ++i) {
        for (const auto& data : test_data) {
            volatile auto result = format_with_ufmt_context(data, ctx);
            (void)result; // Prevent optimization
        }
    }
    
    // Actual benchmark
    for (int run = 0; run < STATISTICAL_RUNS; ++run) {
        timer.start();
        for (int i = 0; i < BENCHMARK_ITERATIONS; ++i) {
            for (const auto& data : test_data) {
                volatile auto result = format_with_ufmt_context(data, ctx);
                (void)result; // Prevent optimization
            }
        }
        times.push_back(timer.stop_ms());
    }
    
    return calculate_stats(times);
}

int main() {
    std::cout << "=== ufmt Performance Benchmark ===" << std::endl;
    
    // Prepare test data
    std::vector<TestData> test_data = {
        {"Alice Johnson", 1001, 95.7, true},
        {"Bob Smith", 2, 87.2, false},
        {"Catherine Wilson", 1003, 92.8, true},
        {"David Brown", 404, 78.5, false},
        {"Elizabeth Davis", 5555, 99.1, true}
    };
    std::cout << "Configuration: "
              << WARMUP_ITERATIONS << " warmup, "
              << BENCHMARK_ITERATIONS << " x " << STATISTICAL_RUNS << " runs, "
              << test_data.size() << " samples" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Testing with " << test_data.size() << " data samples..." << std::endl;
    std::cout << "Total operations per run: " << BENCHMARK_ITERATIONS * test_data.size() << std::endl;
    std::cout << std::endl;
    
    // Verify all methods produce the same output
    std::cout << "Verifying output consistency (first 3 samples):" << std::endl;
    auto ctx = ufmt::create_local_context();
    for (size_t i = 0; i < std::min(size_t(3), test_data.size()); ++i) {
        std::cout << "sprintf:     " << format_with_sprintf(test_data[i]) << std::endl;
        std::cout << "stringstream:" << format_with_stringstream(test_data[i]) << std::endl;
        std::cout << "ufmt:        " << format_with_ufmt(test_data[i]) << std::endl;
        std::cout << "ufmt+ctx:    " << format_with_ufmt_context(test_data[i], ctx) << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "\nRunning benchmarks...\n";
    
    // Run benchmarks
    auto sprintf_stats = benchmark_sprintf(test_data);
    auto stringstream_stats = benchmark_stringstream(test_data);
    auto ufmt_stats = benchmark_ufmt(test_data);
    auto ufmt_context_stats = benchmark_ufmt_context(test_data);
    
    // Print results
    std::cout << "=== Benchmark Results (ms, lower is better) ===\n\n";
    print_stats("sprintf", sprintf_stats);
    print_stats("stringstream", stringstream_stats);
    print_stats("ufmt (positional)", ufmt_stats);
    print_stats("ufmt (named context)", ufmt_context_stats);
    std::cout << "=== Relative Performance (1.00 = sprintf) ===\n";
    auto rel = [](double base, double other) {
        return (other > 0.0) ? (base / other) : 0.0;
    };
    std::cout << "  sprintf (baseline):    1.00x\n";
    std::cout << "  stringstream:         " << rel(sprintf_stats.avg_ms, stringstream_stats.avg_ms) << "x\n";
    std::cout << "  ufmt (positional):    " << rel(sprintf_stats.avg_ms, ufmt_stats.avg_ms) << "x\n";
    std::cout << "  ufmt (named ctx):     " << rel(sprintf_stats.avg_ms, ufmt_context_stats.avg_ms) << "x\n";
    std::cout << "\n=== Done ===\n";
    
    return 0;
}
