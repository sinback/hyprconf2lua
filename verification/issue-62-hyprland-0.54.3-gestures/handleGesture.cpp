std::optional<std::string> CConfigManager::handleGesture(const std::string& command, const std::string& value) {
    CVarList2                 data((std::string(value)));

    size_t                    fingerCount = 0;
    eTrackpadGestureDirection direction   = TRACKPAD_GESTURE_DIR_NONE;

    try {
        fingerCount = std::stoul(std::string{data[0]});
    } catch (...) { return std::format("Invalid value {} for finger count", data[0]); }

    if (fingerCount <= 1 || fingerCount >= 10)
        return std::format("Invalid value {} for finger count", data[0]);

    direction = g_pTrackpadGestures->dirForString(data[1]);

    if (direction == TRACKPAD_GESTURE_DIR_NONE)
        return std::format("Invalid direction: {}", data[1]);

    int      startDataIdx   = 2;
    uint32_t modMask        = 0;
    float    deltaScale     = 1.F;
    bool     disableInhibit = false;

    for (const auto arg : command.substr(7)) {
        switch (arg) {
            case 'p': disableInhibit = true; break;
            default: return "gesture: invalid flag";
        }
    }

    while (true) {

        if (data[startDataIdx].starts_with("mod:")) {
            modMask = g_pKeybindManager->stringToModMask(std::string{data[startDataIdx].substr(4)});
            startDataIdx++;
            continue;
        } else if (data[startDataIdx].starts_with("scale:")) {
            try {
                deltaScale = std::clamp(std::stof(std::string{data[startDataIdx].substr(6)}), 0.1F, 10.F);
                startDataIdx++;
                continue;
            } catch (...) { return std::format("Invalid delta scale: {}", std::string{data[startDataIdx].substr(6)}); }
        }

        break;
    }

    std::expected<void, std::string> result;

    if (data[startDataIdx] == "dispatcher")
        result = g_pTrackpadGestures->addGesture(makeUnique<CDispatcherTrackpadGesture>(std::string{data[startDataIdx + 1]}, data.join(",", startDataIdx + 2)), fingerCount,
                                                 direction, modMask, deltaScale, disableInhibit);
    else if (data[startDataIdx] == "workspace")
        result = g_pTrackpadGestures->addGesture(makeUnique<CWorkspaceSwipeGesture>(), fingerCount, direction, modMask, deltaScale, disableInhibit);
    else if (data[startDataIdx] == "resize")
        result = g_pTrackpadGestures->addGesture(makeUnique<CResizeTrackpadGesture>(), fingerCount, direction, modMask, deltaScale, disableInhibit);
    else if (data[startDataIdx] == "move")
        result = g_pTrackpadGestures->addGesture(makeUnique<CMoveTrackpadGesture>(), fingerCount, direction, modMask, deltaScale, disableInhibit);
    else if (data[startDataIdx] == "special")
        result =
            g_pTrackpadGestures->addGesture(makeUnique<CSpecialWorkspaceGesture>(std::string{data[startDataIdx + 1]}), fingerCount, direction, modMask, deltaScale, disableInhibit);
    else if (data[startDataIdx] == "close")
        result = g_pTrackpadGestures->addGesture(makeUnique<CCloseTrackpadGesture>(), fingerCount, direction, modMask, deltaScale, disableInhibit);
    else if (data[startDataIdx] == "float")
        result =
            g_pTrackpadGestures->addGesture(makeUnique<CFloatTrackpadGesture>(std::string{data[startDataIdx + 1]}), fingerCount, direction, modMask, deltaScale, disableInhibit);
    else if (data[startDataIdx] == "fullscreen")
        result = g_pTrackpadGestures->addGesture(makeUnique<CFullscreenTrackpadGesture>(std::string{data[startDataIdx + 1]}), fingerCount, direction, modMask, deltaScale,
                                                 disableInhibit);
    else if (data[startDataIdx] == "cursorZoom") {
        result = g_pTrackpadGestures->addGesture(makeUnique<CCursorZoomTrackpadGesture>(std::string{data[startDataIdx + 1]}, std::string{data[startDataIdx + 2]}), fingerCount,
                                                 direction, modMask, deltaScale, disableInhibit);
    } else if (data[startDataIdx] == "unset")
        result = g_pTrackpadGestures->removeGesture(fingerCount, direction, modMask, deltaScale, disableInhibit);
    else
        return std::format("Invalid gesture: {}", data[startDataIdx]);

    if (!result)
        return result.error();

    return std::nullopt;
}
