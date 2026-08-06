#include "e213_lvgl_port.hpp"

#include <Arduino.h>
#include <heltec-eink-modules.h>
#include <lvgl.h>
#include <string.h>

#include "lv_mem_psram.h"
#include "target.h"

#ifndef E213_LVGL_FULL_REFRESH_INTERVAL
#define E213_LVGL_FULL_REFRESH_INTERVAL 20
#endif
#ifndef E213_LVGL_STARTUP_FULL_REFRESH
#define E213_LVGL_STARTUP_FULL_REFRESH 1
#endif
#ifndef E213_LVGL_REFRESH_QUIET_MS
#define E213_LVGL_REFRESH_QUIET_MS 150
#endif
#ifndef E213_LVGL_CRITICAL_REFRESH_MAX_WAIT_MS
#define E213_LVGL_CRITICAL_REFRESH_MAX_WAIT_MS 2000
#endif
#ifndef E213_LVGL_DEFERRED_REFRESH_MAX_WAIT_MS
#define E213_LVGL_DEFERRED_REFRESH_MAX_WAIT_MS 10000
#endif
#ifndef E213_LVGL_FULL_REFRESH_AREA_PCT
#define E213_LVGL_FULL_REFRESH_AREA_PCT 0
#endif
#ifndef E213_LVGL_AUTO_REFRESH
#define E213_LVGL_AUTO_REFRESH 1
#endif

