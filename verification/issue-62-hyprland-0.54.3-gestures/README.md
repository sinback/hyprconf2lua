# Verifying Hyprland 0.54.3's `gesture =` grammar

This directory supports the fix for
[Prateek-squadron/hyprconf2lua#62](https://github.com/Prateek-squadron/hyprconf2lua/issues/62)
("converted gestures missing mods"). It answers one question: **where in a legacy `gesture =` line
may `mod:` and `scale:` appear?** It does that by running Hyprland's real parser on test lines,
instead of reading the source or the docs and guessing.

It isn't part of hyprconf2lua and the Python test suite doesn't run it.

## Run it

```bash
./run.sh
```

You need `git`, `bash`, `g++` ≥ 13 and network access to github.com. `run.sh`:

1. Shallow-clones [hyprwm/Hyprland](https://github.com/hyprwm/Hyprland) at `v0.54.3` and
   [hyprwm/hyprutils](https://github.com/hyprwm/hyprutils) at `v0.11.0` into `.work/`, and fails
   if either tag doesn't resolve to the pinned commit SHA.
2. Runs `extract_upstream.sh`, which copies the upstream helpers into `upstream/` with
   `git show` and `sed` line ranges. Nothing is retyped. Each extract is checked to start with the
   expected declaration and end on a closing brace, so a drifting line range fails loudly.
3. Diffs the checked-in `handleGesture.cpp` against the function in Hyprland's
   `src/config/ConfigManager.cpp` and stops if they differ.
4. Compiles the harness and runs it. The exit code is non-zero if any case doesn't match.

clang++ 18 with libstdc++ 13 does **not** work: libstdc++ 13's `<expected>` requires
`__cpp_concepts >= 202002L`, and clang 18 reports `201907L`.

## Why v0.54.3

It's the last Hyprland release before native Lua config (v0.55). Configs from that era are the
ones hyprconf2lua converts. hyprutils v0.11.0 is the `HYPRUTILS_MINIMUM_VERSION` in that release's
`CMakeLists.txt`.

## Where each part of the binary comes from

| File | Origin |
|---|---|
| `handleGesture.cpp` | Hyprland v0.54.3, `CConfigManager::handleGesture`, verbatim. `run.sh` checks this with `diff`. |
| `upstream/*` (generated, not checked in) | Hyprland v0.54.3 and hyprutils v0.11.0, copied by `extract_upstream.sh`: the direction enum, the modifier enum, `dirForString`, `stringForDir`, `stringToModMask`, and hyprutils' `CVarList2` (the comma splitter) plus `trim`. |
| `test_handleGesture.cpp` | Hand-written: stubs and test cases, labelled in the file. |

The stubs are minimal. `addGesture` records what `handleGesture` registered and returns success.
The real `CTrackpadGestures::addGesture` only returns an error when a new gesture collides with
one already registered. Every case starts with an empty registry, so the real one would return
success too. `unset` (`removeGesture`) isn't tested, because the real one returns an error on an
empty registry.

To check that the harness can fail at all, we ran a copy with one deliberately wrong expectation.
It reported `FAIL` and exited non-zero.

## Results

```text
[PASS] issue #62 line, as written
       gesture = 3, up, mod: SUPER, fullscreen
       hyprland: OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x40 scale=1 disable_inhibit=false
[PASS] mod: in slot 0 (before fingers)
       gesture = mod: SUPER, 3, up, fullscreen
       hyprland: REJECTED: Invalid value mod: SUPER for finger count
[PASS] mod: in slot 1 (before direction)
       gesture = 3, mod: SUPER, up, fullscreen
       hyprland: REJECTED: Invalid direction: mod: SUPER
[PASS] scale: in slot 2 (after direction)
       gesture = 3, up, scale: 1.5, fullscreen
       hyprland: OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x0 scale=1.5 disable_inhibit=false
[PASS] scale: in slot 0 (before fingers)
       gesture = scale: 1.5, 3, up, fullscreen
       hyprland: REJECTED: Invalid value scale: 1.5 for finger count
[PASS] scale: in slot 1 (before direction)
       gesture = 3, scale: 1.5, up, fullscreen
       hyprland: REJECTED: Invalid direction: scale: 1.5
[PASS] mod: then scale:
       gesture = 3, up, mod: SUPER, scale: 1.5, fullscreen
       hyprland: OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x40 scale=1.5 disable_inhibit=false
[PASS] scale: then mod:
       gesture = 3, up, scale: 1.5, mod: SUPER, fullscreen
       hyprland: OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x40 scale=1.5 disable_inhibit=false
[PASS] mod: repeated (last one wins?)
       gesture = 3, up, mod: SUPER, mod: ALT, close
       hyprland: OK action=close args=[] fingers=3 dir=UP mods=0x8 scale=1 disable_inhibit=false
[PASS] mod: after action w/ arg slot -> becomes the action's arg
       gesture = 3, up, fullscreen, mod: SUPER
       hyprland: OK action=fullscreen args=["mod: SUPER"] fingers=3 dir=UP mods=0x0 scale=1 disable_inhibit=false
[PASS] mod: after action w/o arg slot -> silently ignored
       gesture = 3, up, close, mod: SUPER
       hyprland: OK action=close args=[] fingers=3 dir=UP mods=0x0 scale=1 disable_inhibit=false
[PASS] dispatcher action keeps trailing args
       gesture = 3, left, mod: SUPER, dispatcher, workspace, e+1
       hyprland: OK action=dispatcher args=["workspace", "e+1"] fingers=3 dir=LEFT mods=0x40 scale=1 disable_inhibit=false
[PASS] gesturep keyword sets disable_inhibit
       gesturep = 3, up, close
       hyprland: OK action=close args=[] fingers=3 dir=UP mods=0x0 scale=1 disable_inhibit=true

13 cases, 0 mismatches
```

In Hyprland 0.54.3, then:

- Fingers and direction must be the first two fields. `mod:` or `scale:` in either of those
  slots makes Hyprland reject the line.
- After the direction come any number of `mod:` and `scale:` fields, in any order. If one
  repeats, the last one wins.
- The first field that isn't `mod:` or `scale:` is the action. Fields after it are the action's
  arguments, so a `mod:` placed after the action is never treated as a modifier.
- `gesturep` is the same keyword with the `p` flag, which sets `disable_inhibit`.

## License

`handleGesture.cpp` and the generated `upstream/` files are code from Hyprland and hyprutils,
both under the BSD 3-Clause License. Their license texts are copied verbatim in
`LICENSE.Hyprland` and `LICENSE.hyprutils`.
