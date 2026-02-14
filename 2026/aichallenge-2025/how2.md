# 自動運転AIチャレンジ活動履歴
## 工夫ポイント
・速度と舵角、前方距離などのパラスたをAIと実施
・AIの蛇行＆膨らみ傾向を先読みした軌跡設計
　→ぶつからない車
　→設定した目線と自車の間に壁がないこと
・パラメータの自動適合
------------------------------------------------------------------
## ■ よく使うコマンド
#gitユーザ登録
  git config --global user.email "you@example.com"
  git config --global user.name "Your Name"

#大会用リポジトリのクローン
cd (任意のフォルダ)
git clone https://github.com/AutomotiveAIChallenge/aichallenge-2025.git

#大会用リポジトリに入ります。 <br>
cd data/aichallenge-2025_shio_way/aichallenge-2025
cd data/aichallenge-2025_shio/aichallenge-2025

#Dockerイメージのビルドを行います。 <br>
./docker_build.sh dev
docker images

#Dockerコンテナを立ち上げます。 <br>
./docker_run.sh dev cpu
ls

#Dockerコンテナ内で以下を実行してAutowareをビルドします。 <br>
cd /aichallenge
./build_autoware.bash

#Dockerコンテナ上でのAutowareとSimulatorの実行 <br>
./run_evaluation.bash

# aichallenge-trajectory-editor
./run_simulator.bash awsim
./run_autoware.bash awsim

# 提出方法
cd data/aichallenge-2025_shio/aichallenge-2025
./create_submit_file.bash

# 仮想環境
source ~/data/myenv2/bin/activate
# パラスタ値＆最新走行結果自動転機
cd data/aichallenge-2025_shio
python3 lap_time_tracker_v2.py
------------------------------------------------------------------
## ■ 環境構築
https://automotiveaichallenge.github.io/aichallenge-documentation-2025/setup/requirements.html
------------------------------------------------------------------
## ■ 車速制御
現在車速と目標加速度を用いて所望のペダル踏み込み量に変換する際、AutowareではFF制御を用いています。

FF制御とは。
システムの入力に基づいて制御動作を行う制御方式。 <BR>
方式は、予測可能な外乱やシステムの変化に対して、システムの出力が目的の値から逸脱しないように事前に制御を行う。<BR>
FF制御は、FB制御（入力と出力の差に基づく制御）とは異なり、システムが外乱に反応する前に制御動作を開始することが特徴。<BR>
今回のAI Challengeでは制御入力がペダルの入力(0~100%)とブレーキ油圧(0~6000kPa)になっている。<BR>
使用車両特性は以下のページに記載。<BR>
https://autonomalabs.github.io/AWSIM/RacingSim/VehicleDynamics/

### accel_map / brake_mapとは
ワークスペースの/src/aichallenge_submit/aichallenge_submit_launch/data/にaccel_map.csv brake_map.csvというファイルがある。<br>

1行目は速度[m/s]を、1列目はアクセル開度を、表内の各数値は目標加速度を表している。<br>
　→速度からのアクセルペダル変換器の出力は0~1にしておき、車両I/Fのほうで整数倍して0~100にしている
　→現在の速度とsimple_pure_pursuitなどから算出された目標加速度から、車両I/Fで目標アクセル量を参照する。<br>
　→ブレーキ無し走行するには、brake_map.csv の1列目をALL=0とすればよい。

https://autonomalabs.github.io/AWSIM/RacingSim/VehicleDynamics/

#### accel_map.csv
    default, 0.0    ,2.0    ,4.0    ,6.0    ,8.0    ,10.0   // m/s
        0.0, -0.3   ,-0.3   ,-0.3   ,-0.3   ,-0.3   ,-0.3   // m/s2
        0.1, 0.0    ,0.0    ,0.0    ,0.0    ,0.0    ,0.0
        0.2, 0.01   ,0.01   ,0.01   ,0.01   ,0.01   ,0.01
        0.3, 0.05   ,0.05   ,0.05   ,0.05   ,0.05   ,0.05
        0.4, 0.1    ,0.1    ,0.1    ,0.1    ,0.1    ,0.1
        0.5, 0.175  ,0.175  ,0.175  ,0.175  ,0.175  ,0.175
        0.6, 0.25   ,0.25   ,0.25   ,0.25   ,0.25   ,0.25 
        0.8, 0.7    ,0.7    ,0.7    ,0.7    ,0.7    ,0.7 
        1.0, 1.0    ,1.0    ,1.0    ,1.0    ,1.0    ,1.0 