namespace heltec::meshcore::dal::e213 {
namespace {

static constexpr int kWidth = 250;
static constexpr int kHeight = 122;
static constexpr uint32_t kFullBufferPixels =
    static_cast<uint32_t>(kWidth) * static_cast<uint32_t>(kHeight);

static lv_color_t* s_draw_buffer = nullptr;
static uint8_t* s_pending_frame = nullptr;
static uint8_t* s_committed_frame = nullptr;
static lv_disp_draw_buf_t s_lv_draw_buffer;
static lv_disp_drv_t s_lv_driver;
static lv_disp_t* s_lv_display = nullptr;
static lv_timer_t* s_commit_timer = nullptr;
static BaseDisplay* s_panel = nullptr;
static bool s_power_claimed = false;
static bool s_panel_on = false;
static bool s_panel_dirty = false;
static bool s_force_full_refresh = E213_LVGL_STARTUP_FULL_REFRESH != 0;
static bool s_fast_mode_ready = false;
static uint16_t s_partial_refreshes = 0;
static uint32_t s_refresh_count = 0;
static uint32_t s_last_refresh_ms = 0;
static uint32_t s_flush_count = 0;
static uint32_t s_last_flush_ms = 0;
static uint32_t s_lvgl_flush_cycles = 0;
static uint32_t s_lvgl_flush_total = 0;
static bool s_refresh_pending = false;
enum class RefreshRequest : uint8_t {
  None,
  Deferred,
  Critical,
};
static RefreshRequest s_refresh_request = RefreshRequest::None;
static uint32_t s_refresh_request_started_ms = 0;
static uint32_t s_refresh_request_count = 0;
static uint32_t s_refresh_request_after_cycle = 0;
static char s_refresh_reason[32] = {};
static lv_area_t s_dirty_area = {kWidth, kHeight, -1, -1};
static bool s_navigation_active = false;
static bool s_navigation_first_pending = false;
static bool s_navigation_hold = false;
static bool s_navigation_final_wait = false;
static bool s_navigation_commit_completed = false;

struct FrameDiff {
  uint32_t pixels = 0;
  lv_area_t area = {kWidth, kHeight, -1, -1};
};

static void powerOn() {
  if (s_power_claimed) return;
  board.periph_power.claim();
  s_power_claimed = true;
  delay(50);
}

static void powerOff() {
  if (!s_power_claimed) return;
  board.periph_power.release();
  s_power_claimed = false;
}

static BaseDisplay* detectPanel() {
  // The two E213 revisions expose opposite BUSY polarity while RESET is held.
  pinMode(DISP_RST, OUTPUT);
  digitalWrite(DISP_RST, LOW);
  delay(10);
  pinMode(DISP_BUSY, INPUT);
  const bool busy_level = digitalRead(DISP_BUSY);
  pinMode(DISP_RST, INPUT);

  if (busy_level == LOW) return new EInkDisplay_VisionMasterE213();
  return new EInkDisplay_VisionMasterE213V1_1();
}

static const char* refreshRequestName(RefreshRequest request) {
  switch (request) {
    case RefreshRequest::Critical:
      return "CRITICAL";
    case RefreshRequest::Deferred:
      return "DEFERRED";
    default:
      return "NONE";
  }
}

static bool isNavigationRefreshReason(const char* reason) {
  if (!reason) return false;
  return strcmp(reason, "nav-open") == 0 ||
         strcmp(reason, "nav-activity") == 0 ||
         strcmp(reason, "nav-close") == 0;
}

static void clearRefreshRequest() {
  s_refresh_request = RefreshRequest::None;
  s_refresh_request_started_ms = 0;
  s_refresh_request_count = 0;
  s_refresh_request_after_cycle = 0;
  s_refresh_reason[0] = '\0';
}

static void clearDirtyState() {
  s_flush_count = 0;
  s_dirty_area = {kWidth, kHeight, -1, -1};
  s_panel_dirty = false;
}

static void settleNavigationFrame(bool physical_update) {
  if (!s_navigation_active || !s_navigation_first_pending) return;
  s_navigation_first_pending = false;
  s_navigation_hold = true;
  s_navigation_commit_completed = true;
  Serial.printf("[e213][nav] settled frame %s; holding intermediate frames\n",
                physical_update ? "committed" : "unchanged");
}

static FrameDiff calculateFrameDiff() {
  FrameDiff diff;
  if (!s_pending_frame || !s_committed_frame) return diff;

  for (int y = 0; y < kHeight; ++y) {
    for (int x = 0; x < kWidth; ++x) {
      const uint32_t index = static_cast<uint32_t>(y) * kWidth + x;
      if (s_pending_frame[index] == s_committed_frame[index]) continue;
      ++diff.pixels;
      if (x < diff.area.x1) diff.area.x1 = x;
      if (y < diff.area.y1) diff.area.y1 = y;
      if (x > diff.area.x2) diff.area.x2 = x;
      if (y > diff.area.y2) diff.area.y2 = y;
    }
  }
  return diff;
}

static void refreshPanel(const char* reason = "unspecified") {
  if (!s_panel || !s_panel_on || !s_panel_dirty) return;

  const FrameDiff frame_diff = calculateFrameDiff();
  if (!s_force_full_refresh && frame_diff.pixels == 0) {
    Serial.printf("[e213][refresh] skip unchanged reason=%s flushes=%lu\n",
                  reason && *reason ? reason : "unspecified",
                  static_cast<unsigned long>(s_flush_count));
    clearDirtyState();
    settleNavigationFrame(false);
    return;
  }

  const uint32_t dirty_width = s_dirty_area.x2 >= s_dirty_area.x1
                                   ? static_cast<uint32_t>(s_dirty_area.x2 - s_dirty_area.x1 + 1)
                                   : 0U;
  const uint32_t dirty_height = s_dirty_area.y2 >= s_dirty_area.y1
                                    ? static_cast<uint32_t>(s_dirty_area.y2 - s_dirty_area.y1 + 1)
                                    : 0U;
  const uint32_t dirty_pixels = dirty_width * dirty_height;
  const bool large_change =
      E213_LVGL_FULL_REFRESH_AREA_PCT > 0 &&
      dirty_pixels * 100U >=
          kFullBufferPixels * static_cast<uint32_t>(E213_LVGL_FULL_REFRESH_AREA_PCT);
  const bool interval_full_due =
      E213_LVGL_FULL_REFRESH_INTERVAL > 0 &&
      static_cast<uint32_t>(s_partial_refreshes) + 1U >=
          static_cast<uint32_t>(E213_LVGL_FULL_REFRESH_INTERVAL);
  const bool navigation_refresh = isNavigationRefreshReason(reason);
  const bool interval_full_deferred = interval_full_due && navigation_refresh;
  const bool full_refresh =
      s_force_full_refresh || large_change ||
      (interval_full_due && !navigation_refresh);
  const char* full_cause = s_force_full_refresh
                               ? "forced"
                               : (large_change
                                      ? "area"
                                      : (full_refresh ? "interval" : "none"));
  if (interval_full_deferred) {
    Serial.printf("[e213][maintenance] FULL deferred reason=%s partials=%u\n",
                  reason && *reason ? reason : "unspecified",
                  static_cast<unsigned>(s_partial_refreshes));
  }
  if (full_refresh) {
    s_panel->fastmodeOff();
    s_fast_mode_ready = false;
  } else if (!s_fast_mode_ready) {
    s_panel->fastmodeOn(false);
    s_fast_mode_ready = true;
  }

  const uint32_t now_ms = millis();
  const uint32_t interval_ms = s_refresh_count == 0 ? 0 : now_ms - s_last_refresh_ms;
  ++s_refresh_count;
  s_last_refresh_ms = now_ms;
  Serial.printf("[e213][refresh] #%lu %s reason=%s cause=%s interval=%lums flushes=%lu partials=%u lvgl_area=(%d,%d)-(%d,%d) lvgl_coverage=%lu%% changed=%lu changed_area=(%d,%d)-(%d,%d) changed_coverage=%lu%% maintenance_due=%u nav=%u\n",
                static_cast<unsigned long>(s_refresh_count),
                full_refresh ? "FULL" : "PARTIAL",
                reason && *reason ? reason : "unspecified",
                full_cause,
                static_cast<unsigned long>(interval_ms),
                static_cast<unsigned long>(s_flush_count),
                static_cast<unsigned>(s_partial_refreshes),
                static_cast<int>(s_dirty_area.x1), static_cast<int>(s_dirty_area.y1),
                static_cast<int>(s_dirty_area.x2), static_cast<int>(s_dirty_area.y2),
                static_cast<unsigned long>((dirty_pixels * 100U) / kFullBufferPixels),
                static_cast<unsigned long>(frame_diff.pixels),
                static_cast<int>(frame_diff.area.x1), static_cast<int>(frame_diff.area.y1),
                static_cast<int>(frame_diff.area.x2), static_cast<int>(frame_diff.area.y2),
                static_cast<unsigned long>((frame_diff.pixels * 100U) / kFullBufferPixels),
                interval_full_due ? 1U : 0U, navigation_refresh ? 1U : 0U);
  const uint32_t update_started_ms = millis();
  s_panel->update();
  Serial.printf("[e213][refresh] #%lu done duration=%lums\n",
                static_cast<unsigned long>(s_refresh_count),
                static_cast<unsigned long>(millis() - update_started_ms));
  if (s_pending_frame && s_committed_frame) {
    memcpy(s_committed_frame, s_pending_frame, kFullBufferPixels);
  }
  clearDirtyState();
  s_force_full_refresh = false;
  settleNavigationFrame(true);
  if (full_refresh) {
    s_partial_refreshes = 0;
    s_panel->fastmodeOn(false);
    s_fast_mode_ready = true;
  } else {
    ++s_partial_refreshes;
  }
}

static void commitTimerCallback(lv_timer_t* timer) {
  if (s_navigation_final_wait) {
    // Closing navigation invalidates the frame asynchronously. Do not reuse
    // the previous timer deadline before that final LVGL flush arrives.
    if (timer) lv_timer_pause(timer);
    return;
  }
  if (!s_refresh_pending || !s_panel_dirty) {
    if (timer) lv_timer_pause(timer);
    if (!s_panel_dirty) {
      s_refresh_pending = false;
      settleNavigationFrame(false);
      clearRefreshRequest();
    }
    return;
  }

  const uint32_t quiet_ms = millis() - s_last_flush_ms;
  const RefreshRequest request = s_refresh_request;
  const uint32_t request_age_ms =
      request == RefreshRequest::None ? 0U : millis() - s_refresh_request_started_ms;
  if (request != RefreshRequest::None &&
      s_lvgl_flush_cycles <= s_refresh_request_after_cycle) {
    return;
  }
  if (request == RefreshRequest::Critical &&
      quiet_ms < E213_LVGL_REFRESH_QUIET_MS &&
      request_age_ms < E213_LVGL_CRITICAL_REFRESH_MAX_WAIT_MS) {
    return;
  }
  if (request == RefreshRequest::Deferred &&
      request_age_ms < E213_LVGL_DEFERRED_REFRESH_MAX_WAIT_MS) {
    return;
  }
#if !E213_LVGL_AUTO_REFRESH
  if (request == RefreshRequest::None) {
    if (timer) lv_timer_pause(timer);
    return;
  }
#else
  if (request == RefreshRequest::None &&
      quiet_ms < E213_LVGL_REFRESH_QUIET_MS) {
    return;
  }
#endif

  // Pause before the blocking panel update. A later LVGL flush will resume
  // the timer and schedule another commit.
  const char* reason = s_refresh_reason[0] ? s_refresh_reason : "unspecified";
  s_refresh_pending = false;
  if (timer) lv_timer_pause(timer);
  Serial.printf("[e213][commit] mode=%s quiet=%lums age=%lums requests=%lu\n",
                refreshRequestName(request), static_cast<unsigned long>(quiet_ms),
                static_cast<unsigned long>(request_age_ms),
                static_cast<unsigned long>(s_refresh_request_count));
  refreshPanel(reason);
  clearRefreshRequest();
}

static void schedulePanelCommit() {
  if (s_navigation_active && s_navigation_hold &&
      s_refresh_request != RefreshRequest::Critical) {
    // LVGL has completed rendering, but this is an intermediate navigation
    // frame. The pixels remain available in s_pending_frame for the next
    // semantic commit.
    s_refresh_pending = false;
    if (s_commit_timer) lv_timer_pause(s_commit_timer);
    return;
  }

  s_refresh_pending = true;
  s_last_flush_ms = millis();
  s_navigation_final_wait = false;
  if (!s_commit_timer) {
    const char* reason = s_refresh_reason[0] ? s_refresh_reason : "unspecified";
    refreshPanel(reason);
    clearRefreshRequest();
    s_refresh_pending = false;
    return;
  }
#if !E213_LVGL_AUTO_REFRESH
  if (s_refresh_request == RefreshRequest::None) {
    Serial.printf("[e213][commit] held pending=1 cycles=%lu\n",
                  static_cast<unsigned long>(s_lvgl_flush_cycles));
    lv_timer_pause(s_commit_timer);
    return;
  }
#endif
  lv_timer_reset(s_commit_timer);
  lv_timer_resume(s_commit_timer);
}

static void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* colors) {
  if (!driver || !area || !colors || !s_panel) {
    if (driver) lv_disp_flush_ready(driver);
    return;
  }

  const int32_t source_width = area->x2 - area->x1 + 1;
  bool drew_pixels = false;
  bool changed_pixels = false;
  for (int32_t y = area->y1; y <= area->y2; ++y) {
    if (y < 0 || y >= kHeight) continue;
    for (int32_t x = area->x1; x <= area->x2; ++x) {
      if (x < 0 || x >= kWidth) continue;
      const uint32_t source_index =
          static_cast<uint32_t>(y - area->y1) * static_cast<uint32_t>(source_width) +
          static_cast<uint32_t>(x - area->x1);
      const bool white = lv_color_brightness(colors[source_index]) >= 128;
      s_panel->drawPixel(static_cast<int16_t>(x), static_cast<int16_t>(y), white ? WHITE : BLACK);
      const uint32_t frame_index = static_cast<uint32_t>(y) * static_cast<uint32_t>(kWidth) +
                                   static_cast<uint32_t>(x);
      if (s_pending_frame) {
        if (s_pending_frame[frame_index] != static_cast<uint8_t>(white)) changed_pixels = true;
        s_pending_frame[frame_index] = static_cast<uint8_t>(white);
      }
      drew_pixels = true;
    }
  }

  if (changed_pixels) s_panel_dirty = true;
  if (changed_pixels) {
    if (area->x1 < s_dirty_area.x1) s_dirty_area.x1 = area->x1;
    if (area->y1 < s_dirty_area.y1) s_dirty_area.y1 = area->y1;
    if (area->x2 > s_dirty_area.x2) s_dirty_area.x2 = area->x2;
    if (area->y2 > s_dirty_area.y2) s_dirty_area.y2 = area->y2;
  }
  if (drew_pixels) {
    ++s_flush_count;
    ++s_lvgl_flush_total;
  }

  // LVGL can produce several refresh cycles while completing one semantic UI
  // transition. Retain every pixel in the panel cache and let the configured
  // policy decide whether the final flush arms a panel commit.
  if (lv_disp_flush_is_last(driver)) {
    ++s_lvgl_flush_cycles;
    Serial.printf("[e213][flush] cycle=%lu total=%lu area=(%d,%d)-(%d,%d) changed=%u dirty=%u request=%s hold=%u\n",
                  static_cast<unsigned long>(s_lvgl_flush_cycles),
                  static_cast<unsigned long>(s_lvgl_flush_total),
                  static_cast<int>(area->x1), static_cast<int>(area->y1),
                  static_cast<int>(area->x2), static_cast<int>(area->y2),
                  changed_pixels ? 1U : 0U, s_panel_dirty ? 1U : 0U,
                  refreshRequestName(s_refresh_request),
                  s_navigation_hold ? 1U : 0U);
    schedulePanelCommit();
  }
  lv_disp_flush_ready(driver);
}

static bool initPanel() {
  if (s_panel) {
    powerOn();
    s_panel_on = true;
    s_force_full_refresh = true;
    s_fast_mode_ready = false;
    s_panel_dirty = true;
    s_navigation_active = false;
    s_navigation_first_pending = false;
    s_navigation_hold = false;
    s_navigation_final_wait = false;
    s_navigation_commit_completed = false;
    clearRefreshRequest();
    return true;
  }

  powerOn();
  s_panel = detectPanel();
  if (!s_panel) {
    powerOff();
    return false;
  }

  s_panel->begin();
  s_panel->setRotation(3);
  if (s_panel->width() != kWidth || s_panel->height() != kHeight) {
    delete s_panel;
    s_panel = nullptr;
    powerOff();
    return false;
  }

  // Initialize the panel driver's unavoidable 1-bit cache without performing
  // an extra visible full refresh. LVGL supplies the first visible frame.
  s_panel->clearMemory();
  s_panel_on = true;
  s_panel_dirty = false;
#if E213_LVGL_STARTUP_FULL_REFRESH
  s_force_full_refresh = true;
  s_fast_mode_ready = false;
#else
  // Partial refresh compares the controller's old and new frame memories.
  // Seed both with white before the first LVGL frame so startup can use the
  // differential waveform without missing pixels.
  s_panel->fastmodeOn(true);
  s_force_full_refresh = false;
  s_fast_mode_ready = true;
#endif
  s_partial_refreshes = 0;
  s_navigation_active = false;
  s_navigation_first_pending = false;
  s_navigation_hold = false;
  s_navigation_final_wait = false;
  s_navigation_commit_completed = false;
  clearRefreshRequest();
  return true;
}

}  // namespace

