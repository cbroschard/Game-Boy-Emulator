// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef IOREGISTERS_H_INCLUDED
#define IOREGISTERS_H_INCLUDED

#include <cstdint>

namespace IORegisters
{
    namespace APU
    {
        constexpr uint16_t Start = 0xFF10;
        constexpr uint16_t End   = 0xFF3F;

        constexpr uint16_t NR10 = 0xFF10;
        constexpr uint16_t NR11 = 0xFF11;
        constexpr uint16_t NR12 = 0xFF12;
        constexpr uint16_t NR13 = 0xFF13;
        constexpr uint16_t NR14 = 0xFF14;
        constexpr uint16_t NR15 = 0xFF15;

        constexpr uint16_t NR1F = 0xFF1F;

        constexpr uint16_t NR21 = 0xFF16;
        constexpr uint16_t NR22 = 0xFF17;
        constexpr uint16_t NR23 = 0xFF18;
        constexpr uint16_t NR24 = 0xFF19;

        constexpr uint16_t NR30 = 0xFF1A;
        constexpr uint16_t NR31 = 0xFF1B;
        constexpr uint16_t NR32 = 0xFF1C;
        constexpr uint16_t NR33 = 0xFF1D;
        constexpr uint16_t NR34 = 0xFF1E;

        constexpr uint16_t NR41 = 0xFF20;
        constexpr uint16_t NR42 = 0xFF21;
        constexpr uint16_t NR43 = 0xFF22;
        constexpr uint16_t NR44 = 0xFF23;

        constexpr uint16_t NR50 = 0xFF24;
        constexpr uint16_t NR51 = 0xFF25;
        constexpr uint16_t NR52 = 0xFF26;

        constexpr uint16_t WaveRAMStart = 0xFF30;
        constexpr uint16_t WaveRAMEnd   = 0xFF3F;
    }

    namespace Boot
    {
        constexpr uint16_t Disable = 0xFF50;
    }

    namespace CGB
    {
        constexpr uint16_t KEY0 = 0xFF4C;
        constexpr uint16_t KEY1 = 0xFF4D;
        constexpr uint16_t SVBK = 0xFF70;
    }

    namespace Interrupt
    {
        constexpr uint16_t IF = 0xFF0F;
        constexpr uint16_t IE = 0xFFFF;
    }

    namespace Joypad
    {
        constexpr uint16_t JOYP = 0xFF00;
    }

    namespace PPU
    {
        constexpr uint16_t LCDC = 0xFF40;
        constexpr uint16_t STAT = 0xFF41;
        constexpr uint16_t SCY  = 0xFF42;
        constexpr uint16_t SCX  = 0xFF43;
        constexpr uint16_t LY   = 0xFF44;
        constexpr uint16_t LYC  = 0xFF45;
        constexpr uint16_t DMA  = 0xFF46;
        constexpr uint16_t BGP  = 0xFF47;
        constexpr uint16_t OBP0 = 0xFF48;
        constexpr uint16_t OBP1 = 0xFF49;
        constexpr uint16_t WY   = 0xFF4A;
        constexpr uint16_t WX   = 0xFF4B;

        constexpr uint16_t VBK = 0xFF4F;

        constexpr uint16_t HDMA1 = 0xFF51;
        constexpr uint16_t HDMA2 = 0xFF52;
        constexpr uint16_t HDMA3 = 0xFF53;
        constexpr uint16_t HDMA4 = 0xFF54;
        constexpr uint16_t HDMA5 = 0xFF55;

        constexpr uint16_t BGPI = 0xFF68;
        constexpr uint16_t BGPD = 0xFF69;
        constexpr uint16_t OBPI = 0xFF6A;
        constexpr uint16_t OBPD = 0xFF6B;
        constexpr uint16_t OPRI = 0xFF6C;
    }

    namespace Timer
    {
        constexpr uint16_t DIV  = 0xFF04;
        constexpr uint16_t TIMA = 0xFF05;
        constexpr uint16_t TMA  = 0xFF06;
        constexpr uint16_t TAC  = 0xFF07;
    }
}

#endif // IOREGISTERS_H_INCLUDED
