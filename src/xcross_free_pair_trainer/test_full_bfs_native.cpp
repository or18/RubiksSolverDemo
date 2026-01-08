// Undefine main if solver_dev.cpp has one
#define SKIP_SOLVER_MAIN
#include "solver_dev.cpp"
#undef main

#include <iostream>
#include <iomanip>
#include <chrono>

int main(int argc, char* argv[]) {
    int max_depth = 5;
    
    if (argc > 1) {
        max_depth = std::atoi(argv[1]);
        if (max_depth < 1 || max_depth > 9) {
            std::cerr << "Error: max_depth must be between 1 and 9" << std::endl;
            return 1;
        }
    }
    
    std::cout << "===========================================\n";
    std::cout << "Full BFS Mode Test (Native Build)\n";
    std::cout << "===========================================\n";
    std::cout << "Target depth: " << max_depth << "\n";
    std::cout << "Configuration: Adjacent slots\n";
    std::cout << "Mode: Full BFS (no local expansion)\n";
    std::cout << "===========================================\n\n";
    
    // Set environment variables for full BFS mode
    setenv("FORCE_FULL_BFS_TO_DEPTH", std::to_string(max_depth).c_str(), 1);
    setenv("ENABLE_LOCAL_EXPANSION", "0", 1);
    setenv("COLLECT_DETAILED_STATISTICS", "1", 1);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        // Create solver with full BFS mode
        // Using very large memory limit to ensure full BFS completes
        xxcross_search solver(true, 1, 1, 1, 1); // adjacent=true, buckets not used in full BFS
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "\n===========================================\n";
        std::cout << "✅ Full BFS completed successfully\n";
        std::cout << "Total time: " << std::fixed << std::setprecision(2) 
                  << (duration.count() / 1000.0) << " seconds\n";
        std::cout << "===========================================\n";
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