bool init() {
  if (s_lv_display) return true;
  if (!initPanel()) return false;

  s_draw_buffer = static_cast<lv_color_t*>(
      lv_mem_psram_alloc(kFullBufferPixels * sizeof(lv_color_t)));
  if (!s_draw_buffer) {
    powerOff();
    s_panel_on = false;
    return false;
  }
  s_pending_frame = static_cast<uint8_t*>(lv_mem_psram_alloc(kFullBufferPixels));
  s_committed_frame = static_cast<uint8_t*>(lv_mem_psram_alloc(kFullBufferPixels));
  if (!s_pending_frame || !s_committed_frame) {
    if (s_pending_frame) lv_mem_psram_free(s_pending_frame);
    if (s_committed_frame) lv_mem_psram_free(s_committed_frame);
    s_pending_frame = nullptr;
    s_committed_frame = nullptr;
    lv_mem_psram_free(s_draw_buffer);
    s_draw_buffer = nullptr;
    powerOff();
    s_panel_on = false;
    return false;
  }
  memset(s_pending_frame, 1, kFullBufferPixels);
  memset(s_committed_frame, 1, kFullBufferPixels);

  lv_disp_draw_buf_init(&s_lv_draw_buffer, s_draw_buffer, nullptr, kFullBufferPixels);
  lv_disp_drv_init(&s_lv_driver);
  s_lv_driver.hor_res = kWidth;
  s_lv_driver.ver_res = kHeight;
  s_lv_driver.draw_buf = &s_lv_draw_buffer;
  // The buffer is screen-sized, but full_refresh stays disabled. LVGL renders
  // and flushes only invalidated regions instead of redrawing the whole screen.
  s_lv_driver.full_refresh = 0;
  s_lv_driver.direct_mode = 0;
  s_lv_driver.flush_cb = flush;
  s_lv_display = lv_disp_drv_register(&s_lv_driver);
  if (!s_lv_display) {
    lv_mem_psram_free(s_draw_buffer);
    s_draw_buffer = nullptr;
    return false;
  }
  s_commit_timer = lv_timer_create(commitTimerCallback, 20U, nullptr);
  if (s_commit_timer) lv_timer_pause(s_commit_timer);
  Serial.printf("[e213][refresh] policy auto=%u startup_full=%u quiet=%u critical_max=%u deferred=%u full_interval=%u\n",
                static_cast<unsigned>(E213_LVGL_AUTO_REFRESH),
                static_cast<unsigned>(E213_LVGL_STARTUP_FULL_REFRESH),
                static_cast<unsigned>(E213_LVGL_REFRESH_QUIET_MS),
                static_cast<unsigned>(E213_LVGL_CRITICAL_REFRESH_MAX_WAIT_MS),
                static_cast<unsigned>(E213_LVGL_DEFERRED_REFRESH_MAX_WAIT_MS),
                static_cast<unsigned>(E213_LVGL_FULL_REFRESH_INTERVAL));
  return true;
}

