#!/usr/bin/env python3
"""
走行結果を output/lap_time_results.xlsx へ追記するツール。

行構造（既存 xlsx と互換）:
  1  : タイトル行
  2  : total_laps
  3  : total_time
  4  : best_lap_time
  5〜10: lap_1_time 〜 lap_6_time
  11 : trajectory_csv
  12 : control_method
  13〜22: simple_mpc パラメータ
  23〜30: simple_lqr パラメータ
  31〜 : セクションタイム (section_A_time, section_B_time, ...)

使い方:
    python3 lap_time_tracker.py
    python3 lap_time_tracker.py --result /path/to/result-details.json
    python3 lap_time_tracker.py --launch /path/to/reference.launch.xml
    python3 lap_time_tracker.py --section-times '{"A": 12.3, "B": 14.5}'
    python3 lap_time_tracker.py --output /path/to/lap_time_results.xlsx
"""

import argparse
import json
import os
import re
import sys
from datetime import datetime
from pathlib import Path

import openpyxl
from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
from openpyxl.utils import get_column_letter

# ============================================================
# パス設定
# ============================================================
_SCRIPT_DIR = Path(__file__).resolve().parent
_REPO_ROOT = _SCRIPT_DIR.parents[1]  # aichallenge-racingkart/

_DEFAULT_RESULT_CANDIDATES = [
    _REPO_ROOT / "output" / "latest" / "d1" / "result-details.json",
]

_DEFAULT_SECTION_CANDIDATES = [
    _REPO_ROOT / "output" / "result-section.json",
]

_DEFAULT_LAUNCH_CANDIDATES = [
    _REPO_ROOT / "aichallenge" / "workspace" / "src" / "aichallenge_submit"
    / "aichallenge_submit_launch" / "launch" / "reference.launch.xml",
]

_DEFAULT_OUTPUT = _REPO_ROOT / "output" / "lap_time_results.xlsx"

# ============================================================
# 固定行番号（既存 xlsx 構造と一致させる）
# ============================================================
ROW_TITLE        = 1
ROW_TOTAL_LAPS   = 2
ROW_TOTAL_TIME   = 3
ROW_BEST_LAP     = 4
ROW_LAP_1        = 5   # 5〜10
ROW_TRAJ_CSV     = 11
ROW_PARAMS_START = 12
ROW_SECTIONS_START = 31  # セクションタイムは31行目以降

# ============================================================
# 転記するパラメータ定義（既存 xlsx の行12〜30 に対応）
# (title_col1, param_name)
# ============================================================
PARAM_ROWS = [
    ("method", "control_method"),         # 12
    ("mpc",    "simple_mpc_horizon"),      # 13
    (None,     "simple_mpc_dt"),           # 14
    (None,     "simple_mpc_q0"),           # 15
    (None,     "simple_mpc_q1"),           # 16
    (None,     "simple_mpc_q2"),           # 17
    (None,     "simple_mpc_pid_kp"),       # 18
    (None,     "simple_mpc_pid_ki"),       # 19
    (None,     "simple_mpc_pid_kd"),       # 20
    (None,     "simple_mpc_accel_min"),    # 21
    (None,     "simple_mpc_accel_max"),    # 22
    ("lqr",    "simple_lqr_steering_lpf_alpha"),  # 23
    (None,     "simple_lqr_recovery_threshold"),  # 24
    (None,     "simple_lqr_recovery_max_gain"),   # 25
    (None,     "simple_lqr_pid_kp"),       # 26
    (None,     "simple_lqr_pid_ki"),       # 27
    (None,     "simple_lqr_pid_kd"),       # 28
    (None,     "simple_lqr_accel_min"),    # 29
    (None,     "simple_lqr_accel_max"),    # 30
]
# PARAM_ROWS は 19 行 → 行12〜30

# セクションラベル（最大 20 セクション = 行31〜50）
SECTION_LABELS = list("ABCDEFGHIJKLMNOPQRST")

