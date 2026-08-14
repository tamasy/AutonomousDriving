#!/usr/bin/env python3
"""
PurePursuit形式 → MPC形式 変換ツール

  入力形式: x, y, z, qx, qy, qz, qw, speed
  出力形式: s_m, x_m, y_m, psi_rad, kappa_radpm, vx_mps, ax_mps2

使い方:
    python3 pp2mpc.py <input>            # 出力ファイル名を自動生成
    python3 pp2mpc.py <input> <output>   # 出力ファイル名を明示

ファイル名のみ指定した場合のデフォルトパス:
    入力: simple_trajectory_generator/data/<input>
    出力: multi_purpose_mpc_ros/env/<output>

出力ファイル名の自動生成ルール:
    入力stem末尾の _pp を除去し、_mpc を付与する
    例) raceline_awsim_15km.csv    → raceline_awsim_15km_mpc.csv
    例) raceline_awsim_15km_pp.csv → raceline_awsim_15km_mpc.csv
"""
import argparse
import csv
import math
import os
import sys

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
_BASE = os.path.normpath(os.path.join(_SCRIPT_DIR, '..', 'workspace', 'src', 'aichallenge_submit'))
_PP_DIR  = os.path.join(_BASE, 'simple_trajectory_generator', 'data')
_MPC_DIR = os.path.join(_BASE, 'multi_purpose_mpc_ros', 'env')


def resolve(path, default_dir):
    if os.path.isabs(path):
        return path
    if os.path.exists(path):
        return os.path.abspath(path)
    return os.path.join(default_dir, path)


def auto_output_name(input_name):
    stem, ext = os.path.splitext(os.path.basename(input_name))
    stem = stem.removesuffix('_pp')
    return stem + '_mpc' + (ext or '.csv')


def main():
    parser = argparse.ArgumentParser(description='PurePursuit CSV → MPC CSV 変換')
    parser.add_argument('input',  help='入力PPファイル (ファイル名のみでsimple_trajectory_generator/dataを参照)')
    parser.add_argument('output', nargs='?', help='出力MPCファイル (省略時: 入力stem末尾を_mpcに変換してmulti_purpose_mpc_ros/envへ出力)')
    args = parser.parse_args()

    input_path  = resolve(args.input, _PP_DIR)
    out_name    = args.output if args.output else auto_output_name(args.input)
    output_path = resolve(out_name, _MPC_DIR)

    if not os.path.exists(input_path):
        print(f"エラー: ファイルが見つかりません: {input_path}", file=sys.stderr)
        sys.exit(1)

    with open(input_path) as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    x     = [float(r['x'])     for r in rows]
    y     = [float(r['y'])     for r in rows]
    qz    = [float(r['qz'])    for r in rows]
    qw    = [float(r['qw'])    for r in rows]
    speed = [float(r['speed']) for r in rows]
    n = len(x)

    # 累積弧長
    ds = [0.0] * n
    for i in range(1, n):
        dx = x[i] - x[i-1]
        dy = y[i] - y[i-1]
        ds[i] = math.sqrt(dx*dx + dy*dy)
    s_m = [0.0] * n
    for i in range(1, n):
        s_m[i] = s_m[i-1] + ds[i]

    # クォータニオン → ヨー角 (unwrap)
    psi_raw = [2.0 * math.atan2(qz[i], qw[i]) for i in range(n)]
    psi_rad = list(psi_raw)
    for i in range(1, n):
        diff = psi_rad[i] - psi_rad[i-1]
        while diff >  math.pi: diff -= 2 * math.pi
        while diff < -math.pi: diff += 2 * math.pi
        psi_rad[i] = psi_rad[i-1] + diff

    # 曲率
    kappa_radpm = [0.0] * n
    for i in range(n - 1):
        seg = ds[i+1] if ds[i+1] > 1e-6 else 1e-6
        kappa_radpm[i] = (psi_rad[i+1] - psi_rad[i]) / seg

    # 加速度
    vx_mps  = speed
    ax_mps2 = [0.0] * n
    for i in range(n - 1):
        seg = ds[i+1] if ds[i+1] > 1e-6 else 1e-6
        ax_mps2[i] = vx_mps[i] * (vx_mps[i+1] - vx_mps[i]) / seg

    with open(output_path, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['s_m', 'x_m', 'y_m', 'psi_rad', 'kappa_radpm', 'vx_mps', 'ax_mps2'])
        for i in range(n):
            writer.writerow([
                f"{s_m[i]:.7f}",
                f"{x[i]:.7f}",
                f"{y[i]:.7f}",
                f"{psi_rad[i]:.7f}",
                f"{kappa_radpm[i]:.7f}",
                f"{vx_mps[i]:.7f}",
                f"{ax_mps2[i]:.7f}",
            ])

    print(f"入力: {input_path}  ({n} 点)")
    print(f"出力: {output_path}")
    print("\n--- 先頭3行の確認 ---")
    print("s_m,x_m,y_m,psi_rad,kappa_radpm,vx_mps,ax_mps2")
    for i in range(min(3, n)):
        print(f"{s_m[i]:.7f},{x[i]:.7f},{y[i]:.7f},{psi_rad[i]:.7f},{kappa_radpm[i]:.7f},{vx_mps[i]:.7f},{ax_mps2[i]:.7f}")


if __name__ == '__main__':
    main()
