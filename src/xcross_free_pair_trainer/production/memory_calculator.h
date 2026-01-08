#ifndef MEMORY_CALCULATOR_H
#define MEMORY_CALCULATOR_H

#include <cstddef>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

/**
 * @file memory_calculator.h
 * @brief Memory usage calculation and overhead analysis for xxcross solver
 * 
 * Purpose:
 * - Calculate theoretical memory usage from node counts and bucket sizes
 * - Compare theoretical calculations with actual RSS measurements
 * - Analyze overhead per phase (Phase 1 BFS, Phase 2-4 local expansion)
 * - Provide RSS prediction for developer safety checks
 * 
 * Design Philosophy:
 * - Standalone header (no dependencies on solver_dev.cpp internals)
 * - Easy to integrate into solver_dev.cpp when needed
 * - Supports both theoretical estimation and empirical validation
 * 
 * Future Integration with solver_dev.cpp:
 * - Pre-flight RSS prediction before database construction
 * - Per-phase overhead monitoring and reporting
 * - Empirical parameter tuning (load factors, memory factors)
 * - Research mode statistics collection
 * 
 * @date 2026-01-01
 * @version 1.0
 */

namespace memory_calc {

// =============================================================================
// Constants (from MEMORY_BUDGET_DESIGN.md Section 4.1)
// =============================================================================

constexpr size_t BYTES_PER_NODE = 32;       // robin_set(24) + index_pairs(8)
constexpr size_t BYTES_PER_BUCKET = 4;      // robin_hash bucket overhead
constexpr size_t BYTES_PER_INDEX_PAIR = 8;  // uint64_t in index_pairs

constexpr double DEFAULT_MEMORY_FACTOR = 2.5;  // Theoretical → RSS multiplier
constexpr double CPP_INTERNAL_MARGIN = 0.02;   // 2% C++ runtime variation

// =============================================================================
// Phase Definitions
// =============================================================================

enum class Phase {
    PHASE_0_INIT,        // Initialization (tables, overhead)
    PHASE_1_BFS,         // Full BFS (depth 0 to BFS_DEPTH, typically 6)
    PHASE_2_DEPTH7,      // Partial expansion depth 7
    PHASE_3_DEPTH8,      // Local expansion depth 7→8
    PHASE_4_DEPTH9,      // Local expansion depth 8→9
    TOTAL               // Total across all phases
};

inline const char* phase_name(Phase phase) {
    switch (phase) {
        case Phase::PHASE_0_INIT:   return "Phase 0 (Init)";
        case Phase::PHASE_1_BFS:    return "Phase 1 (BFS)";
        case Phase::PHASE_2_DEPTH7: return "Phase 2 (Depth 7)";
        case Phase::PHASE_3_DEPTH8: return "Phase 3 (Depth 8)";
        case Phase::PHASE_4_DEPTH9: return "Phase 4 (Depth 9)";
        case Phase::TOTAL:          return "Total";
        default:                    return "Unknown";
    }
}

// =============================================================================
// Memory Components Breakdown
// =============================================================================

struct MemoryComponents {
    // Data structure memory (theoretical)
    size_t robin_set_buckets_mb;    // Bucket array memory
    size_t robin_set_nodes_mb;      // Node storage in robin_set
    size_t index_pairs_mb;          // index_pairs vectors
    size_t theoretical_total_mb;    // Sum of above
    
    // Overhead
    size_t cpp_overhead_mb;         // C++ runtime, allocator, etc.
    size_t predicted_rss_mb;        // theoretical_total × memory_factor
    
    // Empirical (if available)
    size_t measured_rss_mb;         // Actual RSS measurement
    int overhead_mb;                // measured - predicted (can be negative)
    double overhead_percent;        // (measured - predicted) / predicted * 100
    
