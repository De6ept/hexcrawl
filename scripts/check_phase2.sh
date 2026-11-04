#!/usr/bin/env bash
# Phase 2 check: the demo must give the same totals whether the cards come
# from C++ or from data/cards.json.
#
#   bash scripts/check_phase2.sh [path/to/hexcrawl_console]
#
# Run it from the repo root.
set -u
bin="${1:-build/hexcrawl_console}"

if ! out="$("$bin" --data data/cards.json 2>&1)"; then
    echo "FAIL: the console exited with an error:"
    echo "$out" | tail -5
    exit 1
fi

if diff <(grep '^=>' <<<"$out") docs/demo_totals.txt; then
    echo "PASS: all demo totals match"
else
    echo "FAIL: totals differ  (< is yours, > is expected)"
    exit 1
fi