# ============================================================
# スタイル定義
# ============================================================
HEADER_FILL  = PatternFill("solid", fgColor="1F4E79")
HEADER_FONT  = Font(bold=True, color="FFFFFF", size=10)
NO_FILL = PatternFill(fill_type=None)  # 塗りつぶしなし
RESULT_FILLS = [NO_FILL, NO_FILL]
THIN_BORDER  = Border(
    left=Side(style="thin"), right=Side(style="thin"),
    top=Side(style="thin"),  bottom=Side(style="thin"),
)
CENTER = Alignment(horizontal="center", vertical="center", wrap_text=True)
LEFT   = Alignment(horizontal="left",   vertical="center", wrap_text=True)

# ============================================================
# ユーティリティ
# ============================================================

def find_file(candidates: list) -> "Path | None":
    for p in candidates:
        if p.exists():
            return p
    return None


def load_result(path: Path) -> dict:
    with open(path) as f:
        return json.load(f)


def parse_launch_xml(path: Path) -> dict:
    """reference.launch.xml から arg/let の name と default/value を抽出する。"""
    params = {}
    text = path.read_text()
    for tag in ("arg", "let"):
        attr = "default" if tag == "arg" else "value"
        pattern = rf'<{tag}\s[^>]*name="([^"]+)"[^>]*{attr}="([^"]+)"'
        for m in re.finditer(pattern, text):
            name, val = m.group(1), m.group(2)
            if name not in params:
                params[name] = val
    # csv_path は node 要素の param から取る
    m = re.search(r'<param name="csv_path"\s+value="([^"]+)"', text)
    if m:
        params["csv_path"] = m.group(1)
    return params


def get_csv_basename(params: dict) -> str:
    raw = params.get("csv_path", "")
    return Path(raw).name if raw else ""


# ============================================================
# xlsx 操作
# ============================================================

COL_WIDTH_DATA = 11.1  # 3cm ≈ 11.1 文字幅

def ensure_structure(ws):
    """シートが空の場合は行ラベル（列1・列2）を書き込む。"""
    def write_label(row, col1, col2):
        # 1行目はヘッダー色、2行目以降は塗りつぶしなし
        fill = HEADER_FILL if row == ROW_TITLE else NO_FILL
        font = HEADER_FONT if row == ROW_TITLE else Font(bold=True, size=10)

        c1 = ws.cell(row=row, column=1)
        c1.value = col1
        c1.fill = fill
        c1.font = font
        c1.alignment = CENTER
        c1.border = THIN_BORDER
        c2 = ws.cell(row=row, column=2)
        c2.value = col2
        c2.fill = fill
        c2.font = font
        c2.alignment = LEFT
        c2.border = THIN_BORDER

    write_label(ROW_TITLE,      "title",  "Parameter")
    write_label(ROW_TOTAL_LAPS, "total",  "total_laps")
    write_label(ROW_TOTAL_TIME, None,     "total_time")
    write_label(ROW_BEST_LAP,   None,     "best_lap_time")
    for i in range(6):
        prefix = "lap" if i == 0 else None
        write_label(ROW_LAP_1 + i, prefix, f"lap_{i+1}_time")
    write_label(ROW_TRAJ_CSV,   None,     "trajectory_csv")

    for pi, (col1, pname) in enumerate(PARAM_ROWS):
        write_label(ROW_PARAMS_START + pi, col1, pname)

    # セクション行
    for si, label in enumerate(SECTION_LABELS):
        r = ROW_SECTIONS_START + si
        prefix = "section" if si == 0 else None
        write_label(r, prefix, f"section_{label}_time")

    ws.column_dimensions["A"].width = 14
    ws.column_dimensions["B"].width = 36


def next_run_col(ws) -> int:
    col = 3
    while ws.cell(row=ROW_TITLE, column=col).value is not None:
        col += 1
    return col