void deinit() {
  if (s_commit_timer) {
    lv_timer_del(s_commit_timer);
    s_commit_timer = nullptr;
  }
  s_refresh_pending = false;
  s_navigation_active = false;
  s_navigation_first_pending = false;
  s_navigation_hold = false;
  s_navigation_final_wait = false;
  s_navigation_commit_completed = false;
  clearRefreshRequest();
  if (s_lv_display) {
    lv_disp_remove(s_lv_display);
    s_lv_display = nullptr;
  }
  if (s_draw_buffer) {
    lv_mem_psram_free(s_draw_buffer);
    s_draw_buffer = nullptr;
  }
  if (s_pending_frame) {
    lv_mem_psram_free(s_pending_frame);
    s_pending_frame = nullptr;
  }
  if (s_committed_frame) {
    lv_mem_psram_free(s_committed_frame);
    s_committed_frame = nullptr;
  }
  if (s_panel_on) {
    refreshPanel("deinit");
    powerOff();
  }
  s_panel_on = false;
}

void setBacklightOn(bool on) {
  if (!s_panel || on == s_panel_on) return;

  if (!on) {
    refreshPanel("power-off");
    powerOff();
    s_panel_on = false;
    s_fast_mode_ready = false;
    return;
  }

  powerOn();
  s_panel_on = true;
  s_force_full_refresh = true;
  s_fast_mode_ready = false;
  s_panel_dirty = true;
  refreshPanel("power-on");
}

