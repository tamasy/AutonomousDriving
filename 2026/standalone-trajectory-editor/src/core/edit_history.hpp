#pragma once

#include "trajectory_data.hpp"
#include <vector>
#include <memory>
#include <string>

namespace trajectory_editor {

// 編集コマンドの基底クラス
class EditCommand {
public:
    virtual ~EditCommand() = default;
    virtual void execute(TrajectoryData& data) = 0;
    virtual void undo(TrajectoryData& data) = 0;
    virtual std::string getDescription() const = 0;
};

// 点移動コマンド
class MovePointCommand : public EditCommand {
public:
    MovePointCommand(size_t index, double old_x, double old_y, double new_x, double new_y);
    void execute(TrajectoryData& data) override;
    void undo(TrajectoryData& data) override;
    std::string getDescription() const override;

private:
    size_t index_;
    double old_x_, old_y_, new_x_, new_y_;
};

// 点追加コマンド
class AddPointCommand : public EditCommand {
public:
    AddPointCommand(size_t index, const TrajectoryPoint& point);
    void execute(TrajectoryData& data) override;
    void undo(TrajectoryData& data) override;
    std::string getDescription() const override;

private:
    size_t index_;
    TrajectoryPoint point_;
};

// 点削除コマンド
class RemovePointCommand : public EditCommand {
public:
    RemovePointCommand(size_t index, const TrajectoryPoint& point);
    void execute(TrajectoryData& data) override;
    void undo(TrajectoryData& data) override;
    std::string getDescription() const override;

private:
    size_t index_;
    TrajectoryPoint point_;
};

// 速度変更コマンド
class ChangeVelocityCommand : public EditCommand {
public:
    ChangeVelocityCommand(size_t index, double old_velocity, double new_velocity);
    void execute(TrajectoryData& data) override;
    void undo(TrajectoryData& data) override;
    std::string getDescription() const override;

private:
    size_t index_;
    double old_velocity_, new_velocity_;
};

// 範囲速度変更コマンド
class ChangeRangeVelocityCommand : public EditCommand {
public:
    ChangeRangeVelocityCommand(size_t start_index, size_t end_index, 
                              const std::vector<double>& old_velocities, double new_velocity);
    void execute(TrajectoryData& data) override;
    void undo(TrajectoryData& data) override;
    std::string getDescription() const override;

private:
    size_t start_index_, end_index_;
    std::vector<double> old_velocities_;
    double new_velocity_;
};

// 編集履歴管理クラス
class EditHistory {
public:
    EditHistory();
    ~EditHistory() = default;
    
    // 履歴管理
    void executeCommand(std::unique_ptr<EditCommand> command, TrajectoryData& data);
    void undo(TrajectoryData& data);
    void redo(TrajectoryData& data);
    void clear();
    
    // 状態確認
    bool canUndo() const;
    bool canRedo() const;
    std::string getUndoDescription() const;
    std::string getRedoDescription() const;
    
    // 設定
    void setMaxHistorySize(size_t max_size) { max_history_size_ = max_size; }
    size_t getMaxHistorySize() const { return max_history_size_; }

private:
    std::vector<std::unique_ptr<EditCommand>> commands_;
    size_t current_index_;
    size_t max_history_size_;
    
    void trimHistory();
};

} // namespace trajectory_editor