    MemoryComponents()
        : robin_set_buckets_mb(0)
        , robin_set_nodes_mb(0)
        , index_pairs_mb(0)
        , theoretical_total_mb(0)
        , cpp_overhead_mb(0)
        , predicted_rss_mb(0)
        , measured_rss_mb(0)
        , overhead_mb(0)
        , overhead_percent(0.0)
    {}
};

// =============================================================================
// Basic Memory Calculations
// =============================================================================

/**
 * @brief Calculate next power of 2 (for bucket size calculations)
 */
inline size_t next_power_of_2(size_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

/**
 * @brief Calculate bucket size required for target nodes at given load factor
 * 
 * Formula: buckets = next_power_of_2(ceil(nodes / load_factor))
 * 
 * @param target_nodes Number of nodes to store
 * @param load_factor Target load factor (e.g., 0.9 for 90%)
 * @return Required bucket count (power of 2)
 */
inline size_t calculate_bucket_size(size_t target_nodes, double load_factor = 0.9) {
    if (target_nodes == 0) return 0;
    return next_power_of_2(static_cast<size_t>(std::ceil(
        static_cast<double>(target_nodes) / load_factor)));
}

/**
 * @brief Calculate memory usage of robin_set buckets
 * 
 * @param bucket_count Number of buckets
 * @return Memory in MB
 */
inline size_t calculate_bucket_memory_mb(size_t bucket_count) {
    return (bucket_count * BYTES_PER_BUCKET) / (1024 * 1024);
}

/**
 * @brief Calculate memory usage of robin_set nodes
 * 
 * @param node_count Number of nodes stored
 * @return Memory in MB (assumes 24 bytes per node in robin_set)
 */
inline size_t calculate_node_memory_mb(size_t node_count) {
    return (node_count * 24) / (1024 * 1024);  // 24 bytes per node in robin_set
}

/**
 * @brief Calculate memory usage of index_pairs vector
 * 
 * @param node_count Number of uint64_t elements
 * @return Memory in MB
 */
inline size_t calculate_index_pairs_memory_mb(size_t node_count) {
    return (node_count * BYTES_PER_INDEX_PAIR) / (1024 * 1024);
}

/**
 * @brief Calculate theoretical memory for a single depth
 * 
 * @param node_count Number of nodes at this depth
 * @param load_factor Load factor for robin_set
 * @return Theoretical memory in MB
 */
inline size_t calculate_depth_memory_mb(size_t node_count, double load_factor = 0.9) {
    if (node_count == 0) return 0;
    
    size_t buckets = calculate_bucket_size(node_count, load_factor);
    size_t bucket_mb = calculate_bucket_memory_mb(buckets);
    size_t node_mb = calculate_node_memory_mb(node_count);
    size_t index_pairs_mb = calculate_index_pairs_memory_mb(node_count);
    
    return bucket_mb + node_mb + index_pairs_mb;
}

// =============================================================================
// Phase-Based Memory Calculations
// =============================================================================

/**
 * @brief Memory usage per depth (for multi-depth calculations)
 */
struct DepthMemory {
    int depth;
    size_t nodes;
    size_t theoretical_mb;
    size_t measured_rss_mb;  // Optional (0 if not measured)
    
    DepthMemory(int d = 0, size_t n = 0, size_t theo = 0, size_t rss = 0)
        : depth(d), nodes(n), theoretical_mb(theo), measured_rss_mb(rss) {}
};

/**
 * @brief Calculate Phase 1 (BFS) memory usage
 * 
 * Phase 1 performs full BFS from depth 0 to max_bfs_depth (typically 6).
 * Uses SlidingDepthSets (prev, cur, next) with 3 active depths at peak.
 * 
 * @param node_counts Vector of node counts per depth [0..max_bfs_depth]
 * @param load_factor Load factor for robin_sets
 * @param memory_factor Theoretical → RSS multiplier (default 2.5)
 * @return Memory components breakdown
 */
inline MemoryComponents calculate_phase1_memory(
    const std::vector<size_t>& node_counts,
    double load_factor = 0.9,
    double memory_factor = DEFAULT_MEMORY_FACTOR)
{
    MemoryComponents mem;
    
    if (node_counts.empty()) return mem;
    
    // Phase 1 uses SlidingDepthSets (prev, cur, next)
    // Peak memory occurs at depth N-1 where all three sets are populated:
    // - prev: depth N-2
    // - cur:  depth N-1
    // - next: depth N (being built)
    
    // Find peak memory configuration
    size_t peak_bucket_mb = 0;
    size_t peak_node_mb = 0;
    
    for (size_t d = 2; d < node_counts.size(); ++d) {
        // At depth d, we have prev(d-2), cur(d-1), next(d)
        size_t prev_nodes = (d >= 2) ? node_counts[d-2] : 0;
        size_t cur_nodes = (d >= 1) ? node_counts[d-1] : 0;
        size_t next_nodes = node_counts[d];
        
        size_t total_buckets = 0;
        size_t total_nodes = prev_nodes + cur_nodes + next_nodes;
        
        // Calculate buckets for each set
        total_buckets += calculate_bucket_size(prev_nodes, load_factor);
        total_buckets += calculate_bucket_size(cur_nodes, load_factor);
        total_buckets += calculate_bucket_size(next_nodes, load_factor);
        
        size_t bucket_mb = calculate_bucket_memory_mb(total_buckets);
        size_t node_mb = calculate_node_memory_mb(total_nodes);
        
        if (bucket_mb + node_mb > peak_bucket_mb + peak_node_mb) {
            peak_bucket_mb = bucket_mb;
            peak_node_mb = node_mb;
        }
    }
    
    // index_pairs stores all depths (cumulative)
    size_t total_index_pairs = 0;
    for (size_t nodes : node_counts) {
        total_index_pairs += nodes;
    }
    size_t index_pairs_mb = calculate_index_pairs_memory_mb(total_index_pairs);
    
    mem.robin_set_buckets_mb = peak_bucket_mb;
    mem.robin_set_nodes_mb = peak_node_mb;
    mem.index_pairs_mb = index_pairs_mb;
    mem.theoretical_total_mb = peak_bucket_mb + peak_node_mb + index_pairs_mb;
    mem.predicted_rss_mb = static_cast<size_t>(
        std::ceil(static_cast<double>(mem.theoretical_total_mb) * memory_factor));
    mem.cpp_overhead_mb = static_cast<size_t>(
        std::ceil(static_cast<double>(mem.predicted_rss_mb) * CPP_INTERNAL_MARGIN));
    
    return mem;
}

/**
 * @brief Calculate Phase 2-4 (local expansion) memory usage
 * 
 * Phase 2: Partial expansion depth 7
 * Phase 3: Local expansion depth 7→8 (with depth 6 backtrace check)
 * Phase 4: Local expansion depth 8→9 (with depth 6 distance check)
 * 
 * @param depth_n_nodes Nodes at depth N (e.g., depth 7 for Phase 2)
 * @param depth_n_buckets Bucket size for depth N
 * @param load_factor Load factor for robin_set
 * @param memory_factor Theoretical → RSS multiplier
 * @return Memory components breakdown
 */
inline MemoryComponents calculate_local_expansion_memory(
    size_t depth_n_nodes,
    size_t depth_n_buckets,
    double load_factor = 0.9,
    double memory_factor = DEFAULT_MEMORY_FACTOR)
{
    MemoryComponents mem;
    
    // Local expansion uses single robin_set at depth N
    mem.robin_set_buckets_mb = calculate_bucket_memory_mb(depth_n_buckets);
    mem.robin_set_nodes_mb = calculate_node_memory_mb(depth_n_nodes);
    mem.index_pairs_mb = calculate_index_pairs_memory_mb(depth_n_nodes);
    mem.theoretical_total_mb = mem.robin_set_buckets_mb + mem.robin_set_nodes_mb + mem.index_pairs_mb;
    mem.predicted_rss_mb = static_cast<size_t>(
        std::ceil(static_cast<double>(mem.theoretical_total_mb) * memory_factor));
    mem.cpp_overhead_mb = static_cast<size_t>(
        std::ceil(static_cast<double>(mem.predicted_rss_mb) * CPP_INTERNAL_MARGIN));
    
    return mem;
}

// =============================================================================
// Overhead Analysis
// =============================================================================

/**
 * @brief Analyze overhead between predicted and measured RSS
 * 
 * @param predicted_rss_mb Predicted RSS from theoretical calculation
 * @param measured_rss_mb Actual measured RSS
 * @param[out] overhead_mb Absolute overhead (measured - predicted)
 * @param[out] overhead_percent Relative overhead percentage
 */
inline void analyze_overhead(
    size_t predicted_rss_mb,
    size_t measured_rss_mb,
    int& overhead_mb,
    double& overhead_percent)
{
    overhead_mb = static_cast<int>(measured_rss_mb) - static_cast<int>(predicted_rss_mb);
    overhead_percent = (predicted_rss_mb > 0)
        ? (static_cast<double>(overhead_mb) / static_cast<double>(predicted_rss_mb) * 100.0)
        : 0.0;
}

/**
 * @brief Update memory components with measured RSS and calculate overhead
 */
inline void set_measured_rss(MemoryComponents& mem, size_t measured_rss_mb) {
    mem.measured_rss_mb = measured_rss_mb;
    analyze_overhead(mem.predicted_rss_mb, measured_rss_mb, 
                     mem.overhead_mb, mem.overhead_percent);
}

// =============================================================================
// Estimation from Bucket Sizes (for custom configurations)
// =============================================================================

/**
 * @brief Estimate RSS from bucket sizes (for WASM pre-computation)
 * 
 * Used when developer specifies custom bucket sizes but doesn't have
 * empirical node count data yet.
 * 
 * @param bucket_7 Bucket size for depth 7
 * @param bucket_8 Bucket size for depth 8
 * @param bucket_9 Bucket size for depth 9
 * @param load_factor Assumed load factor
 * @param memory_factor Theoretical → RSS multiplier
 * @return Estimated peak RSS in MB
 */
inline size_t estimate_rss_from_buckets(
    size_t bucket_7,
    size_t bucket_8,
    size_t bucket_9,
    double load_factor = 0.88,  // Conservative for local expansion
    double memory_factor = 2.5)
{
    // Estimate node counts from buckets
    size_t est_nodes_7 = static_cast<size_t>(bucket_7 * load_factor);
    size_t est_nodes_8 = static_cast<size_t>(bucket_8 * load_factor);
    size_t est_nodes_9 = static_cast<size_t>(bucket_9 * load_factor);
    
    // Phase 1: Assume depth 6 is 4.2M nodes (empirical for xxcross)
    const size_t DEPTH_6_NODES = 4200000;
    std::vector<size_t> phase1_nodes = {1, 18, 243, 3007, 35413, 391951, DEPTH_6_NODES};
    MemoryComponents phase1 = calculate_phase1_memory(phase1_nodes, 0.9, memory_factor);
    
    // Phase 2: depth 7
    MemoryComponents phase2 = calculate_local_expansion_memory(
        est_nodes_7, bucket_7, load_factor, memory_factor);
    
    // Phase 3: depth 8
    MemoryComponents phase3 = calculate_local_expansion_memory(
        est_nodes_8, bucket_8, load_factor, memory_factor);
    
    // Phase 4: depth 9
    MemoryComponents phase4 = calculate_local_expansion_memory(
        est_nodes_9, bucket_9, load_factor, memory_factor);
    
    // Peak RSS is max across all phases (not cumulative, due to local expansion)
    size_t peak_rss = std::max({
        phase1.predicted_rss_mb,
        phase2.predicted_rss_mb,
        phase3.predicted_rss_mb,
        phase4.predicted_rss_mb
    });
    
    // Add C++ internal margin
    size_t cpp_margin = static_cast<size_t>(std::ceil(peak_rss * CPP_INTERNAL_MARGIN));
    
    return peak_rss + cpp_margin;
}

// =============================================================================
// Reporting and Debugging
// =============================================================================

/**
 * @brief Format memory components as human-readable string
 */
inline std::string format_memory_report(const MemoryComponents& mem, const std::string& phase_name) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    
    oss << "=== " << phase_name << " Memory Report ===" << "\n";
    oss << "Theoretical Breakdown:\n";
    oss << "  robin_set buckets: " << mem.robin_set_buckets_mb << " MB\n";
    oss << "  robin_set nodes:   " << mem.robin_set_nodes_mb << " MB\n";
    oss << "  index_pairs:       " << mem.index_pairs_mb << " MB\n";
    oss << "  Theoretical Total: " << mem.theoretical_total_mb << " MB\n";
    oss << "Predicted RSS:       " << mem.predicted_rss_mb << " MB (factor × theoretical)\n";
    
    if (mem.measured_rss_mb > 0) {
        oss << "Measured RSS:        " << mem.measured_rss_mb << " MB\n";
        oss << "Overhead:            " << mem.overhead_mb << " MB (" 
            << mem.overhead_percent << "%)\n";
    }
    
    return oss.str();
}

/**
 * @brief Format CSV header for statistics collection
 */
inline std::string csv_header() {
    return "phase,depth,nodes,theoretical_mb,predicted_rss_mb,measured_rss_mb,overhead_mb,overhead_percent";
}

/**
 * @brief Format memory components as CSV row
 */
inline std::string format_csv_row(
    const std::string& phase_name,
    int depth,
    size_t nodes,
    const MemoryComponents& mem)
{
    std::ostringstream oss;
    oss << phase_name << ","
        << depth << ","
        << nodes << ","
        << mem.theoretical_total_mb << ","
        << mem.predicted_rss_mb << ","
        << mem.measured_rss_mb << ","
        << mem.overhead_mb << ","
        << std::fixed << std::setprecision(2) << mem.overhead_percent;
    return oss.str();
}

} // namespace memory_calc

#endif // MEMORY_CALCULATOR_H
