#ifndef EXPANSION_PARAMETERS_H
#define EXPANSION_PARAMETERS_H

#include <cstddef>
#include <map>
#include <tuple>

// Parameters for depth and bucket size combinations
struct ExpansionParameters {
    // Effective load factor (Step 2: Random expansion)
    double effective_load_factor;
    
    // Backtrace load factor (Step 5: Backtrace expansion)
    double backtrace_load_factor;
    
    // Measured safety factor (calculated memory → actual RSS)
    double measured_memory_factor;
    
    // Whether parameters are configured
    bool is_configured;
    
    ExpansionParameters() 
        : effective_load_factor(0.0)
        , backtrace_load_factor(0.0)
        , measured_memory_factor(1.0)
        , is_configured(false) {}
    
    ExpansionParameters(double eff_load, double bt_load, double mem_factor)
        : effective_load_factor(eff_load)
        , backtrace_load_factor(bt_load)
        , measured_memory_factor(mem_factor)
        , is_configured(true) {}
};

// Parameter table: (max_depth, bucket_size) -> ExpansionParameters
class ExpansionParameterTable {
private:
    std::map<std::pair<int, size_t>, ExpansionParameters> params_;
    
public:
    ExpansionParameterTable() {
        initialize_default_parameters();
    }
    
    void initialize_default_parameters() {
        // Bucket size constants
        const size_t BUCKET_2M  = (1 << 21);
        const size_t BUCKET_4M  = (1 << 22);
        const size_t BUCKET_8M  = (1 << 23);
        const size_t BUCKET_16M = (1 << 24);
        const size_t BUCKET_32M = (1 << 25);
        const size_t BUCKET_64M = (1 << 26);
        
        // ========================================
        // depth=4 parameters (estimated)
        // ========================================
        // 2M bucket: small bucket, conservative values
        set(4, BUCKET_2M,  0.65, 0.80, 2.0);   // Conservative
        // 4M bucket: high load factor achievable with good hash
        set(4, BUCKET_4M,  0.70, 0.85, 1.8);   // Conservative
        set(4, BUCKET_8M,  0.75, 0.88, 1.7);
        set(4, BUCKET_16M, 0.78, 0.90, 1.6);
        set(4, BUCKET_32M, 0.80, 0.90, 1.5);
        
        // ========================================
        // depth=5 parameters (estimated from measurements)
        // ========================================
        // Number of nodes: ~380K
        // BFS memory: ~200MB estimated
        set(5, BUCKET_2M,  0.45, 0.75, 2.2);   // Smallest bucket: highest factor
        set(5, BUCKET_4M,  0.50, 0.80, 2.0);   // Small bucket, low depth: high factor
        set(5, BUCKET_8M,  0.60, 0.85, 1.8);
        set(5, BUCKET_16M, 0.70, 0.88, 1.6);
        set(5, BUCKET_32M, 0.75, 0.90, 1.5);
        
        // ========================================
        // depth=6 parameters (measured)
        // ========================================
        // Number of nodes: 4.2M
        // BFS memory: ~1.2GB (measured)
        
        // 2M bucket: estimated (more conservative than 4M)
        set(6, BUCKET_2M,  0.85, 0.65, 2.8);   // Conservative estimate
        // 4M bucket: measured 3.8M nodes achieved (theoretical 0.13M)
        // → effective load = 3.8M / 4.2M = 90%
        set(6, BUCKET_4M,  0.90, 0.70, 2.5);   // Measured base
        
        // 8M bucket: measured 7.5M nodes achieved (theoretical 0.42M)
        // → effective load = 7.5M / 8.4M = 90%
        set(6, BUCKET_8M,  0.90, 0.75, 2.0);   // Measured base
        
        // 16M bucket: estimated (lower load than 32M)
        set(6, BUCKET_16M, 0.80, 0.80, 1.6);
        
        // 32M bucket: measured (1.9GB limit)
        // Measured: 19.5M nodes, RSS=2140MB, calculated=1583MB
        // → factor = 2140/1583 = 1.35 (including cur_set)
        set(6, BUCKET_32M, 0.58, 0.855, 1.30); // Measured base (1.9GB environment)
        
        // 64M bucket: empirical
        set(6, BUCKET_64M, 0.70, 0.70, 1.25);
        
        // ========================================
        // depth=7 parameters (estimated)
        // ========================================
        // Estimated number of nodes: ~50M
        // Estimated BFS memory: 3-5GB
        set(7, BUCKET_2M,  0.80, 0.60, 2.5);   // Smallest bucket: highest factor (important for 4GB smartphones)
        set(7, BUCKET_4M,  0.83, 0.65, 2.3);   // Small bucket: high factor
        set(7, BUCKET_8M,  0.85, 0.70, 2.2);   // Small bucket: high factor
        set(7, BUCKET_16M, 0.88, 0.75, 1.8);
        set(7, BUCKET_32M, 0.90, 0.80, 1.5);
        set(7, BUCKET_64M, 0.70, 0.85, 1.35);
    }
    
