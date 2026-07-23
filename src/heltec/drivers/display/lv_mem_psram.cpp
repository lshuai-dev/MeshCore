#include "lv_mem_psram.h"

#if defined(ESP_PLATFORM)
#include <esp_heap_caps.h>
#endif
#include <stdlib.h>

void* lv_mem_psram_alloc(size_t size) {
  if (size == 0) return nullptr;
#if defined(ESP_PLATFORM)
  void* p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = heap_caps_malloc(size, MALLOC_CAP_8BIT);
  return p;
#else
  return malloc(size);
#endif
}

void lv_mem_psram_free(void* ptr) {
  if (!ptr) return;
#if defined(ESP_PLATFORM)
  heap_caps_free(ptr);
#else
  free(ptr);
#endif
}

void* lv_mem_psram_realloc(void* ptr, size_t size) {
  if (size == 0) {
    lv_mem_psram_free(ptr);
    return nullptr;
  }
#if defined(ESP_PLATFORM)
  void* p = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) p = heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT);
  return p;
#else
  return realloc(ptr, size);
#endif
}
