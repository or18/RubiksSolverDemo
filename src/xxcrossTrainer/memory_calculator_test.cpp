/**
 * @file memory_calculator_test.cpp
 * @brief Test program for memory_calculator.h
 * 
 * Demonstrates usage patterns and validates calculations against
 * known xxcross measurements.
 * 
 * Compile:
 *   g++ -std=c++17 -O2 memory_calculator_test.cpp -o memory_calc_test
 * 
 * Run:
 *   ./memory_calc_test
 */

#include "memory_calculator.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace memory_calc;

void test_basic_calculations() {
    std::cout << "=== Test 1: Basic Calculations ===" << std::endl;
    
    // Test bucket size calculation
    size_t buckets = calculate_bucket_size(1000000, 0.9);
    std::cout << "Bucket size for 1M nodes at 0.9 load: " << buckets 
              << " (" << (buckets >> 20) << "M)" << std::endl;
    
    // Test memory calculations
    size_t bucket_mb = calculate_bucket_memory_mb(buckets);
    size_t node_mb = calculate_node_memory_mb(1000000);
    size_t index_mb = calculate_index_pairs_memory_mb(1000000);
    
    std::cout << "Memory breakdown for 1M nodes:" << std::endl;
    std::cout << "  Buckets:     " << bucket_mb << " MB" << std::endl;
    std::cout << "  Nodes:       " << node_mb << " MB" << std::endl;
    std::cout << "  Index pairs: " << index_mb << " MB" << std::endl;
    std::cout << "  Total:       " << (bucket_mb + node_mb + index_mb) << " MB" << std::endl;
    std::cout << std::endl;
}

void test_phase1_calculation() {
    std::cout << "=== Test 2: Phase 1 (BFS) Memory Calculation ===" << std::endl;
    
    // Known xxcross node distribution (depth 0-6)
    std::vector<size_t> node_counts = {
        1,          // depth 0
        18,         // depth 1
        243,        // depth 2
        3007,       // depth 3
        35413,      // depth 4
        391951,     // depth 5
        4123285     // depth 6 (4.2M nodes)
    };
    
    MemoryComponents mem = calculate_phase1_memory(node_counts, 0.9, 2.5);
    
    std::cout << format_memory_report(mem, "Phase 1 (BFS to depth 6)") << std::endl;
    
    // Compare with known measurements (if available)
    // Example: measured RSS after Phase 1 was ~450 MB
    size_t measured_rss = 450;  // MB (example)
    set_measured_rss(mem, measured_rss);
    
    std::cout << "After adding measured RSS:" << std::endl;
    std::cout << "  Measured:  " << mem.measured_rss_mb << " MB" << std::endl;
    std::cout << "  Predicted: " << mem.predicted_rss_mb << " MB" << std::endl;
    std::cout << "  Overhead:  " << mem.overhead_mb << " MB (" 
              << std::fixed << std::setprecision(1) << mem.overhead_percent << "%)" << std::endl;
    std::cout << std::endl;
}

void test_local_expansion_calculation() {
    std::cout << "=== Test 3: Phase 2 (Depth 7 Local Expansion) ===" << std::endl;
    
    // Example: STANDARD model (8M bucket, ~7M nodes achieved)
    size_t bucket_7 = 8 << 20;  // 8M buckets
    size_t nodes_7 = 7000000;   // 7M nodes (example)
    
    MemoryComponents mem = calculate_local_expansion_memory(nodes_7, bucket_7, 0.88, 2.5);
    
    std::cout << format_memory_report(mem, "Phase 2 (Depth 7)") << std::endl;
    std::cout << std::endl;
}

void test_bucket_estimation() {
    std::cout << "=== Test 4: RSS Estimation from Bucket Sizes ===" << std::endl;
    
    // Test STANDARD model: 8M / 16M / 16M
    size_t bucket_7 = 8 << 20;
    size_t bucket_8 = 16 << 20;
    size_t bucket_9 = 16 << 20;
    
    size_t estimated_rss = estimate_rss_from_buckets(bucket_7, bucket_8, bucket_9, 0.88, 2.5);
    
    std::cout << "STANDARD Model (8M/16M/16M):" << std::endl;
    std::cout << "  Estimated peak RSS: " << estimated_rss << " MB" << std::endl;
    std::cout << std::endl;
    
    // Test HIGH model: 16M / 32M / 32M
    bucket_7 = 16 << 20;
    bucket_8 = 32 << 20;
    bucket_9 = 32 << 20;
    
    estimated_rss = estimate_rss_from_buckets(bucket_7, bucket_8, bucket_9, 0.88, 2.5);
    
    std::cout << "HIGH Model (16M/32M/32M):" << std::endl;
    std::cout << "  Estimated peak RSS: " << estimated_rss << " MB" << std::endl;
    std::cout << std::endl;
}

