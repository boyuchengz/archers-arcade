#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d)
trap 'rm -rf "$build_dir"' EXIT
for test_name in test_milestone1 test_gomoku test_ai test_rivals; do
  "${CC:-cc}" -O2 -Wno-unknown-pragmas -Iplugins/tabletop/engine/include -Iplugins/tabletop/engine/src/states "tests/${test_name}.c" -o "$build_dir/$test_name"
  "$build_dir/$test_name"
done
