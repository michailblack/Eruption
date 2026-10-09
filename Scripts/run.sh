#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
config="${1:-Debug}"
if (( $# > 0 )); then
    shift
fi

"$project_root/Scripts/build.sh" "$config"
cd -- "$project_root"
exec "$project_root/build/clang-$config/Binaries/Eruption-Editor" "$@"
