#include "graphics_trajectory_view.hpp"
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtCore/QDebug>
#include <cmath>
#include <algorithm>

namespace trajectory_editor {

GraphicsTrajectoryView::GraphicsTrajectoryView(QWidget* parent)
    : QGraphicsView(parent)
    , trajectory_data_(nullptr)
    , trajectory_data_2_(nullptr)
    , track_boundaries_(nullptr)
    , scene_(new QGraphicsScene(this))
    , point_size_(0.5)
    , line_width_(0.5)
    , min_speed_(0.0)
    , mid_speed_(20.0)
    , max_speed_(40.0)
    , coordinate_system_(EAST_SOUTH)
    , show_speed_text_(false)  // デフォルトは非表示
    , edit_mode_(VIEWING)
    , selected_point_index_(SIZE_MAX)
    , dragging_point_index_(SIZE_MAX)
    , is_dragging_(false)
    , is_selecting_(false)
    , selection_start_index_(0)
    , is_panning_(false)
    , pan_start_pos_(0, 0)
    , maintain_zoom_on_update_(false) {
    
    setScene(scene_);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setMinimumSize(400, 300);
    
    // ズーム・パン設定
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    
    // シーン設定
    scene_->setBackgroundBrush(QColor(240, 240, 240));  // 薄いグレー背景
    
    connect(scene_, &QGraphicsScene::selectionChanged,
            this, &GraphicsTrajectoryView::onSceneSelectionChanged);
}

GraphicsTrajectoryView::~GraphicsTrajectoryView() = default;

void GraphicsTrajectoryView::setTrajectoryData(const TrajectoryData* data) {
    trajectory_data_ = data;
    updateDisplay();
}

void GraphicsTrajectoryView::setTrajectoryData2(const TrajectoryData* data) {
    trajectory_data_2_ = data;
    updateDisplay();
}


void GraphicsTrajectoryView::setTrackBoundaries(const TrackBoundaries* boundaries) {
    track_boundaries_ = boundaries;
    updateDisplay();
}

void GraphicsTrajectoryView::updateDisplay() {
    clearScene();
    
    // 境界線を最初に描画（背景として）
    if (track_boundaries_ && !track_boundaries_->empty()) {
        createBoundaryItems();
    }
    
    // 1つ目の軌跡を描画（グリーン系）
    if (trajectory_data_ && !trajectory_data_->empty()) {
        createTrajectoryItems();
    }
    
    // 2つ目の軌跡を描画（ブルー系）
    if (trajectory_data_2_ && !trajectory_data_2_->empty()) {
        createTrajectoryItems2();
    }
    
    // ズーム維持フラグが設定されていない場合のみ自動フィット
    if (!maintain_zoom_on_update_) {
        fitTrajectoryInView();
    }
}

void GraphicsTrajectoryView::setSpeedColorRange(double min_speed, double mid_speed, double max_speed) {
    min_speed_ = min_speed;
    mid_speed_ = mid_speed;
    max_speed_ = max_speed;
    updateItemColors();
}

void GraphicsTrajectoryView::setPointSize(double size) {
    point_size_ = size;
    for (auto* item : point_items_) {
        QRectF rect = item->rect();
        QPointF center = rect.center();
        item->setRect(center.x() - size/2, center.y() - size/2, size, size);
    }
}

void GraphicsTrajectoryView::setLineWidth(double width) {
    line_width_ = width;
    QPen pen;
    pen.setWidth(width);
    pen.setColor(QColor(100, 100, 100));  // ダークグレー
    
    for (auto* item : line_items_) {
        item->setPen(pen);
    }
}

void GraphicsTrajectoryView::setBoundariesVisible(bool visible) {
    for (auto* item : boundary_items_) {
        item->setVisible(visible);
    }
}

void GraphicsTrajectoryView::fitTrajectoryInView() {
    if (!trajectory_data_ || trajectory_data_->empty()) {
        return;
    }
    
    // 手動でfitを呼び出した場合はズーム維持を無効化
    maintain_zoom_on_update_ = false;
    
    // 軌跡の境界を計算
    double min_x, max_x, min_y, max_y;
    trajectory_data_->getBounds(min_x, max_x, min_y, max_y);
    
    if (min_x == max_x && min_y == max_y) {
        return;  // 単一点の場合
    }
    
    // 座標変換を適用した境界を計算
    QPointF transformed_min = transformPoint(min_x, min_y);
    QPointF transformed_max = transformPoint(max_x, max_y);
    
    // 変換後の最小・最大値を再計算
    double display_min_x = std::min(transformed_min.x(), transformed_max.x());
    double display_max_x = std::max(transformed_min.x(), transformed_max.x());
    double display_min_y = std::min(transformed_min.y(), transformed_max.y());
    double display_max_y = std::max(transformed_min.y(), transformed_max.y());
    
    // 少し余裕を持たせて表示
    double margin = std::max(display_max_x - display_min_x, display_max_y - display_min_y) * 0.1;
    QRectF bounds(display_min_x - margin, display_min_y - margin,
                  (display_max_x - display_min_x) + 2 * margin,
                  (display_max_y - display_min_y) + 2 * margin);
    
    fitInView(bounds, Qt::KeepAspectRatio);
}

void GraphicsTrajectoryView::zoomIn() {
    scale(1.25, 1.25);
    maintain_zoom_on_update_ = true;  // ズーム維持を有効化
}

void GraphicsTrajectoryView::zoomOut() {
    scale(0.8, 0.8);
    maintain_zoom_on_update_ = true;  // ズーム維持を有効化
}

void GraphicsTrajectoryView::resetZoom() {
    resetTransform();
    maintain_zoom_on_update_ = false;  // フラグをリセット
    fitTrajectoryInView();
}

void GraphicsTrajectoryView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        // 中ボタンでパン開始
        is_panning_ = true;
        pan_start_pos_ = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    
    if (event->button() == Qt::LeftButton && trajectory_data_) {
        QPointF scene_pos = mapToScene(event->pos());
        last_mouse_pos_ = scene_pos;
        
        switch (edit_mode_) {
        case VIEWING:
        case SELECTING: {
            size_t index = findNearestPointIndex(scene_pos);
            // クリック判定の許容距離（点のサイズに応じて調整、最小5ピクセル確保）
            const double CLICK_TOLERANCE = std::max(5.0, point_size_ * 5.0);
            
            if (index < trajectory_data_->size()) {
                const auto& point = trajectory_data_->getPoints()[index];
                // 元座標を表示座標に変換
                QPointF point_pos = transformPoint(point.x, point.y);
                QPointF view_point = mapFromScene(point_pos);
                QPointF click_point = event->pos();
                
                double distance = QPointF(view_point - click_point).manhattanLength();
                
                if (distance <= CLICK_TOLERANCE) {
                    // 点をクリックした場合
                    selected_point_index_ = index;
                    emit pointClicked(index);
                    highlightPoint(index, true);
                    
                    // ドラッグ開始準備
                    dragging_point_index_ = index;
                    is_dragging_ = false;
                } else {
                    // 背景をクリックした場合、選択解除
                    clearSelection();
                    emit selectionCleared();
                    dragging_point_index_ = SIZE_MAX;  // ドラッグ対象もクリア
                }
            } else {
                // 有効な点がない場合も選択解除
                clearSelection();
                emit selectionCleared();
                dragging_point_index_ = SIZE_MAX;  // ドラッグ対象もクリア
            }
            break;
        }
        case ADDING_POINT: {
            // 新しい点の追加位置を決定
            size_t insert_index = findInsertIndex(scene_pos);
            double velocity = 20.0; // デフォルト速度
            // 表示座標を元座標に逆変換してから信号を発行
            QPointF original_pos = inverseTransformPoint(scene_pos);
            emit pointAdded(insert_index, original_pos.x(), original_pos.y(), velocity);
            break;
        }
        case DRAGGING_POINT:
            // すでにドラッグモードの場合は何もしない
            break;
        }
    } else if (event->button() == Qt::RightButton && trajectory_data_ && edit_mode_ == VIEWING) {
        // 右クリックで点を削除
        QPointF scene_pos = mapToScene(event->pos());
        size_t index = findNearestPointIndex(scene_pos);
        if (index < trajectory_data_->size()) {
            emit pointDeleted(index);
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void GraphicsTrajectoryView::mouseMoveEvent(QMouseEvent* event) {
    if (is_panning_ && (event->buttons() & Qt::MiddleButton)) {
        // パン処理 - ビュー変換を使用
        QPoint delta = event->pos() - pan_start_pos_;
        pan_start_pos_ = event->pos();
        
        // 現在のビュー中心点を取得
        QPointF scene_center = mapToScene(viewport()->rect().center());
        
        // デルタをシーン座標に変換してパンを実行
        QPointF scene_delta = mapToScene(delta) - mapToScene(QPoint(0, 0));
        scene_center -= scene_delta;
        
        // ビューの中心を新しい位置に設定
        centerOn(scene_center);
        
        event->accept();
        return;
    }
    
    if (event->buttons() & Qt::LeftButton && trajectory_data_ && 
        dragging_point_index_ < trajectory_data_->size()) {
        
        QPointF scene_pos = mapToScene(event->pos());
        QPointF delta = scene_pos - last_mouse_pos_;
        
        // ドラッグ距離が閾値を超えたらドラッグ開始（小さい点でも反応しやすくする）
        if (!is_dragging_ && (std::abs(delta.x()) > 2.0 || std::abs(delta.y()) > 2.0)) {
            is_dragging_ = true;
            edit_mode_ = DRAGGING_POINT;
            setCursor(Qt::ClosedHandCursor);
        }
        
        if (is_dragging_) {
            // 表示座標を元座標に逆変換してから信号を発行
            QPointF original_pos = inverseTransformPoint(scene_pos);
            emit pointMoved(dragging_point_index_, original_pos.x(), original_pos.y());
            last_mouse_pos_ = scene_pos;
        }
    }
    
    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsTrajectoryView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        // パン終了
        is_panning_ = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    
    if (event->button() == Qt::LeftButton) {
        if (is_dragging_) {
            // ドラッグ終了
            is_dragging_ = false;
            edit_mode_ = VIEWING;
            setCursor(Qt::ArrowCursor);
        }
        
        dragging_point_index_ = SIZE_MAX;
    }
    
    QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsTrajectoryView::wheelEvent(QWheelEvent* event) {
    // ズーム操作
    const double scale_factor = 1.15;
    
    if (event->angleDelta().y() > 0) {
        scale(scale_factor, scale_factor);
    } else {
        scale(1.0 / scale_factor, 1.0 / scale_factor);
    }
    
    maintain_zoom_on_update_ = true;  // ズーム維持を有効化
}

void GraphicsTrajectoryView::onSceneSelectionChanged() {
    // 選択変更時の処理
}

void GraphicsTrajectoryView::clearScene() {
    // 1つ目の軌跡アイテムをクリア
    for (auto* item : point_items_) {
        scene_->removeItem(item);
        delete item;
    }
    point_items_.clear();
    
    for (auto* item : line_items_) {
        scene_->removeItem(item);
        delete item;
    }
    line_items_.clear();
    
    for (auto* item : speed_text_items_) {
        scene_->removeItem(item);
        delete item;
    }
    speed_text_items_.clear();
    
    // 2つ目の軌跡アイテムをクリア
    for (auto* item : point_items_2_) {
        scene_->removeItem(item);
        delete item;
    }
    point_items_2_.clear();
    
    for (auto* item : line_items_2_) {
        scene_->removeItem(item);
        delete item;
    }
    line_items_2_.clear();
    
    for (auto* item : speed_text_items_2_) {
        scene_->removeItem(item);
        delete item;
    }
    speed_text_items_2_.clear();
    
    // 境界アイテムをクリア
    for (auto* item : boundary_items_) {
        scene_->removeItem(item);
        delete item;
    }
    boundary_items_.clear();
}

void GraphicsTrajectoryView::createTrajectoryItems() {
    if (!trajectory_data_ || trajectory_data_->empty()) {
        return;
    }
    
    const auto& points = trajectory_data_->getPoints();
    
    // 線分を作成
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const auto& p1 = points[i];
        const auto& p2 = points[i + 1];
        
        // 座標変換を適用
        QPointF transformed_p1 = transformPoint(p1.x, p1.y);
        QPointF transformed_p2 = transformPoint(p2.x, p2.y);
        
        QGraphicsLineItem* line = scene_->addLine(
            transformed_p1.x(), transformed_p1.y(), 
            transformed_p2.x(), transformed_p2.y());
        
        QPen pen;
        pen.setWidth(line_width_);
        pen.setColor(QColor(100, 100, 100));  // ダークグレー
        line->setPen(pen);
        
        line_items_.push_back(line);
    }
    
    // 点を作成
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& point = points[i];
        
        // 座標変換を適用
        QPointF transformed_point = transformPoint(point.x, point.y);
        
        QGraphicsEllipseItem* circle = scene_->addEllipse(
            transformed_point.x() - point_size_/2, 
            transformed_point.y() - point_size_/2,
            point_size_, point_size_);
        
        // 速度に基づく色設定
        QColor color = getSpeedColor(point.velocity);
        circle->setBrush(QBrush(color));
        circle->setPen(QPen(Qt::NoPen));  // 枠線をなしに
        
        // クリック可能にする
        circle->setFlag(QGraphicsItem::ItemIsSelectable, true);
        circle->setData(0, static_cast<int>(i));  // インデックスを保存
        circle->setZValue(2);  // テキストより上のレイヤー
        
        point_items_.push_back(circle);
        
        // 速度テキストを作成（プロットの左側に表示）
        QString speed_text = QString::number(point.velocity, 'f', 1);  // 小数点第一位まで
        QGraphicsTextItem* text = scene_->addText(speed_text);
        
        // フォントサイズを最小に調整
        QFont font = text->font();
        font.setPointSizeF(0.5);  // 固定で0.5ポイント（小さいサイズ）
        text->setFont(font);
        
        // テキストの位置を設定（プロットの中央に重ねて配置）
        QRectF text_rect = text->boundingRect();
        double text_x = transformed_point.x() - text_rect.width() / 2;  // プロットの中央に水平位置合わせ
        double text_y = transformed_point.y() - text_rect.height() / 2;  // プロットの中央に垂直位置合わせ
        text->setPos(text_x, text_y);
        
        // テキスト色を設定（読みやすい色）
        text->setDefaultTextColor(QColor(0, 0, 0));  // 黒色
        text->setZValue(1);  // テキストをプロットより下に
        text->setVisible(show_speed_text_);  // 表示フラグに従う
        
        speed_text_items_.push_back(text);
    }
}

void GraphicsTrajectoryView::createTrajectoryItems2() {
    if (!trajectory_data_2_ || trajectory_data_2_->empty()) {
        return;
    }
    
    const auto& points = trajectory_data_2_->getPoints();
    
    // 線分を作成
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const auto& p1 = points[i];
        const auto& p2 = points[i + 1];
        
        // 座標変換を適用
        QPointF transformed_p1 = transformPoint(p1.x, p1.y);
        QPointF transformed_p2 = transformPoint(p2.x, p2.y);
        
        QGraphicsLineItem* line = scene_->addLine(
            transformed_p1.x(), transformed_p1.y(), 
            transformed_p2.x(), transformed_p2.y());
        
        QPen pen;
        pen.setWidth(line_width_);
        pen.setColor(QColor(50, 50, 150));  // ブルー系のライン
        line->setPen(pen);
        
        line_items_2_.push_back(line);
    }
    
    // 点を作成
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& point = points[i];
        
        // 座標変換を適用
        QPointF transformed_point = transformPoint(point.x, point.y);
        
        QGraphicsEllipseItem* circle = scene_->addEllipse(
            transformed_point.x() - point_size_/2, 
            transformed_point.y() - point_size_/2,
            point_size_, point_size_);
        
        // 速度に基づく色設定（ブルー系）
        QColor color = getSpeedColorBlue(point.velocity);
        circle->setBrush(QBrush(color));
        circle->setPen(QPen(Qt::NoPen));  // 枠線をなしに
        
        // クリック可能にする
        circle->setFlag(QGraphicsItem::ItemIsSelectable, true);
        circle->setData(0, static_cast<int>(i));  // インデックスを保存
        circle->setZValue(2);  // テキストより上のレイヤー
        
        point_items_2_.push_back(circle);
        
        // 速度テキストを作成（プロットの左側に表示）
        QString speed_text = QString::number(point.velocity, 'f', 1);  // 小数点第一位まで
        QGraphicsTextItem* text = scene_->addText(speed_text);
        
        // フォントサイズを最小に調整
        QFont font = text->font();
        font.setPointSizeF(0.5);  // 固定で0.5ポイント（小さいサイズ）
        text->setFont(font);
        
        // テキストの位置を設定（プロットの中央に重ねて配置）
        QRectF text_rect = text->boundingRect();
        double text_x = transformed_point.x() - text_rect.width() / 2;  // プロットの中央に水平位置合わせ
        double text_y = transformed_point.y() - text_rect.height() / 2;  // プロットの中央に垂直位置合わせ
        text->setPos(text_x, text_y);
        
        // テキスト色を設定（読みやすい色）
        text->setDefaultTextColor(QColor(0, 0, 0));  // 黒色
        text->setZValue(1);  // テキストをプロットより下に
        text->setVisible(show_speed_text_);  // 表示フラグに従う
        
        speed_text_items_2_.push_back(text);
    }
}

