#!/usr/bin/env bash
# Copies real upstream code into ./upstream/ mechanically (git show + sed line ranges).
# Nothing in ./upstream/ is hand-written. Usually invoked by run.sh.
#   usage: extract_upstream.sh <Hyprland checkout> <hyprutils checkout>
set -euo pipefail
HL=$1     # hyprwm/Hyprland checkout containing tag $TAG
HU=$2     # hyprwm/hyprutils checkout containing tag $HU_TAG
TAG=v0.54.3
HU_TAG=v0.11.0   # Hyprland 0.54.3's CMakeLists.txt sets HYPRUTILS_MINIMUM_VERSION 0.11.0
mkdir -p upstream

# Direction enum, whole file.
git -C "$HL" show "$TAG:src/managers/input/trackpad/GestureTypes.hpp" > upstream/GestureTypes.hpp

# eKeyboardModifiers enum (IKeyboard.hpp lines 13-22).
git -C "$HL" show "$TAG:src/devices/IKeyboard.hpp" | sed -n '13,22p' > upstream/eKeyboardModifiers.inc

# CTrackpadGestures::dirForString definition (TrackpadGestures.cpp lines 13-39).
git -C "$HL" show "$TAG:src/managers/input/trackpad/TrackpadGestures.cpp" | sed -n '13,39p' > upstream/dirForString.inc

# CTrackpadGestures::stringForDir definition (TrackpadGestures.cpp lines 41-56), used only for printing.
git -C "$HL" show "$TAG:src/managers/input/trackpad/TrackpadGestures.cpp" | sed -n '41,56p' > upstream/stringForDir.inc

# CKeybindManager::stringToModMask definition (KeybindManager.cpp lines 240-261).
git -C "$HL" show "$TAG:src/managers/KeybindManager.cpp" | sed -n '240,261p' > upstream/stringToModMask.inc

# The full handleGesture definition, for run.sh to diff against our checked-in handleGesture.cpp.
git -C "$HL" show "$TAG:src/config/ConfigManager.cpp" |
    awk '/^std::optional<std::string> CConfigManager::handleGesture\(/ {on=1} on {print} on && /^}$/ {exit}' > upstream/handleGesture.upstream.cpp

# CVarList2 + trim from hyprutils.
git -C "$HU" show "$HU_TAG:include/hyprutils/string/VarList2.hpp" > upstream/VarList2.hpp
git -C "$HU" show "$HU_TAG:include/hyprutils/string/String.hpp"   > upstream/String.hpp
git -C "$HU" show "$HU_TAG:src/string/VarList2.cpp"               > upstream/VarList2.cpp
git -C "$HU" show "$HU_TAG:src/string/String.cpp"                 > upstream/String.cpp
# Their #includes say <hyprutils/string/X.hpp>; lay out a matching include dir.
mkdir -p upstream/include/hyprutils/string
cp upstream/VarList2.hpp upstream/String.hpp upstream/include/hyprutils/string/

# Guard against a line range drifting: each extract must start with the declaration we expect.
check_first_line() {
    if [[ "$(head -n1 "$1")" != "$2" ]]; then
        echo "extract_upstream.sh: $1 does not start with: $2" >&2
        exit 1
    fi
}
check_first_line upstream/eKeyboardModifiers.inc 'enum eKeyboardModifiers {'
check_first_line upstream/dirForString.inc       'eTrackpadGestureDirection CTrackpadGestures::dirForString(const std::string_view& s) {'
check_first_line upstream/stringForDir.inc       'const char* CTrackpadGestures::stringForDir(eTrackpadGestureDirection dir) {'
check_first_line upstream/stringToModMask.inc    'uint32_t CKeybindManager::stringToModMask(std::string mods) {'
for f in upstream/eKeyboardModifiers.inc upstream/dirForString.inc upstream/stringForDir.inc upstream/stringToModMask.inc; do
    last="$(tail -n1 "$f")"
    if [[ "$last" != "}" && "$last" != "};" ]]; then
        echo "extract_upstream.sh: $f does not end at a closing brace (got: $last)" >&2
        exit 1
    fi
done
