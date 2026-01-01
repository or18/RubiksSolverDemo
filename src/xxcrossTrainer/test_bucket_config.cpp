/**
 * Test program for bucket configuration and research mode
 * 
 * Compile:
 *   g++ -std=c++17 -O3 -I. test_bucket_config.cpp -o test_bucket_config
 * 
 * Run tests:
 *   ./test_bucket_config
 */

#include "bucket_config.h"
#include <iostream>
#include <cassert>

void test_power_of_two() {
    std::cout << "Testing is_power_of_two()... ";
    assert(is_power_of_two(1 << 20));  // 1M
    assert(is_power_of_two(2 << 20));  // 2M
    assert(is_power_of_two(4 << 20));  // 4M
    assert(!is_power_of_two(3 << 20)); // 3M (not power of 2)
    assert(!is_power_of_two(0));       // 0
    std::cout << "PASSED ✓" << std::endl;
}

void test_model_selection() {
    std::cout << "Testing select_model()... ";
    
    assert(select_model(100) == BucketModel::MINIMAL);      // <145 MB
    assert(select_model(200) == BucketModel::LOW);          // 190-289 MB
    assert(select_model(300) == BucketModel::BALANCED);     // 290-439 MB
    assert(select_model(500) == BucketModel::MEDIUM);       // 440-539 MB
    assert(select_model(700) == BucketModel::STANDARD);     // 540-979 MB
    assert(select_model(1000) == BucketModel::HIGH);        // 980-1099 MB
    assert(select_model(1500) == BucketModel::ULTRA_HIGH);  // 1100+ MB
    
    std::cout << "PASSED ✓" << std::endl;
}

void test_custom_bucket_validation() {
    std::cout << "Testing custom bucket validation... ";
    
    BucketConfig cfg;
    cfg.model = BucketModel::CUSTOM;
    cfg.custom_bucket_7 = 4 << 20;  // 4M
    cfg.custom_bucket_8 = 8 << 20;  // 8M
    cfg.custom_bucket_9 = 16 << 20; // 16M
    
    ResearchConfig research;
    research.enable_custom_buckets = false;
    
    // Should throw: custom buckets require flag
    try {
        validate_custom_buckets(cfg, research);
        assert(false && "Should have thrown");
    } catch (const std::runtime_error& e) {
        // Expected
    }
    
    // Enable flag, should pass
    research.enable_custom_buckets = true;
    validate_custom_buckets(cfg, research);
    
    // Invalid bucket size (not power of 2)
    cfg.custom_bucket_7 = 3 << 20;  // 3M (invalid)
    try {
        validate_custom_buckets(cfg, research);
        assert(false && "Should have thrown");
    } catch (const std::runtime_error& e) {
        // Expected
    }
    
    std::cout << "PASSED ✓" << std::endl;
}

void test_rss_estimation() {
    std::cout << "Testing estimate_custom_rss()... ";
    
    // Test case: 4M / 8M / 16M buckets
    size_t rss = estimate_custom_rss(4 << 20, 8 << 20, 16 << 20);
    
    // Expected: (4*0.9) + (8*0.89) + (16*0.7) + 50 = 3.6 + 7.12 + 11.2 + 50 ≈ 71.92 MB
    std::cout << "Estimated RSS for 4M/8M/16M: " << rss << " MB... ";
    assert(rss >= 70 && rss <= 75);  // Rough check
    
    std::cout << "PASSED ✓" << std::endl;
}

void test_developer_memory_limit() {
    std::cout << "Testing developer memory limit check... ";
    
    BucketConfig cfg;
    cfg.model = BucketModel::CUSTOM;
    cfg.custom_bucket_7 = 32 << 20;  // 32M
    cfg.custom_bucket_8 = 64 << 20;  // 64M
    cfg.custom_bucket_9 = 64 << 20;  // 64M
    // Estimated RSS: ~140 MB
    
    ResearchConfig research;
    research.enable_custom_buckets = true;
    research.developer_memory_limit_mb = 100;  // Low limit
    
    // Should throw: exceeds developer limit
    try {
        validate_model_safety(BucketModel::CUSTOM, 1600, cfg, research);
        assert(false && "Should have thrown");
    } catch (const std::runtime_error& e) {
        std::cout << "\n  Expected error: " << e.what() << std::endl;
    }
    
    // Increase limit, should pass
    research.developer_memory_limit_mb = 200;
    validate_model_safety(BucketModel::CUSTOM, 1600, cfg, research);
    
    std::cout << "PASSED ✓" << std::endl;
}

void test_ultra_high_config() {
    std::cout << "Testing get_ultra_high_config()... ";
    
    ModelData config = get_ultra_high_config();
    
    #ifdef __EMSCRIPTEN__
        // WASM: 32M/32M/32M
        assert(config.d7 == (32 << 20));
        assert(config.d8 == (32 << 20));
        assert(config.d9 == (32 << 20));
        std::cout << "WASM config... ";
    #else
        // Native: 32M/64M/64M
        assert(config.d7 == (32 << 20));
        assert(config.d8 == (64 << 20));
        assert(config.d9 == (64 << 20));
        std::cout << "Native config... ";
    #endif
    
    std::cout << "PASSED ✓" << std::endl;
}

int main() {
    std::cout << "\n=== Bucket Configuration Tests ===" << std::endl;
    
    test_power_of_two();
    test_model_selection();
    test_custom_bucket_validation();
    test_rss_estimation();
    test_developer_memory_limit();
    test_ultra_high_config();
    
    std::cout << "\n=== All Tests Passed ✓ ===" << std::endl;
    
    return 0;
}
