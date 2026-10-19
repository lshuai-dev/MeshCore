#pragma once

#define HELTEC_UI_INPUT_KEY      (1U << 0)
#define HELTEC_UI_INPUT_TOUCH    (1U << 1)
#define HELTEC_UI_INPUT_KEYBOARD (1U << 2)

#ifndef HELTEC_UI_INPUT_CAPS
#define HELTEC_UI_INPUT_CAPS HELTEC_UI_INPUT_KEY
#endif

#ifndef HELTEC_UI_BUTTON_COUNT
#define HELTEC_UI_BUTTON_COUNT 1
#endif

#define HELTEC_UI_HAS_KEY \
    ((HELTEC_UI_INPUT_CAPS & HELTEC_UI_INPUT_KEY) != 0U)
#define HELTEC_UI_HAS_TOUCH \
    ((HELTEC_UI_INPUT_CAPS & HELTEC_UI_INPUT_TOUCH) != 0U)
#define HELTEC_UI_HAS_PHYSICAL_KEYBOARD \
    ((HELTEC_UI_INPUT_CAPS & HELTEC_UI_INPUT_KEYBOARD) != 0U)

// Device builds may turn optional UI features off. GPS screen availability is
// derived directly from MeshCore's ENV_INCLUDE_GPS feature flag.
#ifndef HELTEC_UI_ENABLE_KEYBOARD
#define HELTEC_UI_ENABLE_KEYBOARD 1
#endif
