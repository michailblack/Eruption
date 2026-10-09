#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
config="${1:-Debug}"
if (( $# > 1 )); then
    echo "Usage: $0 [Debug|Release|Dist]" >&2
    exit 1
fi
case "$config" in
    Debug|Release|Dist) ;;
    *) echo "Usage: $0 [Debug|Release|Dist]" >&2; exit 1 ;;
esac

for tool in cmake clang clang++ ninja; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Required tool not found: $tool" >&2
        exit 1
    fi
done

build_dir="$project_root/build/clang-$config"
database_link="$project_root/compile_commands.json"

cmake -S "$project_root" -B "$build_dir" -G Ninja \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_BUILD_TYPE="$config" \
    -DCMAKE_COLOR_DIAGNOSTICS=ON \
    -DER_ENABLE_CPPCHECK=ON \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [[ -d "$database_link" ]]; then
    echo "Cannot replace directory: $database_link" >&2
    exit 1
fi
if [[ -e "$database_link" && ! -L "$database_link" ]]; then
    backup="$(mktemp "$project_root/compile_commands.json.backup.XXXXXX")"
    mv -- "$database_link" "$backup"
    echo "Saved existing compilation database to $backup"
fi
ln -sfn -- "build/clang-$config/compile_commands.json" "$database_link"

CLICOLOR_FORCE=1 cmake --build "$build_dir" --parallel
