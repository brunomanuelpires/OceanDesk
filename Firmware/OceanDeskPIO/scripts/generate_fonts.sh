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

echo "Generating montserrat_96_pt..."
npx lv_font_conv \
    --font "$FONT" \
    --symbols '0123456789:.- mZH' \
    --size 96 \
    --bpp 4 \
    --format lvgl \
    --no-compress \
    --no-prefilter \
    --force-fast-kern-format \
    --lv-include lvgl.h \
    --lv-font-name montserrat_96_pt \
    --output "$OUTPUT/montserrat_96_pt.c"

echo "Generating montserrat_96_medium..."
npx lv_font_conv \
    --font "tools/fonts/Montserrat-Medium.ttf" \
    --symbols '0123456789:.- mZH' \
    --size 96 \
    --bpp 4 \
    --format lvgl \
    --no-compress \
    --no-prefilter \
    --force-fast-kern-format \
    --lv-include lvgl.h \
    --lv-font-name montserrat_96_medium \
    --output "$OUTPUT/montserrat_96_medium.c"

generate_medium_font () {
    SIZE=$1
    npx lv_font_conv \
        --font "tools/fonts/Montserrat-Medium.ttf" \
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
        --lv-font-name "montserrat_${SIZE}_medium" \
        --output "$OUTPUT/montserrat_${SIZE}_medium.c"
}

generate_medium_font 14
generate_medium_font 20
generate_medium_font 30

echo "Generating montserrat_18_bold..."
npx lv_font_conv \
    --font "tools/fonts/Montserrat-Bold.ttf" \
    --range 0x20-0xFF \
    --range 0x20AC \
    --range 0x2022 \
    --size 18 \
    --bpp 4 \
    --format lvgl \
    --no-compress \
    --no-prefilter \
    --force-fast-kern-format \
    --lv-include lvgl.h \
    --lv-font-name montserrat_18_bold \
    --output "$OUTPUT/montserrat_18_bold.c"

echo "Generating montserrat_112_bold..."
npx lv_font_conv \
    --font "tools/fonts/Montserrat-Bold.ttf" \
    --symbols '0123456789:' \
    --size 112 \
    --bpp 4 \
    --format lvgl \
    --no-compress \
    --no-prefilter \
    --force-fast-kern-format \
    --lv-include lvgl.h \
    --lv-font-name montserrat_112_bold \
    --output "$OUTPUT/montserrat_112_bold.c"

echo "Generating montserrat_128_bold..."
npx lv_font_conv \
    --font "tools/fonts/Montserrat-Bold.ttf" \
    --symbols '0123456789:' \
    --size 128 \
    --bpp 4 \
    --format lvgl \
    --no-compress \
    --no-prefilter \
    --force-fast-kern-format \
    --lv-include lvgl.h \
    --lv-font-name montserrat_128_bold \
    --output "$OUTPUT/montserrat_128_bold.c"

echo "Generating montserrat_96_bold..."
npx lv_font_conv \
    --font "tools/fonts/Montserrat-Bold.ttf" \
    --symbols '0123456789:.- mZH' \
    --size 96 \
    --bpp 4 \
    --format lvgl \
    --no-compress \
    --no-prefilter \
    --force-fast-kern-format \
    --lv-include lvgl.h \
    --lv-font-name montserrat_96_bold \
    --output "$OUTPUT/montserrat_96_bold.c"

echo
echo "OceanDesk fonts generated successfully."
