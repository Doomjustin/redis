#!/usr/bin/env bash
set -euo pipefail

name="${1:-}"
dir_input="${2:-}"
workspace="${3:-}"
ns="${4:-}"

if [[ -z "$name" ]]; then
  printf "%s\n" "ERROR: file name cannot be empty."
  exit 1
fi

if [[ ! "$name" =~ ^[A-Za-z][A-Za-z0-9_]*$ ]]; then
  printf "%s\n" "ERROR: file name must start with a letter and contain only letters, digits, or underscores."
  exit 1
fi

if [[ -z "$workspace" ]]; then
  printf "%s\n" "ERROR: workspace path is required."
  exit 1
fi

if [[ -z "$dir_input" ]]; then
  dir="$workspace"
elif [[ "$dir_input" == /* ]]; then
  dir="$dir_input"
else
  dir="$workspace/$dir_input"
fi

dir=$(realpath -m "$dir")
workspace=$(realpath -m "$workspace")
mkdir -p "$dir"

rel_dir="$dir"
if [[ "$dir" == "$workspace"/* ]]; then
  rel_dir="${dir#$workspace/}"
fi

if [[ -z "$ns" ]]; then
  if [[ "$rel_dir" == xin/* ]]; then
    ns=$(printf "%s" "$rel_dir" | sed 's|/|::|g')
  else
    ns="xin"
  fi
fi

h="$dir/$name.h"
cpp="$dir/$name.cpp"
test="$dir/$name.test.cpp"

for f in "$h" "$cpp" "$test"; do
  if [[ -e "$f" ]]; then
    printf "%s\n" "ERROR: target file already exists:" "  - $f" "No files were overwritten."
    exit 2
  fi
done

guard_name=$(echo "$name" | tr "[:lower:]" "[:upper:]")
if [[ "$guard_name" == XIN_* ]]; then
  guard="${guard_name}_H"
else
  guard="XIN_${guard_name}_H"
fi

cat > "$h" <<EOF
#ifndef $guard
#define $guard

namespace $ns {

} // namespace $ns

#endif // $guard
EOF

cat > "$cpp" <<EOF
#include "$name.h"

namespace $ns {

} // namespace $ns
EOF

cat > "$test" <<EOF
#include "$name.h"

#include <doctest/doctest.h>
EOF

printf "%s\n" \
  "Scaffold created successfully:" \
  "  name      : $name" \
  "  namespace : $ns" \
  "  dir       : $dir" \
  "  files:" \
  "    - $h" \
  "    - $cpp" \
  "    - $test"
