#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="${TMPDIR:-/tmp}/oceandesk-beach-catalog-test"
mkdir -p "$build_dir"

${CXX:-c++} \
    -std=c++17 \
    -Wall \
    -Wextra \
    -Werror \
    -I "$project_dir/src" \
    "$project_dir/src/locations/BeachCatalog.cpp" \
    "$project_dir/test/beach_catalog/test_beach_catalog.cpp" \
    -o "$build_dir/test_beach_catalog"

"$build_dir/test_beach_catalog"