void GraphicsTrajectoryView::createBoundaryItems() {
    if (!track_boundaries_ || track_boundaries_->empty()) {
        return;
    }
    
    // 境界点描画設定（線描画用のPenは不要）
    
    // 左境界点を描画
    const auto& left_boundary = track_boundaries_->getLeftBoundary();
    const double boundary_point_size = 1.0;  // 境界点のサイズ
    
    for (size_t i = 0; i < left_boundary.size(); ++i) {
        const auto& point = left_boundary[i];
        
        // 座標変換を適用
        QPointF transformed_point = transformPoint(point.x, point.y);
        
        QGraphicsEllipseItem* circle = scene_->addEllipse(
            transformed_point.x() - boundary_point_size/2,
            transformed_point.y() - boundary_point_size/2,
            boundary_point_size, boundary_point_size);
        
        circle->setBrush(QBrush(QColor(128, 128, 128)));  // グレー色で塗りつぶし
        circle->setPen(QPen(QColor(128, 128, 128), 0.5));  // グレー色の輪郭
        circle->setZValue(-1);  // 軌跡より背景に
        
        boundary_items_.push_back(circle);
    }
    
    // 右境界点を描画
    const auto& right_boundary = track_boundaries_->getRightBoundary();
    
    for (size_t i = 0; i < right_boundary.size(); ++i) {
        const auto& point = right_boundary[i];
        
        // 座標変換を適用
        QPointF transformed_point = transformPoint(point.x, point.y);
        
        QGraphicsEllipseItem* circle = scene_->addEllipse(
            transformed_point.x() - boundary_point_size/2,
            transformed_point.y() - boundary_point_size/2,
            boundary_point_size, boundary_point_size);
        
        circle->setBrush(QBrush(QColor(128, 128, 128)));  // グレー色で塗りつぶし
        circle->setPen(QPen(QColor(128, 128, 128), 0.5));  // グレー色の輪郭
        circle->setZValue(-1);  // 軌跡より背景に
        
        boundary_items_.push_back(circle);
    }
}


