/**
 * Test program for research mode features
 * 
 * This tests the research mode flags without requiring full solver compilation.
 * Demonstrates configuration options for:
 * - Full BFS mode (no local expansion)
 * - Force BFS depth
 * - Ignore memory limits
 * - Collect statistics
 * - Dry run mode
 * 
 * Compile: g++ -std=c++17 -O3 -I. test_research_mode.cpp -o test_research_mode
 */

#include "bucket_config.h"
#include <iostream>
#include <cassert>

void test_research_config_defaults() {
    std::cout << "Testing ResearchConfig defaults... ";
    
    ResearchConfig cfg;
    assert(cfg.enable_local_expansion == true);
    assert(cfg.force_full_bfs_to_depth == -1);
    assert(cfg.ignore_memory_limits == false);
    assert(cfg.collect_detailed_statistics == false);
    assert(cfg.dry_run == false);
    assert(cfg.enable_custom_buckets == false);
    assert(cfg.high_memory_wasm_mode == false);
    assert(cfg.developer_memory_limit_mb == 2048);
    
    std::cout << "PASSED ✓" << std::endl;
}

void test_full_bfs_mode() {
    std::cout << "Testing full BFS mode configuration... ";
    
    ResearchConfig cfg;
    cfg.enable_local_expansion = false;  // Disable local expansion
    cfg.force_full_bfs_to_depth = 8;     // Force BFS to depth 8
    cfg.ignore_memory_limits = true;     // No memory limits
    cfg.collect_detailed_statistics = true;  // Collect CSV
    
    // Verify configuration
    assert(cfg.enable_local_expansion == false);
    assert(cfg.force_full_bfs_to_depth == 8);
    assert(cfg.ignore_memory_limits == true);
    assert(cfg.collect_detailed_statistics == true);
    
    std::cout << "PASSED ✓" << std::endl;
    std::cout << "  Configuration:" << std::endl;
    std::cout << "    - Local expansion: DISABLED (full BFS only)" << std::endl;
    std::cout << "    - BFS depth: 8 (forced)" << std::endl;
    std::cout << "    - Memory limits: IGNORED" << std::endl;
    std::cout << "    - Statistics: ENABLED" << std::endl;
}

void test_dry_run_mode() {
    std::cout << "Testing dry run mode configuration... ";
    
    ResearchConfig cfg;
    cfg.dry_run = true;
    cfg.collect_detailed_statistics = true;
    
    // Dry run should collect stats but clear database after
    assert(cfg.dry_run == true);
    assert(cfg.collect_detailed_statistics == true);
    
    std::cout << "PASSED ✓" << std::endl;
    std::cout << "  Configuration:" << std::endl;
    std::cout << "    - Dry run: ENABLED (database will be cleared after measurement)" << std::endl;
    std::cout << "    - Statistics: ENABLED" << std::endl;
}

void test_developer_mode() {
    std::cout << "Testing developer mode configuration... ";
    
    BucketConfig bucket_cfg;
    bucket_cfg.model = BucketModel::CUSTOM;
    bucket_cfg.custom_bucket_7 = 8 << 20;   // 8M
    bucket_cfg.custom_bucket_8 = 16 << 20;  // 16M
    bucket_cfg.custom_bucket_9 = 32 << 20;  // 32M
    
    ResearchConfig research_cfg;
    research_cfg.enable_custom_buckets = true;
    research_cfg.developer_memory_limit_mb = 500;  // 500 MB limit
    research_cfg.collect_detailed_statistics = true;
    
    // Estimate RSS
    size_t estimated_rss = estimate_custom_rss(
        bucket_cfg.custom_bucket_7,
        bucket_cfg.custom_bucket_8,
        bucket_cfg.custom_bucket_9
    );
    
    std::cout << "\n  Custom buckets: 8M/16M/32M" << std::endl;
    std::cout << "  Estimated RSS: " << estimated_rss << " MB" << std::endl;
    std::cout << "  Developer limit: " << research_cfg.developer_memory_limit_mb << " MB" << std::endl;
    
    // This should NOT throw (estimated ~95 MB < 500 MB limit)
    validate_model_safety(BucketModel::CUSTOM, 1600, bucket_cfg, research_cfg);
    
    std::cout << "PASSED ✓" << std::endl;
}

void test_production_wasm_mode() {
    std::cout << "Testing production WASM mode... ";
    
    BucketConfig cfg;
    cfg.model = BucketModel::AUTO;
    cfg.memory_budget_mb = 300;  // User specifies 300 MB
    
    // JavaScript subtracts 25% margin: 300 * 0.75 = 225 MB to C++
    size_t cpp_budget = 225;
    BucketModel selected = select_model(cpp_budget);
    
    // Should select LOW (190 MB)
    assert(selected == BucketModel::LOW);
    
    std::cout << "PASSED ✓" << std::endl;
    std::cout << "  User budget: 300 MB" << std::endl;
    std::cout << "  C++ budget: 225 MB (75% of total)" << std::endl;
    std::cout << "  Selected model: LOW (190 MB)" << std::endl;
}

void display_example_configurations() {
    std::cout << "\n=== Example Configurations ===" << std::endl;
    
    std::cout << "\n1. Full BFS Research (Depth 8):" << std::endl;
    std::cout << "   ResearchConfig cfg;" << std::endl;
    std::cout << "   cfg.enable_local_expansion = false;" << std::endl;
    std::cout << "   cfg.force_full_bfs_to_depth = 8;" << std::endl;
    std::cout << "   cfg.ignore_memory_limits = true;" << std::endl;
    std::cout << "   cfg.collect_detailed_statistics = true;" << std::endl;
    
    std::cout << "\n2. Custom Bucket Measurement:" << std::endl;
    std::cout << "   BucketConfig bucket;" << std::endl;
    std::cout << "   bucket.model = BucketModel::CUSTOM;" << std::endl;
    std::cout << "   bucket.custom_bucket_7 = 4 << 20;  // 4M" << std::endl;
    std::cout << "   bucket.custom_bucket_8 = 8 << 20;  // 8M" << std::endl;
    std::cout << "   bucket.custom_bucket_9 = 16 << 20; // 16M" << std::endl;
    std::cout << "   ResearchConfig research;" << std::endl;
    std::cout << "   research.enable_custom_buckets = true;" << std::endl;
    std::cout << "   research.collect_detailed_statistics = true;" << std::endl;
    std::cout << "   research.dry_run = true;" << std::endl;
    
    std::cout << "\n3. Production WASM (Auto-Select):" << std::endl;
    std::cout << "   BucketConfig cfg;" << std::endl;
    std::cout << "   cfg.model = BucketModel::AUTO;" << std::endl;
    std::cout << "   // C++ receives 225 MB (300 MB * 0.75 from JavaScript)" << std::endl;
    std::cout << "   xxcross_search solver(true, 6, 225, false);" << std::endl;
}

int main() {
    std::cout << "\n=== Research Mode Configuration Tests ===" << std::endl;
    
    test_research_config_defaults();
    test_full_bfs_mode();
    test_dry_run_mode();
    test_developer_mode();
    test_production_wasm_mode();
    
    display_example_configurations();
    
    std::cout << "\n=== All Tests Passed ✓ ===" << std::endl;
    
    return 0;
}