bool isBacklightOn() { return s_panel_on; }

void reinitPanelAfterSharedReset() {
  if (!s_panel || !s_panel_on) return;
  s_force_full_refresh = true;
  s_fast_mode_ready = false;
  s_panel_dirty = true;
  refreshPanel("shared-reset");
}

bool commitBootFrame(const char* reason, bool prepare_white_partial_baseline) {
  if (!s_panel || !s_panel_on || !s_lv_display) return false;

  const char* commit_reason = reason && *reason ? reason : "boot-frame";
  const uint32_t before = s_refresh_count;

  if (prepare_white_partial_baseline) {
    Serial.println("[e213][boot] prepare white PARTIAL baseline");

    // Seed both controller image memories with black without activating the
    // panel. The following partial update then has a known black reference and
    // visibly drives every pixel to white without showing an intermediate
    // black frame or invoking the multi-stage full-refresh waveform.
    s_panel->fastmodeOff();
    s_panel->setBackgroundColor(BLACK);
    s_panel->clearMemory();
    s_panel->fastmodeOn(false);
    s_panel->setBackgroundColor(WHITE);
    s_fast_mode_ready = true;

    s_force_full_refresh = false;
    s_panel->fillScreen(WHITE);
    if (s_pending_frame) memset(s_pending_frame, 1, kFullBufferPixels);
    if (s_committed_frame) memset(s_committed_frame, 0, kFullBufferPixels);
    s_panel_dirty = true;
    s_dirty_area = {0, 0, kWidth - 1, kHeight - 1};
    refreshPanel("splash-baseline");
  }

  Serial.printf("[e213][boot] render and commit PARTIAL reason=%s\n",
                commit_reason);

  // The Splash object is complete and stable here, so bypass the normal quiet
  // window. Waiting for the asynchronous timer would let its stack-owned
  // object be destroyed before the panel ever receives this frame.
  lv_refr_now(s_lv_display);
  if (s_commit_timer) lv_timer_pause(s_commit_timer);
  s_refresh_pending = false;
  refreshPanel(commit_reason);
  clearRefreshRequest();

  const bool committed = s_refresh_count != before;
  Serial.printf("[e213][boot] commit reason=%s result=%s\n",
                commit_reason, committed ? "updated" : "unchanged");
  return committed;
}

