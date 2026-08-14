#!/usr/bin/env bash
# post_run.bash — make eval の直後に実行する後処理ラッパー
#
# 使い方:
#   ./post_run.bash              # output/latest を自動検索
#   ./post_run.bash --label "memo_001"  # 列ラベルを指定
#
# このファイルは環境更新（ベーススクリプト変更）の影響を受けない自作コードへの
# 唯一のエントリポイント。中身は aichallenge/tools/ 以下のスクリプトに委譲する。

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="${SCRIPT_DIR}/aichallenge/tools"

# ============================================================
# 引数をそのまま lap_time_tracker.py へ転送
# ============================================================
python3 "${TOOLS_DIR}/lap_time_tracker.py" "$@"

# 転記完了後に result-section.json を削除する。
# 次回走行で周回未完了だった場合に前回値が再転記されるのを防ぐ。
SECTION_JSON="${SCRIPT_DIR}/output/result-section.json"
if [ -f "${SECTION_JSON}" ]; then
    rm "${SECTION_JSON}"
    echo "[post_run] result-section.json を削除しました（次回走行の前回値混入を防止）"
fi
