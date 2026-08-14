# 4台同時走行ガイド

## 使うスクリプト

`run_parallel_submissions.bash`（リポジトリ直下）

> **注意:** `./docker_build.sh parallel ...` や `make parallel VEHICLES=4` というコマンドは存在しません。

---

## 手順

### 1. 提出物 tar.gz をリポジトリ配下に置く

Docker build context の制約上、tar.gz はリポジトリ直下（例: `submit/`）に置く必要があります。

```bash
cp team_a.tar.gz /path/to/aichallenge-racingkart/submit/
cp team_b.tar.gz /path/to/aichallenge-racingkart/submit/
cp team_c.tar.gz /path/to/aichallenge-racingkart/submit/
cp team_d.tar.gz /path/to/aichallenge-racingkart/submit/
```

### 2. 並列起動

```bash
cd aichallenge-racingkart
./run_parallel_submissions.bash \
  --submit \
    submit/team_a.tar.gz \
    submit/team_b.tar.gz \
    submit/team_c.tar.gz \
    submit/team_d.tar.gz
```

スクリプトが以下を自動実行します：

1. 各 tar.gz を `autoware-d1`〜`autoware-d4` イメージとしてビルド
2. `simulator.launch.xml`（AWSIM + awsim_state_manager、`ROS_DOMAIN_ID=0`）を起動
3. `autoware-d1`〜`autoware-d4` を並列起動（各 Domain ID 1〜4）

### 3. 停止

```bash
./run_parallel_submissions.bash down
```

---

## ログの確認場所

```
output/<YYYYMMDD-HHMMSS>/
  run_parallel_submissions.log  # ホスト側制御ログ
  awsim.log                     # シミュレータログ
  d1/autoware.log               # 台1 の Autoware ログ
  d2/autoware.log               # 台2
  d3/autoware.log               # 台3
  d4/autoware.log               # 台4
```

---

## 制約・注意点

- tar.gz は **リポジトリ配下**に配置すること（Docker build context 制約）
- 対応台数は 1〜4 台（`--submit` に渡したファイル数で自動決定）
- 再実行すると同名の image tag（`autoware-dN`）を上書きする
- `run_parallel_submissions.bash` は admin-ready/finish 待機を行わない（手動管理）

---

## 処理が重い場合の詳細分析

### 外部通信について

シミュレーション走行（`make dev` / `make eval` / `run_parallel_submissions.bash`）では**外部への通信はありません**。

実車走行時のみ以下が発生します（`make zenoh` を明示的に起動した場合のみ）：

| サービス | 接続先 | プロトコル | 用途 |
|---------|--------|-----------|------|
| Zenoh bridge | `zenoh.dev.aichallenge-board.jsae.or.jp:7448〜7451` | TLS | ROS2トピックを外部にブリッジ |
| NTRIP（オプション） | 設定次第 | TCP | GNSS補正信号の受信 |

クラウドストレージへのアップロードやテレメトリ送信は行われていません。

---

### 重さの原因（3段階）

#### 1. ビルド時間が長い

`run_parallel_submissions.bash` は各 tar.gz に対して `docker build --target eval` を**順番に**実行します。`eval` target は tar.gz を展開して `colcon build` を走らせるため、1件あたり数分かかります。4件なら単純に4倍。

```
# run_parallel_submissions.bash 内部
docker build --target eval --build-arg SUBMIT_TAR=... -t autoware-d1 .
docker build --target eval --build-arg SUBMIT_TAR=... -t autoware-d2 .  # d1完了後に実行
...
```

ただし Docker のレイヤーキャッシュは有効なので、ベースイメージ層は2回目以降は再利用されます。変更差分（submit 内容）のみが毎回ビルドされます。

#### 2. RViz 起動 → Running になるまでが遅い

`docker-compose.yml` の `autoware-d1`〜`autoware-d4` はすべて `run_rviz:=true` が**ハードコード**されています。

