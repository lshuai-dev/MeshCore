#include "heltec/core/board_features.hpp"

namespace heltec::meshcore::board {
bool __attribute__((weak)) lnaCanControl() { return false; }
bool __attribute__((weak)) setLnaEnable(bool) { return false; }
}
