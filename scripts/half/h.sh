#!/usr/bin/env bash
# Half Loop runner for Claude Code (Linux / macOS / git-bash on Windows).
# A skill /half le CLAUDE.md + progress.txt e executa 1 task por iteracao.
#
# Uso:
#   ./scripts/half/h.sh                # usa "claude" (default)
#   ./scripts/half/h.sh --tool codex   # usa outro CLI compativel
#   MAX_ITERS=100 ./scripts/half/h.sh  # sobrescreve limite (default 50)

set -u

TOOL="claude"
MAX_ITERS="${MAX_ITERS:-50}"

while [ $# -gt 0 ]; do
    case "$1" in
        --tool) TOOL="$2"; shift 2 ;;
        --max)  MAX_ITERS="$2"; shift 2 ;;
        *) echo "arg desconhecido: $1"; exit 2 ;;
    esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$ROOT_DIR"

if [ ! -f CLAUDE.md ]; then echo "CLAUDE.md nao encontrado em $ROOT_DIR"; exit 1; fi
if [ ! -f progress.txt ]; then echo "No tasks completed yet. Start from Task 1.1." > progress.txt; fi

echo "=== Half Loop start (tool=$TOOL max=$MAX_ITERS root=$ROOT_DIR) ==="

for ((i=1; i<=MAX_ITERS; i++)); do
    if grep -q "ALL TASKS COMPLETE" progress.txt; then
        echo "[half] ALL TASKS COMPLETE detected. Stopping at iter $i."
        exit 0
    fi

    echo ""
    echo "=== Iter $i / $MAX_ITERS ==="

    "$TOOL" --dangerously-skip-permissions -p "/half"
    rc=$?
    if [ $rc -ne 0 ]; then
        echo "[half] $TOOL exit code $rc. Abortando."
        exit $rc
    fi
done

echo "[half] MAX_ITERS ($MAX_ITERS) atingido sem ALL TASKS COMPLETE."
exit 1
