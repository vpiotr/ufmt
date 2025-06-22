#include "../include/ufmt/ufmt.h"
#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::cout << "=== Transparent Thread-Local API Demo ===\n\n";
    
    // Create a shared context for demonstration
    auto ctx = ufmt::get_shared_context("transparent_demo");
    
    std::cout << "1. Main thread sets global variables:\n";
    ctx->set_var("app_name", "TransparentApp");
    ctx->set_var("log_level", "INFO");
    ctx->set_var("main_thread_var", "main_value");
    
    std::cout << "   Main thread view: " << ctx->format("{app_name} [{log_level}] {main_thread_var}") << std::endl;
    
    std::cout << "\n2. Starting worker threads...\n";
    std::vector<std::thread> workers;
    std::vector<std::string> worker_results(3);
    
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back([i, ctx, &worker_results]() {
            // Worker threads can see main thread's global variables
            std::string initial = ctx->format("Worker {0} initial view: {app_name} [{log_level}]", i);
            
            // Worker threads set their own variables (goes to thread-local storage)
            ctx->set_var("log_level", "DEBUG");  // Override global
            ctx->set_var("worker_id", std::to_string(i));  // New thread-local var
            ctx->set_var("worker_msg", "Hello from worker " + std::to_string(i));
            
            // Worker sees: global app_name + thread-local overrides
            std::string final = ctx->format("Worker {worker_id}: {app_name} [{log_level}] - {worker_msg}");
            
            worker_results[static_cast<size_t>(i)] = initial + "\n   " + final;
        });
    }
    
    // Wait for workers
    for (auto& worker : workers) {
        worker.join();
    }
    
    // Display worker results
    for (int i = 0; i < 3; ++i) {
        std::cout << "   " << worker_results[static_cast<size_t>(i)] << std::endl;
    }
    
    std::cout << "\n3. Main thread after workers complete:\n";
    std::cout << "   Main thread view: " << ctx->format("{app_name} [{log_level}] {main_thread_var}") << std::endl;
    std::cout << "   Main thread can see worker variables: " << (ctx->has_var("worker_id") ? "YES" : "NO") << std::endl;
    std::cout << "   Main thread can see worker_msg: " << (ctx->has_var("worker_msg") ? "YES" : "NO") << std::endl;
    
    std::cout << "\n4. Testing main thread modifications:\n";
    ctx->set_var("log_level", "ERROR");  // Main thread modifies global
    ctx->set_var("new_main_var", "added_by_main");
    std::cout << "   Main thread modified global log_level: " << ctx->format("{app_name} [{log_level}] {new_main_var}") << std::endl;
    
    std::cout << "\n=== Key Observations ===\n";
    std::cout << "✓ Main thread sets global variables that all threads can read\n";
    std::cout << "✓ Worker threads write to thread-local storage (transparent to user)\n";
    std::cout << "✓ Thread-local variables override global ones for that thread\n";
    std::cout << "✓ Main thread is unaffected by worker thread modifications\n";
    std::cout << "✓ Worker thread variables are isolated and don't leak to main thread\n";
    std::cout << "✓ Same API (set_var, clear_var, has_var) used everywhere - completely transparent!\n";
    
    return 0;
}
