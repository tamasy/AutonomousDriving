#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtCore/QDebug>

#include "core/trajectory_data.hpp"
#include "core/track_boundaries.hpp"
#include "core/edit_history.hpp"
#include "gui/graphics_trajectory_view.hpp"

// 単位変換関数
inline double msToKmh(double velocity_ms) {
    return velocity_ms * 3.6;  // m/s to km/h
}

inline double kmhToMs(double velocity_kmh) {
    return velocity_kmh / 3.6;  // km/h to m/s
}

class TrajectoryEditor : public QMainWindow {
    Q_OBJECT

public:
    TrajectoryEditor(QWidget* parent = nullptr) : QMainWindow(parent), current_selected_index_(SIZE_MAX) {
        setupUI();
        connectSignals();
        loadDefaultBoundaries();
        statusBar()->showMessage("Ready - Load a CSV file to start");
    }

private slots:
    void openFile() {
        QString filename = QFileDialog::getOpenFileName(
            this, "Open CSV File (Green)", "data", "CSV Files (*.csv)");
        
        if (!filename.isEmpty()) {
            if (trajectory_data_.loadFromCSV(filename.toStdString())) {
                trajectory_view_->setTrajectoryData(&trajectory_data_);
                // ファイル名ラベルを更新
                QString basename = filename.split('/').last().split('\\').last();
                filename_label_1_->setText(basename);
                edit_history_.clear(); // 新しいファイル読み込み時は履歴をクリア
                updateInfoDisplay();
                updateVelocityUI();
                updateHistoryButtons();
                statusBar()->showMessage("Loaded (Green): " + filename, 3000);
            } else {
                QMessageBox::warning(this, "Error", "Failed to load file: " + filename);
            }
        }
    }
    
    void openFile2() {
        QString filename = QFileDialog::getOpenFileName(
            this, "Open CSV File (Blue)", "data", "CSV Files (*.csv)");
        
        if (!filename.isEmpty()) {
            if (trajectory_data_2_.loadFromCSV(filename.toStdString())) {
                trajectory_view_->setTrajectoryData2(&trajectory_data_2_);
                // ファイル名ラベルを更新
                QString basename = filename.split('/').last().split('\\').last();
                filename_label_2_->setText(basename);
                updateInfoDisplay();
                statusBar()->showMessage("Loaded (Blue): " + filename, 3000);
            } else {
                QMessageBox::warning(this, "Error", "Failed to load file: " + filename);
            }
        }
    }
    
    void saveFile() {
        if (trajectory_data_.empty()) {
            QMessageBox::information(this, "Info", "No green trajectory data to save");
            return;
        }
        
        QString filename = QFileDialog::getSaveFileName(
            this, "Save CSV File (Green)", "", "CSV Files (*.csv)");
        
        if (!filename.isEmpty()) {
            if (trajectory_data_.saveToCSV(filename.toStdString())) {
                statusBar()->showMessage("Saved (Green): " + filename, 3000);
            } else {
                QMessageBox::warning(this, "Error", "Failed to save file: " + filename);
            }
        }
    }
    
    void saveFile2() {
        if (trajectory_data_2_.empty()) {
            QMessageBox::information(this, "Info", "No blue trajectory data to save");
            return;
        }
        
        QString filename = QFileDialog::getSaveFileName(
            this, "Save CSV File (Blue)", "", "CSV Files (*.csv)");
        
        if (!filename.isEmpty()) {
            if (trajectory_data_2_.saveToCSV(filename.toStdString())) {
                statusBar()->showMessage("Saved (Blue): " + filename, 3000);
            } else {
                QMessageBox::warning(this, "Error", "Failed to save file: " + filename);
            }
        }
    }
    
    void fitAll() {
        trajectory_view_->fitTrajectoryInView();
    }
    
    void zoomIn() {
        trajectory_view_->zoomIn();
    }
    
    void zoomOut() {
        trajectory_view_->zoomOut();
    }
    
    void resetZoom() {
        trajectory_view_->resetZoom();
    }
    
    void onPointSizeChanged(double value) {
        trajectory_view_->setPointSize(value);
    }
    
    void onLineWidthChanged(double value) {
        trajectory_view_->setLineWidth(value);
    }
    
    void onPointClicked(size_t index) {
        current_selected_index_ = index;
        const auto& points = trajectory_data_.getPoints();
        if (index < points.size()) {
            const auto& point = points[index];
            QString info = QString("Point %1: (%2, %3, %4) v=%5 km/h")
                          .arg(index)
                          .arg(point.x, 0, 'f', 2)
                          .arg(point.y, 0, 'f', 2)
                          .arg(point.z, 0, 'f', 2)
                          .arg(msToKmh(point.velocity), 0, 'f', 2);
            statusBar()->showMessage(info, 5000);
            
            // 速度編集UIを更新
            updateVelocityUI();
        }
    }
    
