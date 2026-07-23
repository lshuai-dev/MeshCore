#pragma once

#include <Arduino.h>
#include <stdint.h>

#define HT_LICENSE_WORDS 4U

#if defined(NRF52_PLATFORM) || defined(ESP32) || defined(ESP_PLATFORM)
#include <helpers/IdentityStore.h>
/** Bind filesystem volume for CDKEY file storage. */
void heltecLicenseSetFilesystem(FILESYSTEM* fs);
bool heltecLicenseClearStored();
#endif

uint64_t heltecLicenseGetChipId64();
void heltecLicenseFormatChipIdHex(char out13[13]);
void heltecLicenseFormatSerialLine(char* out, size_t out_len);
int heltecLicenseCalRtc(const uint32_t license[HT_LICENSE_WORDS]);
bool heltecLicenseReadStored(uint32_t license[HT_LICENSE_WORDS]);
bool heltecLicenseWriteStored(const uint32_t license[HT_LICENSE_WORDS]);
bool heltecLicenseIsAuthorized();
bool heltecLicenseParseCdkeyHex(const char* hex32, uint32_t license[HT_LICENSE_WORDS]);
bool heltecLicenseTryCdkeyCommand(const char* line);

#if defined(HELTEC_MESH_UI) && HELTEC_MESH_UI
/** 1: block boot until CDKEY authorized; 0: skip boot authorization gate. */
#ifndef HELTEC_LICENSE_BOOT_GATE
#define HELTEC_LICENSE_BOOT_GATE 1
#endif
bool heltecLicenseBootGate(bool has_display);
void heltecLicenseUiTick();
#endif
