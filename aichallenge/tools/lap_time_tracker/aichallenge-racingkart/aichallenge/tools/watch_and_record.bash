#!/usr/bin/env bash
# watch_and_record.bash
# result-details.json の更新を検知して docker compose down + post_run.bash を実行する。
# Makefile の eval-*-and-record からバックグラウンド起動される。

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
RESULT_PATH="${REPO_ROOT}/output/latest/d1/result-details.json"
SECTION_JSON="${REPO_ROOT}/output/result-section.json"

LOG="${REPO_ROOT}/output/watch_and_record.log"
mkdir -p "$(dirname "${LOG}")"

{
    # シミュレーション開始時に result-section.json を初期化（空の状態で作成）
    mkdir -p "$(dirname "${SECTION_JSON}")"
    echo '{"status": "running", "section_times": {}, "gate_pass_elapsed": {}}' > "${SECTION_JSON}"
    echo "[post_run] result-section.json を初期化しました: ${SECTION_JSON}"

    echo "[post_run] result-details.json の更新を待機中..."
    before=$(stat -c %Y "${RESULT_PATH}" 2>/dev/null || echo 0)
    container_stopped=0

    while true; do
        after=$(stat -c %Y "${RESULT_PATH}" 2>/dev/null || echo 0)
        if [ "${after}" != "${before}" ] && [ "${after}" != "0" ]; then
            break
        fi
        # コンテナが消えた場合（make down 手動実行 / スタック後）も抜ける
        if ! docker compose ps --services --filter status=running 2>/dev/null | grep -q .; then
            echo "[post_run] コンテナ停止を検知（手動 make down？）。転記を続行します..."
            container_stopped=1
            break
        fi
        sleep 2
    done

    cd "${REPO_ROOT}"
    if [ "${container_stopped}" -eq 0 ]; then
        echo "[post_run] 走行完了を検知。停止 + 転記します..."
        sleep 2
        docker compose down --remove-orphans
    fi

    # セクションタイム計算
    echo "[post_run] セクションタイムを計算中..."
    python3 "${REPO_ROOT}/aichallenge/tools/calc_section_times.py" \
        --output "${SECTION_JSON}" \
        && echo "[post_run] セクションタイム計算完了" \
        || echo "[post_run] セクションタイム計算失敗（xlsx転記は続行）"

    if [ "${container_stopped}" -eq 1 ]; then
        ./post_run.bash --na-lap
    else
        ./post_run.bash
    fi
} >> "${LOG}" 2>&1
