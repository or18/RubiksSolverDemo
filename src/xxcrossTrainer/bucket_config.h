#ifndef BUCKET_CONFIG_H
#define BUCKET_CONFIG_H

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_map>

// =============================================================================
// Bucket Model Enumeration
// =============================================================================

enum class BucketModel {
    AUTO,              // Automatic selection based on environment and memory budget
    
    // Preset models (7 variants, shared by WASM and Native except ULTRA_HIGH)
    // Note: Wide gaps between models - developers should add intermediate models
    MINIMAL,           // 2M/4M/4M (~145 MB)
    LOW,               // 4M/4M/4M (~190 MB)
    BALANCED,          // 4M/8M/8M (~290 MB) - Default
    MEDIUM,            // 8M/8M/8M (~440 MB)
    STANDARD,          // 8M/16M/16M (~540 MB)
    HIGH,              // 16M/32M/32M (~980 MB)
    ULTRA_HIGH,        // WASM: 32M/32M/32M (~1100 MB), Native: 32M/64M/64M (~1850 MB)
    
    // WASM-specific tier models (name-based selection for HTML UI)
    WASM_MOBILE_LOW,      // 1M/1M/2M/4M (618 MB dual-heap)
    WASM_MOBILE_MIDDLE,   // 2M/4M/4M/4M (896 MB dual-heap)
    WASM_MOBILE_HIGH,     // 4M/4M/4M/4M (1070 MB dual-heap)
    WASM_DESKTOP_STD,     // 8M/8M/8M/8M (1512 MB dual-heap)
    WASM_DESKTOP_HIGH,    // 8M/16M/16M/16M (2786 MB dual-heap)
    WASM_DESKTOP_ULTRA,   // 16M/16M/16M/16M (2884 MB dual-heap, 4GB limit)
    
    CUSTOM,            // User-specified (requires enable_custom_buckets=true)
    FULL_BFS           // Research mode: no local expansion
};

// =============================================================================
// Model Data Structure
// =============================================================================

struct ModelData {
    size_t d7, d8, d9;           // Bucket sizes (bytes)
    size_t measured_rss_mb;      // Peak RSS in MB (no margin)
    size_t total_nodes_million;  // Total nodes explored (debugging)
    double load_d7, load_d8, load_d9;  // Load factors (debugging)
    
    ModelData() : d7(0), d8(0), d9(0), measured_rss_mb(0), 
                  total_nodes_million(0), load_d7(0), load_d8(0), load_d9(0) {}
    
    ModelData(size_t bucket7, size_t bucket8, size_t bucket9,
              size_t rss, size_t nodes, double ld7, double ld8, double ld9)
        : d7(bucket7), d8(bucket8), d9(bucket9)
        , measured_rss_mb(rss), total_nodes_million(nodes)
        , load_d7(ld7), load_d8(ld8), load_d9(ld9) {}
};

// =============================================================================
// Configuration Structures
// =============================================================================

struct BucketConfig {
    BucketModel model = BucketModel::AUTO;
    
    // For CUSTOM model (requires ResearchConfig.enable_custom_buckets = true)
    size_t custom_bucket_7 = 0;  // Must be power of 2 (1M-64M)
    size_t custom_bucket_8 = 0;
    size_t custom_bucket_9 = 0;
    size_t custom_bucket_10 = 0; // Depth 10 bucket (NEW: for volume peak coverage)
    
    // For AUTO model (optional override)
    size_t memory_budget_mb = 0;  // 0 = use constructor's MEMORY_LIMIT_MB
};

struct ResearchConfig {
    // Core research modes
    bool enable_local_expansion = true;       // false = full BFS only
    int force_full_bfs_to_depth = -1;         // -1=auto, 7-10=force BFS to this depth
    bool ignore_memory_limits = false;        // true = no budget checks
    bool collect_detailed_statistics = false; // true = per-depth RSS, nodes, time
    bool dry_run = false;                     // true = measure only, don't build database
    
    // Custom bucket controls (safety gates)
    bool enable_custom_buckets = false;       // true = allow CUSTOM bucket model
    #ifdef __EMSCRIPTEN__
    bool high_memory_wasm_mode = true;        // WASM: Always true (allow >1200MB, custom buckets)
    #else
    bool high_memory_wasm_mode = false;       // Native: false by default
    #endif
    
    // Developer memory limit (early theoretical check)
    size_t developer_memory_limit_mb = 2048;  // Warn/block if estimated RSS exceeds (0 = no limit)
    
    // Next-depth reserve optimization (for full BFS mode)
    bool enable_next_depth_reserve = true;         // true = reserve next depth using predicted size
    float next_depth_reserve_multiplier = 12.5f;   // Prediction multiplier (e.g., 12.5x previous depth)
    size_t max_reserve_nodes = 200000000;          // Upper limit for reserve (200M nodes, ~1.6GB)
    
