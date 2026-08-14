#!/usr/bin/env python3
"""
MPC形式 → PurePursuit形式 変換ツール

  入力形式: s_m, x_m, y_m, psi_rad, kappa_radpm, vx_mps, ax_mps2
  出力形式: x, y, z, qx, qy, qz, qw, speed

使い方:
    python3 mpc2pp.py <input>            # 出力ファイル名を自動生成
    python3 mpc2pp.py <input> <output>   # 出力ファイル名を明示

ファイル名のみ指定した場合のデフォルトパス:
    入力: multi_purpose_mpc_ros/env/<input>
    出力: simple_trajectory_generator/data/<output>

出力ファイル名の自動生成ルール:
    入力stem末尾の _mpc を除去し、_pp を付与する
    例) traj_mincurv_35kph.csv     → traj_mincurv_35kph_pp.csv
    例) traj_mincurv_35kph_mpc.csv → traj_mincurv_35kph_pp.csv
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

Z_VALUE = 0.0


def resolve(path, default_dir):
    if os.path.isabs(path):
        return path
    if os.path.exists(path):
        return os.path.abspath(path)
    return os.path.join(default_dir, path)


def auto_output_name(input_name):
    stem, ext = os.path.splitext(os.path.basename(input_name))
    stem = stem.removesuffix('_mpc')
    return stem + '_pp' + (ext or '.csv')


def main():
    parser = argparse.ArgumentParser(description='MPC CSV → PurePursuit CSV 変換')
    parser.add_argument('input',  help='入力MPCファイル (ファイル名のみでmulti_purpose_mpc_ros/envを参照)')
    parser.add_argument('output', nargs='?', help='出力PPファイル (省略時: 入力stem末尾を_ppに変換してsimple_trajectory_generator/dataへ出力)')
    args = parser.parse_args()

    input_path  = resolve(args.input, _MPC_DIR)
    out_name    = args.output if args.output else auto_output_name(args.input)
    output_path = resolve(out_name, _PP_DIR)

    if not os.path.exists(input_path):
        print(f"エラー: ファイルが見つかりません: {input_path}", file=sys.stderr)
        sys.exit(1)

    with open(input_path) as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    x_m     = [float(r['x_m'])     for r in rows]
    y_m     = [float(r['y_m'])     for r in rows]
    psi_rad = [float(r['psi_rad']) for r in rows]
    vx_mps  = [float(r['vx_mps']) for r in rows]
    n = len(x_m)

    qx = [0.0] * n
    qy = [0.0] * n
    qz = [math.sin(psi_rad[i] / 2.0) for i in range(n)]
    qw = [math.cos(psi_rad[i] / 2.0) for i in range(n)]

    with open(output_path, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['x', 'y', 'z', 'qx', 'qy', 'qz', 'qw', 'speed'])
        for i in range(n):
            writer.writerow([
                f"{x_m[i]:.7f}",
                f"{y_m[i]:.7f}",
                f"{Z_VALUE:.1f}",
                f"{qx[i]:.6f}",
                f"{qy[i]:.6f}",
                f"{qz[i]:.6f}",
                f"{qw[i]:.6f}",
                f"{vx_mps[i]:.7f}",
            ])

    print(f"入力: {input_path}  ({n} 点)")
    print(f"出力: {output_path}")
    print("\n--- 先頭3行の確認 ---")
    print("x,y,z,qx,qy,qz,qw,speed")
    for i in range(min(3, n)):
        print(f"{x_m[i]:.7f},{y_m[i]:.7f},{Z_VALUE:.1f},{qx[i]:.6f},{qy[i]:.6f},{qz[i]:.6f},{qw[i]:.6f},{vx_mps[i]:.7f}")


if __name__ == '__main__':
    main()