QColor GraphicsTrajectoryView::getSpeedColor(double velocity) const {
    // 速度に基づく色分け (グリーン系)
    if (velocity <= min_speed_) {
        return QColor(0, 128, 0);  // 暗緑
    } else if (velocity <= mid_speed_) {
        double t = (velocity - min_speed_) / (mid_speed_ - min_speed_);
        int green = static_cast<int>(128 + 127 * t);  // 128-255
        return QColor(0, green, 0);  // 暗緑から明緑
    } else if (velocity <= max_speed_) {
        double t = (velocity - mid_speed_) / (max_speed_ - mid_speed_);
        int red = static_cast<int>(255 * t);
        int green = static_cast<int>(255 * (1.0 - t));
        return QColor(red, green, 0);  // 緑から黄緑
    } else {
        return QColor(255, 255, 0);  // 黄色
    }
}

QColor GraphicsTrajectoryView::getSpeedColorBlue(double velocity) const {
    // 速度に基づく色分け (ブルー系)
    if (velocity <= min_speed_) {
        return QColor(0, 0, 128);  // 暗青
    } else if (velocity <= mid_speed_) {
        double t = (velocity - min_speed_) / (mid_speed_ - min_speed_);
        int blue = static_cast<int>(128 + 127 * t);  // 128-255
        return QColor(0, 0, blue);  // 暗青から明青
    } else if (velocity <= max_speed_) {
        double t = (velocity - mid_speed_) / (max_speed_ - mid_speed_);
        int green = static_cast<int>(255 * t);
        int blue = static_cast<int>(255 * (1.0 - t));
        return QColor(0, green, blue);  // 青からシアン
    } else {
        return QColor(0, 255, 255);  // シアン
    }
}

