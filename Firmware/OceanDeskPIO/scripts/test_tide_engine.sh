#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="${TMPDIR:-/tmp}/oceandesk-tide-engine-test"
mkdir -p "$build_dir"

${CXX:-c++} \
    -std=c++17 \
    -Wall \
    -Wextra \
    -Werror \
    -I "$project_dir/src" \
    "$project_dir/src/tides/TideEngine.cpp" \
    "$project_dir/src/tides/data/NazareTideData.cpp" \
    "$project_dir/test/tide_engine/test_tide_engine.cpp" \
    -o "$build_dir/test_tide_engine"

"$build_dir/test_tide_engine"
