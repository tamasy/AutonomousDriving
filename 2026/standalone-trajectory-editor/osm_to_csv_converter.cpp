#include "src/utils/osm_parser.hpp"
#include <iostream>
#include <fstream>

int main() {
    std::cout << "🔍 OSM to CSV converter for track boundaries..." << std::endl;
    
    trajectory_editor::OSMParser parser;
    
    // OSMファイルを読み込み
    if (!parser.loadFromFile("data/lanelet2_map.osm")) {
        std::cout << "❌ Failed to load OSM file" << std::endl;
        return 1;
    }
    
    // 境界線データを抽出
    auto boundaries = parser.extractTrackBoundaries();
    
    if (boundaries.empty()) {
        std::cout << "❌ No track boundaries found in OSM file" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Found " << boundaries.size() << " lane boundaries" << std::endl;
    
    // 全ての境界線を結合
    std::vector<trajectory_editor::OSMNode> all_left_nodes, all_right_nodes;
    
    for (const auto& [left_boundary, right_boundary] : boundaries) {
        all_left_nodes.insert(all_left_nodes.end(), left_boundary.begin(), left_boundary.end());
        all_right_nodes.insert(all_right_nodes.end(), right_boundary.begin(), right_boundary.end());
        
        std::cout << "  Left: " << left_boundary.size() << " points, Right: " << right_boundary.size() << " points" << std::endl;
    }
    
    std::cout << "🏁 Total combined points - Left: " << all_left_nodes.size() << ", Right: " << all_right_nodes.size() << std::endl;
    
    // CSVファイルに出力
    std::ofstream csv_file("data/track_boundaries.csv");
    if (!csv_file.is_open()) {
        std::cout << "❌ Failed to create CSV file" << std::endl;
        return 1;
    }
    
    // ヘッダー
    csv_file << "left_x,left_y,left_z,right_x,right_y,right_z" << std::endl;
    
    // データ行（左右の点数を合わせる）
    size_t max_points = std::max(all_left_nodes.size(), all_right_nodes.size());
    size_t left_size = all_left_nodes.size();
    size_t right_size = all_right_nodes.size();
    
    for (size_t i = 0; i < max_points; ++i) {
        // 左側の点（足りない場合は最後の点を使用）
        size_t left_idx = std::min(i, left_size - 1);
        const auto& left_point = all_left_nodes[left_idx];
        
        // 右側の点（足りない場合は最後の点を使用）
        size_t right_idx = std::min(i, right_size - 1);
        const auto& right_point = all_right_nodes[right_idx];
        
        csv_file << left_point.local_x << "," << left_point.local_y << "," << left_point.elevation << ","
                 << right_point.local_x << "," << right_point.local_y << "," << right_point.elevation << std::endl;
    }
    
    csv_file.close();
    
    std::cout << "✅ CSV file generated: data/track_boundaries.csv" << std::endl;
    std::cout << "📊 Total rows: " << max_points << std::endl;
    std::cout << "🎯 Ready to test with trajectory editor!" << std::endl;
    
    return 0;
}