def find_param_row(ws, param_name: str) -> "int | None":
    for r in range(1, ws.max_row + 1):
        if ws.cell(row=r, column=2).value == param_name:
            return r
    return None


def write_run(ws, result: dict, params: dict, section_times: dict, run_label: str, na_lap: bool = False):
    col = next_run_col(ws)
    run_index = col - 3  # 0始まり
    fill = RESULT_FILLS[run_index % 2]

    def write(row, value):
        c = ws.cell(row=row, column=col)
        c.value = value
        c.alignment = CENTER
        c.border = THIN_BORDER
        c.fill = fill

    # タイトル行
    c = ws.cell(row=ROW_TITLE, column=col)
    c.value = run_label
    c.alignment = CENTER
    c.border = THIN_BORDER
    c.fill = HEADER_FILL
    c.font = HEADER_FONT

    # ラップ集計
    laps = result.get("laps", [])
    lap_count = result.get("lap_count", len(laps))
    total_time = result.get("total_lap_time", None)
    best_lap   = result.get("min_lap_time",   None)

    if na_lap:
        write(ROW_TOTAL_LAPS, "n/a")
        write(ROW_TOTAL_TIME, "n/a")
        write(ROW_BEST_LAP,   "n/a")
        for i in range(6):
            write(ROW_LAP_1 + i, "n/a")
    else:
        write(ROW_TOTAL_LAPS, lap_count if lap_count else None)
        write(ROW_TOTAL_TIME, f"{total_time:.3f}" if total_time else None)
        write(ROW_BEST_LAP,   f"{best_lap:.3f}"   if best_lap   else None)
        for i in range(6):
            val = f"{laps[i]:.3f}" if i < len(laps) else None
            write(ROW_LAP_1 + i, val)

    # trajectory CSV
    write(ROW_TRAJ_CSV, get_csv_basename(params))

    # パラメータ（列2の param_name で行を検索して書き込む）
    for _, pname in PARAM_ROWS:
        row = find_param_row(ws, pname)
        if row is not None:
            write(row, params.get(pname, None))

    # セクションタイム（列2の section_X_time で行を検索）
    for label, t in section_times.items():
        row = find_param_row(ws, f"section_{label}_time")
        if row is None:
            # 行がなければ末尾に追加
            row = ws.max_row + 1
            si = ord(label) - ord("A")
            prefix = None
            c1 = ws.cell(row=row, column=1)
            c1.value = prefix
            c1.fill  = NO_FILL
            c1.font  = Font(bold=True, size=10)
            c1.alignment = CENTER
            c1.border = THIN_BORDER
            c2 = ws.cell(row=row, column=2)
            c2.value = f"section_{label}_time"
            c2.fill  = NO_FILL
            c2.font  = Font(bold=True, size=10)
            c2.alignment = LEFT
            c2.border = THIN_BORDER
        write(row, f"{t:.3f}" if isinstance(t, (int, float)) else t)

    # C列以降の列幅を3cm固定
    col_letter = get_column_letter(col)
    ws.column_dimensions[col_letter].width = COL_WIDTH_DATA


# ============================================================
# メイン
# ============================================================

