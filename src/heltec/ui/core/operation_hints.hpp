#pragma once

#ifndef MOMENTARY_BUTTON_MAX
#define MOMENTARY_BUTTON_MAX 1
#endif

namespace heltec::meshcore::ui::operation_hint {

#if defined(HELTEC_V4_R8_TFT) && defined(HELTEC_HAS_TOUCH) && HELTEC_HAS_TOUCH

constexpr const char* kRecentDetail = "Swipe:msg Tap:reply";
constexpr const char* kMessageSelect = "Tap:select";
constexpr const char* kRadioPreset = "Swipe:select Tap:apply";
constexpr const char* kChoicePicker = "<:back Swipe:move Tap:ok";
constexpr const char* kPreviewClose = "Tap:close";
constexpr const char* kCalibrationCancel = "2x tap:cancel";
constexpr const char* kCalibrationClose = "Tap:close";
constexpr const char* kCalibrationRetry = "Tap:retry";

#elif MOMENTARY_BUTTON_MAX == 1

constexpr const char* kRecentDetail = "Click:msg Hold:reply";
constexpr const char* kMessageSelect = "Click:move Hold:select";
constexpr const char* kRadioPreset = "Click:move Hold:apply";
constexpr const char* kChoicePicker = "1x:next Hold:ok 2x:back";
constexpr const char* kPreviewClose = "Click/Hold:close";
constexpr const char* kDestructiveConfirm = "Hold:confirm 2x:cancel";
constexpr const char* kCalibrationCancel = "2x:cancel";
constexpr const char* kCalibrationClose = "Hold/2x:close";
constexpr const char* kCalibrationRetry = "Hold:retry 2x:close";

#else

constexpr const char* kRecentDetail = "U/D:msg Hold:reply";
constexpr const char* kMessageSelect = "U/D:move Hold:select";
constexpr const char* kRadioPreset = "U/D:move Hold:apply";
constexpr const char* kChoicePicker = "U/D:move Hold:ok 2x:back";
constexpr const char* kPreviewClose = "Click/Hold:close";
constexpr const char* kDestructiveConfirm = "Hold:confirm 2x:cancel";
constexpr const char* kCalibrationCancel = "2x:cancel";
constexpr const char* kCalibrationClose = "Hold/2x:close";
constexpr const char* kCalibrationRetry = "Hold:retry 2x:close";

#endif

}  // namespace heltec::meshcore::ui::operation_hint
