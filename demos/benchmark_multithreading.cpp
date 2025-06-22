#include "../include/ufmt/ufmt.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cmath>

// Benchmark configuration
const int WARMUP_SECONDS = 1;
const int BENCHMARK_SECONDS = 1;       // Reduced from 2 seconds to 1 second
const int STATISTICAL_RUNS = 2;        // Reduced from 3 to 2
const std::vector<int> THREAD_COUNTS = {1, 2, 4}; // Keep 3 thread counts

struct ThreadBenchmarkResult {
    int thread_count;
    double ops_per_second;
    double avg_latency_us;
    long long total_operations;
    double duration_seconds;
};

struct BenchmarkStats {
    double min_ops;
    double max_ops;
    double avg_ops;
    double stddev_ops;
    
    BenchmarkStats() : min_ops(0), max_ops(0), avg_ops(0), stddev_ops(0) {}
};

BenchmarkStats calculate_stats(const std::vector<double>& values) {
    BenchmarkStats stats;
    
    if (values.empty()) return stats;
    
    stats.min_ops = *std::min_element(values.begin(), values.end());
    stats.max_ops = *std::max_element(values.begin(), values.end());
    stats.avg_ops = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    
    // Calculate standard deviation
    double variance = 0.0;
    for (double val : values) {
        variance += (val - stats.avg_ops) * (val - stats.avg_ops);
    }
    variance /= values.size();
    stats.stddev_ops = std::sqrt(variance);
    
    return stats;
}

// Worker function for benchmarking
void benchmark_worker(int thread_id, std::atomic<bool>& running, std::atomic<long long>& operations_counter,
                     std::chrono::steady_clock::time_point& /* start_time */) {
    auto ctx = ufmt::create_local_context();
    ctx->set_var("thread_id", thread_id);
    
    long long local_ops = 0;
    
    while (running.load()) {
        // Mix of different formatting operations to simulate real usage
        volatile auto result1 = ufmt::format("Simple: {0} {1}", thread_id, local_ops);
        volatile auto result2 = ctx->format("Named: Thread {thread_id}, Op {0}", local_ops);
        volatile auto result3 = ufmt::format("Numeric: {0:.3f} {1:x}", local_ops * 0.001, local_ops);
        volatile auto result4 = ctx->format("Complex: T{thread_id} #{0} Score:{1:.2f}", local_ops, local_ops * 0.01);
        
        local_ops += 4;
        
        // Prevent optimization
        (void)result1; (void)result2; (void)result3; (void)result4;
    }
    
    operations_counter += local_ops;
}

// Shared context worker
void shared_context_worker(int thread_id, std::atomic<bool>& running, std::atomic<long long>& operations_counter,
                          std::chrono::steady_clock::time_point& /* start_time */, const std::string& context_name) {
    auto ctx = ufmt::get_shared_context(context_name);
    
    long long local_ops = 0;
    
    while (running.load()) {
        ctx->set_var("thread_id", thread_id);
        ctx->set_var("operation", local_ops);
        
        volatile auto result1 = ctx->format("Shared: Thread {thread_id}, Op {operation}");
        volatile auto result2 = ufmt::format("Mixed: {0} from shared context", thread_id);
        
        local_ops += 2;
        
        // Prevent optimization
        (void)result1; (void)result2;
    }
    
    operations_counter += local_ops;
}

ThreadBenchmarkResult run_local_context_benchmark(int num_threads, int duration_seconds) {
    std::atomic<bool> running{false};
    std::atomic<long long> operations_counter{0};
    std::vector<std::thread> threads;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Start worker threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(benchmark_worker, i, std::ref(running), std::ref(operations_counter), std::ref(start_time));
    }
    
    // Start the benchmark
    start_time = std::chrono::steady_clock::now();
    running = true;
    
    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    
    // Stop the benchmark
    running = false;
    auto end_time = std::chrono::steady_clock::now();
    
    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }
    
    auto actual_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double duration_sec = actual_duration.count() / 1000000.0;
    
    ThreadBenchmarkResult result;
    result.thread_count = num_threads;
    result.total_operations = operations_counter.load();
    result.duration_seconds = duration_sec;
    result.ops_per_second = result.total_operations / duration_sec;
    result.avg_latency_us = (duration_sec * 1000000.0) / result.total_operations;
    
    return result;
}

