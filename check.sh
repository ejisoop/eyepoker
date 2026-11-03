#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
compiler="${CXX:-g++}"
build_dir="${POKEREYE_BUILD_DIR:-build/local}"
mkdir -p "$build_dir"
flags=(-std=c++20 -O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror -pthread -Iinclude)
if [[ "${SANITIZE:-0}" == 1 ]]; then
  flags=(-std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -pthread -Iinclude -fsanitize=address,undefined -fno-omit-frame-pointer)
fi
sources=(src/core/*.cpp)
"$compiler" "${flags[@]}" "${sources[@]}" tests/core_test.cpp -o "$build_dir/poker_tests"
"$build_dir/poker_tests" "$@"
"$compiler" "${flags[@]}" "${sources[@]}" tests/allocation_test.cpp -o "$build_dir/poker_allocation_tests"
"$build_dir/poker_allocation_tests"
"$compiler" "${flags[@]}" "${sources[@]}" src/cli/main.cpp -o "$build_dir/poker_sim"
"$compiler" "${flags[@]}" "${sources[@]}" examples/custom_variant.cpp -o "$build_dir/poker_example"
"$build_dir/poker_sim" equity --hand AsAh --hand KsKh --trials 5000 --threads 2
"$build_dir/poker_sim" play --variant omaha --players 4 --hands 100
if "$build_dir/poker_sim" equity --hand AsAs; then
  echo 'Expected duplicate-card validation to fail' >&2
  exit 1
fi
