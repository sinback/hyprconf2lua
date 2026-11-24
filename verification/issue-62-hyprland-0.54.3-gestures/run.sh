#!/usr/bin/env bash
# Reproduces the issue #62 verification end to end:
#   1. clone Hyprland v0.54.3 and hyprutils v0.11.0 (shallow), and check their commit SHAs
#   2. extract the upstream helpers handleGesture depends on
#   3. confirm handleGesture.cpp in this directory is byte-identical to upstream
#   4. build and run the harness
# Needs: git, bash, g++ >= 13 (needs C++23 std::expected and std::format), network access to github.com.
# Note: clang++ 18 paired with libstdc++ 13 does NOT work: that <expected> requires __cpp_concepts >= 202002L.
set -euo pipefail
cd "$(dirname "$0")"

HL_TAG=v0.54.3
HL_SHA=521ece463c4a9d3d128670688a34756805a4328f
HU_TAG=v0.11.0
HU_SHA=fe686486ac867a1a24f99c753bb40ffed338e4b0
WORK=.work
CXX="${CXX:-g++}"

fetch() { # url dir tag sha
    if [[ ! -d "$2" ]]; then
        git -c advice.detachedHead=false clone -q --depth 1 --branch "$3" "$1" "$2"
    fi
    local got
    got="$(git -C "$2" rev-parse "$3^{commit}")"
    if [[ "$got" != "$4" ]]; then
        echo "run.sh: $1 tag $3 is $got, expected $4" >&2
        exit 1
    fi
}

mkdir -p "$WORK"
echo "==> fetching upstream sources"
fetch https://github.com/hyprwm/Hyprland.git  "$WORK/Hyprland"  "$HL_TAG" "$HL_SHA"
fetch https://github.com/hyprwm/hyprutils.git "$WORK/hyprutils" "$HU_TAG" "$HU_SHA"

echo "==> extracting upstream helpers"
bash extract_upstream.sh "$WORK/Hyprland" "$WORK/hyprutils"

echo "==> checking handleGesture.cpp against Hyprland $HL_TAG"
diff upstream/handleGesture.upstream.cpp handleGesture.cpp
echo "    identical"

echo "==> building with $CXX"
"$CXX" -std=c++23 -Wall -Iupstream/include -o "$WORK/test_handleGesture" \
    test_handleGesture.cpp upstream/VarList2.cpp upstream/String.cpp

echo "==> running"
"$WORK/test_handleGesture"
