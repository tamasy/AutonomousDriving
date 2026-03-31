import csv
import math

# 入力・出力パス
input_path  = "/home/shiokawa/data/aichallenge/2026/aichallenge-racingkart/aichallenge/workspace/src/aichallenge_submit/multi_purpose_mpc_ros/env/final_ver3/traj_mincurv6.csv"
output_path = "/home/shiokawa/data/aichallenge/2026/aichallenge-racingkart/aichallenge/workspace/src/aichallenge_submit/simple_trajectory_generator/data/traj_mincurv6.csv"

# z座標（高さ）の固定値
Z_VALUE = 0.0

# --- 読み込み ---
with open(input_path) as f:
    reader = csv.DictReader(f)
    rows = list(reader)

x_m     = [float(r['x_m'])     for r in rows]
y_m     = [float(r['y_m'])     for r in rows]
psi_rad = [float(r['psi_rad']) for r in rows]
vx_mps  = [float(r['vx_mps']) for r in rows]
n = len(x_m)

# psi_rad → クォータニオン (2D: qx=0, qy=0, qz=sin(psi/2), qw=cos(psi/2))
qx = [0.0] * n
qy = [0.0] * n
qz = [math.sin(psi_rad[i] / 2.0) for i in range(n)]
qw = [math.cos(psi_rad[i] / 2.0) for i in range(n)]

# --- 書き出し ---
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

print(f"Saved: {output_path}")
print(f"Total points: {n}")
print("x,y,z,qx,qy,qz,qw,speed")
for i in range(min(3, n)):
    print(f"{x_m[i]:.7f},{y_m[i]:.7f},{Z_VALUE:.1f},{qx[i]:.6f},{qy[i]:.6f},{qz[i]:.6f},{qw[i]:.6f},{vx_mps[i]:.7f}")
