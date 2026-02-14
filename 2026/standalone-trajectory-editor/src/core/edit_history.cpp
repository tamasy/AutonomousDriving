#include "edit_history.hpp"
#include <sstream>

namespace trajectory_editor {

// MovePointCommand implementation
MovePointCommand::MovePointCommand(size_t index, double old_x, double old_y, double new_x, double new_y)
    : index_(index), old_x_(old_x), old_y_(old_y), new_x_(new_x), new_y_(new_y) {}

void MovePointCommand::execute(TrajectoryData& data) {
    data.movePoint(index_, new_x_, new_y_);
}

void MovePointCommand::undo(TrajectoryData& data) {
    data.movePoint(index_, old_x_, old_y_);
}

std::string MovePointCommand::getDescription() const {
    std::ostringstream oss;
    oss << "Move point " << index_;
    return oss.str();
}

// AddPointCommand implementation
AddPointCommand::AddPointCommand(size_t index, const TrajectoryPoint& point)
    : index_(index), point_(point) {}

void AddPointCommand::execute(TrajectoryData& data) {
    data.insertPoint(index_, point_);
}

void AddPointCommand::undo(TrajectoryData& data) {
    data.removePoint(index_);
}

std::string AddPointCommand::getDescription() const {
    std::ostringstream oss;
    oss << "Add point at " << index_;
    return oss.str();
}

// RemovePointCommand implementation
RemovePointCommand::RemovePointCommand(size_t index, const TrajectoryPoint& point)
    : index_(index), point_(point) {}

void RemovePointCommand::execute(TrajectoryData& data) {
    data.removePoint(index_);
}

void RemovePointCommand::undo(TrajectoryData& data) {
    data.insertPoint(index_, point_);
}

std::string RemovePointCommand::getDescription() const {
    std::ostringstream oss;
    oss << "Remove point " << index_;
    return oss.str();
}

// ChangeVelocityCommand implementation
ChangeVelocityCommand::ChangeVelocityCommand(size_t index, double old_velocity, double new_velocity)
    : index_(index), old_velocity_(old_velocity), new_velocity_(new_velocity) {}

void ChangeVelocityCommand::execute(TrajectoryData& data) {
    const auto& points = data.getPoints();
    if (index_ < points.size()) {
        TrajectoryPoint updated_point = points[index_];
        updated_point.velocity = new_velocity_;
        data.updatePoint(index_, updated_point);
    }
}

void ChangeVelocityCommand::undo(TrajectoryData& data) {
    const auto& points = data.getPoints();
    if (index_ < points.size()) {
        TrajectoryPoint updated_point = points[index_];
        updated_point.velocity = old_velocity_;
        data.updatePoint(index_, updated_point);
    }
}

std::string ChangeVelocityCommand::getDescription() const {
    std::ostringstream oss;
    oss << "Change velocity of point " << index_;
    return oss.str();
}

// ChangeRangeVelocityCommand implementation
ChangeRangeVelocityCommand::ChangeRangeVelocityCommand(size_t start_index, size_t end_index,
                                                      const std::vector<double>& old_velocities, double new_velocity)
    : start_index_(start_index), end_index_(end_index), old_velocities_(old_velocities), new_velocity_(new_velocity) {}

void ChangeRangeVelocityCommand::execute(TrajectoryData& data) {
    data.updateVelocityRange(start_index_, end_index_, new_velocity_);
}

void ChangeRangeVelocityCommand::undo(TrajectoryData& data) {
    const auto& points = data.getPoints();
    for (size_t i = 0; i < old_velocities_.size() && (start_index_ + i) <= end_index_; ++i) {
        size_t point_index = start_index_ + i;
        if (point_index < points.size()) {
            TrajectoryPoint updated_point = points[point_index];
            updated_point.velocity = old_velocities_[i];
            data.updatePoint(point_index, updated_point);
        }
    }
}

std::string ChangeRangeVelocityCommand::getDescription() const {
    std::ostringstream oss;
    oss << "Change velocity range " << start_index_ << "-" << end_index_;
    return oss.str();
}

// EditHistory implementation
EditHistory::EditHistory() : current_index_(0), max_history_size_(50) {}

void EditHistory::executeCommand(std::unique_ptr<EditCommand> command, TrajectoryData& data) {
    // 現在の位置以降のコマンドを削除（redo履歴をクリア）
    commands_.erase(commands_.begin() + current_index_, commands_.end());
    
    // コマンドを実行
    command->execute(data);
    
    // 履歴に追加
    commands_.push_back(std::move(command));
    current_index_ = commands_.size();
    
    // 履歴サイズを制限
    trimHistory();
}

void EditHistory::undo(TrajectoryData& data) {
    if (canUndo()) {
        --current_index_;
        commands_[current_index_]->undo(data);
    }
}

void EditHistory::redo(TrajectoryData& data) {
    if (canRedo()) {
        commands_[current_index_]->execute(data);
        ++current_index_;
    }
}

void EditHistory::clear() {
    commands_.clear();
    current_index_ = 0;
}

bool EditHistory::canUndo() const {
    return current_index_ > 0;
}

bool EditHistory::canRedo() const {
    return current_index_ < commands_.size();
}

std::string EditHistory::getUndoDescription() const {
    if (canUndo()) {
        return commands_[current_index_ - 1]->getDescription();
    }
    return "";
}

std::string EditHistory::getRedoDescription() const {
    if (canRedo()) {
        return commands_[current_index_]->getDescription();
    }
    return "";
}

void EditHistory::trimHistory() {
    if (commands_.size() > max_history_size_) {
        size_t excess = commands_.size() - max_history_size_;
        commands_.erase(commands_.begin(), commands_.begin() + excess);
        current_index_ = (current_index_ > excess) ? current_index_ - excess : 0;
    }
}

} // namespace trajectory_editor