#### brake_map.csv
    default,  0.0,  2.0,  4.0,  6.0,  8.0,  10.0
        0.0, -0.3, -0.3, -0.3, -0.3, -0.3, -0.3
        0.1, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5
        0.2, -0.7, -0.7, -0.7, -0.7, -0.7, -0.7
        0.3, -0.9, -0.9, -0.9, -0.9, -0.9, -0.9
        0.4, -1.1, -1.1, -1.1, -1.1, -1.1, -1.1
        0.5, -1.3, -1.3, -1.3, -1.3, -1.3, -1.3
        0.6, -1.5, -1.5, -1.5, -1.5, -1.5, -1.5
        1.0, -2.5, -2.5, -2.5, -2.5, -2.5, -2.5

## トラシュー
○ "Waiting for /awsim/control_cmd topic to be available..."が出たとき
　うまくいくときは本行の代わりに「System is ready, executing publish commands...」と出る
　＝エラー時は初期状態にセットできていないと考える
　　→　GNSS初期位置までカートが移動できておらず初期セットアップできていないと推定
　　　→　accl/brake操作いずれかの加速度が過大である or 設定に誤りがあり、topicを送信できていない
　･･･というのは仮設で、変更箇所に不備があった。'を消した、半角が全角になってた、など。diffffで差分見よう。


## lanelet2_map.osmファイルについて
  現在開いているlanelet2_map.osmファイルは道路レーン情報の定義で、走行軌跡waypointとは別もの。
  Lanelet2形式の地図データ。
  道路構造を定義しているが実際の走行経路はCSVファイルで制御している。

## 関連サイト
環境構築 <br> 
https://zenn.dev/iasl/articles/da5a3ea8442fde

AIChallenge入門ガイド(2)  <br> 
https://zenn.dev/ttanaka3/articles/4c1b6dea256ea5

accel/brake map編  <br> 
https://zenn.dev/iasl/articles/2f94613abe1ccb

Tier4 Vector Map Bilder <br> 
https://tools.tier4.jp/feature/vector_map_builder_ll2/

自作AIモデルで車両を制御する（データ収集編） <br> 
https://zenn.dev/bushio_tech/articles/ca80640216138d

障害物を回避する経路を生成する <br> 
https://blog.jbs.co.jp/entry/2024/08/28/111046  

コース最適化AI
https://zenn.dev/tamago117/articles/b021d2fcb875cc

## コースの走行軌跡waypointを編集するには？？
  主要な編集対象ファイル
  1. レースライン（Waypoint）データファイル
  /home/shiokawa/data/aichallenge-2025_shio/aichallenge-2025/aichallenge/workspace/s
  rc/aichallenge_submit/simple_trajectory_generator/data/
  ├── raceline_awsim_15km.csv
  ├── raceline_awsim_25km.csv
  └── raceline_awsim_30km.csv

  これらのCSVファイルが実際のwaypoint座標データです。各ファイルは異なる速度設定に対
  応しています。

  2. 軌道生成ノードの設定

  /home/shiokawa/data/aichallenge-2025_shio/aichallenge-2025/aichallenge/workspace/s
  rc/aichallenge_submit/aichallenge_submit_launch/launch/reference.launch.xml

  120-123行目：
  <node pkg="simple_trajectory_generator" exec="simple_trajectory_generator_node" 
  name="simple_trajectory_generator" output="screen">
      <param name="csv_path" value="$(find-pkg-share 
  simple_trajectory_generator)/data/raceline_awsim_25km.csv"/>
      <param name="z" value="6.5"/>
  </node>

  CSVファイルの形式

● Read(aichallenge-2025/aichallenge/workspace/src/aichallenge_submit/simple_trajecto
      ry_generator/data/raceline_awsim_25km.csv)
  ⎿  Read 10 lines (ctrl+r to expand)

● CSVファイルの形式は以下の通りです：
  x,y,z,x_quat,y_quat,z_quat,w_quat,velocity

  - x, y: waypoint座標（AWSIM座標系）
  - z: 高度（通常0.0、launch.xmlで6.5に設定される）
  - x_quat, y_quat, z_quat, w_quat: クォータニオン（車両の姿勢）
  - velocity: 目標速度（最後の列）

  編集方法

  1. 既存軌跡の修正: 上記CSVファイルを直接編集
  2. 新しい軌跡の追加: 新しいCSVファイルを作成し、launch.xmlで参照パスを変更
  3. 速度の調整: CSVの最後の列（velocity）を変更