void beginNavigationSession() {
  if (!s_panel || !s_panel_on) return;
  s_navigation_active = true;
  s_navigation_first_pending = true;
  s_navigation_hold = false;
  s_navigation_final_wait = false;
  s_navigation_commit_completed = false;
  Serial.println("[e213][nav] begin; next settled frame uses normal refresh policy");
  requestRefresh("nav-open");
}

void navigationActivity() {
  if (!s_navigation_active || !s_panel || !s_panel_on) return;
  // Release the cache hold for one settled position. Any number of LVGL
  // flushes generated before the quiet window expires are coalesced.
  s_navigation_first_pending = true;
  s_navigation_hold = false;
  s_navigation_commit_completed = false;
  Serial.println("[e213][nav] activity; coalescing until quiet");
  requestRefresh("nav-activity");
}

void endNavigationSession() {
  if (!s_navigation_active || !s_panel) return;
  s_navigation_active = false;
  s_navigation_first_pending = false;
  s_navigation_hold = false;
  s_navigation_final_wait = true;
  s_navigation_commit_completed = false;
  if (!s_panel_on) return;
  if (s_commit_timer) lv_timer_pause(s_commit_timer);
  Serial.println("[e213][nav] end; waiting for final LVGL frame");
  requestRefresh("nav-close");
}