```yaml
# docker-compose.yml（現状）
command: ["bash", "-lc", "exec env ROS_DOMAIN_ID=1 ros2 launch ... run_rviz:=true ..."]
```

4台分の RViz が同時起動するため、起動時の GPU/CPU 競合が発生します。加えて各台が `domain_bridge` プロセスも起動するため、Running 状態になるまでの時間が長くなります。

#### 3. 走行中の描画が極端に遅い（1m進むのに10秒以上）← **本質的な原因**

**AWSIM が `--start-mode sync` で動いています。**

```bash
# run_simulator.bash
# 並列モード（1p〜4p）はすべて sync
start_mode="sync"
```

```xml
<!-- simulator.launch.xml -->
<executable cmd="/aichallenge/simulator/AWSIM.x86_64 --start-mode sync --vehicles N ..."/>
```

`sync` モードとは：**シミュレータが1ステップ進むたびに、接続しているすべての Autoware インスタンスの処理完了を待つ**仕組みです。

```
AWSIM step N → [d1処理待ち] [d2処理待ち] [d3処理待ち] [d4処理待ち] → 全完了 → AWSIM step N+1
```

4台の Autoware + 4台の RViz + 4台の `domain_bridge` が同一ホストで競合すると、最も遅いインスタンスに引きずられてシミュレーション全体が止まります。これが「実時間10秒で1m」の正体です。

さらに `domain_bridge` はカメラ画像（`sensor_msgs/msg/Image`）もブリッジしており、帯域負荷が高い：

```yaml
# d1_bridge_config.yaml
d1/sensing/camera/image_raw:
  type: sensor_msgs/msg/Image   # ← 高帯域
d1/sensing/lidar/scan:
  type: sensor_msgs/msg/LaserScan
```

---

### 対処の優先順位

#### 最優先: RViz を無効化する

`docker-compose.yml` の `autoware-d1`〜`autoware-d4` の `run_rviz:=true` を `run_rviz:=false` に変更するだけで、最大の負荷源がなくなります。

```yaml
# 変更前
command: ["bash", "-lc", "exec env ROS_DOMAIN_ID=1 ros2 launch aichallenge_system_launch aichallenge_system.launch.xml simulation:=true use_sim_time:=true run_rviz:=true domain_id:=1 > ..."]

# 変更後
command: ["bash", "-lc", "exec env ROS_DOMAIN_ID=1 ros2 launch aichallenge_system_launch aichallenge_system.launch.xml simulation:=true use_sim_time:=true run_rviz:=false domain_id:=1 > ..."]
```

確認したい場合は別途 `make rviz2` で単体起動できます。

#### 次点: カメラブリッジを止める（評価に不要なら）

`d1_bridge_config.yaml`〜`d4_bridge_config.yaml` から `camera_image_raw` / `camera_info` の行を削除すると、domain_bridge の帯域負荷が下がります（ビルド後に反映）。

#### 参考: sync モードは設計上の仕様

`--start-mode sync` は「全車両を公平に同一条件で評価する」ための意図的な設計です。これを `off`（非同期）に変えると評価の公平性が損なわれます。RViz 無効化で1台あたりの処理が軽くなれば、sync でも十分な速度が出ます。

---

### 原因と対処のまとめ

| フェーズ | 重さの原因 | 対処 |
|---------|-----------|------|
| **ビルド** | 4件のDockerビルドを順番に実行、各件でcolcon build | 2回目以降はDockerキャッシュが効く。初回は待つしかない |
| **起動〜Running** | `run_rviz:=true` で4台RVizが同時起動。`domain_bridge` x4 も起動 | `run_rviz:=false` に変更（**最優先**） |
| **走行中の描画遅延** | `--start-mode sync` により全Autowareの処理完了待ちが発生。RViz+domain_bridge(カメラ画像)の負荷が重なりボトルネックになる | RViz無効化で解消できる。syncモード自体は評価公平性のための仕様 |
