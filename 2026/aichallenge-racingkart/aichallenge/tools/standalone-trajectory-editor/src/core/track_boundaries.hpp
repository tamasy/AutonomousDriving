#pragma once

#include <vector>
#include <string>

namespace trajectory_editor {

struct BoundaryPoint {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    
    BoundaryPoint() = default;
    BoundaryPoint(double x, double y, double z = 0.0) 
        : x(x), y(y), z(z) {}
};

class TrackBoundaries {
public:
    TrackBoundaries();
    ~TrackBoundaries();
    
    // データアクセス
    const std::vector<BoundaryPoint>& getLeftBoundary() const { return left_boundary_; }
    const std::vector<BoundaryPoint>& getRightBoundary() const { return right_boundary_; }
    
    bool hasLeftBoundary() const { return !left_boundary_.empty(); }
    bool hasRightBoundary() const { return !right_boundary_.empty(); }
    bool empty() const { return left_boundary_.empty() && right_boundary_.empty(); }
    
    // データ操作
    void clear();
    void setLeftBoundary(const std::vector<BoundaryPoint>& points);
    void setRightBoundary(const std::vector<BoundaryPoint>& points);
    
    // バウンディング情報
    void getBounds(double& min_x, double& max_x, double& min_y, double& max_y) const;
    
    // ファイル操作
    bool loadFromCSV(const std::string& filepath);
    
    // 表示設定
    bool isVisible() const { return is_visible_; }
    void setVisible(bool visible) { is_visible_ = visible; }
    
private:
    std::vector<BoundaryPoint> left_boundary_;
    std::vector<BoundaryPoint> right_boundary_;
    bool is_visible_;
    
    // CSVファイル形式の判定と読み込み
    bool loadSeparateBoundaries(const std::string& filepath);  // 左右別々の列
    bool loadInterleaved(const std::string& filepath);         // 左右交互
    bool loadSingleBoundary(const std::string& filepath);      // 単一境界線
};

} // namespace trajectory_editor