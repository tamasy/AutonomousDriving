# AutonomousDriving
自動運転の学習に関するPrj.です。

## ✅ よく使うコマンド
### 🎯 AIチャレンジコード実行 2025
#### 大会用リポジトリのクローン
```bash
cd (任意のフォルダ)
cd dgit        # sample
git clone https://github.com/AutomotiveAIChallenge/aichallenge-2025.git
```

#### Dockerコンテナのビルド＆起動初回
```bash
# 大会用リポジトリに入ります。
cd data/aichallenge/2026/aichallenge-2025

# Dockerイメージのビルドを行います。
./docker_build.sh dev
docker images

# Dockerコンテナを立ち上げます。
./docker_run.sh dev gpu     # GPU有
./docker_run.sh dev cpu     # GPUなし
ls

# Dockerコンテナ内で以下を実行してAutowareをビルドします。
cd /aichallenge
./build_autoware.bash
```
#### Dockerコンテナ上でのAutowareとSimulatorの実行
```bash
./run_evaluation.bash
```
#### 提出方法
```bash
cd data/aichallenge-2025_shio/aichallenge-2025
./create_submit_file.bash
```

### 🎯 AIチャレンジコード実行 2026

セットアップ手順は以下参照 <br>
https://automotiveaichallenge.github.io/aichallenge-documentation-racingkart/setup/introduction.html

#### Autoware/ROS 2 overlay をビルド
```bash
cd data/aichallenge/2026/aichallenge-racingkart
make autoware-build
```

#### AWSIM + Autoware を開発モードで起動
```bash
make dev
```

#### コンテナ停止
```bash
make down
```

### 🎯 関連ツール
#### trajectoryのポイントを追加・削除したい
```bash
# standalone-trajectory-editor
cd
cd data/aichallenge/2026/standalone-trajectory-editor/build
./trajectory_editor
```
#### 軌道最適化ツール
```bash
# global_racetrajectory_optimization
cd data/aichallenge/2026/global_racetrajectory_optimization
python3 main_globaltraj.py
```

### 🎯 関連ツール 自作
#### クォータニオンを再計算したい
```bash
# 以下フォルダへpythonファイルを格納する
# 2025環境はこちら
cd aichallenge/workspace/src/aichallenge_submit/simple_trajectory_generator/data
# 2026環境はこちら
cd data/aichallenge/2026/aichallenge-racingkart/aichallenge/tools

# 再計算したいcsvを指定する
python3 update_quaternions.py raceline_2026_mincurv6_ReQx.csv
```

#### PurePursuit -> MPC 変換ツール
```bash
cd data/aichallenge/2026/aichallenge-racingkart/aichallenge/memo
python3 pp2mpc.py
python3 mpc2pp.py
```

#### パラスタ値＆最新走行結果自動転機
```bash
cd data/aichallenge-2025
python3 lap_time_tracker_v2.py
```

### 🎯 Optuna Pure Pursuit パラメータ自動調整
```bash
# Docker起動してから以下実行
cd /aichallenge/workspace/src/aichallenge_submit/aichallenge_submit_launch/scripts

# 新規実行
python3 optimize_pure_pursuit.py

# 途中から再開
python3 optimize_pure_pursuit.py --resume

# デフォルト値に戻す
python3 optimize_pure_pursuit.py --restore-defaults

```

### 🎯 Optuna Pure Pursuit ダッシュボードで確認
```bash
# Docker起動してから以下実行
cd /aichallenge/workspace
pip3 install optuna-dashboard
optuna-dashboard sqlite:///pure_pursuit_study.db

```

### 🎯 git
#### gitユーザ登録
```bash
git config --global user.email "you@example.com"
git config --global user.name "Your Name"
```
#### コミット＆プッシュ
```bash
cd dgit                             # リポジトリのディレクトリに移動
git add .                           # 変更をステージング
git commit -m "commit message"      # 変更をコミット
git push origin main                # リモートリポジトリにプッシュ
git push origin master              # エラーが出るときはこちらで試してみよう
```

#### プル
```bash
cd dgit                             # リポジトリのディレクトリに移動
git pull origin main                # リモートリポジトリから変更をプル:
```

#### マージする @ ブランチを作成後
```bash
# ①新しいブランチを作成後に手動で切り替える
# ブランチを作成する              :例.feature-branch という名前のブランチを作成する
git branch feature-branch
# 作成したブランチに切り替える      :例.上記の例で作成した feature-branch に切り替える
git checkout feature-branch
# ②新しいブランチを作成しつつ切り替える
git checkout -b feature-branch
```

#### マージする @ mainブランチに.
```bash
# developブランチからmainブランチへマージしながら戻る
# developブランチでの作業を保存し、最新の状態にしておく。
git add .
git commit -m "Save current work on develop"
# mainブランチに戻る    ※ main ブランチにいる場合はこのステップは不要.
git checkout main
## developブランチをmainブランチにマージする。
git merge develop
# マージ結果を確認し、必要に応じてコンフリクトを解決する。
# コンフリクトが発生した場合は、ファイルを修正し、次のコマンドでコンフリクト解決を完了。
git add <resolved_file>
git commit -m "Resolve merge conflict"
# マージ結果をリモートリポジトリにプッシュする。
git push origin main
```

#### コンフリクトの解決
##### もしマージ中にコンフリクトが発生した場合、手動でコンフリクトを解決し、変更をコミットする必要があります。
```bash
# 解決後に以下のコマンドを実行します。
git add <conflicted-file>
# マージをリモートリポジトリにプッシュする:
git commit -m "Merge branch 'feature-branch' into main"
git push origin main
```
#### ブランチの削除
```bash
# 削除したいローカルブランチを確認して削除します。
git branch -d <branch_name>
```




### 🎯 Python
#### 仮想環境
```bash
source ~/data/myenv2/bin/activate
```
------------------------------------------------------------------
## ■ 環境構築
https://automotiveaichallenge.github.io/aichallenge-documentation-2025/setup/requirements.html