size_t GraphicsTrajectoryView::findNearestPointIndex(const QPointF& scene_pos) const {
    if (!trajectory_data_ || trajectory_data_->empty()) {
        return 0;
    }
    
    const auto& points = trajectory_data_->getPoints();
    size_t nearest_index = 0;
    double min_distance = std::numeric_limits<double>::max();
    
    // 検索範囲を制限して性能を向上（最大50ピクセル範囲内のみ検索）
    const double MAX_SEARCH_RANGE = 50.0;
    
    for (size_t i = 0; i < points.size(); ++i) {
        const auto& point = points[i];
        // 元座標を表示座標に変換してから距離計算
        QPointF transformed_point = transformPoint(point.x, point.y);
        double dx = scene_pos.x() - transformed_point.x();
        double dy = scene_pos.y() - transformed_point.y();
        double distance = std::sqrt(dx * dx + dy * dy);
        
        // 検索範囲外の点は無視
        if (distance > MAX_SEARCH_RANGE) {
            continue;
        }
        
        if (distance < min_distance) {
            min_distance = distance;
            nearest_index = i;
        }
    }
    
    // 範囲内に点が見つからない場合は最初の点を返す
    return nearest_index;
}

void GraphicsTrajectoryView::highlightPoint(size_t index, bool highlight) {
    if (index >= point_items_.size()) {
        return;
    }
    
    QGraphicsEllipseItem* item = point_items_[index];
    
    if (highlight) {
        QPen pen(QColor(255, 255, 0), 1);  // 黄色の細い枠線
        item->setPen(pen);
    } else {
        // 枠線をなしに戻す
        item->setPen(QPen(Qt::NoPen));
    }
}

