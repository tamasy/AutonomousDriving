#!/usr/bin/env python3
"""
rosbag から セクション通過タイムを計算して result-section.json に出力する。

visualize_sections.py と同じ曲率ベースのゲート座標を使用。
ゲート通過の判定: 車両がゲート法線を跨いだ瞬間（符号が反転）を検知。

使い方:
    python3 calc_section_times.py
    python3 calc_section_times.py --bag /path/to/rosbag2_autoware.mcap
    python3 calc_section_times.py --csv /path/to/trajectory.csv
    python3 calc_section_times.py --threshold 0.17 --n-sections 10
"""

import argparse
import csv
import json
import sys
from pathlib import Path

import numpy as np
from scipy.signal import find_peaks

# ============================================================
# パス設定（visualize_sections.py と同じ候補）
# ============================================================
_SCRIPT_DIR = Path(__file__).resolve().parent
_REPO_ROOT   = _SCRIPT_DIR.parents[1]

_CSV_FILENAME = "raceline_traj_mincurv_20kph.csv"
_CSV_CANDIDATES = [
    _REPO_ROOT / "aichallenge" / "workspace" / "install" / "simple_trajectory_generator"
    / "share" / "simple_trajectory_generator" / "data" / _CSV_FILENAME,
    _REPO_ROOT / "aichallenge" / "workspace" / "src" / "aichallenge_submit"
    / "simple_trajectory_generator" / "data" / _CSV_FILENAME,
]

_BAG_CANDIDATES = [
    _REPO_ROOT / "output" / "latest" / "d1" / "rosbag2_autoware.mcap",
]

_DEFAULT_OUTPUT = _REPO_ROOT / "output" / "result-section.json"

# visualize_sections.py と同じデフォルト値
DEFAULT_THRESHOLD   = 0.17
DEFAULT_MIN_DIST    = 15
DEFAULT_N_SECTIONS  = 10
DEFAULT_EXIT_OFFSET = 5
DEFAULT_GATE_LEN    = 5.0

# ============================================================
# visualize_sections.py から流用: 曲率計算・セクション抽出
# ============================================================

def load_trajectory(csv_path: str):
    xs, ys, qzs, qws = [], [], [], []
    with open(csv_path) as f:
        reader = csv.DictReader(f)
        for row in reader:
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
            qzs.append(float(row["qz"]))
            qws.append(float(row["qw"]))
    return np.array(xs), np.array(ys), np.array(qzs), np.array(qws)


def compute_curvature(xs, ys, qzs, qws, smooth_window: int = 5):
    yaws = 2.0 * np.arctan2(qzs, qws)
    dx = np.diff(xs)
    dy = np.diff(ys)
    ds = np.hypot(dx, dy)
    dyaw = np.diff(yaws)
    dyaw = (dyaw + np.pi) % (2 * np.pi) - np.pi
    kappa = dyaw / (ds + 1e-9)
    abs_k = np.abs(kappa)
    kernel = np.ones(smooth_window) / smooth_window
    abs_k_smooth = np.convolve(abs_k, kernel, mode="same")
    return yaws, kappa, abs_k_smooth


def find_hairpin_exit(abs_k_smooth, peak_idx: int, threshold: float) -> int:
    n = len(abs_k_smooth)
    for i in range(peak_idx + 1, n):
        if abs_k_smooth[i] < threshold:
            return i
    return peak_idx


def extract_sections(xs, ys, yaws, kappa, abs_k_smooth,
                     threshold, min_dist, n_sections, exit_offset):
    peaks, _ = find_peaks(abs_k_smooth, height=threshold, distance=min_dist)
    if len(peaks) > n_sections:
        top_idx = np.argsort(abs_k_smooth[peaks])[::-1][:n_sections]
        peaks = np.sort(peaks[top_idx])

    n = len(xs)
    sections = []
    for i, p in enumerate(peaks):
        exit_idx = find_hairpin_exit(abs_k_smooth, int(p), threshold)
        pos = min(max(exit_idx + exit_offset, 0), n - 1)
        yaw = yaws[pos]
        label = chr(ord("A") + i)
        sections.append({
            "label":   label,
            "x":       float(xs[pos]),
            "y":       float(ys[pos]),
            "normal":  (-float(np.sin(yaw)), float(np.cos(yaw))),
        })
    return sections