def main():
    parser = argparse.ArgumentParser(description="走行結果を xlsx に追記する")
    parser.add_argument("--result", type=Path, default=None,
                        help="result-details.json のパス")
    parser.add_argument("--launch", type=Path, default=None,
                        help="reference.launch.xml のパス")
    parser.add_argument("--output", type=Path, default=_DEFAULT_OUTPUT,
                        help=f"出力 xlsx パス (default: {_DEFAULT_OUTPUT})")
    parser.add_argument("--label", type=str, default=None,
                        help="実行列のラベル（省略時は Run_<timestamp>）")
    parser.add_argument("--section-json", type=Path, default=None,
                        help="result-section.json のパス（省略時は output/latest/d1/ を自動検索）")
    parser.add_argument("--na-lap", action="store_true",
                        help="ラップタイムを n/a として記録する（スタック・手動停止時）")
    args = parser.parse_args()

    # --- result-details.json ---
    result_path = args.result or find_file(_DEFAULT_RESULT_CANDIDATES)
    if args.na_lap:
        # スタック停止時: result-details.json がなくてもラップタイムを n/a で転記
        result = {}
        if result_path and result_path.exists():
            try:
                result = load_result(result_path)
                print(f"[INFO] result (partial): {result_path}")
            except Exception:
                print("[WARN] result-details.json の読み込みに失敗。ラップタイムは n/a で転記します。")
        else:
            print("[INFO] result-details.json なし。ラップタイムは n/a で転記します。")
    else:
        if result_path is None or not result_path.exists():
            print("[ERROR] result-details.json が見つかりません。--result で指定してください。",
                  file=sys.stderr)
            sys.exit(1)
        print(f"[INFO] result: {result_path}")
        result = load_result(result_path)

    # --- reference.launch.xml ---
    launch_path = args.launch or find_file(_DEFAULT_LAUNCH_CANDIDATES)
    if launch_path is None or not launch_path.exists():
        print("[WARN] reference.launch.xml が見つかりません。パラメータは空欄になります。",
              file=sys.stderr)
        params = {}
    else:
        print(f"[INFO] launch:  {launch_path}")
        params = parse_launch_xml(launch_path)

    # --- セクションタイム（result-section.json を自動検索）---
    section_times = {}
    section_path = args.section_json or find_file(_DEFAULT_SECTION_CANDIDATES)
    if section_path and section_path.exists():
        print(f"[INFO] section: {section_path}")
        with open(section_path) as f:
            section_data = json.load(f)
        section_times = {k: v for k, v in section_data.get("section_times", {}).items()
                         if v is not None}
    else:
        print("[INFO] result-section.json なし。セクションタイムは空欄になります。")

    # --- xlsx を開く or 新規作成 ---
    output_path: Path = args.output
    output_path.parent.mkdir(parents=True, exist_ok=True)

    if output_path.exists():
        wb = openpyxl.load_workbook(output_path)
        ws = wb.active
        print(f"[INFO] 既存 xlsx に追記: {output_path}")
    else:
        wb = openpyxl.Workbook()
        ws = wb.active
        ws.title = "lap_time_results"
        ensure_structure(ws)
        print(f"[INFO] 新規 xlsx を作成: {output_path}")

    run_label = args.label or f"Run_{datetime.now().strftime('%Y%m%d-%H%M%S')}"
    write_run(ws, result, params, section_times, run_label, na_lap=args.na_lap)
    wb.save(output_path)

    print(f"[OK] 書き込み完了: {output_path}  (列: {run_label})")

    laps = result.get("laps", [])
    if args.na_lap:
        print("\n  周回数      : n/a (スタック停止)")
        print("  トータル時間: n/a")
        print("  ベストラップ: n/a")
        for i in range(6):
            print(f"  Lap {i+1}       : n/a")
    else:
        print(f"\n  周回数      : {result.get('lap_count', len(laps))}")
        if result.get("total_lap_time"):
            print(f"  トータル時間: {result['total_lap_time']:.3f} s")
        if result.get("min_lap_time"):
            print(f"  ベストラップ: {result['min_lap_time']:.3f} s")
        for i, t in enumerate(laps[:6]):
            print(f"  Lap {i+1}       : {t:.3f} s")
    if get_csv_basename(params):
        print(f"  Trajectory  : {get_csv_basename(params)}")
    if section_times:
        for label, t in section_times.items():
            print(f"  Section {label}   : {t:.3f} s" if isinstance(t, (int, float)) else f"  Section {label}   : {t}")
    else:
        print("  Section     : (なし)")


if __name__ == "__main__":
    main()
