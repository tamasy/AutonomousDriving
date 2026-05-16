# trans_format_mpcpp — PurePursuit ↔ MPC 軌道フォーマット変換ツール

MPC形式とPurePursuit形式の軌道CSVを相互変換する。コマンド1つで変換でき、出力ファイル名は自動生成される。

---

## 考え方

### なぜ作ったか

MPC（`multi_purpose_mpc_ros`）とPurePursuit（`simple_trajectory_generator`）では軌道CSVのフォーマットが異なる。
同じ軌道を両方のコントローラで試したいとき、手作業で変換するのは手間がかかりミスも起きやすい。

そこで **「ファイル名を渡すだけで変換が完了する」** スクリプトを2本用意した。
出力先ディレクトリはデフォルトで各パッケージの標準パスを向いており、ファイル名のみ渡せばすぐに使える。

### 変換の方向

```
MPC形式  ──── mpc2pp.py ────→  PurePursuit形式
MPC形式  ←─── pp2mpc.py ────  PurePursuit形式
```

---

## フォーマット仕様

### MPC形式（`multi_purpose_mpc_ros/env/`）

| カラム | 内容 |
|---|---|
| `s_m` | 累積弧長 [m] |
| `x_m` | X座標 [m] |
| `y_m` | Y座標 [m] |
| `psi_rad` | ヨー角 [rad]（unwrap済み） |
| `kappa_radpm` | 曲率 [rad/m] |
| `vx_mps` | 速度 [m/s] |
| `ax_mps2` | 加速度 [m/s²] |

### PurePursuit形式（`simple_trajectory_generator/data/`）

| カラム | 内容 |
|---|---|
| `x` | X座標 [m] |
| `y` | Y座標 [m] |
| `z` | Z座標 [m]（固定値 0.0） |
| `qx` | クォータニオン x（固定値 0.0） |
| `qy` | クォータニオン y（固定値 0.0） |
| `qz` | クォータニオン z（ヨー角から算出） |
| `qw` | クォータニオン w（ヨー角から算出） |
| `speed` | 速度 [m/s] |

---

## ファイル構成

| ファイル | 役割 |
|---|---|
| `aichallenge/tools/mpc2pp.py` | MPC形式 → PurePursuit形式 変換 |
| `aichallenge/tools/pp2mpc.py` | PurePursuit形式 → MPC形式 変換 |

---

## 使い方

```bash
cd ~/data/aichallenge/2026/aichallenge-racingkart/aichallenge/tools
```

### MPC → PurePursuit

```bash
python3 mpc2pp.py <input>            # 出力ファイル名を自動生成
python3 mpc2pp.py <input> <output>   # 出力ファイル名末尾に _pp と明示
```

### PurePursuit → MPC

```bash
python3 pp2mpc.py <input>            # 出力ファイル名を自動生成
python3 pp2mpc.py <input> <output>   # 出力ファイル名末尾に _mpc と明示
```

---

## デフォルトパス

ファイル名のみ（パスなし）で渡した場合、各スクリプトが自動的に標準ディレクトリを参照・出力先とする。

| スクリプト | 入力デフォルトディレクトリ | 出力デフォルトディレクトリ |
|---|---|---|
| `mpc2pp.py` | `multi_purpose_mpc_ros/env/` | `simple_trajectory_generator/data/` |
| `pp2mpc.py` | `simple_trajectory_generator/data/` | `multi_purpose_mpc_ros/env/` |

絶対パスまたはカレントディレクトリに存在するパスを渡した場合はそのまま使用される。

---

## 出力ファイル名の自動生成ルール

出力ファイル名を省略した場合、入力ファイルのステム末尾を書き換えて自動生成する。

### mpc2pp.py（MPC → PP）

- 末尾の `_mpc` を除去し `_pp` を付与
- 例: `traj_mincurv_35kph.csv`     → `traj_mincurv_35kph_pp.csv`
- 例: `traj_mincurv_35kph_mpc.csv` → `traj_mincurv_35kph_pp.csv`

### pp2mpc.py（PP → MPC）

- 末尾の `_pp` を除去し `_mpc` を付与
- 例: `raceline_awsim_15km.csv`    → `raceline_awsim_15km_mpc.csv`
- 例: `raceline_awsim_15km_pp.csv` → `raceline_awsim_15km_mpc.csv`

---

## 変換ロジック

### mpc2pp（MPC → PP）

- `psi_rad` → クォータニオン: `qz = sin(psi/2)`, `qw = cos(psi/2)`（`qx`, `qy` は 0 固定）
- `vx_mps` → `speed`（そのままコピー）
- `z` は 0.0 固定

### pp2mpc（PP → MPC）

- クォータニオン → ヨー角: `psi = 2 * atan2(qz, qw)`（unwrap処理あり）
- 累積弧長 `s_m`: 点間距離の積算
- 曲率 `kappa_radpm`: `Δpsi / Δs`
- 加速度 `ax_mps2`: `vx * Δvx / Δs`
- 末尾点の曲率・加速度は 0.0

---

## アピールポイント

### 1. ファイル名1つで変換が完了する

入力ファイル名だけ渡せば、出力先ディレクトリと出力ファイル名を自動決定して変換まで完了する。
パスを調べたりファイル名を手で考えたりする手間がない。

### 2. 命名規則が統一される

`_mpc` / `_pp` サフィックスの自動付与・除去により、どちらの形式かがファイル名から一目でわかる。
手作業でリネームすると起きる `_mpc_pp.csv` のような二重サフィックス問題も防ぐ。

### 3. 変換結果を即座に目視確認できる

実行後に先頭3行が標準出力に表示されるため、値の桁や符号が壊れていないかをコマンドラインでその場で確認できる。
ファイルを開いてエディタで確認する手間が省ける。

### 4. 両方向の変換をカバーしている

MPC→PP・PP→MPC の両方向に対応しており、どちらのコントローラから先に軌道を作っても相互に使い回せる。
「MPC用に最適化した軌道をPurePursuitでも試す」「PPで作ったラインをMPCに読み込む」といった運用が気軽にできる。

---

## 実行例

```bash
# MPC形式のファイルをPurePursuit形式に変換（デフォルトパス使用）
python3 mpc2pp.py traj_mincurv_35kph_mpc.csv
# → simple_trajectory_generator/data/traj_mincurv_35kph_pp.csv

# PurePursuit形式のファイルをMPC形式に変換（デフォルトパス使用）
python3 pp2mpc.py raceline_awsim_15km_pp.csv
# → multi_purpose_mpc_ros/env/raceline_awsim_15km_mpc.csv

# 絶対パスで出力先を明示
python3 mpc2pp.py /path/to/traj.csv /path/to/output_pp.csv
```

実行すると変換後の先頭3行が標準出力に表示されるので、変換結果を即座に確認できる。

```
入力: .../multi_purpose_mpc_ros/env/traj_mincurv_35kph_mpc.csv  (1234 点)
出力: .../simple_trajectory_generator/data/traj_mincurv_35kph_pp.csv

--- 先頭3行の確認 ---
x,y,z,qx,qy,qz,qw,speed
...
```

---

## トラブルシューティング

| 症状 | 原因 | 対処 |
|---|---|---|
| `エラー: ファイルが見つかりません` | 入力ファイルが存在しない | パスを確認、またはデフォルトディレクトリに配置 |
| 出力ファイルが意図しない場所に生成された | ファイル名のみ渡したため自動パスが適用された | 絶対パスまたは相対パスで出力先を明示 |
| 曲率・加速度が末尾で 0 になる | 仕様（末尾点は差分計算不可） | MPC側で末尾の扱いを確認 |
