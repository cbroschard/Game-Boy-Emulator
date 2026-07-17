// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MEMORYMAP_H_INCLUDED
#define MEMORYMAP_H_INCLUDED

#include <cstdint>

namespace MemoryMap
{

    constexpr uint16_t BOOT_ROM_START       = 0x0000;
    constexpr uint16_t BOOT_ROM_END         = 0x00FF;

    constexpr uint16_t ROM_FIXED_START      = 0x0000;
    constexpr uint16_t ROM_FIXED_END        = 0x3FFF;

    constexpr uint16_t ROM_SWITCHABLE_START = 0x4000;
    constexpr uint16_t ROM_SWITCHABLE_END   = 0x7FFF;

    constexpr uint16_t VRAM_START           = 0x8000;
    constexpr uint16_t VRAM_END             = 0x9FFF;

    constexpr uint16_t EXTERNAL_RAM_START   = 0xA000;
    constexpr uint16_t EXTERNAL_RAM_END     = 0xBFFF;

    constexpr uint16_t WRAM_START           = 0xC000;
    constexpr uint16_t WRAM_END             = 0xDFFF;

    constexpr uint16_t ECHO_RAM_START       = 0xE000;
    constexpr uint16_t ECHO_RAM_END         = 0xFDFF;

    constexpr uint16_t OAM_START            = 0xFE00;
    constexpr uint16_t OAM_END              = 0xFE9F;

    constexpr uint16_t UNUSABLE_START       = 0xFEA0;
    constexpr uint16_t UNUSABLE_END         = 0xFEFF;

    constexpr uint16_t IO_START             = 0xFF00;
    constexpr uint16_t IO_END               = 0xFF7F;

    constexpr uint16_t HRAM_START           = 0xFF80;
    constexpr uint16_t HRAM_END             = 0xFFFE;
}

#endif