    void clearSelection() {
        current_selected_index_ = SIZE_MAX;
        trajectory_view_->clearSelection();
        updateVelocityUI();
        statusBar()->showMessage("Selection cleared", 2000);
    }
    
    void onPointMoved(size_t index, double new_x, double new_y) {
        try {
            const auto& points = trajectory_data_.getPoints();
            if (index < points.size()) {
                const auto& point = points[index];
                auto command = std::make_unique<trajectory_editor::MovePointCommand>(
                    index, point.x, point.y, new_x, new_y);
                edit_history_.executeCommand(std::move(command), trajectory_data_);
                trajectory_view_->updateDisplay();
                updateHistoryButtons();
                statusBar()->showMessage(QString("Point %1 moved to (%2, %3)")
                                       .arg(index).arg(new_x, 0, 'f', 2).arg(new_y, 0, 'f', 2), 2000);
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Error", QString("Failed to move point: %1").arg(e.what()));
        }
    }
    
    void onPointAdded(size_t index, double x, double y, double velocity) {
        try {
            // velocityはkm/hで渡されるので、m/sに変換してから保存
            trajectory_editor::TrajectoryPoint new_point(x, y, 6.5, kmhToMs(velocity));
            auto command = std::make_unique<trajectory_editor::AddPointCommand>(index, new_point);
            edit_history_.executeCommand(std::move(command), trajectory_data_);
            trajectory_view_->updateDisplay();
            updateHistoryButtons();
            updateVelocityUI();
            statusBar()->showMessage(QString("Point added at index %1").arg(index), 2000);
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Error", QString("Failed to add point: %1").arg(e.what()));
        }
    }
    
    void onPointDeleted(size_t index) {
        try {
            if (trajectory_data_.size() <= 2) {
                QMessageBox::information(this, "Info", "Cannot delete point: minimum 2 points required");
                return;
            }
            
            const auto& points = trajectory_data_.getPoints();
            if (index < points.size()) {
                trajectory_editor::TrajectoryPoint deleted_point = points[index];
                auto command = std::make_unique<trajectory_editor::RemovePointCommand>(index, deleted_point);
                edit_history_.executeCommand(std::move(command), trajectory_data_);
                trajectory_view_->updateDisplay();
                updateHistoryButtons();
                updateVelocityUI();
                statusBar()->showMessage(QString("Point %1 deleted").arg(index), 2000);
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Error", QString("Failed to delete point: %1").arg(e.what()));
        }
    }
    
    void toggleBoundaries(bool visible) {
        trajectory_view_->setBoundariesVisible(visible);
    }
    
    void toggleSpeedText(bool visible) {
        trajectory_view_->setSpeedTextVisible(visible);
    }
    
    void onCoordinateSystemChanged(int index) {
        trajectory_editor::GraphicsTrajectoryView::CoordinateSystem coord_system;
        QString message;
        
        switch (index) {
        case 0:
            coord_system = trajectory_editor::GraphicsTrajectoryView::EAST_NORTH;
            message = "Coordinate system: East-North (X=East+, Y=North+)";
            break;
        case 1:
            coord_system = trajectory_editor::GraphicsTrajectoryView::EAST_SOUTH;
            message = "Coordinate system: East-South (X=East+, Y=South+)";
            break;
        case 2:
            coord_system = trajectory_editor::GraphicsTrajectoryView::SOUTH_WEST;
            message = "Coordinate system: South-West (X=West+, Y=South+)";
            break;
        case 3:
            coord_system = trajectory_editor::GraphicsTrajectoryView::NORTH_WEST;
            message = "Coordinate system: North-West (X=West+, Y=North+)";
            break;
        default:
            coord_system = trajectory_editor::GraphicsTrajectoryView::EAST_NORTH;
            message = "Coordinate system: East-North (X=East+, Y=North+)";
            break;
        }
        
        trajectory_view_->setCoordinateSystem(coord_system);
        statusBar()->showMessage(message, 3000);
    }
    
    void onViewModeClicked() {
        if (view_mode_button_->isChecked()) {
            add_mode_button_->setChecked(false);
            trajectory_view_->setEditMode(trajectory_editor::GraphicsTrajectoryView::VIEWING);
            edit_info_label_->setText("View Mode:\n• Click: Select point\n• Drag: Move point\n• Right-click: Delete");
        }
    }
    
    void onAddModeClicked() {
        if (add_mode_button_->isChecked()) {
            view_mode_button_->setChecked(false);
            trajectory_view_->setEditMode(trajectory_editor::GraphicsTrajectoryView::ADDING_POINT);
            edit_info_label_->setText("Add Mode:\n• Click: Add new point\n• Point will be inserted automatically");
        }
    }
    
    void onApplyVelocity() {
        size_t current_index = getCurrentSelectedIndex();
        if (current_index != SIZE_MAX) {
            try {
                const auto& points = trajectory_data_.getPoints();
                if (current_index < points.size()) {
                    double old_velocity = points[current_index].velocity;  // m/s単位
                    double new_velocity_kmh = velocity_spin_->value();    // km/h単位（UI）
                    double new_velocity = kmhToMs(new_velocity_kmh);      // m/s単位に変換
                    
                    if (std::abs(old_velocity - new_velocity) > 0.01) { // 微小変更を避ける
                        auto command = std::make_unique<trajectory_editor::ChangeVelocityCommand>(
                            current_index, old_velocity, new_velocity);
                        edit_history_.executeCommand(std::move(command), trajectory_data_);
                        trajectory_view_->updateDisplay();
                        updateHistoryButtons();
                        statusBar()->showMessage(QString("Point %1 velocity updated to %2 km/h")
                                               .arg(current_index).arg(new_velocity_kmh, 0, 'f', 1), 2000);
                    }
                }
            } catch (const std::exception& e) {
                QMessageBox::warning(this, "Error", QString("Failed to update velocity: %1").arg(e.what()));
            }
        }
    }
    
    void onApplyRangeVelocity() {
        try {
            size_t start_idx = static_cast<size_t>(range_start_spin_->value());
            size_t end_idx = static_cast<size_t>(range_end_spin_->value());
            double new_velocity_kmh = range_velocity_spin_->value();  // km/h単位（UI）
            double new_velocity = kmhToMs(new_velocity_kmh);       // m/s単位に変換
            
            if (start_idx > end_idx) {
                QMessageBox::information(this, "Info", "Start index must be less than or equal to end index");
                return;
            }
            
            if (end_idx >= trajectory_data_.size()) {
                QMessageBox::information(this, "Info", QString("End index %1 exceeds trajectory size %2")
                                        .arg(end_idx).arg(trajectory_data_.size()));
                return;
            }
            
            // 元の速度を保存
            const auto& points = trajectory_data_.getPoints();
            std::vector<double> old_velocities;
            for (size_t i = start_idx; i <= end_idx; ++i) {
                if (i < points.size()) {
                    old_velocities.push_back(points[i].velocity);
                }
            }
            
            auto command = std::make_unique<trajectory_editor::ChangeRangeVelocityCommand>(
                start_idx, end_idx, old_velocities, new_velocity);
            edit_history_.executeCommand(std::move(command), trajectory_data_);
            trajectory_view_->updateDisplay();
            updateHistoryButtons();
            updateInfoDisplay();
            statusBar()->showMessage(QString("Velocity updated for points %1-%2 to %3 km/h")
                                   .arg(start_idx).arg(end_idx).arg(new_velocity_kmh, 0, 'f', 1), 3000);
            
        } catch (const std::exception& e) {
            QMessageBox::warning(this, "Error", QString("Failed to update range velocity: %1").arg(e.what()));
        }
    }
    
    void onUndo() {
        edit_history_.undo(trajectory_data_);
        trajectory_view_->updateDisplay();
        updateHistoryButtons();
        updateVelocityUI();
        updateInfoDisplay();
        
        std::string undo_desc = edit_history_.getRedoDescription(); // 次に redo できる操作
        if (!undo_desc.empty()) {
            statusBar()->showMessage(QString("Undone: %1").arg(QString::fromStdString(undo_desc)), 2000);
        } else {
            statusBar()->showMessage("Undone", 2000);
        }
    }
    
    void onRedo() {
        std::string redo_desc = edit_history_.getRedoDescription();
        edit_history_.redo(trajectory_data_);
        trajectory_view_->updateDisplay();
        updateHistoryButtons();
        updateVelocityUI();
        updateInfoDisplay();
        
        if (!redo_desc.empty()) {
            statusBar()->showMessage(QString("Redone: %1").arg(QString::fromStdString(redo_desc)), 2000);
        } else {
            statusBar()->showMessage("Redone", 2000);
        }
    }

private:
    trajectory_editor::TrajectoryData trajectory_data_;
    trajectory_editor::TrajectoryData trajectory_data_2_;  // 2つ目の軌跡データ
    trajectory_editor::TrackBoundaries track_boundaries_;
    trajectory_editor::EditHistory edit_history_;
    
    // 選択状態
    size_t current_selected_index_;
    
    // UI要素
    QWidget* central_widget_;
    QSplitter* main_splitter_;
    trajectory_editor::GraphicsTrajectoryView* trajectory_view_;
    QWidget* left_control_panel_;
    QWidget* right_control_panel_;
    
    QVBoxLayout* control_layout_;  // 後方互換性のため残す
    QVBoxLayout* left_layout_;     // 左列レイアウト
    QVBoxLayout* right_layout_;    // 右列レイアウト
    QGroupBox* file_group_;
    QGroupBox* view_group_;
    QGroupBox* display_group_;
    QGroupBox* info_group_;
    
    QPushButton* open_button_;
    QPushButton* open_button_2_;   // 2つ目のCSV読み込み
    QPushButton* save_button_;
    QPushButton* save_button_2_;   // 2つ目のCSV保存
    QLabel* filename_label_1_;     // 1つ目のファイル名表示
    QLabel* filename_label_2_;     // 2つ目のファイル名表示
    QPushButton* undo_button_;
    QPushButton* redo_button_;
    QPushButton* fit_all_button_;
    QPushButton* zoom_in_button_;
    QPushButton* zoom_out_button_;
    QPushButton* reset_zoom_button_;
    
    QDoubleSpinBox* point_size_spin_;
    QDoubleSpinBox* line_width_spin_;
    QCheckBox* boundaries_checkbox_;
    QCheckBox* speed_text_checkbox_;
    QComboBox* coordinate_system_combo_;
    
    // 編集モード
    QGroupBox* edit_group_;
    QPushButton* view_mode_button_;
    QPushButton* add_mode_button_;
    QLabel* edit_info_label_;
    
    // 速度編集
    QGroupBox* velocity_group_;
    QLabel* selected_point_label_;
    QDoubleSpinBox* velocity_spin_;
    QPushButton* apply_velocity_button_;
    QPushButton* range_velocity_button_;
    QDoubleSpinBox* range_start_spin_;
    QDoubleSpinBox* range_end_spin_;
    QDoubleSpinBox* range_velocity_spin_;
    
    QLabel* info_label_;
    
    void setupUI() {
        setWindowTitle("Trajectory Editor - Graphics View");
        setMinimumSize(1300, 600);
        
        central_widget_ = new QWidget;
        setCentralWidget(central_widget_);
        
        // メインスプリッター
        main_splitter_ = new QSplitter(Qt::Horizontal, central_widget_);
        
        // 軌跡表示ビュー
        trajectory_view_ = new trajectory_editor::GraphicsTrajectoryView;
        trajectory_view_->setMinimumSize(600, 400);
        
        // 左側コントロールパネル
        left_control_panel_ = new QWidget;
        left_control_panel_->setMaximumWidth(320);
        left_control_panel_->setMinimumWidth(300);
        
        // 右側コントロールパネル
        right_control_panel_ = new QWidget;
        right_control_panel_->setMaximumWidth(320);
        right_control_panel_->setMinimumWidth(300);
        
        // 各パネルのレイアウト
        QVBoxLayout* left_layout = new QVBoxLayout(left_control_panel_);
        QVBoxLayout* right_layout = new QVBoxLayout(right_control_panel_);
        
        // レイアウトポインタを保存
        left_layout_ = left_layout;
        right_layout_ = right_layout;
        control_layout_ = left_layout;  // 後方互換性のため
        
        setupFileControls();
        setupViewControls();
        setupEditControls();
        setupVelocityControls();
        setupDisplayControls();
        setupInfoDisplay();
        
        // スプリッターに追加（左パネル、軌跡ビュー、右パネルの順）
        main_splitter_->addWidget(left_control_panel_);
        main_splitter_->addWidget(trajectory_view_);
        main_splitter_->addWidget(right_control_panel_);
        main_splitter_->setStretchFactor(0, 0);  // 左パネルは固定
        main_splitter_->setStretchFactor(1, 1);  // 軌跡ビューを伸縮可能に
        main_splitter_->setStretchFactor(2, 0);  // 右パネルは固定
        
        // レイアウト
        QHBoxLayout* main_layout = new QHBoxLayout(central_widget_);
        main_layout->addWidget(main_splitter_);
        main_layout->setContentsMargins(5, 5, 5, 5);
    }
    
    void setupFileControls() {
        file_group_ = new QGroupBox("File Operations");
        QVBoxLayout* file_layout = new QVBoxLayout(file_group_);
        
        open_button_ = new QPushButton("Open CSV (Green)");
        open_button_->setStyleSheet("font-size: 11px; padding: 4px 8px; background-color: #e8f5e8;");
        open_button_2_ = new QPushButton("Open CSV (Blue)");
        open_button_2_->setStyleSheet("font-size: 11px; padding: 4px 8px; background-color: #e8f0ff;");
        save_button_ = new QPushButton("Save CSV (Green)");
        save_button_->setStyleSheet("font-size: 11px; padding: 4px 8px; background-color: #e8f5e8;");
        save_button_2_ = new QPushButton("Save CSV (Blue)");
        save_button_2_->setStyleSheet("font-size: 11px; padding: 4px 8px; background-color: #e8f0ff;");
        
        file_layout->addWidget(open_button_);
        
        // 1つ目のファイル名表示
        filename_label_1_ = new QLabel("No file loaded (Green)");
        filename_label_1_->setStyleSheet("font-size: 8px; color: #006600; padding: 1px; font-style: italic;");
        filename_label_1_->setWordWrap(true);
        filename_label_1_->setMaximumHeight(16);
        file_layout->addWidget(filename_label_1_);
        
        file_layout->addWidget(open_button_2_);
        
        // 2つ目のファイル名表示
        filename_label_2_ = new QLabel("No file loaded (Blue)");
        filename_label_2_->setStyleSheet("font-size: 8px; color: #000066; padding: 1px; font-style: italic;");
        filename_label_2_->setWordWrap(true);
        filename_label_2_->setMaximumHeight(16);
        file_layout->addWidget(filename_label_2_);
        
        file_layout->addWidget(save_button_);
        file_layout->addWidget(save_button_2_);
        
        // Undo/Redo buttons
        QHBoxLayout* history_layout = new QHBoxLayout;
        undo_button_ = new QPushButton("Undo");
        undo_button_->setStyleSheet("font-size: 11px; padding: 2px 6px;");
        redo_button_ = new QPushButton("Redo");
        redo_button_->setStyleSheet("font-size: 11px; padding: 2px 6px;");
        undo_button_->setEnabled(false);
        redo_button_->setEnabled(false);
        history_layout->addWidget(undo_button_);
        history_layout->addWidget(redo_button_);
        file_layout->addLayout(history_layout);
        
        left_layout_->addWidget(file_group_);
    }
    
    void setupViewControls() {
        view_group_ = new QGroupBox("View Controls");
        QVBoxLayout* view_layout = new QVBoxLayout(view_group_);
        
        fit_all_button_ = new QPushButton("Fit All");
        fit_all_button_->setStyleSheet("font-size: 11px; padding: 3px 6px;");
        zoom_in_button_ = new QPushButton("Zoom In");
        zoom_in_button_->setStyleSheet("font-size: 11px; padding: 3px 6px;");
        zoom_out_button_ = new QPushButton("Zoom Out");
        zoom_out_button_->setStyleSheet("font-size: 11px; padding: 3px 6px;");
        reset_zoom_button_ = new QPushButton("Reset Zoom");
        reset_zoom_button_->setStyleSheet("font-size: 11px; padding: 3px 6px;");
        
        view_layout->addWidget(fit_all_button_);
        view_layout->addWidget(zoom_in_button_);
        view_layout->addWidget(zoom_out_button_);
        view_layout->addWidget(reset_zoom_button_);
        
        left_layout_->addWidget(view_group_);
    }
    
    void setupEditControls() {
        edit_group_ = new QGroupBox("Edit Mode");
        QVBoxLayout* edit_layout = new QVBoxLayout(edit_group_);
        
        view_mode_button_ = new QPushButton("View Mode");
        view_mode_button_->setCheckable(true);
        view_mode_button_->setChecked(true);
        
        add_mode_button_ = new QPushButton("Add Points");
        add_mode_button_->setCheckable(true);
        
        edit_layout->addWidget(view_mode_button_);
        edit_layout->addWidget(add_mode_button_);
        
        edit_info_label_ = new QLabel("View Mode:\n• Click: Select point\n• Drag: Move point\n• Right-click: Delete");
        edit_info_label_->setWordWrap(true);
        edit_info_label_->setStyleSheet("font-size: 10px; color: #666;");
        edit_layout->addWidget(edit_info_label_);
        
        right_layout_->addWidget(edit_group_);
    }
    
    void setupVelocityControls() {
        velocity_group_ = new QGroupBox("Velocity Editor");
        QVBoxLayout* velocity_layout = new QVBoxLayout(velocity_group_);
        velocity_layout->setSpacing(4);  // スペーシングを減らす
        velocity_layout->setContentsMargins(8, 8, 8, 8);  // マージンを減らす
        
        // 選択点の速度編集
        selected_point_label_ = new QLabel("No point selected");
        selected_point_label_->setStyleSheet("font-size: 10px; font-weight: bold; color: #333; padding: 1px;");
        selected_point_label_->setMinimumHeight(16);
        selected_point_label_->setMaximumHeight(16);
        velocity_layout->addWidget(selected_point_label_);
        
        velocity_layout->addSpacing(3);  // スペースを減らす
        
        // 単一点速度編集 - 縦配置
        QLabel* speed_label = new QLabel("Point Speed:");
        speed_label->setStyleSheet("font-size: 9px;");
        speed_label->setMinimumHeight(12);
        speed_label->setMaximumHeight(12);
        velocity_layout->addWidget(speed_label);
        
        velocity_spin_ = new QDoubleSpinBox;
        velocity_spin_->setRange(0.0, 100.0);
        velocity_spin_->setValue(20.0);
        velocity_spin_->setSuffix(" km/h");
        velocity_spin_->setDecimals(1);
        velocity_spin_->setEnabled(false);
        velocity_spin_->setMinimumHeight(22);
        velocity_spin_->setMaximumHeight(22);
        velocity_spin_->setStyleSheet("font-size: 9px;");
        velocity_layout->addWidget(velocity_spin_);
        
        velocity_layout->addSpacing(3);  // スペースを減らす
        
        // Apply と Clear ボタン - 横配置
        QHBoxLayout* button_layout = new QHBoxLayout;
        button_layout->setSpacing(4);
        apply_velocity_button_ = new QPushButton("Apply");
        apply_velocity_button_->setEnabled(false);
        apply_velocity_button_->setMinimumHeight(22);
        apply_velocity_button_->setMaximumHeight(22);
        apply_velocity_button_->setStyleSheet("font-size: 9px; padding: 1px 4px;");
        
        QPushButton* clear_selection_button_ = new QPushButton("Clear");
        clear_selection_button_->setMinimumHeight(22);
        clear_selection_button_->setMaximumHeight(22);
        clear_selection_button_->setStyleSheet("font-size: 9px; padding: 1px 4px;");
        
        button_layout->addWidget(apply_velocity_button_);
        button_layout->addWidget(clear_selection_button_);
        velocity_layout->addLayout(button_layout);
        
        velocity_layout->addSpacing(4);  // セパレーター前のスペース
        
        // セパレーター
        QFrame* separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        separator->setFrameShadow(QFrame::Sunken);
        separator->setMaximumHeight(1);
        velocity_layout->addWidget(separator);
        
        velocity_layout->addSpacing(4);  // セパレーター後のスペース
        
        // 範囲速度編集
        QLabel* range_label = new QLabel("Range Edit:");
        range_label->setStyleSheet("font-size: 10px; font-weight: bold; color: #333; padding: 1px;");
        range_label->setMinimumHeight(14);
        range_label->setMaximumHeight(14);
        velocity_layout->addWidget(range_label);
        
        velocity_layout->addSpacing(2);
        
        // From/To を横配置で1行に
        QHBoxLayout* range_indices_layout = new QHBoxLayout;
        range_indices_layout->setSpacing(4);
        
        QLabel* from_label = new QLabel("From:");
        from_label->setStyleSheet("font-size: 9px;");
        from_label->setMinimumWidth(30);
        range_indices_layout->addWidget(from_label);
        
        range_start_spin_ = new QDoubleSpinBox;
        range_start_spin_->setRange(0, 200);
        range_start_spin_->setDecimals(0);
        range_start_spin_->setMinimumHeight(20);
        range_start_spin_->setMaximumHeight(20);
        range_start_spin_->setMinimumWidth(50);
        range_start_spin_->setStyleSheet("font-size: 9px;");
        range_indices_layout->addWidget(range_start_spin_);
        
        QLabel* to_label = new QLabel("To:");
        to_label->setStyleSheet("font-size: 9px;");
        to_label->setMinimumWidth(18);
        range_indices_layout->addWidget(to_label);
        
        range_end_spin_ = new QDoubleSpinBox;
        range_end_spin_->setRange(0, 200);
        range_end_spin_->setDecimals(0);
        range_end_spin_->setMinimumHeight(20);
        range_end_spin_->setMaximumHeight(20);
        range_end_spin_->setMinimumWidth(50);
        range_end_spin_->setStyleSheet("font-size: 9px;");
        range_indices_layout->addWidget(range_end_spin_);
        
        velocity_layout->addLayout(range_indices_layout);
        
        velocity_layout->addSpacing(3);
        
        // 範囲速度設定
        QLabel* range_speed_label = new QLabel("Range Speed:");
        range_speed_label->setStyleSheet("font-size: 9px;");
        range_speed_label->setMinimumHeight(12);
        range_speed_label->setMaximumHeight(12);
        velocity_layout->addWidget(range_speed_label);
        
        range_velocity_spin_ = new QDoubleSpinBox;
        range_velocity_spin_->setRange(0.0, 100.0);
        range_velocity_spin_->setValue(30.0);
        range_velocity_spin_->setSuffix(" km/h");
        range_velocity_spin_->setDecimals(1);
        range_velocity_spin_->setMinimumHeight(22);
        range_velocity_spin_->setMaximumHeight(22);
        range_velocity_spin_->setStyleSheet("font-size: 9px;");
        velocity_layout->addWidget(range_velocity_spin_);
        
        velocity_layout->addSpacing(3);
        
        range_velocity_button_ = new QPushButton("Apply Range");
        range_velocity_button_->setMinimumHeight(22);
        range_velocity_button_->setMaximumHeight(22);
        range_velocity_button_->setStyleSheet("font-size: 9px; padding: 1px 6px;");
        velocity_layout->addWidget(range_velocity_button_);
        
        // Clear selectionボタンのシグナル接続
        connect(clear_selection_button_, &QPushButton::clicked, this, &TrajectoryEditor::clearSelection);
        
        left_layout_->addWidget(velocity_group_);
    }
    
    void setupDisplayControls() {
        display_group_ = new QGroupBox("Display Settings");
        QVBoxLayout* display_layout = new QVBoxLayout(display_group_);
        
        // 点のサイズ
        QHBoxLayout* point_size_layout = new QHBoxLayout;
        point_size_layout->addWidget(new QLabel("Point Size:"));
        point_size_spin_ = new QDoubleSpinBox;
        point_size_spin_->setRange(0.5, 20.0);
        point_size_spin_->setValue(0.5);
        point_size_spin_->setDecimals(1);
        point_size_spin_->setSingleStep(0.5);
        point_size_layout->addWidget(point_size_spin_);
        display_layout->addLayout(point_size_layout);
        
        // 線の幅
        QHBoxLayout* line_width_layout = new QHBoxLayout;
        line_width_layout->addWidget(new QLabel("Line Width:"));
        line_width_spin_ = new QDoubleSpinBox;
        line_width_spin_->setRange(0.5, 10.0);
        line_width_spin_->setValue(0.5);
        line_width_spin_->setDecimals(1);
        line_width_spin_->setSingleStep(0.5);
        line_width_layout->addWidget(line_width_spin_);
        display_layout->addLayout(line_width_layout);
        
        // 境界線表示
        boundaries_checkbox_ = new QCheckBox("Show Track Boundaries");
        boundaries_checkbox_->setChecked(true);
        display_layout->addWidget(boundaries_checkbox_);
        
        // 速度テキスト表示
        speed_text_checkbox_ = new QCheckBox("Show Speed Numbers");
        speed_text_checkbox_->setChecked(false);  // デフォルトは非表示
        display_layout->addWidget(speed_text_checkbox_);
        
        // 座標系設定
        QHBoxLayout* coord_layout = new QHBoxLayout;
        QLabel* coord_label = new QLabel("Coordinate:");
        coord_label->setStyleSheet("font-size: 10px;");
        coord_layout->addWidget(coord_label);
        
        coordinate_system_combo_ = new QComboBox;
        coordinate_system_combo_->addItem("East-North (X=E+, Y=N+)");
        coordinate_system_combo_->addItem("East-South (X=E+, Y=S+)");
        coordinate_system_combo_->addItem("South-West (X=W+, Y=S+)");
        coordinate_system_combo_->addItem("North-West (X=W+, Y=N+)");
        coordinate_system_combo_->setCurrentIndex(1);  // East-Southを初期選択
        coordinate_system_combo_->setStyleSheet("font-size: 9px;");
        coord_layout->addWidget(coordinate_system_combo_);
        display_layout->addLayout(coord_layout);
        
        right_layout_->addWidget(display_group_);
    }
    
    void setupInfoDisplay() {
        info_group_ = new QGroupBox("Information");
        QVBoxLayout* info_layout = new QVBoxLayout(info_group_);
        
        info_label_ = new QLabel("No data loaded");
        info_label_->setWordWrap(true);
        info_label_->setAlignment(Qt::AlignTop);
        info_layout->addWidget(info_label_);
        
        right_layout_->addWidget(info_group_, 1);  // 残りスペースを使用
    }
    
    void connectSignals() {
        // ファイル操作
        connect(open_button_, &QPushButton::clicked, this, &TrajectoryEditor::openFile);
        connect(open_button_2_, &QPushButton::clicked, this, &TrajectoryEditor::openFile2);
        connect(save_button_, &QPushButton::clicked, this, &TrajectoryEditor::saveFile);
        connect(save_button_2_, &QPushButton::clicked, this, &TrajectoryEditor::saveFile2);
        connect(undo_button_, &QPushButton::clicked, this, &TrajectoryEditor::onUndo);
        connect(redo_button_, &QPushButton::clicked, this, &TrajectoryEditor::onRedo);
        
        // ビュー操作
        connect(fit_all_button_, &QPushButton::clicked, this, &TrajectoryEditor::fitAll);
        connect(zoom_in_button_, &QPushButton::clicked, this, &TrajectoryEditor::zoomIn);
        connect(zoom_out_button_, &QPushButton::clicked, this, &TrajectoryEditor::zoomOut);
        connect(reset_zoom_button_, &QPushButton::clicked, this, &TrajectoryEditor::resetZoom);
        
        // 表示設定
        connect(point_size_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &TrajectoryEditor::onPointSizeChanged);
        connect(line_width_spin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                this, &TrajectoryEditor::onLineWidthChanged);
        connect(boundaries_checkbox_, &QCheckBox::toggled,
                this, &TrajectoryEditor::toggleBoundaries);
        connect(speed_text_checkbox_, &QCheckBox::toggled,
                this, &TrajectoryEditor::toggleSpeedText);
        connect(coordinate_system_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &TrajectoryEditor::onCoordinateSystemChanged);
        
        // 編集モード
        connect(view_mode_button_, &QPushButton::clicked,
                this, &TrajectoryEditor::onViewModeClicked);
        connect(add_mode_button_, &QPushButton::clicked,
                this, &TrajectoryEditor::onAddModeClicked);
        
        // 速度編集
        connect(apply_velocity_button_, &QPushButton::clicked,
                this, &TrajectoryEditor::onApplyVelocity);
        connect(range_velocity_button_, &QPushButton::clicked,
                this, &TrajectoryEditor::onApplyRangeVelocity);
        
        // 軌跡ビュー
        connect(trajectory_view_, &trajectory_editor::GraphicsTrajectoryView::pointClicked,
                this, &TrajectoryEditor::onPointClicked);
        connect(trajectory_view_, &trajectory_editor::GraphicsTrajectoryView::pointMoved,
                this, &TrajectoryEditor::onPointMoved);
        connect(trajectory_view_, &trajectory_editor::GraphicsTrajectoryView::pointAdded,
                this, &TrajectoryEditor::onPointAdded);
        connect(trajectory_view_, &trajectory_editor::GraphicsTrajectoryView::pointDeleted,
                this, &TrajectoryEditor::onPointDeleted);
        connect(trajectory_view_, &trajectory_editor::GraphicsTrajectoryView::selectionCleared,
                this, &TrajectoryEditor::clearSelection);
    }
    
    void updateInfoDisplay() {
        QString info;
        
        // 1つ目の軌跡情報（グリーン系）
        if (!trajectory_data_.empty()) {
            info += QString("Green Trajectory:\nPoints: %1\n").arg(trajectory_data_.size());
        } else {
            info += "Green Trajectory: No data\n";
        }
        
        // 2つ目の軌跡情報（ブルー系）
        if (!trajectory_data_2_.empty()) {
            info += QString("Blue Trajectory:\nPoints: %1\n\n").arg(trajectory_data_2_.size());
        } else {
            info += "Blue Trajectory: No data\n\n";
        }
        
        // どちらかのデータがある場合のみ詳細表示
        if (trajectory_data_.empty() && trajectory_data_2_.empty()) {
            info_label_->setText("No data loaded");
            return;
        }
        
        double min_x, max_x, min_y, max_y;
        trajectory_data_.getBounds(min_x, max_x, min_y, max_y);
        
        double min_vel, max_vel;
        trajectory_data_.getVelocityRange(min_vel, max_vel);
        
        info += QString("Bounds:\nX: [%1, %2]\nY: [%3, %4]\n\n")
               .arg(min_x, 0, 'f', 1).arg(max_x, 0, 'f', 1).arg(min_y, 0, 'f', 1).arg(max_y, 0, 'f', 1);
        info += QString("Velocity:\n[%1, %2] km/h\n\n")
               .arg(min_vel, 0, 'f', 1).arg(max_vel, 0, 'f', 1);
        
        info += "Speed Colors:\n";
        info += "• Blue: Low speed\n";
        info += "• Green: Medium speed\n";
        info += "• Red: High speed\n\n";
        
        info += "Controls:\n";
        info += "• Left click: Select point\n";
        info += "• Mouse wheel: Zoom\n";
        info += "• Drag: Pan view\n";
        
        info_label_->setText(info);
    }
    
    void loadDefaultBoundaries() {
        if (track_boundaries_.loadFromCSV("data/track_boundaries.csv")) {
            trajectory_view_->setTrackBoundaries(&track_boundaries_);
            qDebug() << "Track boundaries loaded successfully";
        } else {
            qDebug() << "Failed to load track boundaries";
        }
    }
    
    size_t getCurrentSelectedIndex() const {
        return current_selected_index_;
    }
    
    void updateVelocityUI() {
        if (current_selected_index_ != SIZE_MAX && current_selected_index_ < trajectory_data_.size()) {
            const auto& points = trajectory_data_.getPoints();
            const auto& point = points[current_selected_index_];
            
            selected_point_label_->setText(QString("Point %1 selected").arg(current_selected_index_));
            velocity_spin_->setValue(msToKmh(point.velocity));  // m/sからkm/hに変換して表示
            velocity_spin_->setEnabled(true);
            apply_velocity_button_->setEnabled(true);
            
            // 範囲編集の上限を更新
            size_t max_index = trajectory_data_.size() - 1;
            range_start_spin_->setMaximum(max_index);
            range_end_spin_->setMaximum(max_index);
            
            if (range_end_spin_->value() == 0) {
                range_end_spin_->setValue(max_index);
            }
        } else {
            selected_point_label_->setText("No point selected");
            velocity_spin_->setEnabled(false);
            apply_velocity_button_->setEnabled(false);
        }
    }
    
    void updateHistoryButtons() {
        bool can_undo = edit_history_.canUndo();
        bool can_redo = edit_history_.canRedo();
        
        undo_button_->setEnabled(can_undo);
        redo_button_->setEnabled(can_redo);
        
        // ツールチップに操作説明を追加
        if (can_undo) {
            std::string undo_desc = edit_history_.getUndoDescription();
            undo_button_->setToolTip(QString("Undo: %1").arg(QString::fromStdString(undo_desc)));
        } else {
            undo_button_->setToolTip("Undo");
        }
        
        if (can_redo) {
            std::string redo_desc = edit_history_.getRedoDescription();
            redo_button_->setToolTip(QString("Redo: %1").arg(QString::fromStdString(redo_desc)));
        } else {
            redo_button_->setToolTip("Redo");
        }
    }
};

#include "main.moc"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    TrajectoryEditor window;
    window.show();
    
    return app.exec();
}