#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
format_args=(-i)
if (( $# == 1 )) && [[ "$1" == --check ]]; then
    format_args=(--dry-run --Werror)
elif (( $# != 0 )); then
    echo "Usage: $0 [--check]" >&2
    exit 1
fi
if ! command -v clang-format >/dev/null 2>&1; then
    echo "Required tool not found: clang-format" >&2
    exit 1
fi

mapfile -d '' -t sources < <(find "$project_root/Eruption/Source" "$project_root/Editor/Source" \
    -type f \( -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.c' \) -print0)
if (( ${#sources[@]} > 0 )); then
    clang-format "${format_args[@]}" -- "${sources[@]}"
fi
