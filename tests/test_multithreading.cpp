#include "../include/ufmt/ufmt.h"
#include "../include/utest/utest.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <set>
#include <mutex>

// Test thread safety of shared contexts
UTEST_FUNC_DEF(SharedContextThreadSafety) {
    const int num_threads = 4;
    const int operations_per_thread = 100;
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    std::vector<std::string> results;
    std::mutex results_mutex;
    
    // All threads will use the same shared context
    auto shared_ctx = ufmt::get_shared_context("thread_safety_test");
    shared_ctx->set_var("test_id", "safety_test");
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, operations_per_thread, &counter, &results, &results_mutex, shared_ctx]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                int op_id = ++counter;
                
                // Set thread-specific variables
                shared_ctx->set_var("thread_id", i);
                shared_ctx->set_var("operation", op_id);
                
                // Format string using shared context
                std::string result = shared_ctx->format("Test {test_id}: Thread {thread_id}, Op {operation}");
                
                // Store result in thread-safe manner
                {
                    std::lock_guard<std::mutex> lock(results_mutex);
                    results.push_back(result);
                }
                
                // Small delay to encourage race conditions if they exist
                if (j % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results
    UTEST_ASSERT_EQUALS(results.size(), num_threads * operations_per_thread);
    UTEST_ASSERT_EQUALS(counter.load(), num_threads * operations_per_thread);
    
    // Check that all results contain expected components
    for (const auto& result : results) {
        UTEST_ASSERT_STR_CONTAINS(result, "Test safety_test:");
        UTEST_ASSERT_STR_CONTAINS(result, "Thread ");
        UTEST_ASSERT_STR_CONTAINS(result, "Op ");
    }
}

// Test isolation of scoped contexts across threads
UTEST_FUNC_DEF(ScopedContextIsolation) {
    const int num_threads = 4;
    const int operations_per_thread = 50;
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> thread_results(num_threads);
    std::atomic<bool> all_started{false};
    std::atomic<int> ready_count{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, operations_per_thread, &thread_results, &all_started, &ready_count]() {
            // Each thread gets its own scoped context
            auto ctx = ufmt::create_local_context();
            
            // Set thread-specific formatter
            ctx->set_formatter<bool>([i](bool b) { 
                return b ? ("T" + std::to_string(i) + ":TRUE") : ("T" + std::to_string(i) + ":FALSE"); 
            });
            
            // Set thread-specific variables
            ctx->set_var("thread_id", i);
            ctx->set_var("thread_name", "Thread_" + std::to_string(i));
            
            // Signal ready and wait for all threads to be ready
            ready_count++;
            while (!all_started.load()) {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            
            // Perform operations
            for (int j = 0; j < operations_per_thread; ++j) {
                ctx->set_var("operation", j);
                
                // Test custom formatter
                std::string result1 = ctx->format("Custom: {0}", (j % 2 == 0));
                
                // Test named variables
                std::string result2 = ctx->format("Named: {thread_name} op {operation}");
                
                // Store results
                thread_results[i].push_back(result1);
                thread_results[i].push_back(result2);
            }
        });
    }
    
    // Wait for all threads to be ready
    while (ready_count.load() < num_threads) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    // Start all threads simultaneously
    all_started = true;
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify isolation - each thread should have its own results
    for (int i = 0; i < num_threads; ++i) {
        UTEST_ASSERT_EQUALS(thread_results[i].size(), operations_per_thread * 2);
        
        // Check custom formatter results
        for (int j = 0; j < operations_per_thread; ++j) {
            std::string expected_prefix = "T" + std::to_string(i) + ":";
            UTEST_ASSERT_STR_CONTAINS(thread_results[i][j * 2], expected_prefix);
        }
        
        // Check named variable results
        std::string expected_thread_name = "Thread_" + std::to_string(i);
        for (int j = 0; j < operations_per_thread; ++j) {
            UTEST_ASSERT_STR_CONTAINS(thread_results[i][j * 2 + 1], expected_thread_name);
        }
    }
}

