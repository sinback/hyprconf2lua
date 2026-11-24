// Throwaway harness: runs Hyprland v0.54.3's real CConfigManager::handleGesture on gesture lines.
//
// Provenance of every piece compiled into this binary:
//   handleGesture.cpp          upstream, verbatim (run.sh diffs it against ConfigManager.cpp@v0.54.3 L2902-2983)
//   upstream/*                 upstream, copied mechanically by extract_upstream.sh (git show + sed), not checked in
//   this file                  HAND-WRITTEN by Claude: stubs + test cases, marked below
//
// Stub fidelity note: the real CTrackpadGestures::addGesture only returns an error when the new
// gesture collides with an already-registered one. Every case below starts with an empty registry,
// so the real one would return success too; the stub records the arguments and returns success.
// "unset" (removeGesture) is NOT exercised, because the real one errors on an empty registry.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <hyprutils/string/VarList2.hpp> // upstream (hyprutils v0.11.0)
using namespace Hyprutils::String;

#include "upstream/GestureTypes.hpp"     // upstream: eTrackpadGestureDirection
#include "upstream/eKeyboardModifiers.inc" // upstream: HL_MODIFIER_*

// ===================== HAND-WRITTEN STUBS (Claude) =====================

template <typename T>
using UP = std::unique_ptr<T>;
template <typename T, typename... Args>
UP<T> makeUnique(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// Gesture objects: just remember which kind was constructed and with what string args.
struct ITrackpadGesture {
    std::string              kind;
    std::vector<std::string> args;
    virtual ~ITrackpadGesture() = default;
};
struct CDispatcherTrackpadGesture : ITrackpadGesture {
    CDispatcherTrackpadGesture(std::string dispatcher, std::string dispArgs) { kind = "dispatcher"; args = {dispatcher, dispArgs}; }
};
struct CWorkspaceSwipeGesture : ITrackpadGesture {
    CWorkspaceSwipeGesture() { kind = "workspace"; }
};
struct CResizeTrackpadGesture : ITrackpadGesture {
    CResizeTrackpadGesture() { kind = "resize"; }
};
struct CMoveTrackpadGesture : ITrackpadGesture {
    CMoveTrackpadGesture() { kind = "move"; }
};
struct CSpecialWorkspaceGesture : ITrackpadGesture {
    CSpecialWorkspaceGesture(std::string a) { kind = "special"; args = {a}; }
};
struct CCloseTrackpadGesture : ITrackpadGesture {
    CCloseTrackpadGesture() { kind = "close"; }
};
struct CFloatTrackpadGesture : ITrackpadGesture {
    CFloatTrackpadGesture(std::string a) { kind = "float"; args = {a}; }
};
struct CFullscreenTrackpadGesture : ITrackpadGesture {
    CFullscreenTrackpadGesture(std::string a) { kind = "fullscreen"; args = {a}; }
};
struct CCursorZoomTrackpadGesture : ITrackpadGesture {
    CCursorZoomTrackpadGesture(std::string a, std::string b) { kind = "cursorZoom"; args = {a, b}; }
};

struct SRegistered {
    std::string               kind;
    std::vector<std::string>  args;
    size_t                    fingerCount;
    eTrackpadGestureDirection direction;
    uint32_t                  modMask;
    float                     deltaScale;
    bool                      disableInhibit;
};

class CTrackpadGestures {
  public:
    // stub: record what handleGesture registered
    std::expected<void, std::string> addGesture(UP<ITrackpadGesture>&& gesture, size_t fingerCount, eTrackpadGestureDirection direction, uint32_t modMask, float deltaScale,
                                                bool disableInhibit) {
        last = SRegistered{gesture->kind, gesture->args, fingerCount, direction, modMask, deltaScale, disableInhibit};
        return {};
    }
    std::expected<void, std::string> removeGesture(size_t, eTrackpadGestureDirection, uint32_t, float, bool) {
        return std::unexpected("stub: removeGesture not exercised by this harness");
    }
    eTrackpadGestureDirection  dirForString(const std::string_view& s); // upstream body below
    const char*                stringForDir(eTrackpadGestureDirection dir); // upstream body below
    std::optional<SRegistered> last;
};

class CKeybindManager {
  public:
    uint32_t stringToModMask(std::string mods); // upstream body below
};

class CConfigManager {
  public:
    std::optional<std::string> handleGesture(const std::string& command, const std::string& value); // upstream body below
};

UP<CTrackpadGestures> g_pTrackpadGestures;
UP<CKeybindManager>   g_pKeybindManager = makeUnique<CKeybindManager>();

// ===================== UPSTREAM CODE =====================
#include "upstream/dirForString.inc"
#include "upstream/stringForDir.inc"
#include "upstream/stringToModMask.inc"
#include "handleGesture.cpp"

// ===================== HAND-WRITTEN TEST CASES (Claude) =====================
// `expected` is my prediction of what Hyprland does. A mismatch prints FAIL with the real result.

static std::string describe(const std::optional<std::string>& err) {
    if (err)
        return "REJECTED: " + *err;
    const auto& r = *g_pTrackpadGestures->last;
    std::string a;
    for (size_t i = 0; i < r.args.size(); ++i)
        a += std::format("{}\"{}\"", i ? ", " : "", r.args[i]);
    return std::format("OK action={} args=[{}] fingers={} dir={} mods=0x{:x} scale={} disable_inhibit={}", r.kind, a, r.fingerCount,
                       g_pTrackpadGestures->stringForDir(r.direction), r.modMask, r.deltaScale, r.disableInhibit);
}

struct STestCase {
    const char* name;
    const char* command; // the keyword: "gesture" or "gesture" + flags
    const char* value;   // everything after '='
    std::string expected;
};

int main() {
    // HL_MODIFIER_META = 0x40 (SUPER), HL_MODIFIER_ALT = 0x8
    const std::vector<STestCase> cases = {
        {"issue #62 line, as written", "gesture", "3, up, mod: SUPER, fullscreen",
         R"(OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x40 scale=1 disable_inhibit=false)"},

        {"mod: in slot 0 (before fingers)", "gesture", "mod: SUPER, 3, up, fullscreen", "REJECTED: Invalid value mod: SUPER for finger count"},
        {"mod: in slot 1 (before direction)", "gesture", "3, mod: SUPER, up, fullscreen", "REJECTED: Invalid direction: mod: SUPER"},

        {"scale: in slot 2 (after direction)", "gesture", "3, up, scale: 1.5, fullscreen",
         R"(OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x0 scale=1.5 disable_inhibit=false)"},
        {"scale: in slot 0 (before fingers)", "gesture", "scale: 1.5, 3, up, fullscreen", "REJECTED: Invalid value scale: 1.5 for finger count"},
        {"scale: in slot 1 (before direction)", "gesture", "3, scale: 1.5, up, fullscreen", "REJECTED: Invalid direction: scale: 1.5"},

        {"mod: then scale:", "gesture", "3, up, mod: SUPER, scale: 1.5, fullscreen",
         R"(OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x40 scale=1.5 disable_inhibit=false)"},
        {"scale: then mod:", "gesture", "3, up, scale: 1.5, mod: SUPER, fullscreen",
         R"(OK action=fullscreen args=[""] fingers=3 dir=UP mods=0x40 scale=1.5 disable_inhibit=false)"},
        {"mod: repeated (last one wins?)", "gesture", "3, up, mod: SUPER, mod: ALT, close",
         R"(OK action=close args=[] fingers=3 dir=UP mods=0x8 scale=1 disable_inhibit=false)"},

        {"mod: after action w/ arg slot -> becomes the action's arg", "gesture", "3, up, fullscreen, mod: SUPER",
         R"(OK action=fullscreen args=["mod: SUPER"] fingers=3 dir=UP mods=0x0 scale=1 disable_inhibit=false)"},
        {"mod: after action w/o arg slot -> silently ignored", "gesture", "3, up, close, mod: SUPER",
         R"(OK action=close args=[] fingers=3 dir=UP mods=0x0 scale=1 disable_inhibit=false)"},

        {"dispatcher action keeps trailing args", "gesture", "3, left, mod: SUPER, dispatcher, workspace, e+1",
         R"(OK action=dispatcher args=["workspace", "e+1"] fingers=3 dir=LEFT mods=0x40 scale=1 disable_inhibit=false)"},
        {"gesturep keyword sets disable_inhibit", "gesturep", "3, up, close",
         R"(OK action=close args=[] fingers=3 dir=UP mods=0x0 scale=1 disable_inhibit=true)"},
    };

    int fails = 0;
    for (const auto& c : cases) {
        g_pTrackpadGestures = makeUnique<CTrackpadGestures>(); // fresh, empty registry per case
        CConfigManager cm;
        const auto     actual = describe(cm.handleGesture(c.command, c.value));
        const bool     ok     = actual == c.expected;
        fails += !ok;
        std::printf("[%s] %s\n       %s = %s\n       hyprland: %s\n", ok ? "PASS" : "FAIL", c.name, c.command, c.value, actual.c_str());
        if (!ok)
            std::printf("       expected: %s\n", c.expected.c_str());
    }
    std::printf("\n%zu cases, %d mismatches\n", cases.size(), fails);
    return fails ? 1 : 0;
}
