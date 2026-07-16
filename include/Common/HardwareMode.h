#ifndef HARDWAREMODE_H_INCLUDED
#define HARDWAREMODE_H_INCLUDED

#include <cstdint>

enum class HardwareMode : uint8_t
{
    DMG,
    CGB
};

enum class CartridgeColorSupport : uint8_t
{
    DMGOnly,
    CGBCompatible,
    CGBOnly
};

constexpr bool isCGBMode(HardwareMode mode)
{
    return mode == HardwareMode::CGB;
}

constexpr bool isDMGMode(HardwareMode mode)
{
    return mode == HardwareMode::DMG;
}

constexpr bool supportsCGB(CartridgeColorSupport support)
{
    return support == CartridgeColorSupport::CGBCompatible ||
           support == CartridgeColorSupport::CGBOnly;
}

constexpr bool supportsDMG(CartridgeColorSupport support)
{
    return support == CartridgeColorSupport::DMGOnly ||
           support == CartridgeColorSupport::CGBCompatible;
}

#endif // HARDWAREMODE_H_INCLUDED
