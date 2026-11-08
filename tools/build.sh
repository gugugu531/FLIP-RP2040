#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
build_dir="${1:-build-pendant}"
if (( $# )); then shift; fi
sdk_path="${PICO_SDK_PATH:-$HOME/.pico-sdk/sdk/2.2.0}"
args=(-DPENDANT_USE_PICO_VSCODE=OFF -DPICO_SDK_PATH="$sdk_path" -DCMAKE_BUILD_TYPE=Release)
compiler_path="${PICO_TOOLCHAIN_PATH:-$HOME/.pico-sdk/toolchain/14_2_Rel1}"
if [[ -x "$compiler_path/bin/arm-none-eabi-gcc" ]]; then
    args+=(-DPICO_TOOLCHAIN_PATH="$compiler_path")
fi
picotool_path="${picotool_DIR:-$HOME/.pico-sdk/picotool/2.2.0-a4/picotool}"
if [[ -f "$picotool_path/picotoolConfig.cmake" ]]; then
    args+=(-Dpicotool_DIR="$picotool_path")
fi
cmake -S . -B "$build_dir" "${args[@]}" "$@"
cmake --build "$build_dir" -j "$(getconf _NPROCESSORS_ONLN)"
