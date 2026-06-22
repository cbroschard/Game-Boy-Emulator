// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <vector>

class Cartridge
{
    public:
        Cartridge();
        virtual ~Cartridge();

    protected:

    private:
        std::vector<uint8_t> cartridgeROM;
        std::vector<uint8_t> cartridgeRAM;
};

#endif // CARTRIDGE_H