void GraphicsTrajectoryView::updateItemColors() {
    if (!trajectory_data_ || point_items_.empty()) {
        return;
    }
    
    const auto& points = trajectory_data_->getPoints();
    
    for (size_t i = 0; i < point_items_.size() && i < points.size(); ++i) {
        QColor color = getSpeedColor(points[i].velocity);
        point_items_[i]->setBrush(QBrush(color));
        point_items_[i]->setPen(QPen(Qt::NoPen));  // 枠線をなしに
        
        // 速度テキストも更新
        if (i < speed_text_items_.size()) {
            QString speed_text = QString::number(points[i].velocity, 'f', 1);
            speed_text_items_[i]->setPlainText(speed_text);
        }
    }
}

void GraphicsTrajectoryView::setEditMode(EditMode mode) {
    edit_mode_ = mode;
    
    switch (mode) {
    case VIEWING:
        setCursor(Qt::ArrowCursor);
        setDragMode(QGraphicsView::RubberBandDrag);
        break;
    case SELECTING:
        setCursor(Qt::ArrowCursor);
        setDragMode(QGraphicsView::RubberBandDrag);
        break;
    case ADDING_POINT:
        setCursor(Qt::CrossCursor);
        setDragMode(QGraphicsView::NoDrag);
        break;
    case DRAGGING_POINT:
        setCursor(Qt::ClosedHandCursor);
        setDragMode(QGraphicsView::NoDrag);
        break;
    }
}

