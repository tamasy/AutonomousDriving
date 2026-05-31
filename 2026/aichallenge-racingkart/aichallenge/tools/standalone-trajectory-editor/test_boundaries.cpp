#include "src/core/track_boundaries.hpp"
#include <iostream>

int main() {
    trajectory_editor::TrackBoundaries boundaries;
    
    std::cout << "🔍 Testing track boundaries loading..." << std::endl;
    bool success = boundaries.loadFromCSV("data/track_boundaries.csv");
    
    if (success) {
        std::cout << "✅ Success! Loaded track boundaries" << std::endl;
        
        std::cout << "📍 Left boundary: " << boundaries.getLeftBoundary().size() << " points" << std::endl;
        std::cout << "📍 Right boundary: " << boundaries.getRightBoundary().size() << " points" << std::endl;
        
        double min_x, max_x, min_y, max_y;
        boundaries.getBounds(min_x, max_x, min_y, max_y);
        
        std::cout << "🗺️ Boundary bounds: X[" << min_x << ", " << max_x << "] Y[" << min_y << ", " << max_y << "]" << std::endl;
        
        if (!boundaries.getLeftBoundary().empty()) {
            const auto& first = boundaries.getLeftBoundary()[0];
            std::cout << "🔴 First left point: (" << first.x << ", " << first.y << ", " << first.z << ")" << std::endl;
        }
        
        if (!boundaries.getRightBoundary().empty()) {
            const auto& first = boundaries.getRightBoundary()[0];
            std::cout << "🔵 First right point: (" << first.x << ", " << first.y << ", " << first.z << ")" << std::endl;
        }
        
        std::cout << "\n🎯 Track boundaries ready!" << std::endl;
        std::cout << "Run: ./build/trajectory_editor" << std::endl;
        
    } else {
        std::cout << "❌ Failed to load track boundaries" << std::endl;
        return 1;
    }
    
    return 0;
}