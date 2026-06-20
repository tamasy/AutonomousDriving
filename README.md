# 自動運転AIチャレンジ2026 活動履歴

## ✅ 目次
- 概要
- 想定環境
- よく使うコマンド：AIチャレンジ コード実行
- よく使うコマンド：関連ツール OSS/自作
- よく使うコマンド：oputuna
- よく使うコマンド：git操作


## ✅ 概要
本ドキュメントは、Autowareを使った自動運転競技である <strong><span style="font-size:1.4em">自動運転AIチャレンジ2026</span></strong> の開発環境構築やノウハウをまとめたものです。

以下を目標に、自動運転AIチャレンジへの取り組みを通じての実体験をもとに随時アップデートしています。
- シミュレータ環境構築における本家ドキュメントの補足解説
- チームメンバ／ユーザー向けシミュレータ環境の整備・支援
- Autowareのパラメータ調整によるチャレンジコースの完走
- 勝敗にとどまらない技術力向上とチーム間の相互支援

## ✅ 想定環境
- OS: Linux / Ubuntu 22.04 LTS
- NVIDIA GPUなし
- RAM: 64 GB

## ✅ 環境構築
セットアップは以下に従えば基本的にはできる。<br>

https://automotiveaichallenge.github.io/aichallenge-documentation-racingkart/setup/introduction.html


コマンド1つで、環境からシミュレーターの実行テストまでを一気に実行できる。<br>
対話形式で順に実施するため必要な項目だけ Yes とすればよいが迷ったら全部 Yes とすればOK。

```bash
curl -fsSL "https://raw.githubusercontent.com/AutomotiveAIChallenge/aichallenge-racingkart/main/setup.bash" | bash
```

### 💡 環境構築時のポイント

上記手順で環境構築して作成された aichallenge-racingkart は大会環境のコピーでありアップデートする毎に更新が入る。更新を横目に取り込みは自身で判断したうえで独自で開発したい方は、自身でディレクトリを作成して、ディレクトリをコピーしたうえで、その中で開発することを推奨する。
(マージ等による変化点管理をできる方は不要、、、)

```bash
mkdir (自身の aichallenge-racingkart を実行するディレクトリ. 例 mkdir 2026)
cp -r aichallenge-racingkart 2026/aichallenge-racingkart
cd 2026
```

## ✅ よく使うコマンド
インストール後の開発は以下いずれかが頻出。

### 🎯 AIチャレンジコード実行 2026

#### Autoware/ROS 2 overlay をビルド
```bash
cd aichallenge-racingkart
make autoware-build
```

#### AWSIM + Autoware を開発モードで起動 (タイム計測なし && 手動で止めるまで動き続ける)
```bash
# ディレクトリ aichallenge-racingkart にて実行
make dev
# or 
make autoware-build && make dev
```

#### AWSIM + Autoware を本番モードで起動 (タイム計測有)
```bash
# ディレクトリ aichallenge-racingkart にて実行
make eval       # 6周
# 環境移植手順_lap_time_tracker.md を実行済前提
make eval-1lap  # 1周
make eval-2lap
make eval-3lap
```

#### 走行 + 結果自動転記 (eval完了後に post_run.bash を自動実行)
```bash
# ディレクトリ aichallenge-racingkart にて実行
# 環境移植手順_lap_time_tracker.md を実行済前提
make eval-and-record        # 6周 + 転記
make eval-1lap-and-record   # 1周 + 転記
make eval-2lap-and-record   # 2周 + 転記
make eval-3lap-and-record   # 3周 + 転記
```

#### コンテナ停止
```bash
# ディレクトリ aichallenge-racingkart にて実行
make down
```

#### 提出方法
```bash
# ディレクトリ aichallenge-racingkart にて実行
./create_submit_file.bash
./create_submit_file_kai.bash
```

#### ビルド成果物のリセット
```bash
cd workspace
rm -rf build/ install/ log/
cd ..
```

#### スロットルのパブリッシュ値をモニタする
```bash
# ターミナル1# ディレクトリ aichallenge-racingkart にて実行
bash /docker_exec.sh

# ターミナル2
docker exec -it aichallenge-racingkart-autoware-simulator-evaluation-1 bash -c \
  "source /aichallenge/workspace/install/setup.bash && \
   ros2 topic echo /control/command/actuation_cmd --field actuation.accel_cmd"

# ターミナル3 : 速度も同時に見たい場合
docker exec -it aichallenge-racingkart-autoware-simulator-evaluation-1 bash -c \
  "source /aichallenge/workspace/install/setup.bash && \
   ros2 topic echo /vehicle/status/velocity_status --field longitudinal_velocity"
# docker exec -it <コンテナ名> bash -c "source ... && ros2 topic echo ..." の形式なら、コンテナに入らずホスト側のターミナルから直接モニタできます。
```




## ✅ 関連ツール
### 🎯 関連ツール
#### trajectoryのポイントを追加・削除したい
```bash
# standalone-trajectory-editor
# ディレクトリ aichallenge-racingkart にて実行
cd aichallenge-racingkart/tools/standalone-trajectory-editor/build
./trajectory_editor
```
#### 軌道最適化ツール
```bash
# global_racetrajectory_optimization
# ディレクトリ aichallenge-racingkart にて実行
cd aichallenge-racingkart/tools/global_racetrajectory_optimization
python3 main_globaltraj.py
```
#### aichallenge-trajectory-editor

以下実行すれば2026環境でもいけた。ただしgitは導入済みのためRVitz設定のみでOK。
https://zenn.dev/iasl/articles/f7a13f7cc8f146


### 🎯 関連ツール 自作
#### クォータニオンを再計算したい
```bash
# ディレクトリ aichallenge-racingkart にて実行
cd aichallenge-racingkart/tools/

# 再計算したいcsvを指定する. simple_trajectory_generator にあるcsvを参照する.
python3 update_quaternions.py raceline_traj_mincurv_35kph.csv
```

#### PurePursuit <--> MPC 変換ツール
```bash
# ディレクトリ aichallenge-racingkart にて実行
cd tools/
python3 mpc2pp.py <input>            # 出力ファイル名を自動生成
python3 mpc2pp.py <input> <output>   # 出力ファイル名末尾に _pp  と明示

python3 pp2mpc.py <input>            # 出力ファイル名を自動生成
python3 pp2mpc.py <input> <output>   # 出力ファイル名末尾に _mpc と明示
```

#### パラスタ値＆最新走行結果自動転機 (2026版)
```bash
# ディレクトリ aichallenge-racingkart にて実行
# make eval の直後にこの1コマンドだけ実行する
./post_run.bash

# 列ラベルを指定したい場合
./post_run.bash --label "MyTest_001"

# セクションタイム付き (result-section.json がある場合は自動読み込み)
./post_run.bash --section-times '{"A": 12.3, "B": 14.5}'

# 出力先: output/lap_time_results.xlsx (自動追記)
# 実体: aichallenge/tools/lap_time_tracker.py へ委譲
```


## ✅ よく使うコマンド：git
#### gitユーザ登録
```bash
git config --global user.email "you@example.com"
git config --global user.name "Your Name"
```
#### コミット＆プッシュ
```bash
cd (任意のディレクトリ)                # リポジトリのディレクトリに移動
git add .                           # 変更をステージング
git commit -m "commit message"      # 変更をコミット
git push origin main                # リモートリポジトリにプッシュ
```

#### プル
```bash
cd (任意のディレクトリ)                # リポジトリのディレクトリに移動
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