void test_csv_output() {
    std::cout << "=== Test 5: CSV Output Format ===" << std::endl;
    
    std::cout << csv_header() << std::endl;
    
    // Phase 1 example
    std::vector<size_t> node_counts = {1, 18, 243, 3007, 35413, 391951, 4123285};
    MemoryComponents mem1 = calculate_phase1_memory(node_counts);
    set_measured_rss(mem1, 450);
    std::cout << format_csv_row("Phase1", 6, 4123285, mem1) << std::endl;
    
    // Phase 2 example
    MemoryComponents mem2 = calculate_local_expansion_memory(7000000, 8 << 20);
    set_measured_rss(mem2, 520);
    std::cout << format_csv_row("Phase2", 7, 7000000, mem2) << std::endl;
    
    std::cout << std::endl;
}

void test_overhead_analysis() {
    std::cout << "=== Test 6: Overhead Analysis ===" << std::endl;
    
    // Simulate different overhead scenarios
    struct TestCase {
        const char* name;
        size_t predicted;
        size_t measured;
    };
    
    TestCase cases[] = {
        {"Good prediction (Phase 1)",     450,  455},   // +1.1% overhead
        {"Acceptable (Phase 2)",          520,  540},   // +3.8% overhead
        {"High overhead (memory leak?)",  450,  550},   // +22% overhead (problematic)
        {"Negative overhead (too conservative)", 600, 540},  // -10% (prediction too high)
    };
    
    std::cout << std::setw(40) << std::left << "Scenario"
              << std::setw(12) << "Predicted"
              << std::setw(12) << "Measured"
              << std::setw(12) << "Overhead"
              << "Percent" << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (const auto& tc : cases) {
        int overhead_mb;
        double overhead_percent;
        analyze_overhead(tc.predicted, tc.measured, overhead_mb, overhead_percent);
        
        std::cout << std::setw(40) << std::left << tc.name
                  << std::setw(12) << tc.predicted
                  << std::setw(12) << tc.measured
                  << std::setw(12) << overhead_mb
                  << std::fixed << std::setprecision(1) << overhead_percent << "%" << std::endl;
    }
    
    std::cout << std::endl;
}

void demonstrate_solver_integration() {
    std::cout << "=== Demonstration: Integration with solver_dev.cpp ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "// Example 1: Pre-flight RSS prediction (developer safety check)" << std::endl;
    std::cout << "ResearchConfig research;" << std::endl;
    std::cout << "research.enable_custom_buckets = true;" << std::endl;
    std::cout << "research.developer_memory_limit_mb = 2048;  // 2GB limit" << std::endl;
    std::cout << std::endl;
    std::cout << "// Developer specifies custom buckets" << std::endl;
    std::cout << "BucketConfig config;" << std::endl;
    std::cout << "config.model = BucketModel::CUSTOM;" << std::endl;
    std::cout << "config.bucket_7 = 16 << 20;  // 16M" << std::endl;
    std::cout << "config.bucket_8 = 32 << 20;  // 32M" << std::endl;
    std::cout << "config.bucket_9 = 32 << 20;  // 32M" << std::endl;
    std::cout << std::endl;
    std::cout << "// Pre-flight check using memory_calculator" << std::endl;
    std::cout << "size_t estimated_rss = memory_calc::estimate_rss_from_buckets(" << std::endl;
    std::cout << "    config.bucket_7, config.bucket_8, config.bucket_9);" << std::endl;
    std::cout << std::endl;
    
    size_t estimated = estimate_rss_from_buckets(16 << 20, 32 << 20, 32 << 20);
    std::cout << "// Estimated RSS: " << estimated << " MB" << std::endl;
    std::cout << std::endl;
    std::cout << "if (estimated_rss > research.developer_memory_limit_mb) {" << std::endl;
    std::cout << "    throw std::runtime_error(\"Custom config exceeds developer limit!\");" << std::endl;
    std::cout << "}" << std::endl;
    std::cout << std::endl;
    
    std::cout << "// Example 2: Per-phase overhead monitoring" << std::endl;
    std::cout << "// After Phase 1 completes:" << std::endl;
    std::cout << "size_t phase1_rss = get_rss_mb();  // from solver_dev.cpp" << std::endl;
    std::cout << "MemoryComponents phase1_calc = memory_calc::calculate_phase1_memory(node_counts);" << std::endl;
    std::cout << "memory_calc::set_measured_rss(phase1_calc, phase1_rss);" << std::endl;
    std::cout << std::endl;
    std::cout << "if (research.collect_detailed_statistics) {" << std::endl;
    std::cout << "    std::cout << memory_calc::format_memory_report(phase1_calc, \"Phase 1\");" << std::endl;
    std::cout << "    // Output overhead analysis for tuning memory_factor" << std::endl;
    std::cout << "}" << std::endl;
    std::cout << std::endl;
}

int main() {
    std::cout << "======================================" << std::endl;
    std::cout << "  Memory Calculator Test Suite" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;
    
    test_basic_calculations();
    test_phase1_calculation();
    test_local_expansion_calculation();
    test_bucket_estimation();
    test_csv_output();
    test_overhead_analysis();
    demonstrate_solver_integration();
    
    std::cout << "======================================" << std::endl;
    std::cout << "  All tests completed!" << std::endl;
    std::cout << "======================================" << std::endl;
    
    return 0;
}