    // Allocator cache control (for WASM-equivalent measurements on native)
    bool disable_malloc_trim = false;         // true = skip malloc_trim() for WASM-equivalent RSS measurement
    
    // Developer convenience options
    bool skip_search = false;                 // true = exit after database construction (measurement only)
    int benchmark_iterations = 1;             // Number of search iterations for benchmarking (1 = normal)
};

// =============================================================================
// Measured Data Table (Initially Empty - To Be Filled After Spike Fixes)
// =============================================================================

// Note: This table will be populated after memory spike elimination is complete.
// Currently all values are placeholders (0). Developers should run measurement
// tests to fill in actual RSS data.

inline const std::unordered_map<BucketModel, ModelData>& get_measured_data() {
    static const std::unordered_map<BucketModel, ModelData> MEASURED_DATA = {
        // Preset models (7 variants) - PLACEHOLDER VALUES
        // TODO: Measure actual RSS after spike fixes
        {BucketModel::MINIMAL,    ModelData(2<<20, 4<<20, 4<<20,   0, 0, 0.0, 0.0, 0.0)},
        {BucketModel::LOW,        ModelData(4<<20, 4<<20, 4<<20,   0, 0, 0.0, 0.0, 0.0)},
        {BucketModel::BALANCED,   ModelData(4<<20, 8<<20, 8<<20,   0, 0, 0.0, 0.0, 0.0)},
        {BucketModel::MEDIUM,     ModelData(8<<20, 8<<20, 8<<20,   0, 0, 0.0, 0.0, 0.0)},
        {BucketModel::STANDARD,   ModelData(8<<20, 16<<20, 16<<20, 0, 0, 0.0, 0.0, 0.0)},
        {BucketModel::HIGH,       ModelData(16<<20, 32<<20, 32<<20, 0, 0, 0.0, 0.0, 0.0)},
        
        // WASM Tier Models (depth 10 enabled, measured 2026-01-03)
        // Format: d7/d8/d9/d10, single_heap_mb, dual_heap_mb (2x), total_nodes
        {BucketModel::WASM_MOBILE_LOW,    ModelData(1<<20, 1<<20, 2<<20,   309, 12, 0.90, 0.90, 0.90)},
        {BucketModel::WASM_MOBILE_MIDDLE, ModelData(2<<20, 4<<20, 4<<20,   448, 18, 0.90, 0.90, 0.90)},
        {BucketModel::WASM_MOBILE_HIGH,   ModelData(4<<20, 4<<20, 4<<20,   535, 20, 0.90, 0.90, 0.90)},
        {BucketModel::WASM_DESKTOP_STD,   ModelData(8<<20, 8<<20, 8<<20,   756, 35, 0.90, 0.90, 0.90)},
        {BucketModel::WASM_DESKTOP_HIGH,  ModelData(8<<20, 16<<20, 16<<20, 1393, 57, 0.90, 0.90, 0.90)},
        {BucketModel::WASM_DESKTOP_ULTRA, ModelData(16<<20, 16<<20, 16<<20, 1442, 65, 0.90, 0.90, 0.90)},
        
        // ULTRA_HIGH: Different for WASM vs Native (handled by get_ultra_high_config)
    };
    return MEASURED_DATA;
}

// ULTRA_HIGH model uses runtime environment detection
inline ModelData get_ultra_high_config() {
    #ifdef __EMSCRIPTEN__
        return ModelData(32<<20, 32<<20, 32<<20, 0, 0, 0.0, 0.0, 0.0);  // WASM safe (placeholder)
    #else
        return ModelData(32<<20, 64<<20, 64<<20, 0, 0, 0.0, 0.0, 0.0);  // Native high-capacity (placeholder)
    #endif
}

// =============================================================================
// Model Selection Functions
// =============================================================================

inline BucketModel select_model(size_t budget_mb) {
    // TODO: Implement proper selection logic after measurements are complete
    // For now, return a safe default
    
    // Select largest model ≤ budget (same for WASM and Native, except ULTRA_HIGH)
    if (budget_mb >= 1100) return BucketModel::ULTRA_HIGH;  // WASM: 1100 MB, Native: 1850 MB
    if (budget_mb >= 980)  return BucketModel::HIGH;        // 980 MB
    if (budget_mb >= 540)  return BucketModel::STANDARD;    // 540 MB
    if (budget_mb >= 440)  return BucketModel::MEDIUM;      // 440 MB
    if (budget_mb >= 290)  return BucketModel::BALANCED;    // 290 MB (default)
    if (budget_mb >= 190)  return BucketModel::LOW;         // 190 MB
    return BucketModel::MINIMAL;  // 145 MB (minimum)
}

// =============================================================================
// Validation Functions
// =============================================================================

inline bool is_power_of_two(size_t n) {
    return n != 0 && (n & (n - 1)) == 0;
}

