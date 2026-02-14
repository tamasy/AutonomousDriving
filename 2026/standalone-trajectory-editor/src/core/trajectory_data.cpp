#include "trajectory_data.hpp"
#include "../utils/csv_parser.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace trajectory_editor {

TrajectoryData::TrajectoryData() : is_modified_(false), has_extended_format_(false) {}

TrajectoryData::~TrajectoryData() = default;

void TrajectoryData::clear() {
    points_.clear();
    original_header_.clear();
    original_extra_columns_.clear();
    has_extended_format_ = false;
    is_modified_ = true;
}

void TrajectoryData::addPoint(const TrajectoryPoint& point) {
    points_.push_back(point);
    is_modified_ = true;
}

void TrajectoryData::insertPoint(size_t index, const TrajectoryPoint& point) {
    if (index > points_.size()) {
        throw std::out_of_range("Index out of range");
    }
    points_.insert(points_.begin() + index, point);
    is_modified_ = true;
}

void TrajectoryData::removePoint(size_t index) {
    if (!isValidIndex(index)) {
        throw std::out_of_range("Index out of range");
    }
    points_.erase(points_.begin() + index);
    is_modified_ = true;
}

void TrajectoryData::updatePoint(size_t index, const TrajectoryPoint& point) {
    if (!isValidIndex(index)) {
        throw std::out_of_range("Index out of range");
    }
    points_[index] = point;
    is_modified_ = true;
}

void TrajectoryData::movePoint(size_t index, double new_x, double new_y) {
    if (!isValidIndex(index)) {
        throw std::out_of_range("Index out of range");
    }
    points_[index].x = new_x;
    points_[index].y = new_y;
    is_modified_ = true;
}

void TrajectoryData::updateVelocityRange(size_t start_index, size_t end_index, double velocity) {
    if (start_index >= points_.size() || end_index >= points_.size() || start_index > end_index) {
        throw std::out_of_range("Invalid range");
    }
    
    for (size_t i = start_index; i <= end_index; ++i) {
        points_[i].velocity = velocity;
    }
    is_modified_ = true;
}

void TrajectoryData::getBounds(double& min_x, double& max_x, double& min_y, double& max_y) const {
    if (points_.empty()) {
        min_x = max_x = min_y = max_y = 0.0;
        return;
    }
    
    min_x = max_x = points_[0].x;
    min_y = max_y = points_[0].y;
    
    for (const auto& point : points_) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }
}

void TrajectoryData::getVelocityRange(double& min_vel, double& max_vel) const {
    if (points_.empty()) {
        min_vel = max_vel = 0.0;
        return;
    }
    
    min_vel = max_vel = points_[0].velocity;
    
    for (const auto& point : points_) {
        min_vel = std::min(min_vel, point.velocity);
        max_vel = std::max(max_vel, point.velocity);
    }
}

bool TrajectoryData::loadFromCSV(const std::string& filepath) {
    CSVParser parser;
    auto csv_data = parser.parseFile(filepath);
    
    if (csv_data.empty()) {
        return false;
    }
    
    points_.clear();
    original_header_.clear();
    original_extra_columns_.clear();
    has_extended_format_ = false;
    
    // ヘッダーを保存
    if (parser.hasHeader() && !csv_data.empty()) {
        original_header_ = csv_data[0];
        if (csv_data[0].size() >= 8) {
            has_extended_format_ = true;
        }
    }
    
    // ヘッダーをスキップ
    size_t start_row = parser.hasHeader() ? 1 : 0;
    
    for (size_t i = start_row; i < csv_data.size(); ++i) {
        const auto& row = csv_data[i];
        if (row.size() >= 4) {
            try {
                double x = std::stod(row[0]);
                double y = std::stod(row[1]);
                double z = std::stod(row[2]);
                
                // velocityの列を柔軟に検出
                double velocity = 0.0;
                if (row.size() >= 8) {
                    // 8列形式の場合：x,y,z,qx,qy,qz,qw,speed
                    velocity = std::stod(row[7]);  // speedは8番目の列
                    
                    // 追加列（qx,qy,qz,qw）を保存
                    std::vector<std::string> extra_cols;
                    for (size_t j = 3; j < 7; ++j) {  // 3-6列目（qx,qy,qz,qw）
                        extra_cols.push_back(row[j]);
                    }
                    original_extra_columns_.push_back(extra_cols);
                } else if (row.size() >= 4) {
                    // 4列形式の場合：x,y,z,velocity
                    velocity = std::stod(row[3]);
                }
                
                points_.emplace_back(x, y, z, velocity);
            } catch (const std::exception&) {
                continue;
            }
        }
    }
    
    is_modified_ = false;
    return !points_.empty();
}

bool TrajectoryData::saveToCSV(const std::string& filepath) const {
    CSVParser parser;
    
    std::vector<std::vector<std::string>> csv_data;
    
    // ヘッダーを設定
    if (has_extended_format_ && !original_header_.empty()) {
        // 8列形式の場合は元のヘッダーを保持
        csv_data.push_back(original_header_);
    } else {
        // 4列形式の場合は標準ヘッダー
        csv_data.push_back({"x", "y", "z", "velocity_ms"});
    }
    
    // データ行を出力
    for (size_t i = 0; i < points_.size(); ++i) {
        const auto& point = points_[i];
        
        if (has_extended_format_ && i < original_extra_columns_.size()) {
            // 8列形式：x,y,z,qx,qy,qz,qw,speed の順序で出力
            std::vector<std::string> row;
            row.push_back(std::to_string(point.x));    // x
            row.push_back(std::to_string(point.y));    // y
            row.push_back(std::to_string(point.z));    // z
            
            // 元の qx,qy,qz,qw を追加
            const auto& extra_cols = original_extra_columns_[i];
            for (const auto& col : extra_cols) {
                row.push_back(col);
            }
            
            row.push_back(std::to_string(point.velocity)); // speed
            csv_data.push_back(row);
        } else {
            // 4列形式
            csv_data.push_back({
                std::to_string(point.x),
                std::to_string(point.y), 
                std::to_string(point.z),
                std::to_string(point.velocity)
            });
        }
    }
    
    bool success = parser.writeFile(filepath, csv_data);
    if (success) {
        const_cast<TrajectoryData*>(this)->is_modified_ = false;
    }
    
    return success;
}

bool TrajectoryData::isValidIndex(size_t index) const {
    return index < points_.size();
}

} // namespace trajectory_editor