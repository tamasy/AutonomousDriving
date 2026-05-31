#pragma once

#include <vector>
#include <string>

namespace trajectory_editor {

struct TrajectoryPoint {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double velocity = 0.0;
    
    TrajectoryPoint() = default;
    TrajectoryPoint(double x, double y, double z, double vel) 
        : x(x), y(y), z(z), velocity(vel) {}
};

class TrajectoryData {
public:
    TrajectoryData();
    ~TrajectoryData();
    
    // データアクセス
    const std::vector<TrajectoryPoint>& getPoints() const { return points_; }
    size_t size() const { return points_.size(); }
    bool empty() const { return points_.empty(); }
    
    // データ操作
    void clear();
    void addPoint(const TrajectoryPoint& point);
    void insertPoint(size_t index, const TrajectoryPoint& point);
    void removePoint(size_t index);
    void updatePoint(size_t index, const TrajectoryPoint& point);
    void movePoint(size_t index, double new_x, double new_y);
    
    // 範囲操作
    void updateVelocityRange(size_t start_index, size_t end_index, double velocity);
    
    // バウンディング情報
    void getBounds(double& min_x, double& max_x, double& min_y, double& max_y) const;
    void getVelocityRange(double& min_vel, double& max_vel) const;
    
    // ファイル操作
    bool loadFromCSV(const std::string& filepath);
    bool saveToCSV(const std::string& filepath) const;
    
    // 状態管理
    bool isModified() const { return is_modified_; }
    void setModified(bool modified) { is_modified_ = modified; }
    
private:
    std::vector<TrajectoryPoint> points_;
    bool is_modified_;
    
    // 元のCSV形式保持用
    std::vector<std::string> original_header_;
    std::vector<std::vector<std::string>> original_extra_columns_;
    bool has_extended_format_;
    
    bool isValidIndex(size_t index) const;
};

} // namespace trajectory_editor