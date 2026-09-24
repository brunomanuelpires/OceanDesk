#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="${TMPDIR:-/tmp}/oceandesk-time-of-day-test"
mkdir -p "$build_dir"

${CXX:-c++} \
    -std=c++17 \
    -Wall \
    -Wextra \
    -Werror \
    -I "$project_dir/src" \
    "$project_dir/test/time_of_day/test_time_of_day.cpp" \
    -o "$build_dir/test_time_of_day"

"$build_dir/test_time_of_day"