size_t GraphicsTrajectoryView::findInsertIndex(const QPointF& scene_pos) const {
    if (!trajectory_data_ || trajectory_data_->empty()) {
        return 0;
    }
    
    const auto& points = trajectory_data_->getPoints();
    double min_distance = std::numeric_limits<double>::max();
    size_t best_index = 0;
    
    // 各線分との距離を計算し、最も近い線分を見つける
    for (size_t i = 0; i < points.size() - 1; ++i) {
        // 元座標を表示座標に変換
        QPointF p1 = transformPoint(points[i].x, points[i].y);
        QPointF p2 = transformPoint(points[i + 1].x, points[i + 1].y);
        
        // 線分との距離を計算
        double distance = distanceToLineSegment(scene_pos, p1, p2);
        if (distance < min_distance) {
            min_distance = distance;
            best_index = i + 1; // 線分の後に挿入
        }
    }
    
    return best_index;
}

double GraphicsTrajectoryView::distanceToLineSegment(const QPointF& point, const QPointF& line_start, const QPointF& line_end) const {
    QPointF line_vec = line_end - line_start;
    double line_len_sq = QPointF::dotProduct(line_vec, line_vec);
    
    if (line_len_sq < 1e-6) {
        // 線分が点の場合
        QPointF diff = point - line_start;
        return std::sqrt(QPointF::dotProduct(diff, diff));
    }
    
    QPointF point_vec = point - line_start;
    double t = QPointF::dotProduct(point_vec, line_vec) / line_len_sq;
    t = std::max(0.0, std::min(1.0, t)); // [0, 1]にクランプ
    
    QPointF projection = line_start + t * line_vec;
    QPointF diff = point - projection;
    return std::sqrt(QPointF::dotProduct(diff, diff));
}

