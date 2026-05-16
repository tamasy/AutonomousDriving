# 自動転記機能 移植手順

`make eval` 後に `./post_run.bash` 1コマンドで走行結果を xlsx へ転記する機能の移植手順。

---

## 移植が必要なファイル（5つ）

| ファイル | リポジトリ内パス | 役割 |
|------|------|------|
| `post_run.bash` | `aichallenge-racingkart/post_run.bash` | エントリポイント（1コマンド実行用） |
| `lap_time_tracker.py` | `aichallenge-racingkart/aichallenge/tools/lap_time_tracker.py` | xlsx 転記の本体 |
| `watch_and_record.bash` | `aichallenge-racingkart/aichallenge/tools/watch_and_record.bash` | 走行完了を検知して自動停止・転記 |
| `calc_section_times.py` | `aichallenge-racingkart/aichallenge/tools/calc_section_times.py` | rosbag からセクション通過タイムを計算 |
| `Makefile` の追記部分 | `aichallenge-racingkart/Makefile` | `eval-and-record` 系ターゲット |

> **Makefile は丸ごとコピーしない**。環境更新で変わる可能性があるため、  
> 追記部分（後述）だけを手動で貼り付ける。

---

## 手順

### 1. ファイルを用意する

GitHub からクローン後、以下のファイルが存在することを確認する。

```
aichallenge-racingkart/
├── post_run.bash
└── aichallenge/
    └── tools/
        ├── lap_time_tracker.py
        └── watch_and_record.bash
```

確認後、実行権限を付与する。

```bash
chmod +x post_run.bash aichallenge/tools/watch_and_record.bash
```

### 2. Python ライブラリをインストール

`openpyxl`（xlsx転記）と `rosbags`（rosbag読み込み）が必要。

```bash
pip3 install openpyxl rosbags
```

### 3. Makefile に追記

`aichallenge-racingkart/Makefile` を開き、以下を追記する。

> ⚠️ Makefile のインデントは **タブ文字**（スペース不可）。コピペ後に確認すること。

#### 3-1. eval-1lap / eval-2lap / eval-3lap の追加

追記場所: `eval:` ターゲットの直後。

**① `.PHONY` 行に追加**（既存の `.PHONY` 行を編集）:

```makefile
eval-1lap eval-2lap eval-3lap \
```

**② ターゲット本体を追加**:

```makefile
eval-1lap: section-visualizer
	@echo "Start 1-lap evaluation simulation (AWSIM + Autoware + SectionVisualizer, DOMAIN_ID=$(DOMAIN_ID))"
	LAPS=1 EVAL_TIMEOUT=180 docker compose up -d autoware-simulator-evaluation
	@echo "To stop: make down  (docker compose down --remove-orphans)"

eval-2lap: section-visualizer
	@echo "Start 2-lap evaluation simulation (AWSIM + Autoware + SectionVisualizer, DOMAIN_ID=$(DOMAIN_ID))"
	LAPS=2 EVAL_TIMEOUT=360 docker compose up -d autoware-simulator-evaluation
	@echo "To stop: make down  (docker compose down --remove-orphans)"

eval-3lap: section-visualizer
	@echo "Start 3-lap evaluation simulation (AWSIM + Autoware + SectionVisualizer, DOMAIN_ID=$(DOMAIN_ID))"
	LAPS=3 EVAL_TIMEOUT=540 docker compose up -d autoware-simulator-evaluation
	@echo "To stop: make down  (docker compose down --remove-orphans)"
```

#### 3-2. eval-and-record 系の追加

追記場所: 3-1 で追加した `eval-3lap:` ターゲットの直後（`# remote operation` の前）。

**① `.PHONY` 行に追加**（既存の `.PHONY` 行を編集）:

```makefile
eval-and-record eval-1lap-and-record eval-2lap-and-record eval-3lap-and-record \
```

**② ターゲット本体を追加**:

```makefile
# eval + バックグラウンド監視 + post_run.bash (結果自動転記)
# result-details.json の更新を検知して自動で停止 + xlsx 転記。
eval-and-record:
	docker compose down --remove-orphans
	$(MAKE) eval
	@aichallenge/tools/watch_and_record.bash &
	@echo "[post_run] 走行終了後に自動で停止 + xlsx 転記します (バックグラウンド監視中)"

eval-1lap-and-record:
	docker compose down --remove-orphans
	$(MAKE) eval-1lap
	@aichallenge/tools/watch_and_record.bash &
	@echo "[post_run] 走行終了後に自動で停止 + xlsx 転記します (バックグラウンド監視中)"

eval-2lap-and-record:
	docker compose down --remove-orphans
	$(MAKE) eval-2lap
	@aichallenge/tools/watch_and_record.bash &
	@echo "[post_run] 走行終了後に自動で停止 + xlsx 転記します (バックグラウンド監視中)"

eval-3lap-and-record:
	docker compose down --remove-orphans
	$(MAKE) eval-3lap
	@aichallenge/tools/watch_and_record.bash &
	@echo "[post_run] 走行終了後に自動で停止 + xlsx 転記します (バックグラウンド監視中)"
```

---

## 動作確認

```bash
cd aichallenge-racingkart

# post_run.bash 単体テスト（eval後に output/latest が存在する状態で）
./post_run.bash --label "test_001"

# 正常なら output/lap_time_results.xlsx に列が追加される
```

---

## 使い方（移植後）

```bash
# 走行 + 自動転記（1コマンド）
make eval-and-record
make eval-1lap-and-record
make eval-2lap-and-record
make eval-3lap-and-record

# 走行と転記を分けて実行したい場合
make eval-1lap
./post_run.bash --label "任意のメモ"

# 結果は output/lap_time_results.xlsx に自動追記される
```

---

## トラブルシューティング

| 症状 | 原因 | 対処 |
|------|------|------|
| `ModuleNotFoundError: openpyxl` | pip インストール未 | `pip3 install openpyxl` |
| `result-details.json が見つかりません` | eval が未完了 or `output/latest/d1/` が存在しない | `make eval` を先に完走させる |
| Makefile で `missing separator` エラー | インデントがスペースになっている | タブ文字に直す |