# ============================================================
# ゲート通過判定
# ============================================================

def signed_distance_to_gate(px, py, gx, gy, nx, ny):
    """車両位置 (px,py) からゲート中心 (gx,gy) への符号付き距離（法線方向）"""
    return (px - gx) * nx + (py - gy) * ny


def calc_section_times(sections, positions):
    """
    positions: [(time_sec, x, y), ...]  時系列順
    各セクションゲートを通過した時刻を記録し、通過順にソートして
    連続するゲート通過時刻の差をセクションタイムとする。
    """
    n_sec = len(sections)
    gate_pass_times = [None] * n_sec
    prev_signs = [None] * n_sec

    for t, px, py in positions:
        for i, sec in enumerate(sections):
            if gate_pass_times[i] is not None:
                continue
            gx, gy = sec["x"], sec["y"]
            nx, ny = sec["normal"]
            sd = signed_distance_to_gate(px, py, gx, gy, nx, ny)
            sign = 1 if sd >= 0 else -1

            if prev_signs[i] is None:
                prev_signs[i] = sign
                continue

            if prev_signs[i] < 0 and sign > 0:
                gate_pass_times[i] = t
            prev_signs[i] = sign

    # 通過順にソートして A, B, C... とラベルを振り直す
    order = sorted(range(n_sec),
                   key=lambda i: gate_pass_times[i] if gate_pass_times[i] else float("inf"))

    # 通過順ラベル: 最初に通過したゲートを A, 次を B, ...
    relabeled = {}  # 旧ラベル → 新ラベル
    for rank, i in enumerate(order):
        new_label = chr(ord("A") + rank)
        relabeled[sections[i]["label"]] = new_label

    # t=0 を最初の通過ゲート基準に正規化
    t0 = gate_pass_times[order[0]] if gate_pass_times[order[0]] is not None else 0.0
    normalized = {
        relabeled[sections[i]["label"]]: round(gate_pass_times[i] - t0, 3)
        if gate_pass_times[i] is not None else None
        for i in range(n_sec)
    }

    # 連続する通過ゲート間の差 = セクションタイム（新ラベルで返す）
    section_times = {}
    for rank, i in enumerate(order):
        new_label = relabeled[sections[i]["label"]]
        t_this = gate_pass_times[i]
        next_rank = rank + 1
        if next_rank < n_sec:
            j = order[next_rank]
            t_next = gate_pass_times[j]
        else:
            t_next = None

        if t_this is not None and t_next is not None:
            section_times[new_label] = round(t_next - t_this, 3)
        else:
            section_times[new_label] = None

    return section_times, gate_pass_times, normalized, order, relabeled


# ============================================================
# rosbag 読み込み
# ============================================================

def load_positions_from_bag(bag_path: str):
    """rosbag から /localization/kinematic_state の (time, x, y) を返す"""
    from rosbags.rosbag2 import Reader
    from rosbags.typesys import Stores, get_typestore

    typestore = get_typestore(Stores.ROS2_HUMBLE)
    positions = []

    with Reader(bag_path) as reader:
        conns = [c for c in reader.connections
                 if c.topic == "/localization/kinematic_state"]
        if not conns:
            print("[ERROR] /localization/kinematic_state がbagに見つかりません",
                  file=sys.stderr)
            sys.exit(1)

        for conn, timestamp, rawdata in reader.messages(connections=conns):
            msg = typestore.deserialize_cdr(rawdata, conn.msgtype)
            t = timestamp / 1e9  # nanosec → sec
            x = msg.pose.pose.position.x
            y = msg.pose.pose.position.y
            positions.append((t, x, y))

    positions.sort(key=lambda v: v[0])
    return positions


