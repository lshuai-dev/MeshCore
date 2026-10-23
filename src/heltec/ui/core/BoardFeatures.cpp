#include "heltec/core/BoardFeatures.hpp"

namespace Heltec::MeshCore::Board
{
bool __attribute__((weak)) lnaCanControl()
{
    return false;
}
bool __attribute__((weak)) setLnaEnable(bool)
{
    return false;
}
} // namespace Heltec::MeshCore::Board