bool navigationRefreshPending() {
  return s_navigation_active && s_navigation_first_pending;
}

bool consumeNavigationCommitCompleted() {
  const bool completed = s_navigation_commit_completed;
  s_navigation_commit_completed = false;
  return completed;
}

static void queueRefresh(RefreshRequest request, const char* reason) {
  if (!s_panel || !s_panel_on) return;
  const uint32_t now_ms = millis();
  if (request == RefreshRequest::Critical ||
      s_refresh_request == RefreshRequest::None) {
    s_refresh_request_started_ms = now_ms;
  }
  if (static_cast<uint8_t>(request) >= static_cast<uint8_t>(s_refresh_request)) {
    s_refresh_request = request;
    s_refresh_request_after_cycle = s_lvgl_flush_cycles;
    snprintf(s_refresh_reason, sizeof(s_refresh_reason), "%s",
             reason && *reason ? reason : "unspecified");
  }
  ++s_refresh_request_count;
  Serial.printf("[e213][refresh] request mode=%s reason=%s pending=%u dirty=%u after_cycle=%lu coalesced=%lu\n",
                refreshRequestName(s_refresh_request),
                s_refresh_reason[0] ? s_refresh_reason : "unspecified",
                s_refresh_pending ? 1U : 0U, s_panel_dirty ? 1U : 0U,
                static_cast<unsigned long>(s_refresh_request_after_cycle),
                static_cast<unsigned long>(s_refresh_request_count));
  // A request made after LVGL's final flush resumes a paused commit. Requests
  // made before the flush are armed by schedulePanelCommit.
  if (s_refresh_pending && s_commit_timer) {
    lv_timer_reset(s_commit_timer);
    lv_timer_resume(s_commit_timer);
  }
}

void requestRefresh(const char* reason) {
  queueRefresh(RefreshRequest::Critical, reason);
}

void requestSelectionRefresh(const char* reason) {
  if (s_navigation_active) {
    Serial.printf("[e213][selection] coalesced by navigation reason=%s\n",
                  reason && *reason ? reason : "selection-change");
    return;
  }
  requestRefresh(reason && *reason ? reason : "selection-change");
}

void requestDeferredRefresh(const char* reason) {
  queueRefresh(RefreshRequest::Deferred, reason);
}

}  // namespace heltec::meshcore::dal::e213
