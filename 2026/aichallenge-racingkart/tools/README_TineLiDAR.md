## 環境セットアップ

AI Challenge 2026のドキュメントに従って設定する

https://automotiveaichallenge.github.io/aichallenge-documentation-racingkart/ml_sample/getting_started_tiny_lidar_net.html#setup


SimPracticeFor2026内のAWSIM.zipをダウンロードして使用してください。

AWSIMの配置についてはこちらのページを参考に、aichallenge-racingkart/aichallenge/simulatorに配置してください。実行ファイルがaichallenge-racingkart/aichallenge/simulator/AWSIM/AWSIM.x86_64に存在していることを確認してください。

requirements.txt にないライブラリも必要となる。インストールしておく。

```bash
pip3 install rosbags
pip3 install hydra-core omegaconf tensorboard tqdm jaxtyping
```

## 大会用リポジトリのビルド・実行
ここから先の作業は基本的にdocker containerの中で行うことを想定しています。 docker containerの起動は下記のコマンドで行うことができます。

しかし、デフォルトのスクリプトだとDocekrコンテナを起動させない。

** 原因 
aichallenge-2025-devイメージにCMDが定義されていません。ENTRYPOINTが引数なしで実行されて即終了しています。
devステージにはCMDがありません。 evalステージにはCMD ["bash", "/aichallenge/run_evaluation.bash"]があるのに、devステージにはCMDが未定義のため、ENTRYPOINTが引数なしで実行されて即座に終了しています。

** 対策
Dockerfile:46-52 のdevステージにCMD ["bash"]を追加すれば修正できます。

修正後に以下実行すれば起動できる！
```bash
# ターミナル1
cd data/aichallenge/2026/aichallenge-racingkart
./docker_run.sh dev cpu
```

以下を残りターミナル3つで実行する
```bash
# ターミナル2/3/4
cd data/aichallenge/2026/aichallenge-racingkart
bash docker_exec.sh
```

ROS2をコンテナのシェルに認識させる必要があるので、以下のコマンドをコンテナ内で打つようにしてください。

```bash
# ターミナル１
source /opt/ros/humble/setup.bash
source /autoware/install/setup.bash
source /aichallenge/workspace/install/setup.bash
```

## TinyLiDARNetの学習手順
### データ取得

AIチャレンジHPのままではDomain IDの不一致があります。
run_simulator.bash:44 - AWSIMは ROS_DOMAIN_ID=0 で起動。

run_autoware.bash:26 - Autowareは ROS_DOMAIN_ID=1（awsim 1の引数）で起動
Domain 1に送信しているが、AWSIMはDomain 0を見ているので届かない。
さらにsimulatorのモード問題:
./run_simulator.bashを引数なしで起動するとeval→1pモード（start_mode=sync）になる。データ収集ならdevモード（start_mode=off）が正しい。

#### Terminal 1: scan generation nodeの起動
```bash
export ROS_DOMAIN_ID=1
ros2 launch laserscan_generator laserscan_generator.launch.xml   use_sim_time:=true   csv_path:=$(ros2 pkg prefix laserscan_generator)/share/laserscan_generator/map/lane.csv
```

#### Terminal 2: AWSIMの起動
```bash
./run_simulator.bash dev  # ← devモードで起動
```

#### Terminal 3: autowareの起動
```bash
./run_autoware.bash awsim 1
```

#### Initial poseを設定してください。
Initial poseを指定する際は、RvizのviewをThirdPersonFollowerからTopdownOrthoに切り替える必要があります。


#### Terminal 4: rosbagの記録開始
```bash
export ROS_DOMAIN_ID=1
ros2 topic pub --once /awsim/control_mode_request_topic std_msgs/msg/Bool '{data: true}' # 車両の自動モードスタート
cd /aichallenge/ml_workspace;./record_data.bash
```

データ収集が終わったと自身で判断したのちにctrl+Cで走行を終える。
データ収集の目安：

run_simulator.bash dev（devモード）の場合、laps=600・timeout=60000000なので事実上無制限に走り続ける
機械学習用のデータとしては 最低1〜2周、推奨3〜5周以上 あると学習精度が上がる
コースを何周したかは/admin/awsim/stateトピックで確認できる（記録されているトピックの一つ）
記録完了の目安：

RVizでカートが同じコースを数周したのを確認してからCtrl+Cで停止でOK
1周あたり数十秒〜1分程度と思われる
記録終了後のコマンドも忘れずに：
```bash
cp -r aichallenge/ml_workspace/rawdata/20260329-173810 aichallenge/ml_workspace/train
cp -r aichallenge/ml_workspace/rawdata/20260329-173810 aichallenge/ml_workspace/val
```

