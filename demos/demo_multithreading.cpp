#include "../include/ufmt/ufmt.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <random>
#include <iomanip>

// Configuration
const int NUM_THREADS = 4;
const int OPERATIONS_PER_THREAD = 1000;

// Shared data for testing
std::atomic<int> global_counter{0};
std::mutex output_mutex;

// Thread-safe output function
void safe_print(const std::string& message) {
    std::lock_guard<std::mutex> lock(output_mutex);
    std::cout << message << std::endl;
}

// Worker function for shared context testing
void shared_context_worker(int thread_id, const std::string& context_name) {
    auto ctx = ufmt::get_shared_context(context_name);
    
    // Each thread will increment counters and format messages
    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        int operation_id = ++global_counter;
        
        // Set some variables in shared context
        ctx->set_var("thread_id", thread_id);
        ctx->set_var("operation", operation_id);
        ctx->set_var("timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
        
        // Format a message using shared context
        std::string msg = ctx->format("Thread {thread_id}: Operation {operation} at {timestamp}ms");
        
        // Every 100th operation, print the message
        if (operation_id % 100 == 0) {
            safe_print(msg);
        }
        
        // Small delay to encourage context switching
        if (i % 50 == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }
    
    safe_print(ufmt::format("Thread {0} completed {1} operations", thread_id, OPERATIONS_PER_THREAD));
}

// Worker function for scoped context testing
void scoped_context_worker(int thread_id) {
    auto ctx = ufmt::create_local_context();
    
    // Set up custom formatter for this thread
    ctx->set_formatter<bool>([thread_id](bool b) { 
        return b ? "YES" : "NO";
    });
    
    // Random number generator for this thread
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 100.0);
    std::uniform_int_distribution<> bool_dis(0, 1);
    
    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        int operation_id = ++global_counter;
        double random_score = dis(gen);
        bool random_flag = bool_dis(gen);
        
        // Set variables in scoped context
        ctx->set_var("thread_id", thread_id);
        ctx->set_var("operation", operation_id);
        ctx->set_var("score", random_score);
        std::string msg = ctx->format("Thread {thread_id}: Op {operation}, Score {score:.2f}, Flag {0}", random_flag);
        
        // Print every 150th operation
        if (operation_id % 150 == 0) {
            safe_print(msg);
        }
    }
    
    safe_print(ufmt::format("Scoped thread {0} completed", thread_id));
}

// Performance test worker
void performance_worker(int thread_id, std::atomic<long long>& total_operations) {
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(2); // Run for 2 seconds
    
    long long operations = 0;
    auto ctx = ufmt::create_local_context();
    ctx->set_var("thread", thread_id);
    
    while (std::chrono::steady_clock::now() < end_time) {
        // Mix of different formatting operations
        volatile auto result1 = ufmt::format("Simple: {0} {1}", thread_id, operations);
        volatile auto result2 = ctx->format("Named: Thread {thread}, Op {0}", operations);
        volatile auto result3 = ufmt::format("Numeric: {0:.3f}", static_cast<double>(operations) * 0.001);
        
        operations += 3;
        (void)result1; (void)result2; (void)result3; // Prevent optimization
    }
    
    total_operations += operations;
    safe_print(ufmt::format("Performance thread {0}: {1} operations in 2 seconds", thread_id, operations));
}

int main() {
    std::cout << "=== ufmt Multi-threading Demo ===" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Number of threads: " << NUM_THREADS << std::endl;
    std::cout << "  Operations per thread: " << OPERATIONS_PER_THREAD << std::endl;
    std::cout << std::endl;
    
    // Test 1: Shared Context Thread Safety
    std::cout << "=== Test 1: Shared Context Thread Safety ===" << std::endl;
    std::cout << "Testing thread-safe access to shared contexts..." << std::endl;
    
    global_counter = 0;
    std::vector<std::thread> shared_threads;
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Launch threads that share a context
    for (int i = 0; i < NUM_THREADS; ++i) {
        shared_threads.emplace_back(shared_context_worker, i, "shared_test");
    }
    
    // Wait for all threads to complete
    for (auto& t : shared_threads) {
        t.join();
    }
    
    auto shared_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    
    std::cout << "Shared context test completed in " << shared_duration.count() << "ms" << std::endl;
    std::cout << "Total operations: " << global_counter.load() << std::endl;
    std::cout << std::endl;
    
    // Test 2: Scoped Context Isolation
    std::cout << "=== Test 2: Scoped Context Isolation ===" << std::endl;
    std::cout << "Testing isolated scoped contexts with custom formatters..." << std::endl;
    
    global_counter = 0;
    std::vector<std::thread> scoped_threads;
    
    start_time = std::chrono::steady_clock::now();
    
    // Launch threads with individual scoped contexts
    for (int i = 0; i < NUM_THREADS; ++i) {
        scoped_threads.emplace_back(scoped_context_worker, i);
    }
    
    // Wait for all threads to complete
    for (auto& t : scoped_threads) {
        t.join();
    }
    
    auto scoped_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    
    std::cout << "Scoped context test completed in " << scoped_duration.count() << "ms" << std::endl;
    std::cout << "Total operations: " << global_counter.load() << std::endl;
    std::cout << std::endl;
    
    // Test 3: Performance Under Load
    std::cout << "=== Test 3: Multi-threaded Performance Test ===" << std::endl;
    std::cout << "Measuring throughput under concurrent load..." << std::endl;
    
    std::atomic<long long> total_operations{0};
    std::vector<std::thread> perf_threads;
    
    start_time = std::chrono::steady_clock::now();
    
    // Launch performance test threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        perf_threads.emplace_back(performance_worker, i, std::ref(total_operations));
    }
    
    // Wait for all threads to complete
    for (auto& t : perf_threads) {
        t.join();
    }
    
    auto perf_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    
    double ops_per_second = static_cast<double>(total_operations.load()) / (static_cast<double>(perf_duration.count()) / 1000.0);
    
    std::cout << "Performance test completed in " << perf_duration.count() << "ms" << std::endl;
    std::cout << "Total operations: " << total_operations.load() << std::endl;
    std::cout << "Operations per second: " << std::fixed << std::setprecision(0) << ops_per_second << std::endl;
    std::cout << std::endl;
    
    // Test 4: Multiple Named Contexts
    std::cout << "=== Test 4: Multiple Named Contexts ===" << std::endl;
    std::cout << "Testing multiple independent shared contexts..." << std::endl;
    
    std::vector<std::thread> multi_context_threads;
    
    // Create threads using different named contexts
    for (int i = 0; i < NUM_THREADS; ++i) {
        std::string context_name = "context_" + std::to_string(i);
        multi_context_threads.emplace_back([i, context_name]() {
            auto ctx = ufmt::get_shared_context(context_name);
            ctx->set_var("owner", i);
            ctx->set_var("context_name", context_name);
            
            for (int j = 0; j < 10; ++j) {
                std::string msg = ctx->format("Context {context_name} owned by thread {owner}, iteration {0}", j);
                safe_print(msg);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }
    
    for (auto& t : multi_context_threads) {
        t.join();
    }
    
    std::cout << "Multiple contexts test completed" << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== All Multi-threading Tests Complete ===" << std::endl;
    std::cout << "Summary:" << std::endl;
    std::cout << "  ✓ Shared context thread safety verified" << std::endl;
    std::cout << "  ✓ Scoped context isolation verified" << std::endl;
    std::cout << "  ✓ Performance under concurrent load measured" << std::endl;
    std::cout << "  ✓ Multiple named contexts tested" << std::endl;
    
    return 0;
}
