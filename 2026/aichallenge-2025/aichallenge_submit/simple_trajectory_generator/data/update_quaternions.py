#!/usr/bin/env python3
"""
trajectory CSVのx/y絶対座標からクォータニオン(qx,qy,qz,qw)を自動計算して更新するツール
x/yから次の点への方向角（yaw）を計算し、Z軸周りの回転クォータニオン（qx=0, qy=0, qz=sin(θ/2), qw=cos(θ/2)）に変換する

使い方:
    python3 update_quaternions.py <input.csv>
    python3 update_quaternions.py <input.csv> -o <output.csv>

クォータニオンの計算方法:
    各点の向き(yaw角)を「現在点→次点」の方向で計算する。
    末尾の点は「末尾点→先頭点」(ループ閉じる)で計算する。
    Z軸周り回転: qx=0, qy=0, qz=sin(yaw/2), qw=cos(yaw/2)
"""
import argparse
import math
import csv
import sys
import os


def compute_quaternions(rows):
    """x,y座標リストからクォータニオンを計算して返す"""
    n = len(rows)
    result = []

    for i in range(n):
        x_curr = float(rows[i]['x'])
        y_curr = float(rows[i]['y'])

        # 次の点（末尾は先頭へ折り返し）
        next_i = (i + 1) % n
        x_next = float(rows[next_i]['x'])
        y_next = float(rows[next_i]['y'])

        dx = x_next - x_curr
        dy = y_next - y_curr

        yaw = math.atan2(dy, dx)

        qx = 0.0
        qy = 0.0
        qz = math.sin(yaw / 2.0)
        qw = math.cos(yaw / 2.0)

        result.append((qx, qy, qz, qw))

    return result


def main():
    parser = argparse.ArgumentParser(
        description='trajectory CSVのx/yからクォータニオンを自動計算して更新する'
    )
    parser.add_argument('input', help='入力CSVファイルパス')
    parser.add_argument('-o', '--output', help='出力CSVファイルパス（省略時は入力ファイルを上書き）')
    args = parser.parse_args()

    input_path = args.input
    output_path = args.output if args.output else args.input

    if not os.path.exists(input_path):
        print(f"エラー: ファイルが見つかりません: {input_path}", file=sys.stderr)
        sys.exit(1)

    # CSV読み込み
    with open(input_path, newline='') as f:
        reader = csv.DictReader(f)
        fieldnames = reader.fieldnames
        rows = list(reader)

    if not rows:
        print("エラー: CSVにデータがありません", file=sys.stderr)
        sys.exit(1)

    for col in ('x', 'y', 'qx', 'qy', 'qz', 'qw'):
        if col not in fieldnames:
            print(f"エラー: 列 '{col}' がCSVに存在しません", file=sys.stderr)
            sys.exit(1)

    print(f"入力: {input_path}  ({len(rows)} 点)")

    # クォータニオン計算
    quats = compute_quaternions(rows)

    # 更新
    for i, row in enumerate(rows):
        qx, qy, qz, qw = quats[i]
        row['qx'] = f"{qx:.6g}"
        row['qy'] = f"{qy:.6g}"
        row['qz'] = f"{qz:.6f}"
        row['qw'] = f"{qw:.6f}"

    # CSV書き出し
    with open(output_path, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    print(f"出力: {output_path}")
    print("クォータニオンの更新が完了しました。")

    # サンプル確認表示
    print("\n--- 先頭3行の確認 ---")
    print(f"{'#':>4}  {'x':>12}  {'y':>12}  {'qz':>10}  {'qw':>10}")
    for i in range(min(3, len(rows))):
        r = rows[i]
        print(f"{i:>4}  {float(r['x']):>12.4f}  {float(r['y']):>12.4f}  "
              f"{float(r['qz']):>10.6f}  {float(r['qw']):>10.6f}")


if __name__ == '__main__':
    main()
