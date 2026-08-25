#!/bin/bash

set -e

FONT="tools/fonts/Montserrat-Regular.ttf"
OUTPUT="src/fonts"

mkdir -p "$OUTPUT"

generate_font () {
    SIZE=$1
    NAME="montserrat_${SIZE}_pt"

    echo "Generating ${NAME}..."

    npx lv_font_conv \
        --font "$FONT" \
        --range 0x20-0xFF \
        --range 0x20AC \
        --range 0x2022 \
        --size "$SIZE" \
        --bpp 4 \
        --format lvgl \
        --no-compress \
        --no-prefilter \
        --force-fast-kern-format \
        --lv-include lvgl.h \
        --output "$OUTPUT/${NAME}.c"
}

generate_font 14
generate_font 20
generate_font 30
generate_font 48

echo
echo "OceanDesk fonts generated successfully."