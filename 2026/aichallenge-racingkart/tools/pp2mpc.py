import csv
import math

# 入力・出力パス
path_prefix = "/data/aichallenge/2026/aichallenge-2025/aichallenge/workspace/src/aichallenge_submit/simple_trajectory_generator/data/"
input_path  = path_prefix + "raceline_2026_mincurv6.csv"
path_prefix = "/data/aichallenge/2026/aichallenge-racingkart/aichallenge/workspace/src/aichallenge_submit/multi_purpose_mpc_ros/env/final_ver3"
output_path = path_prefix + "traj_mincurv6.csv"

# --- 読み込み ---
with open(input_path) as f:
    reader = csv.DictReader(f)
    rows = list(reader)

x     = [float(r['x'])     for r in rows]
y     = [float(r['y'])     for r in rows]
qz    = [float(r['qz'])    for r in rows]
qw    = [float(r['qw'])    for r in rows]
speed = [float(r['speed']) for r in rows]
n = len(x)

# s_m: 累積弧長
ds = [0.0] * n
for i in range(1, n):
    dx = x[i] - x[i-1]
    dy = y[i] - y[i-1]
    ds[i] = math.sqrt(dx*dx + dy*dy)
s_m = [0.0] * n
for i in range(1, n):
    s_m[i] = s_m[i-1] + ds[i]

# psi_rad: クォータニオン → ヨー角
psi_raw = [2.0 * math.atan2(qz[i], qw[i]) for i in range(n)]

# unwrap
psi_rad = list(psi_raw)
for i in range(1, n):
    diff = psi_rad[i] - psi_rad[i-1]
    while diff >  math.pi: diff -= 2*math.pi
    while diff < -math.pi: diff += 2*math.pi
    psi_rad[i] = psi_rad[i-1] + diff

# kappa_radpm: 曲率 = dψ/ds（前向き差分、末尾は0）
kappa_radpm = [0.0] * n
for i in range(n - 1):
    seg = ds[i+1] if ds[i+1] > 1e-6 else 1e-6
    kappa_radpm[i] = (psi_rad[i+1] - psi_rad[i]) / seg
kappa_radpm[-1] = 0.0

# vx_mps: そのままコピー
vx_mps = speed

# ax_mps2: a = vx * dvx/ds（前向き差分、末尾は0）
ax_mps2 = [0.0] * n
for i in range(n - 1):
    seg = ds[i+1] if ds[i+1] > 1e-6 else 1e-6
    ax_mps2[i] = vx_mps[i] * (vx_mps[i+1] - vx_mps[i]) / seg
ax_mps2[-1] = 0.0

# --- 書き出し ---
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

print(f"Saved: {output_path}")
print(f"Total points: {n}")
print("s_m,x_m,y_m,psi_rad,kappa_radpm,vx_mps,ax_mps2")
for i in range(min(3, n)):
    print(f"{s_m[i]:.7f},{x[i]:.7f},{y[i]:.7f},{psi_rad[i]:.7f},{kappa_radpm[i]:.7f},{vx_mps[i]:.7f},{ax_mps2[i]:.7f}")