### Step2. Dataset conversion

Terminal 5: rosbagを学習用datasetに変換します。
```bash
# ~/data/aichallenge/2026/aichallenge-racingkart/ から実行
python3 aichallenge/ml_workspace/tiny_lidar_net/extract_data_from_bag.py \
  --bags-dir aichallenge/ml_workspace/train/ \
  --outdir aichallenge/ml_workspace/tiny_lidar_net/dataset/train/
```

以下のような出力が得られたら成功です。
```bash
[INFO] [PID:99328] Found 1 bags. Starting processing with 1 workers.
[INFO] [PID:99356] Saved rosbag2_autoware: 413 samples (Total: 0.13s)
[INFO] [PID:99328] All processing finished in 0.34 seconds.
```

trainだけでなく、validation setも変換しておきましょう。
```bash
python3 aichallenge/ml_workspace/tiny_lidar_net/extract_data_from_bag.py \
  --bags-dir aichallenge/ml_workspace/val/ \
  --outdir aichallenge/ml_workspace/tiny_lidar_net/dataset/val/
```
以下のような出力が得られたら成功です。
```bash
[INFO] [PID:35775] Found 1 bags. Starting processing with 1 workers.
[INFO] [PID:35792] Saved 20260329-173810: 36732 samples (Total: 2.38s)
[INFO] [PID:35775] All processing finished in 2.83 seconds.
```

### Step3. Model training

学習epocは"aichallenge/ml_workspace/tiny_lidar_net/config/train.yaml"で定義。
early_stop_patience: 15 も設定されているため、valロスが15エポック改善しなければ100エポック到達前に自動停止する。

**Dockerコンテナ内**（`/aichallenge/ml_workspace`）で実行してください。`train.yaml` のパスがDocker内パスになっているため、ホストから実行するとパスエラーになります。

```bash
# GPU or CUDA対応
cd /aichallenge/ml_workspace
python3 tiny_lidar_net/train.py
```
```bash
# CPU or CUDA未対応
cd /aichallenge/ml_workspace
CUDA_VISIBLE_DEVICES="" python3 tiny_lidar_net/train.py
```

### Step4. Model deployment

.pthから.npyに変換します

#### ターミナル5
```bash
cd aichallenge/ml_workspace
python3 tiny_lidar_net/convert_weight.py \
  --ckpt tiny_lidar_net/checkpoints/best_model.pth \
  --output tiny_lidar_net/weights/converted_weights.npy \
  --input-dim 1080
```
※ `--input-dim` のデフォルトは750だが、今回のデータは1080点なので明示指定が必要。

以下のような出力が得られれば成功です。
```bash
✅ Loaded checkpoint: /aichallenge/ml_workspace/tiny_lidar_net/checkpoints/best_model.pth
✅ Saved NumPy weights to: weights/converted_weights.npy
```

作成したconverted_weights.npyを、ROS 2 package内のckptディレクトリに移動します。

#### ターミナル5
```bash
cp aichallenge/ml_workspace/tiny_lidar_net/weights/converted_weights.npy \
  aichallenge/workspace/src/aichallenge_submit/tiny_lidar_net_controller/ckpt/tinylidarnet_weights.npy
```
### Step5. Run TinyLiDARNet Sample ROS Node

reference.launch.xmlの `control_method` が `tiny_lidar_net` になっていることを確認してください（すでに変更済みのはず）。

eval mode（`./run_simulator.bash`）では autostart_orchestrator が localization 収束を待つため WAIT_START から進まないことがある。
データ収集と同じ **dev mode** で起動するのが確実。

#### Terminal 1: AWSIMの起動

```bash
./run_simulator.bash dev   # eval modeではなくdev modeで起動
```

#### Terminal 2: autowareの起動

```bash
./run_autoware.bash awsim 1
```

#### initial poseを設定してください。
（RVizをTopdownOrthoに切り替えてから、2D Pose Estimateをクリック＆ドラッグ）

#### Terminal 3: 走行開始
```bash
export ROS_DOMAIN_ID=1
ros2 topic pub --once /awsim/control_mode_request_topic std_msgs/msg/Bool '{data: true}' # 車両の自動モードスタート
```


### ----------------------------------------------------------------------
### ↑↑ここまではできた
### ----------------------------------------------------------------------

## ---------------------------------------------------------------
## TinyLiDARNet が学習する制御について