void GraphicsTrajectoryView::clearSelection() {
    // 全ての点のハイライトを解除
    for (auto* item : point_items_) {
        if (item) {
            item->setPen(QPen(Qt::NoPen));
            item->setZValue(0);
        }
    }
    selected_point_index_ = SIZE_MAX;
}

void GraphicsTrajectoryView::setCoordinateSystem(CoordinateSystem coord_system) {
    coordinate_system_ = coord_system;
    updateDisplay();  // 座標系変更時に表示を更新
    fitTrajectoryInView();  // 座標系変更後、トラックをフレームに自動フィット
}

QPointF GraphicsTrajectoryView::transformPoint(double x, double y) const {
    switch (coordinate_system_) {
    case EAST_NORTH:
        // 東北基準（デフォルト）: そのまま
        return QPointF(x, y);
    case EAST_SOUTH:
        // 東南基準: X軸はそのまま（東がプラス）、Y軸を反転（南がプラス）
        return QPointF(x, -y);
    case SOUTH_WEST:
        // 南西基準: X軸を反転（西がプラス）、Y軸を反転（南がプラス）
        return QPointF(-x, -y);
    case NORTH_WEST:
        // 北西基準: X軸を反転（西がプラス）、Y軸はそのまま（北がプラス）
        return QPointF(-x, y);
    default:
        return QPointF(x, y);
    }
}

QPointF GraphicsTrajectoryView::inverseTransformPoint(const QPointF& display_point) const {
    switch (coordinate_system_) {
    case EAST_NORTH:
        // 東北基準（デフォルト）: そのまま
        return display_point;
    case EAST_SOUTH:
        // 表示座標から元座標に逆変換
        return QPointF(display_point.x(), -display_point.y());
    case SOUTH_WEST:
        // 表示座標から元座標に逆変換
        return QPointF(-display_point.x(), -display_point.y());
    case NORTH_WEST:
        // 表示座標から元座標に逆変換
        return QPointF(-display_point.x(), display_point.y());
    default:
        return display_point;
    }
}

void GraphicsTrajectoryView::setSpeedTextVisible(bool visible) {
    show_speed_text_ = visible;
    
    // 1つ目の軌跡の速度テキスト表示制御
    for (auto* text : speed_text_items_) {
        text->setVisible(visible);
    }
    
    // 2つ目の軌跡の速度テキスト表示制御
    for (auto* text : speed_text_items_2_) {
        text->setVisible(visible);
    }
}

} // namespace trajectory_editor