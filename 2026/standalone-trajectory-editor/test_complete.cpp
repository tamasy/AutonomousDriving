#include "src/core/trajectory_data.hpp"
#include "src/core/track_boundaries.hpp"
#include <iostream>

int main() {
    std::cout << "🔍 Testing complete trajectory editor setup..." << std::endl;
    
    // 軌跡データテスト
    trajectory_editor::TrajectoryData trajectory;
    bool traj_success = trajectory.loadFromCSV("data/raceline_base.csv");
    
    // 境界線データテスト
    trajectory_editor::TrackBoundaries boundaries;
    bool bound_success = boundaries.loadFromCSV("data/track_boundaries.csv");
    
    if (traj_success && bound_success) {
        std::cout << "✅ All data loaded successfully!" << std::endl;
        
        // 軌跡データ情報
        std::cout << "\n📍 Trajectory: " << trajectory.size() << " points" << std::endl;
        double traj_min_x, traj_max_x, traj_min_y, traj_max_y;
        trajectory.getBounds(traj_min_x, traj_max_x, traj_min_y, traj_max_y);
        std::cout << "   Range: X[" << traj_min_x << ", " << traj_max_x << "] Y[" << traj_min_y << ", " << traj_max_y << "]" << std::endl;
        
        // 境界線データ情報
        std::cout << "\n🏁 Track boundaries:" << std::endl;
        std::cout << "   Left: " << boundaries.getLeftBoundary().size() << " points" << std::endl;
        std::cout << "   Right: " << boundaries.getRightBoundary().size() << " points" << std::endl;
        
        double bound_min_x, bound_max_x, bound_min_y, bound_max_y;
        boundaries.getBounds(bound_min_x, bound_max_x, bound_min_y, bound_max_y);
        std::cout << "   Range: X[" << bound_min_x << ", " << bound_max_x << "] Y[" << bound_min_y << ", " << bound_max_y << "]" << std::endl;
        
        // データの整合性チェック
        bool coordinates_match = 
            (traj_min_x >= bound_min_x - 100 && traj_max_x <= bound_max_x + 100) &&
            (traj_min_y >= bound_min_y - 100 && traj_max_y <= bound_max_y + 100);
        
        std::cout << "\n🎯 Data consistency: " << (coordinates_match ? "✅ Good" : "⚠️ Check ranges") << std::endl;
        
        std::cout << "\n🚀 Ready to run trajectory editor!" << std::endl;
        std::cout << "   Execute: ./build/trajectory_editor" << std::endl;
        std::cout << "\n🎮 Features available:" << std::endl;
        std::cout << "   • Load/Save CSV trajectory files" << std::endl;
        std::cout << "   • Visual trajectory with speed colors" << std::endl;
        std::cout << "   • Track boundaries display" << std::endl;
        std::cout << "   • Zoom/Pan/Fit controls" << std::endl;
        std::cout << "   • Point selection and info" << std::endl;
        
    } else {
        std::cout << "❌ Data loading failed!" << std::endl;
        if (!traj_success) std::cout << "   Trajectory data not found" << std::endl;
        if (!bound_success) std::cout << "   Boundary data not found" << std::endl;
        return 1;
    }
    
    return 0;
}