**MPC でも Pure Pursuit でもなく、エンドツーエンド模倣学習（Imitation Learning）です。**

### 全体の流れ

```
raceline_2026_mincurv.csv（経路＋速度定義）
  └─ simple_trajectory_generator が読み込む
       └─ /planning/scenario_planning/trajectory として発行
            └─ Autoware の Pure Pursuit / MPC がこの軌道を追従
                 └─ AckermannControlCommand（ステアリング＋アクセル）を生成
                      └─ rosbag に LiDAR スキャンと一緒に記録
                           └─ TinyLiDARNet が模倣学習（Behavioral Cloning）
                                └─ 推論時: LiDAR → [NN] → AckermannControlCommand を直接車両に送信
```

つまり、**`raceline_2026_mincurv.csv` で定義した経路と速度をAutowareが実現したときの制御コマンドを、LiDAR だけで再現するように学習**します。

- CSVの経路を変える → 違うラインの走り方を学習
- CSVの速度を上げる → より速い走り方を学習
- NNはCSVを直接読まず、**結果としての制御コマンドだけ** を LiDAR から再現するように学習する

### ニューラルネットワークの入出力

| | 内容 |
|---|---|
| **入力** | LiDARスキャン（750〜1080点、0〜30m正規化） |
| **出力** | `[acceleration, steering_angle]`（各 -1.0〜1.0） |

### 制御モード（`control_mode` パラメータ）

| モード | 内容 |
|---|---|
| `"ai"` | NNが加速度・操舵角の**両方**を出力 |
| `"fixed"` | NNは**操舵角のみ**出力、加速度は固定値（デフォルト0.3）|

### 関連ファイル

| 役割 | ファイルパス | 内容 |
|---|---|---|
| **走行軌道（経路＋速度）** | `workspace/src/aichallenge_submit/simple_trajectory_generator/data/raceline_2026_mincurv.csv` | x, y, z, 姿勢quaternion, speed。Autowareのplanningが直接参照。**ここを編集することで学習する走り方が変わる。** |
| **仮想LiDAR生成用レーン境界** | `workspace/src/aichallenge_submit/laserscan_generator/map/lane.csv` | lanelet ID・左右境界点の座標。laserscan_generatorが仮想LiDARスキャン生成に使用。速度情報なし。 |
| **軌道エディタのベースファイル** | `workspace/src/aichallenge_submit/aichallenge-trajectory-editor/csv/raceline_base.csv` | 軌道エディタ（editor_tool_server）用のベース。Autowareのplanningには直接使われない。 |


## ---------------------------------------------------------------
## TinyLiDARNetの学習ロジック

【準備フェーズ】ユーザが調整する部分
  trajectory（raceline_awsim_15km.csv）を調整
  + Pure Pursuitパラメータを調整
  → Autowareがきれいに完走できるようになる

【データ収集フェーズ】
  そのAutowareの走りをrosbagに記録
  （LiDAR + ステアリング・アクセルのペア）

【学習フェーズ】
  NNが「このLiDARならこのステアリング・アクセル」を模倣学習

【推論フェーズ】
  trajectory もPure Pursuitも使わず、LiDARだけでAutowareの走りを再現
つまり品質の連鎖

trajectory品質 ─┐
                ├→ Autowareの走りの品質 → 教師データの品質 → NNの走りの品質
PurePursuitパラメータ品質 ─┘

Autowareがうまく走れなければ、NNもうまく走れない。 逆に言えば、Pure Pursuitのパラメータ調整でAutowareが安定したなら、その走りを教師データにすればNNも安定した走りを学習できます。

現実的な開発サイクル

1. trajectory + Pure Pursuitパラメータを調整してAutowareを安定完走させる  ← 今ここ
2. 安定走行データを十分に収集（3〜5周以上）
3. TinyLiDARNetで学習
4. 推論で走らせて評価
5. 必要なら1に戻る
Pure Pursuitのパラメータ調整は「教師の質を上げる」作業なので、学習前に十分やり込む価値があります。


## ここからトラシュー ---------------------------------------------------------------
### Sim.中にフリーズしてPC再起動した場合は･･･

# ロックファイルや残骸の確認・削除
sudo rm -rf ~/.ros/daemon/
sudo rm -f /tmp/.ros*
sudo rm -rf /tmp/fastrtps*
# 環境確認
echo $ROS_DOMAIN_ID
echo $RMW_IMPLEMENTATION
# デーモンなしで直接テスト
ros2 node list

# ホスト側で実行
sudo sysctl -w net.core.rmem_max=2147483647
sudo ip link set multicast on lo

