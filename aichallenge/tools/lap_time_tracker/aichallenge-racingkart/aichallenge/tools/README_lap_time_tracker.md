# lap_time_tracker — 走行結果自動転記ツール

`make eval` 後の1コマンドで、走行タイム・パラメータ・セクションタイムを Excel に自動追記する。
手動コピペ不要で、試行ごとの結果を時系列で蓄積・比較できる。

---

## 考え方

### なぜ作ったか

`make eval` を繰り返すと結果が `output/latest/d1/result-details.json` に上書きされる。
パラメータと結果を手動でスプレッドシートに転記するのは面倒で、転記ミスも起きやすい。

そこで **「走行が終わったら自動で xlsx に列追記する」** 仕組みを作った。
環境側の Makefile や Docker は更新で変わる可能性があるため、自作コードは独立したファイル群に分離し、エントリポイント（`post_run.bash`）を呼ぶだけで動くように設計した。

### 設計方針

```
make eval-1lap-and-record
    ├─ make eval-1lap  ─────────────────── シミュレーション起動（バックグラウンド）
    └─ watch_and_record.bash ──────────── バックグラウンドで監視開始
            │
            │  [起動直後]
            ├─ result-section.json を初期化
            │      {"status": "running", "section_times": {}, ...}
            │
            │  [監視ループ: 2秒ごとにポーリング]
            │
            ├─ result-details.json が更新された
            │      → 正常完走
            │      ├─ docker compose down
            │      ├─ calc_section_times.py  → result-section.json（セクションタイム上書き）
            │      └─ post_run.bash
            │              └─ lap_time_tracker.py
            │                      └─ output/lap_time_results.xlsx に列追記（実タイム）
            │
            └─ コンテナが全停止した（make down 手動実行 / タイムアウト）
                    → スタック・未完走
                    ├─ calc_section_times.py  → result-section.json（取得できた分のみ）
                    └─ post_run.bash --na-lap
                            └─ lap_time_tracker.py --na-lap
                                    └─ output/lap_time_results.xlsx に列追記（ラップタイムは n/a）
```

- **環境非依存**: `post_run.bash` だけが自作コードへのエントリポイント。  
  Makefile や docker-compose が更新されても、このファイルを差し込む行だけ変えれば動く。
- **起動時に result-section.json を作成**: シミュレーション開始と同時に初期ファイルを生成。走行中は `status: running` の状態を保持し、終了後にセクションタイムで上書きされる。
- **スタック時も転記**: コンテナが手動で落とされた場合も検知し、ラップタイムを `n/a` として xlsx に記録する。「このパラメータでは完走できなかった」という事実が残る。
- **セクションタイム自動計算**: rosbag から曲率ベースでコーナー位置を自動検出し、通過タイムを算出。走行開始後に最初に通過したゲートを A とするため、コースのどこからスタートしても一貫したラベリングになる。

---

## ファイル構成

| ファイル | 役割 |
|---|---|
| `post_run.bash` | エントリポイント。引数をそのまま `lap_time_tracker.py` へ転送 |
| `aichallenge/tools/lap_time_tracker.py` | xlsx 追記の本体 |
| `aichallenge/tools/watch_and_record.bash` | 走行完了またはコンテナ停止を検知してバックグラウンドで後処理を実行 |
| `aichallenge/tools/calc_section_times.py` | rosbag からセクション通過タイムを計算 |

---

## 使い方

### 走行 + 自動転記（1コマンド）

```bash
cd aichallenge-racingkart

make eval-and-record        # 6周 + 自動転記
make eval-1lap-and-record   # 1周 + 自動転記
make eval-2lap-and-record   # 2周 + 自動転記
make eval-3lap-and-record   # 3周 + 自動転記
```

シミュレーション終了後、自動で以下が実行される。
1. コンテナ停止
2. セクションタイム計算
3. xlsx 追記

プロンプトは即座に返る。バックグラウンドのログは `output/watch_and_record.log` に記録される。

### 走行と転記を分けて実行

```bash
make eval-1lap          # 走行のみ
./post_run.bash         # 転記のみ（走行後に手動実行）

./post_run.bash --label "Test_001"   # 列ラベルを指定
./post_run.bash --na-lap             # ラップタイムを n/a で転記（スタック時に手動実行する場合）
```

### スタックして make down した場合

```bash
make eval-1lap-and-record   # バックグラウンド監視が走る
# → 途中でスタック
make down                   # 手動で停止

# → コンテナ停止を自動検知し、xlsx に転記される（ラップタイムは n/a）
```

---

## 出力フォーマット（xlsx）

ファイル: `output/lap_time_results.xlsx`

| 列 | 内容 |
|---|---|
| A | ファイル名ラベル（`config.yaml` / `reference.launch.xml`） |
| B | グループラベル（control / pp / mpc / lqr / map など） |
| C | パラメータ名（`Parameter` ヘッダー行、および各行のラベル） |
| D〜 | 各走行の値（右に追記されていく） |

行構成（C列のラベルで決まる）：

| C列ラベル | A列 | 内容 |
|---|---|---|
| `Parameter` | `file` | 列タイトル（実行日時 or 任意ラベル） |
| `trajectory_csv` | `reference.launch.xml` | 使用した軌道ファイル名 |
| 各パラメータ名 | `reference.launch.xml` | `reference.launch.xml` から読み出した値 |
| `mpc.*` 形式のキー | `config.yaml` | `config.yaml` から読み出した値（ドット区切りフラットキー） |
| `total_laps` 〜 `best_lap_time` | — | ラップ集計 |
| `lap_1_time` 〜 `lap_6_time` | — | 各周ラップタイム |
| `section_A_time` 〜 | — | セクションタイム |