    // Set parameters
    void set(int max_depth, size_t bucket_size, 
             double eff_load, double bt_load, double mem_factor) {
        params_[{max_depth, bucket_size}] = 
            ExpansionParameters(eff_load, bt_load, mem_factor);
    }
    
    // Get parameters (return default if not found)
    ExpansionParameters get(int max_depth, size_t bucket_size) const {
        auto key = std::make_pair(max_depth, bucket_size);
        auto it = params_.find(key);
        
        if (it != params_.end()) {
            return it->second;
        }
        
        // Default values (conservative)
        return get_default_parameters(bucket_size);
    }
    
    // Get default values based on bucket size only
    ExpansionParameters get_default_parameters(size_t bucket_size) const {
        const size_t BUCKET_2M  = (1 << 21);
        const size_t BUCKET_4M  = (1 << 22);
        const size_t BUCKET_8M  = (1 << 23);
        const size_t BUCKET_16M = (1 << 24);
        const size_t BUCKET_32M = (1 << 25);
        
        if (bucket_size <= BUCKET_2M) {
            return ExpansionParameters(0.25, 0.65, 2.8);
        } else if (bucket_size <= BUCKET_4M) {
            return ExpansionParameters(0.30, 0.70, 2.5);
        } else if (bucket_size <= BUCKET_8M) {
            return ExpansionParameters(0.40, 0.75, 2.0);
        } else if (bucket_size <= BUCKET_16M) {
            return ExpansionParameters(0.50, 0.80, 1.6);
        } else if (bucket_size <= BUCKET_32M) {
            return ExpansionParameters(0.58, 0.85, 1.4);
        } else {
            return ExpansionParameters(0.70, 0.85, 1.3);
        }
    }
    
    // Check if parameters are set
    bool has_parameters(int max_depth, size_t bucket_size) const {
        auto key = std::make_pair(max_depth, bucket_size);
        return params_.find(key) != params_.end();
    }
    
    // Print all parameters
    void print_all_parameters() const {
        std::cout << "\n=== Expansion Parameter Table ===" << std::endl;
        std::cout << "max_depth | bucket_size | eff_load | bt_load | mem_factor" << std::endl;
        std::cout << "----------|-------------|----------|---------|------------" << std::endl;
        
        for (const auto& entry : params_) {
            int depth = entry.first.first;
            size_t bucket = entry.first.second;
            const auto& p = entry.second;
            
            std::cout << "    " << depth << "     |   "
                      << (bucket >> 20) << "M       |  "
                      << p.effective_load_factor << "  |  "
                      << p.backtrace_load_factor << "  |   "
                      << p.measured_memory_factor << std::endl;
        }
        std::cout << std::endl;
    }
};

// Global parameter table
static ExpansionParameterTable g_expansion_params;

// Parameter retrieval functions based on depth and bucket size
inline double get_effective_load_factor(int max_depth, size_t bucket_size) {
    return g_expansion_params.get(max_depth, bucket_size).effective_load_factor;
}

inline double get_backtrace_load_factor(int max_depth, size_t bucket_size) {
    return g_expansion_params.get(max_depth, bucket_size).backtrace_load_factor;
}

inline double get_measured_memory_factor(int max_depth, size_t bucket_size) {
    return g_expansion_params.get(max_depth, bucket_size).measured_memory_factor;
}

#endif // EXPANSION_PARAMETERS_H