inline void validate_custom_buckets(const BucketConfig& cfg, const ResearchConfig& research) {
    if (cfg.model != BucketModel::CUSTOM) return;
    
    // Check 1: Custom buckets require explicit flag
    if (!research.enable_custom_buckets) {
        throw std::runtime_error(
            "CUSTOM model requires ResearchConfig.enable_custom_buckets=true"
        );
    }
    
    // Check 2: Bucket sizes must be powers of 2
    if (!is_power_of_two(cfg.custom_bucket_7) ||
        !is_power_of_two(cfg.custom_bucket_8) ||
        !is_power_of_two(cfg.custom_bucket_9)) {
        throw std::runtime_error(
            "Custom bucket sizes must be powers of 2 (1M, 2M, 4M, 8M, 16M, 32M, 64M)"
        );
    }
    
    // Check 3: Bucket size range (1M - 64M)
    const size_t MIN_BUCKET = 1 << 20;  // 1M
    const size_t MAX_BUCKET = 64 << 20; // 64M
    
    if (cfg.custom_bucket_7 < MIN_BUCKET || cfg.custom_bucket_7 > MAX_BUCKET ||
        cfg.custom_bucket_8 < MIN_BUCKET || cfg.custom_bucket_8 > MAX_BUCKET ||
        cfg.custom_bucket_9 < MIN_BUCKET || cfg.custom_bucket_9 > MAX_BUCKET) {
        throw std::runtime_error(
            "Custom bucket sizes must be in range [1M, 64M]"
        );
    }
    
    // Check 4: If bucket_d10 specified, validate it too
    if (cfg.custom_bucket_10 > 0) {
        if (!is_power_of_two(cfg.custom_bucket_10)) {
            throw std::runtime_error(
                "Custom bucket_d10 must be power of 2 (1M, 2M, 4M, 8M, 16M, 32M, 64M)"
            );
        }
        if (cfg.custom_bucket_10 < MIN_BUCKET || cfg.custom_bucket_10 > MAX_BUCKET) {
            throw std::runtime_error(
                "Custom bucket_d10 must be in range [1M, 64M]"
            );
        }
    }
}

inline size_t estimate_custom_rss(size_t d7_bucket, size_t d8_bucket, size_t d9_bucket) {
    // Conservative estimates based on empirical load factors
    const double LOAD_D7 = 0.90;  // depth 7 typical load
    const double LOAD_D8 = 0.89;  // depth 8 typical load  
    const double LOAD_D9 = 0.70;  // depth 9 typical load
    const size_t BASE_OVERHEAD_MB = 50;  // Solver overhead, vectors, etc.
    
    size_t d7_rss = (d7_bucket >> 20) * LOAD_D7;  // Convert to MB
    size_t d8_rss = (d8_bucket >> 20) * LOAD_D8;
    size_t d9_rss = (d9_bucket >> 20) * LOAD_D9;
    
    return d7_rss + d8_rss + d9_rss + BASE_OVERHEAD_MB;
}

inline void validate_model_safety(BucketModel model, size_t budget_mb, 
                                  const BucketConfig& cfg, const ResearchConfig& research) {
    // Check 1: Custom buckets validation
    validate_custom_buckets(cfg, research);
    
    // Check 2: WASM-specific checks
    #ifdef __EMSCRIPTEN__
        const size_t WASM_SAFE_LIMIT_MB = 1200;
        
        // Check 2a: Custom buckets in WASM require high-memory mode
        if (model == BucketModel::CUSTOM && !research.high_memory_wasm_mode) {
            throw std::runtime_error(
                "CUSTOM model in WASM requires ResearchConfig.high_memory_wasm_mode=true"
            );
        }
        
        // Check 2b: Budget exceeds WASM safe limit
        if (budget_mb > WASM_SAFE_LIMIT_MB && !research.high_memory_wasm_mode) {
            throw std::runtime_error(
                "Budget exceeds WASM safe limit (" + std::to_string(budget_mb) + 
                " MB > " + std::to_string(WASM_SAFE_LIMIT_MB) + " MB). " +
                "Use ResearchConfig.high_memory_wasm_mode=true if intentional."
            );
        }
    #endif
    
    // Check 3: Custom buckets RSS estimation (developer memory limit)
    if (model == BucketModel::CUSTOM && !research.ignore_memory_limits) {
        size_t estimated_rss_mb = estimate_custom_rss(cfg.custom_bucket_7, 
                                                       cfg.custom_bucket_8, 
                                                       cfg.custom_bucket_9);
        
        size_t limit_mb = research.developer_memory_limit_mb;
        if (limit_mb > 0 && estimated_rss_mb > limit_mb) {
            throw std::runtime_error(
                "Custom buckets exceed developer limit (estimated: " +
                std::to_string(estimated_rss_mb) + " MB > " + 
                std::to_string(limit_mb) + " MB). " +
                "Reduce bucket sizes or set ignore_memory_limits=true."
            );
        }
    }
}

#endif // BUCKET_CONFIG_H