### パラメータ書き込みの仕組み

`lap_time_tracker.py` は **xlsx の C列を上から走査し、書かれているラベルをキーとして参照元ファイルから値を取得して書き込む**。パラメータ名はコードに一切定義していない。

| C列ラベル | 動作 |
|---|---|
| `trajectory_csv` | `reference.launch.xml` の `csv_path` のファイル名 |
| `total_laps` / `total_time` / `best_lap_time` | `result-details.json` から取得 |
| `lap_1_time` 〜 `lap_6_time` | `result-details.json` から取得（出現順に割り当て） |
| `section_*_time` | `result-section.json` から取得。存在しない section は末尾に自動追加 |
| その他のラベル | A列が `config.yaml` → `config.yaml` から取得（ドット区切りキー、例: `mpc.v_max`）<br>A列が空または `reference.launch.xml` → `reference.launch.xml` から取得<br>A列が空でもキーが `config.yaml` にあれば自動的に `config.yaml` を参照 |

- 参照元ファイルに該当キーが存在しない場合は `null` と記載
- `csv_path_accel_map` / `csv_path_brake_map` はファイル名のみ記載（フルパスは省略）
- リスト値（例: `Q`, `R`）は文字列として記載（例: `[1000000.0, 100000000.0, 850000.0]`）
- **記録対象の追加・削除は xlsx の C列を直接編集するだけでよい。コードの変更は不要**
- 新規 xlsx 作成時はヘッダー行（1行目）のみ生成。行ラベルは xlsx 側で管理する

- 列は右に追記されていく（既存データを上書きしない）
- セクションラベルは走行順に A, B, C, ... と自動付番（コース上の位置順ではなく通過順）
- D列以降の列幅は 3cm 固定

---

## アピールポイント

### 1. 「make eval → xlsx 追記」をコマンド1つで完結

`make eval-1lap-and-record` を叩けば、走行→停止→セクションタイム計算→xlsx追記までが全自動。
試行ごとに手動でコピペする必要がなく、転記ミスが起きない。

### 2. スタック時も記録が残る

車両がスタックして `make down` した場合でも、コンテナ停止を検知して xlsx に転記する。
「このパラメータでは完走できなかった」という事実をラップタイム n/a として残せる。

### 3. パラメータと結果が同じ行に並ぶ

`reference.launch.xml` から MPC・LQR パラメータを自動読み出して同じ列に記録するため、
「どのパラメータでどのタイムが出たか」を xlsx 上で一覧比較できる。

### 4. セクションタイムで弱点を特定できる

rosbag から曲率ベースでコーナー位置を自動検出し、セクションごとの通過タイムを計算する。
「コーナーAは速いがコーナーCが遅い」といった区間別の分析ができ、パラメータ調整の優先度を絞れる。

### 5. 環境更新に強い設計

自作コードは `post_run.bash` → `aichallenge/tools/` に完全分離されており、
環境側の Makefile や docker-compose が更新されても影響を受けない。
エントリポイント（`post_run.bash` を呼ぶ1行）だけ Makefile に残せばよい。

### 6. Optuna 自動最適化と連携

`make optimize-lqr` による Optuna パラメータ探索の結果も、同じ xlsx フォーマットで確認できる。
探索→記録→比較のサイクルを統一したワークフローで回せる。

---

## セクションタイムの仕組み

`calc_section_times.py` は `visualize_sections.py` と同じロジックでゲート位置を計算する。

1. 軌道 CSV から曲率を計算し、曲率ピーク（コーナー出口）をゲートとして自動検出
2. rosbag の `/localization/kinematic_state` から車両位置の時系列を取得
3. 各ゲートの法線ベクトルに対する符号付き距離を監視し、符号反転（負→正）で通過判定
4. 通過時刻でソートし、最初に通過したゲートを A、次を B … と再ラベリング
5. 連続するゲート間の時間差をセクションタイムとして出力

`result-section.json` はシミュレーション開始時に作成され、終了後にセクションタイムで上書きされる。

```json
// シミュレーション開始直後（watch_and_record.bash が作成）
{
    "status": "running",
    "section_times": {},
    "gate_pass_elapsed": {}
}

// 正常完走後（calc_section_times.py が上書き）
{
    "section_times": {"A": 12.3, "B": 14.5, ...},
    "gate_pass_elapsed": {"A": 0.0, "B": 12.3, ...}
}
```

転記完了後は `post_run.bash` がファイルを削除する（次回走行で前回値が混入するのを防ぐ）。

---

## セットアップ

```bash
pip3 install -r requirements_lap_time_tracker.txt
```

依存パッケージ（`requirements_lap_time_tracker.txt`）:

| パッケージ | 用途 |
|---|---|
| `openpyxl` | xlsx 読み書き（`lap_time_tracker.py`） |
| `numpy` | 曲率計算（`calc_section_times.py`） |
| `scipy` | ピーク検出（`calc_section_times.py`） |
| `rosbags` | rosbag 読み込み（`calc_section_times.py`） |

---

## トラブルシューティング

| 症状 | 原因 | 対処 |
|---|---|---|
| `ModuleNotFoundError: ...` | pip 未インストール | `pip3 install -r requirements.txt` |
| `result-details.json が見つかりません` | eval が未完了 | `make eval` を先に完走させる |
| セクションタイムが空欄 | rosbag が見つからない or 不完全 | eval 完了後に実行、または `--bag` で指定 |
| xlsx に書き込めない | ファイルが Excel で開かれている | Excel を閉じてから実行 |
| バックグラウンド処理の状況確認 | — | `tail -f output/watch_and_record.log` |
