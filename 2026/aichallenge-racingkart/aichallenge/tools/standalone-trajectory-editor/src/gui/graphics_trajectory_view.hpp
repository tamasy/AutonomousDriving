#pragma once

#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QGraphicsScene>
#include <QtWidgets/QGraphicsEllipseItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QtWidgets/QGraphicsTextItem>
#include <QtGui/QColor>
#include <QtCore/QTimer>
#include "../core/trajectory_data.hpp"
#include "../core/track_boundaries.hpp"

namespace trajectory_editor {

class GraphicsTrajectoryView : public QGraphicsView {
    Q_OBJECT
    
public:
    // 編集モード
    enum EditMode {
        VIEWING,
        SELECTING,
        DRAGGING_POINT,
        ADDING_POINT
    };
    
    // 座標系モード
    enum CoordinateSystem {
        EAST_NORTH,    // 東北基準（X=東+, Y=北+）
        EAST_SOUTH,    // 東南基準（X=東+, Y=南+）
        SOUTH_WEST,    // 南西基準（X=西+, Y=南+）
        NORTH_WEST     // 北西基準（X=西+, Y=北+）
    };

public:
    explicit GraphicsTrajectoryView(QWidget* parent = nullptr);
    ~GraphicsTrajectoryView();
    
    // データ設定
    void setTrajectoryData(const TrajectoryData* data);
    void setTrajectoryData2(const TrajectoryData* data);  // 2つ目の軌跡データ
    void setTrackBoundaries(const TrackBoundaries* boundaries);
    void updateDisplay();
    
    
    // 表示設定
    void setSpeedColorRange(double min_speed, double mid_speed, double max_speed);
    void setPointSize(double size);
    void setLineWidth(double width);
    void setBoundariesVisible(bool visible);
    
    // 座標系設定
    void setCoordinateSystem(CoordinateSystem coord_system);
    CoordinateSystem getCoordinateSystem() const { return coordinate_system_; }
    
    // 編集モード
    void setEditMode(EditMode mode);
    EditMode getEditMode() const { return edit_mode_; }
    
    // ビュー操作
    void fitTrajectoryInView();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    
    // 選択管理
    void clearSelection();
    
    // 速度テキスト表示制御
    void setSpeedTextVisible(bool visible);
    bool isSpeedTextVisible() const { return show_speed_text_; }

signals:
    void pointClicked(size_t index);
    void pointMoved(size_t index, double new_x, double new_y);
    void pointAdded(size_t index, double x, double y, double velocity);
    void pointDeleted(size_t index);
    void selectionCleared();
    void rangeSelected(size_t start_index, size_t end_index);

protected:
    // マウスイベント
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onSceneSelectionChanged();

private:
    // データ
    const TrajectoryData* trajectory_data_;
    const TrajectoryData* trajectory_data_2_;  // 2つ目の軌跡データ
    const TrackBoundaries* track_boundaries_;
    QGraphicsScene* scene_;
    
    // 表示設定
    double point_size_;
    double line_width_;
    double min_speed_, mid_speed_, max_speed_;
    CoordinateSystem coordinate_system_;  // 座標系モード
    bool show_speed_text_;  // 速度テキスト表示フラグ
    
    // グラフィックアイテム
    std::vector<QGraphicsEllipseItem*> point_items_;
    std::vector<QGraphicsLineItem*> line_items_;
    std::vector<QGraphicsTextItem*> speed_text_items_;  // 速度テキスト
    std::vector<QGraphicsEllipseItem*> point_items_2_;  // 2つ目の軌跡の点
    std::vector<QGraphicsLineItem*> line_items_2_;      // 2つ目の軌跡の線
    std::vector<QGraphicsTextItem*> speed_text_items_2_;  // 2つ目の軌跡の速度テキスト
    std::vector<QGraphicsItem*> boundary_items_;
    
    
    // 編集状態  
    EditMode edit_mode_;
    size_t selected_point_index_;
    size_t dragging_point_index_;
    QPointF last_mouse_pos_;
    bool is_dragging_;
    
    // 選択状態
    bool is_selecting_;
    size_t selection_start_index_;
    
    // パン機能状態
    bool is_panning_;
    QPoint pan_start_pos_;
    
    // ズーム維持フラグ
    bool maintain_zoom_on_update_;
    
    // ヘルパー関数
    void clearScene();
    void createTrajectoryItems();
    void createTrajectoryItems2();  // 2つ目の軌跡描画
    void createBoundaryItems();
    QColor getSpeedColor(double velocity) const;
    QColor getSpeedColorBlue(double velocity) const;  // ブルー系の色
    size_t findNearestPointIndex(const QPointF& scene_pos) const;
    size_t findInsertIndex(const QPointF& scene_pos) const;
    double distanceToLineSegment(const QPointF& point, const QPointF& line_start, const QPointF& line_end) const;
    void highlightPoint(size_t index, bool highlight);
    void updateItemColors();
    
    // 座標変換ヘルパー関数
    QPointF transformPoint(double x, double y) const;
    QPointF inverseTransformPoint(const QPointF& display_point) const;
};

} // namespace trajectory_editor