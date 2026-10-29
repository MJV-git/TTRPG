#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd -- "$(dirname -- "$0")" && pwd)"
mode="gui"
if [[ "${1:-}" == "--cli" ]]; then
    mode="cli"
    shift
fi
if [[ "$mode" == "cli" ]]; then
    cmake -S "$project_dir" -B "$project_dir/build-cli" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_GUI=OFF
    cmake --build "$project_dir/build-cli" --parallel
    cd -- "$project_dir"
    exec "$project_dir/build-cli/ttrpg" "$@"
fi
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_GUI=ON
cmake --build "$project_dir/build" --parallel
cd -- "$project_dir"
exec "$project_dir/build/ttrpg_gui" "$@"