# ============================================================
# メイン
# ============================================================

def main():
    parser = argparse.ArgumentParser(description="rosbag からセクション通過タイムを計算する")
    parser.add_argument("--bag", type=Path, default=None,
                        help="rosbag (.mcap) のパス")
    parser.add_argument("--csv", type=Path, default=None,
                        help="trajectory CSV のパス")
    parser.add_argument("--output", type=Path, default=_DEFAULT_OUTPUT,
                        help=f"出力 JSON パス (default: {_DEFAULT_OUTPUT})")
    parser.add_argument("--threshold",  type=float, default=DEFAULT_THRESHOLD)
    parser.add_argument("--min-dist",   type=int,   default=DEFAULT_MIN_DIST)
    parser.add_argument("--n-sections", type=int,   default=DEFAULT_N_SECTIONS)
    parser.add_argument("--exit-offset",type=int,   default=DEFAULT_EXIT_OFFSET)
    parser.add_argument("--gate-len",   type=float, default=DEFAULT_GATE_LEN)
    args = parser.parse_args()

    # --- bag パス ---
    bag_path = args.bag
    if bag_path is None:
        bag_path = next((p for p in _BAG_CANDIDATES if p.exists()), None)
    if bag_path is None or not bag_path.exists():
        print("[ERROR] rosbag が見つかりません。--bag で指定してください。", file=sys.stderr)
        sys.exit(1)
    print(f"[INFO] bag:  {bag_path}")

    # --- CSV パス ---
    csv_path = args.csv
    if csv_path is None:
        csv_path = next((p for p in _CSV_CANDIDATES if p.exists()), None)
    if csv_path is None or not csv_path.exists():
        print("[ERROR] trajectory CSV が見つかりません。--csv で指定してください。", file=sys.stderr)
        sys.exit(1)
    print(f"[INFO] csv:  {csv_path}")

    # --- セクション定義（visualize_sections.py と同じロジック）---
    xs, ys, qzs, qws = load_trajectory(str(csv_path))
    yaws, kappa, abs_k_smooth = compute_curvature(xs, ys, qzs, qws)
    sections = extract_sections(
        xs, ys, yaws, kappa, abs_k_smooth,
        threshold=args.threshold,
        min_dist=args.min_dist,
        n_sections=args.n_sections,
        exit_offset=args.exit_offset,
    )
    print(f"[INFO] セクション数: {len(sections)}")

    # --- rosbag から位置読み込み ---
    print("[INFO] rosbag を読み込み中...")
    positions = load_positions_from_bag(str(bag_path))
    print(f"[INFO] 位置データ: {len(positions)} 点")

    # --- セクションタイム計算 ---
    section_times, gate_pass_times, normalized, order, relabeled = calc_section_times(sections, positions)

    # --- 結果出力 ---
    result = {
        "section_times": section_times,
        "gate_pass_elapsed": normalized,  # 最初のゲート通過を A=0秒とした経過時刻
    }

    out_path: Path = args.output
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w") as f:
        json.dump(result, f, indent=4, ensure_ascii=False)

    print(f"\n[OK] 出力: {out_path}")
    print()
    print(f"  {'Section':>8}  {'経過時刻[s]':>12}  {'区間タイム[s]':>14}")
    print("  " + "-" * 40)
    for i in order:
        new_label = relabeled[sections[i]["label"]]
        t_pass = f"{normalized[new_label]:.3f}" if normalized[new_label] is not None else "  ---"
        t_sec  = f"{section_times[new_label]:.3f}" if section_times[new_label] else "  ---"
        print(f"  {new_label:>8}  {t_pass:>12}  {t_sec:>14}")

    return result


if __name__ == "__main__":
    main()