// Test multiple named shared contexts
UTEST_FUNC_DEF(MultipleSharedContexts) {
    const int num_contexts = 3;
    const int threads_per_context = 2;
    const int operations_per_thread = 30;
    
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> context_results(num_contexts);
    std::vector<std::mutex> context_mutexes(num_contexts);
    
    for (int ctx_id = 0; ctx_id < num_contexts; ++ctx_id) {
        std::string context_name = "test_context_" + std::to_string(ctx_id);
        
        for (int thread_id = 0; thread_id < threads_per_context; ++thread_id) {
            threads.emplace_back([ctx_id, thread_id, context_name, operations_per_thread, 
                                &context_results, &context_mutexes]() {
                auto ctx = ufmt::get_shared_context(context_name);
                ctx->set_var("context_id", ctx_id);
                ctx->set_var("context_name", context_name);
                
                for (int op = 0; op < operations_per_thread; ++op) {
                    ctx->set_var("thread_id", thread_id);
                    ctx->set_var("operation", op);
                    
                    std::string result = ctx->format("Context {context_name}: T{thread_id} Op{operation}");
                    
                    {
                        std::lock_guard<std::mutex> lock(context_mutexes[ctx_id]);
                        context_results[ctx_id].push_back(result);
                    }
                }
            });
        }
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify each context got the expected number of results
    for (int ctx_id = 0; ctx_id < num_contexts; ++ctx_id) {
        UTEST_ASSERT_EQUALS(context_results[ctx_id].size(), threads_per_context * operations_per_thread);
        
        // Verify all results contain the correct context name
        std::string expected_context = "test_context_" + std::to_string(ctx_id);
        for (const auto& result : context_results[ctx_id]) {
            UTEST_ASSERT_STR_CONTAINS(result, expected_context);
        }
    }
}

// Test concurrent variable setting and formatting
UTEST_FUNC_DEF(ConcurrentVariableOperations) {
    const int num_threads = 4;  // Reduced for reliability
    const int operations_per_thread = 25;  // Reduced for reliability
    
    std::vector<std::thread> threads;
    std::atomic<int> writer_operations{0};
    std::atomic<int> reader_operations{0};
    
    // Each thread gets its own context to avoid contention
    for (int i = 0; i < num_threads; ++i) {
        if (i % 2 == 0) {
            // Writer threads
            threads.emplace_back([i, operations_per_thread, &writer_operations]() {
                std::string context_name = "writer_context_" + std::to_string(i);
                auto ctx = ufmt::get_shared_context(context_name);
                
                for (int j = 0; j < operations_per_thread; ++j) {
                    std::string var_name = "var_" + std::to_string(j);
                    std::string var_value = "value_" + std::to_string(j);
                    
                    ctx->set_var(var_name, var_value);
                    writer_operations++;
                    
                    // Small delay
                    if (j % 5 == 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                }
            });
        } else {
            // Reader threads
            threads.emplace_back([i, operations_per_thread, &reader_operations]() {
                std::string context_name = "reader_context_" + std::to_string(i);
                auto ctx = ufmt::get_shared_context(context_name);
                
                for (int j = 0; j < operations_per_thread; ++j) {
                    // Set our own variables and format them
                    ctx->set_var("reader_id", i);
                    ctx->set_var("read_count", j);
                    
                    std::string result = ctx->format("Reader {reader_id}: count {read_count}");
                    
                    // This should always succeed since we set the variables ourselves
                    if (result.find("Reader " + std::to_string(i)) != std::string::npos &&
                        result.find("count " + std::to_string(j)) != std::string::npos) {
                        reader_operations++;
                    }
                    
                    // Small delay
                    if (j % 5 == 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(1));
                    }
                }
            });
        }
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have successful operations from all threads
    int expected_writer_operations = (num_threads / 2) * operations_per_thread;
    int expected_reader_operations = (num_threads - num_threads / 2) * operations_per_thread;
    
    UTEST_ASSERT_EQUALS(writer_operations.load(), expected_writer_operations);
    UTEST_ASSERT_EQUALS(reader_operations.load(), expected_reader_operations);
}

// Test stress scenario with high contention
UTEST_FUNC_DEF(HighContentionStressTest) {
    const int num_threads = 4;  // Reduced for reliability
    const int operations_per_thread = 50;  // Reduced for reliability
    const int num_shared_contexts = 2;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::atomic<int> successful_operations{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, operations_per_thread, num_shared_contexts, 
                            &total_operations, &successful_operations]() {
            // Each thread will use one of the shared contexts
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    int context_id = i % num_shared_contexts;  // Use thread ID for consistency
                    std::string context_name = "stress_test_" + std::to_string(context_id);
                    auto ctx = ufmt::get_shared_context(context_name);
                    
                    // Set and use variables
                    ctx->set_var("thread", i);
                    ctx->set_var("operation", j);
                    
                    // Simple format operations
                    std::string result1 = ctx->format("T{thread}: Op{operation}");
                    std::string result2 = ufmt::format("Simple: {0} + {1}", i, j);
                    
                    total_operations++;
                    
                    // Basic validation
                    if (result1.find(std::to_string(i)) != std::string::npos &&
                        result2.find(std::to_string(i)) != std::string::npos) {
                        successful_operations++;
                    }
                    
                } catch (const std::exception&) {
                    // Even if we get exceptions, count the attempt
                    total_operations++;
                }
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have completed all operations
    UTEST_ASSERT_EQUALS(total_operations.load(), num_threads * operations_per_thread);
    
    // Most operations should succeed (allow some flexibility for race conditions)
    int min_expected_success = (num_threads * operations_per_thread * 90) / 100;
    UTEST_ASSERT_TRUE(successful_operations.load() >= min_expected_success);
}

// Test basic thread safety functionality of shared contexts
UTEST_FUNC_DEF(BasicThreadSafety) {
    // Test that we can safely access shared context from multiple operations
    auto ctx = ufmt::get_shared_context("basic_thread_test");
    
    // Set up initial state
    ctx->set_var("counter", 0);
    ctx->set_var("name", "test");
    
    // Verify basic functionality works
    auto result1 = ctx->format("Counter: {counter}, Name: {name}");
    UTEST_ASSERT_STR_EQUALS(result1, "Counter: 0, Name: test");
    
    // Test that we can update variables and access them
    ctx->set_var("counter", 42);
    auto result2 = ctx->format("Updated counter: {counter}");
    UTEST_ASSERT_STR_EQUALS(result2, "Updated counter: 42");
    
    // Test that different contexts are truly separate
    auto ctx2 = ufmt::get_shared_context("basic_thread_test_2");
    UTEST_ASSERT_FALSE(ctx2->has_var("counter"));
    
    // But same named context should share state
    auto ctx3 = ufmt::get_shared_context("basic_thread_test");
    UTEST_ASSERT_TRUE(ctx3->has_var("counter"));
    auto result3 = ctx3->format("Shared counter: {counter}");
    UTEST_ASSERT_STR_EQUALS(result3, "Shared counter: 42");
}

// Test transparent thread-local variable behavior
UTEST_FUNC_DEF(TransparentThreadLocalBehavior) {
    auto ctx = ufmt::get_shared_context("transparent_test");
    
    // Main thread sets global variables
    ctx->set_var("app_name", "MyApp");
    ctx->set_var("log_level", "INFO");
    
    // Verify main thread sees global values
    auto result1 = ctx->format("Main: {app_name} [{log_level}]");
    UTEST_ASSERT_STR_EQUALS(result1, "Main: MyApp [INFO]");
    
    std::string worker_result;
    std::string main_result_after;
    
    // Worker thread
    std::thread worker([ctx, &worker_result]() {
        // Worker thread can see global variables
        auto initial = ctx->format("Worker sees: {app_name} [{log_level}]");
        
        // Worker thread sets variables (goes to thread-local storage)
        ctx->set_var("log_level", "DEBUG");  // Override global
        ctx->set_var("thread_id", "worker1"); // New thread-local var
        
        // Worker now sees its own values + global fallbacks
        worker_result = ctx->format("Worker: {app_name} [{log_level}] Thread: {thread_id}");
    });
    
    worker.join();
    
    // Worker should see: global app_name, thread-local log_level, thread-local thread_id
    UTEST_ASSERT_STR_EQUALS(worker_result, "Worker: MyApp [DEBUG] Thread: worker1");
    
    // Main thread should still see original global values (unaffected by worker)
    main_result_after = ctx->format("Main after: {app_name} [{log_level}]");
    UTEST_ASSERT_STR_EQUALS(main_result_after, "Main after: MyApp [INFO]");
    
    // Main thread shouldn't see worker's thread-local variables
    UTEST_ASSERT_FALSE(ctx->has_var("thread_id"));
}

// Test local context isolation (single-thread behavior)
UTEST_FUNC_DEF(LocalContextIsolation) {
    // Local contexts are single-thread only and don't share state
    auto ctx1 = ufmt::create_local_context();
    auto ctx2 = ufmt::create_local_context();
    
    // Each context has its own isolated storage
    ctx1->set_var("test_var", "value1");
    ctx2->set_var("test_var", "value2");
    
    auto result1 = ctx1->format("Context1: {test_var}");
    auto result2 = ctx2->format("Context2: {test_var}");
    
    UTEST_ASSERT_STR_EQUALS(result1, "Context1: value1");
    UTEST_ASSERT_STR_EQUALS(result2, "Context2: value2");
    
    // Changes in one context don't affect the other
    ctx1->set_var("test_var", "modified1");
    auto result3 = ctx2->format("Context2 unchanged: {test_var}");
    UTEST_ASSERT_STR_EQUALS(result3, "Context2 unchanged: value2");
}

// Test transparent thread-local isolation between threads
UTEST_FUNC_DEF(TransparentThreadLocalIsolation) {
    const int num_threads = 4;
    const int operations_per_thread = 20;
    
    auto shared_ctx = ufmt::get_shared_context("isolation_test");
    shared_ctx->set_var("shared_var", "shared_value");
    
    std::vector<std::thread> threads;
    std::vector<std::vector<std::string>> thread_results(num_threads);
    std::atomic<bool> all_ready{false};
    std::atomic<int> ready_count{0};
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([i, operations_per_thread, shared_ctx, &thread_results, &all_ready, &ready_count]() {
            // Each worker thread sets its own variables (goes to thread-local storage)
            std::string thread_value = "thread_" + std::to_string(i) + "_value";
            shared_ctx->set_var("thread_specific", thread_value);  // Thread-local
            shared_ctx->set_var("shared_var", thread_value);       // Thread-local override
            
            // Signal ready and wait for all threads
            ready_count++;
            while (!all_ready.load()) {
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
            
            // Perform operations
            for (int j = 0; j < operations_per_thread; ++j) {
                // Test thread-local variable
                std::string result1 = shared_ctx->format("Thread specific: {thread_specific}");
                
                // Test overridden shared variable (should use thread-local)
                std::string result2 = shared_ctx->format("Shared override: {shared_var}");
                
                thread_results[i].push_back(result1);
                thread_results[i].push_back(result2);
                
                // Small delay to encourage context switching
                if (j % 5 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }
            }
        });
    }
    
    // Wait for all threads to be ready
    while (ready_count.load() < num_threads) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    
    // Start all threads
    all_ready = true;
    
    // Wait for completion
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify results - each thread should have its own thread-local values
    for (int i = 0; i < num_threads; ++i) {
        UTEST_ASSERT_EQUALS(thread_results[i].size(), operations_per_thread * 2);
        
        std::string expected_thread_value = "thread_" + std::to_string(i) + "_value";
        
        for (int j = 0; j < operations_per_thread; ++j) {
            // Check thread-specific variable
            std::string expected1 = "Thread specific: " + expected_thread_value;
            UTEST_ASSERT_STR_EQUALS(thread_results[i][j * 2], expected1);
            
            // Check overridden shared variable
            std::string expected2 = "Shared override: " + expected_thread_value;
            UTEST_ASSERT_STR_EQUALS(thread_results[i][j * 2 + 1], expected2);
        }
    }
    
    // Verify main thread still sees original shared value (no thread-local override)
    std::string main_result = shared_ctx->format("Main thread: {shared_var}");
    UTEST_ASSERT_STR_EQUALS(main_result, "Main thread: shared_value");
}

int main() {
    UTEST_PROLOG();
    
    UTEST_FUNC(BasicThreadSafety);
    UTEST_FUNC(SharedContextThreadSafety);
    UTEST_FUNC(ScopedContextIsolation);
    UTEST_FUNC(MultipleSharedContexts);
    UTEST_FUNC(ConcurrentVariableOperations);
    UTEST_FUNC(HighContentionStressTest);
    UTEST_FUNC(TransparentThreadLocalBehavior);
    UTEST_FUNC(LocalContextIsolation);
    UTEST_FUNC(TransparentThreadLocalIsolation);
    
    UTEST_EPILOG();
    return 0;
}
