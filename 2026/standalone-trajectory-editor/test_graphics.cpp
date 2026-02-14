#include "src/core/trajectory_data.hpp"
#include <iostream>

int main() {
    trajectory_editor::TrajectoryData data;
    
    std::cout << "Testing CSV load..." << std::endl;
    bool success = data.loadFromCSV("data/raceline_base.csv");
    
    if (success) {
        std::cout << "✅ Success! Loaded " << data.size() << " points" << std::endl;
        
        double min_x, max_x, min_y, max_y;
        data.getBounds(min_x, max_x, min_y, max_y);
        
        double min_vel, max_vel;
        data.getVelocityRange(min_vel, max_vel);
        
        std::cout << "📍 Bounds: X[" << min_x << ", " << max_x << "] Y[" << min_y << ", " << max_y << "]" << std::endl;
        std::cout << "🚗 Velocity: [" << min_vel << ", " << max_vel << "] km/h" << std::endl;
        
        std::cout << "\n🎯 Graphics trajectory editor ready!" << std::endl;
        std::cout << "Run: ./build/trajectory_editor" << std::endl;
        
    } else {
        std::cout << "❌ Failed to load CSV file" << std::endl;
        return 1;
    }
    
    return 0;
}