ThreadBenchmarkResult run_shared_context_benchmark(int num_threads, int duration_seconds) {
    std::atomic<bool> running{false};
    std::atomic<long long> operations_counter{0};
    std::vector<std::thread> threads;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Start worker threads (all sharing the same context)
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(shared_context_worker, i, std::ref(running), std::ref(operations_counter), 
                           std::ref(start_time), "shared_benchmark");
    }
    
    // Start the benchmark
    start_time = std::chrono::steady_clock::now();
    running = true;
    
    // Run for specified duration
    std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
    
    // Stop the benchmark
    running = false;
    auto end_time = std::chrono::steady_clock::now();
    
    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }
    
    auto actual_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    double duration_sec = actual_duration.count() / 1000000.0;
    
    ThreadBenchmarkResult result;
    result.thread_count = num_threads;
    result.total_operations = operations_counter.load();
    result.duration_seconds = duration_sec;
    result.ops_per_second = result.total_operations / duration_sec;
    result.avg_latency_us = (duration_sec * 1000000.0) / result.total_operations;
    
    return result;
}

int main() {
    std::cout << "=== ufmt Multi-threading Benchmark ===\n";
    std::cout << "Config: "
              << WARMUP_SECONDS << "s warmup, "
              << BENCHMARK_SECONDS << "s/run, "
              << STATISTICAL_RUNS << " runs, threads: ";
    for (size_t i = 0; i < THREAD_COUNTS.size(); ++i) {
        std::cout << THREAD_COUNTS[i];
        if (i < THREAD_COUNTS.size() - 1) std::cout << ", ";
    }
    std::cout << ", HW: " << std::thread::hardware_concurrency() << std::endl << std::endl;
    
    // Warmup
    auto warmup_result = run_local_context_benchmark(2, WARMUP_SECONDS);
    
    // Benchmark 1: Local Context Performance
    std::vector<ThreadBenchmarkResult> local_results;
    double baseline_local_ops = 0.0;
    for (int thread_count : THREAD_COUNTS) {
        std::vector<double> ops_per_sec_runs;
        for (int run = 0; run < STATISTICAL_RUNS; ++run) {
            auto result = run_local_context_benchmark(thread_count, BENCHMARK_SECONDS);
            ops_per_sec_runs.push_back(result.ops_per_second);
            if (run == 0) local_results.push_back(result);
        }
        auto stats = calculate_stats(ops_per_sec_runs);
        local_results.back().ops_per_second = stats.avg_ops;
        if (baseline_local_ops == 0.0) baseline_local_ops = stats.avg_ops;
    }
    // Print compact local table
    std::cout << "[Local Contexts]\nThreads  |  Avg Ops/sec" << std::endl;
    for (const auto& r : local_results) {
        std::cout << std::setw(7) << r.thread_count << "  |  " << std::fixed << std::setprecision(0) << r.ops_per_second << std::endl;
    }
    // Benchmark 2: Shared Context Performance
    std::vector<ThreadBenchmarkResult> shared_results;
    double baseline_shared_ops = 0.0;
    for (int thread_count : THREAD_COUNTS) {
        std::vector<double> ops_per_sec_runs;
        for (int run = 0; run < STATISTICAL_RUNS; ++run) {
            auto result = run_shared_context_benchmark(thread_count, BENCHMARK_SECONDS);
            ops_per_sec_runs.push_back(result.ops_per_second);
            if (run == 0) shared_results.push_back(result);
        }
        auto stats = calculate_stats(ops_per_sec_runs);
        shared_results.back().ops_per_second = stats.avg_ops;
        if (baseline_shared_ops == 0.0) baseline_shared_ops = stats.avg_ops;
    }
    // Print compact shared table
    std::cout << "\n[Shared Contexts]\nThreads  |  Avg Ops/sec" << std::endl;
    for (const auto& r : shared_results) {
        std::cout << std::setw(7) << r.thread_count << "  |  " << std::fixed << std::setprecision(0) << r.ops_per_second << std::endl;
    }
    // Print summary ratio
    std::cout << "\n[Summary: Local/Shared Ratio]\nThreads  |  Ratio (L/Sh)" << std::endl;
    for (size_t i = 0; i < local_results.size() && i < shared_results.size(); ++i) {
        double ratio = local_results[i].ops_per_second / shared_results[i].ops_per_second;
        std::cout << std::setw(7) << local_results[i].thread_count << "  |  " << std::fixed << std::setprecision(2) << ratio << "x" << std::endl;
    }
    std::cout << "\n=== Done ===\n";
    return 0;
}
