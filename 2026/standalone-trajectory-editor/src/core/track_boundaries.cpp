#include "track_boundaries.hpp"
#include "../utils/csv_parser.hpp"
#include <algorithm>
#include <limits>
#include <iostream>

namespace trajectory_editor {

TrackBoundaries::TrackBoundaries() : is_visible_(true) {}

TrackBoundaries::~TrackBoundaries() = default;

void TrackBoundaries::clear() {
    left_boundary_.clear();
    right_boundary_.clear();
}

void TrackBoundaries::setLeftBoundary(const std::vector<BoundaryPoint>& points) {
    left_boundary_ = points;
}

void TrackBoundaries::setRightBoundary(const std::vector<BoundaryPoint>& points) {
    right_boundary_ = points;
}

void TrackBoundaries::getBounds(double& min_x, double& max_x, double& min_y, double& max_y) const {
    if (empty()) {
        min_x = max_x = min_y = max_y = 0.0;
        return;
    }
    
    min_x = max_x = min_y = max_y = 0.0;
    bool first = true;
    
    // 左境界線
    for (const auto& point : left_boundary_) {
        if (first) {
            min_x = max_x = point.x;
            min_y = max_y = point.y;
            first = false;
        } else {
            min_x = std::min(min_x, point.x);
            max_x = std::max(max_x, point.x);
            min_y = std::min(min_y, point.y);
            max_y = std::max(max_y, point.y);
        }
    }
    
    // 右境界線
    for (const auto& point : right_boundary_) {
        if (first) {
            min_x = max_x = point.x;
            min_y = max_y = point.y;
            first = false;
        } else {
            min_x = std::min(min_x, point.x);
            max_x = std::max(max_x, point.x);
            min_y = std::min(min_y, point.y);
            max_y = std::max(max_y, point.y);
        }
    }
}

bool TrackBoundaries::loadFromCSV(const std::string& filepath) {
    clear();
    
    // 複数の形式を試行
    if (loadSeparateBoundaries(filepath)) {
        std::cout << "Loaded as separate boundaries format" << std::endl;
        return true;
    }
    
    if (loadInterleaved(filepath)) {
        std::cout << "Loaded as interleaved format" << std::endl;
        return true;
    }
    
    if (loadSingleBoundary(filepath)) {
        std::cout << "Loaded as single boundary format" << std::endl;
        return true;
    }
    
    return false;
}

bool TrackBoundaries::loadSeparateBoundaries(const std::string& filepath) {
    CSVParser parser;
    auto csv_data = parser.parseFile(filepath);
    
    if (csv_data.empty()) {
        return false;
    }
    
    // ヘッダーをスキップ
    size_t start_row = parser.hasHeader() ? 1 : 0;
    
    // 6列想定: left_x, left_y, left_z, right_x, right_y, right_z
    // または4列: left_x, left_y, right_x, right_y
    for (size_t i = start_row; i < csv_data.size(); ++i) {
        const auto& row = csv_data[i];
        
        if (row.size() >= 6) {
            try {
                // 6列形式 (x, y, z for both)
                double left_x = std::stod(row[0]);
                double left_y = std::stod(row[1]);
                double left_z = std::stod(row[2]);
                double right_x = std::stod(row[3]);
                double right_y = std::stod(row[4]);
                double right_z = std::stod(row[5]);
                
                left_boundary_.emplace_back(left_x, left_y, left_z);
                right_boundary_.emplace_back(right_x, right_y, right_z);
            } catch (const std::exception&) {
                continue;
            }
        } else if (row.size() >= 4) {
            try {
                // 4列形式 (x, y for both)
                double left_x = std::stod(row[0]);
                double left_y = std::stod(row[1]);
                double right_x = std::stod(row[2]);
                double right_y = std::stod(row[3]);
                
                left_boundary_.emplace_back(left_x, left_y, 0.0);
                right_boundary_.emplace_back(right_x, right_y, 0.0);
            } catch (const std::exception&) {
                continue;
            }
        }
    }
    
    return !left_boundary_.empty() && !right_boundary_.empty();
}

bool TrackBoundaries::loadInterleaved(const std::string& filepath) {
    CSVParser parser;
    auto csv_data = parser.parseFile(filepath);
    
    if (csv_data.empty()) {
        return false;
    }
    
    // ヘッダーをスキップ
    size_t start_row = parser.hasHeader() ? 1 : 0;
    
    // 交互形式: 奇数行=左境界、偶数行=右境界
    // または type列で判定
    for (size_t i = start_row; i < csv_data.size(); ++i) {
        const auto& row = csv_data[i];
        
        if (row.size() >= 3) {
            try {
                double x = std::stod(row[0]);
                double y = std::stod(row[1]);
                double z = row.size() >= 3 ? std::stod(row[2]) : 0.0;
                
                // type列がある場合
                if (row.size() >= 4) {
                    std::string type = row[3];
                    if (type == "left" || type == "L") {
                        left_boundary_.emplace_back(x, y, z);
                    } else if (type == "right" || type == "R") {
                        right_boundary_.emplace_back(x, y, z);
                    }
                } else {
                    // 行番号で判定
                    if ((i - start_row) % 2 == 0) {
                        left_boundary_.emplace_back(x, y, z);
                    } else {
                        right_boundary_.emplace_back(x, y, z);
                    }
                }
            } catch (const std::exception&) {
                continue;
            }
        }
    }
    
    return !left_boundary_.empty() || !right_boundary_.empty();
}

bool TrackBoundaries::loadSingleBoundary(const std::string& filepath) {
    CSVParser parser;
    auto csv_data = parser.parseFile(filepath);
    
    if (csv_data.empty()) {
        return false;
    }
    
    // ヘッダーをスキップ
    size_t start_row = parser.hasHeader() ? 1 : 0;
    
    // 単一境界線として左側に読み込み
    for (size_t i = start_row; i < csv_data.size(); ++i) {
        const auto& row = csv_data[i];
        
        if (row.size() >= 2) {
            try {
                double x = std::stod(row[0]);
                double y = std::stod(row[1]);
                double z = row.size() >= 3 ? std::stod(row[2]) : 0.0;
                
                left_boundary_.emplace_back(x, y, z);
            } catch (const std::exception&) {
                continue;
            }
        }
    }
    
    return !left_boundary_.empty();
}

} // namespace trajectory_editor