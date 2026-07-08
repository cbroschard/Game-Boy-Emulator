// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "Cartridge.h"

Cartridge::Cartridge() :
    cartridgeType(CartridgeType::ROMOnly),
    mapperType(MapperType::ROMOnly),
    hasRAM(false),
    ramEnabled(false),
    hasBattery(false),
    hasTimer(false),
    hasRumble(false),
    selectedROMBank(1),
    selectedRAMBank(0),
    bankingMode(0)
{

}

Cartridge::~Cartridge()
{

}

void Cartridge::reset()
{
    selectedROMBank = 1;
    selectedRAMBank = 0;
    ramEnabled      = false;
    bankingMode     = 0;
}

void Cartridge::saveState(StateWriter& wrtr) const
{
    wrtr.beginChunk("CART");

    // Version
    wrtr.writeU32(1);

    wrtr.writeU8(static_cast<uint8_t>(cartridgeType));
    wrtr.writeU8(static_cast<uint8_t>(mapperType));

    wrtr.writeBool(hasRAM);
    wrtr.writeBool(ramEnabled);
    wrtr.writeBool(hasBattery);
    wrtr.writeBool(hasTimer);
    wrtr.writeBool(hasRumble);

    wrtr.writeU16(selectedROMBank);
    wrtr.writeU8(selectedRAMBank);
    wrtr.writeU8(bankingMode);

    wrtr.writeVectorU8(cartridgeROM);
    wrtr.writeVectorU8(cartridgeRAM);

    wrtr.endChunk();
}

bool Cartridge::loadState(const StateReader::Chunk& chunk, StateReader& rdr)
{
    rdr.enterChunkPayload(chunk);

    if (std::memcmp(chunk.tag, "CART", 4) != 0)     { rdr.exitChunkPayload(chunk); return false; }

    uint32_t version = 0;
    if (!rdr.readU32(version))                      { rdr.exitChunkPayload(chunk); return false; }
    if (version != 1)                               { rdr.exitChunkPayload(chunk); return false; }

    uint8_t cartType = 0;
    if (!rdr.readU8(cartType))                      { rdr.exitChunkPayload(chunk); return false; }
    cartridgeType = static_cast<CartridgeType>(cartType);

    uint8_t mapType = 0;
    if (!rdr.readU8(mapType))                       { rdr.exitChunkPayload(chunk); return false; }
    mapperType = static_cast<MapperType>(mapType);

    if (!rdr.readBool(hasRAM))                      { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(ramEnabled))                  { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(hasBattery))                  { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(hasTimer))                    { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readBool(hasRumble))                   { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readU16(selectedROMBank))              { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(selectedRAMBank))               { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readU8(bankingMode))                   { rdr.exitChunkPayload(chunk); return false; }

    if (!rdr.readVectorU8(cartridgeROM))            { rdr.exitChunkPayload(chunk); return false; }
    if (!rdr.readVectorU8(cartridgeRAM))            { rdr.exitChunkPayload(chunk); return false; }

    rdr.exitChunkPayload(chunk);

    return true;
}

bool Cartridge::loadCartridge(const std::string& path)
{
    // Reset state
    cartridgeROM.clear();
    cartridgeRAM.clear();

    loadFile(path, cartridgeROM);

    if (cartridgeROM.size() < 0x150)
        throw std::runtime_error("Cartridge file is too small to contain a valid header.");

    // Load the data and identify
    std::memcpy(&cartridgeHeader, &cartridgeROM[0x0100], sizeof(CartridgeHeader));

    const uint8_t type = cartridgeHeader.cartridgeType;
    const uint8_t romSizeCode = cartridgeHeader.romSizeCode;
    const uint8_t ramSizeCode = cartridgeHeader.ramSizeCode;

    determineCartridgeType(type);

    std::cout
    << std::hex
    << "Cartridge type=$" << int(type)
    << " ROM size code=$" << int(romSizeCode)
    << " RAM size code=$" << int(ramSizeCode)
    << "\n";

    const size_t expectedROMSize = getROMSizeFromCode(romSizeCode);

    if (expectedROMSize == 0)
        throw std::runtime_error("Unsupported cartridge ROM size code.");

    if (cartridgeROM.size() < expectedROMSize)
        throw std::runtime_error("Cartridge ROM file is smaller than header-declared ROM size.");

    const size_t expectedRAMSize = getRAMSizeFromCode(ramSizeCode);

    cartridgeRAM.assign(expectedRAMSize, 0xFF);

    reset();

    return true;
}

uint8_t Cartridge::readROM(uint16_t address)
{
    if (cartridgeROM.empty())
        return 0xFF;

    // Fixed ROM bank 0: $0000-$3FFF
    // This must work for ROMOnly, MBC1, MBC3, MBC5, etc.
    if (address <= 0x3FFF)
    {
        if (address >= cartridgeROM.size())
            return 0xFF;

        return cartridgeROM[address];
    }

    // Switchable ROM bank area: $4000-$7FFF
    if (address <= 0x7FFF)
    {
        switch (mapperType)
        {
            case MapperType::ROMOnly:
            {
                if (address >= cartridgeROM.size())
                    return 0xFF;

                return cartridgeROM[address];
            }

            case MapperType::MBC1:
            {
                const size_t bankOffset = size_t(selectedROMBank) * 0x4000;
                const size_t romOffset = bankOffset + (address - 0x4000);

                if (romOffset >= cartridgeROM.size())
                    return 0xFF;

                return cartridgeROM[romOffset];
            }

            case MapperType::MBC3:
            case MapperType::MBC5:
            {
                // Temporary simple support so boot can complete.
                // Full MBC3/MBC5 write handling can come later.
                const size_t bankOffset = size_t(selectedROMBank) * 0x4000;
                const size_t romOffset = bankOffset + (address - 0x4000);

                if (romOffset >= cartridgeROM.size())
                    return 0xFF;

                return cartridgeROM[romOffset];
            }

            default:
                return 0xFF;
        }
    }

    return 0xFF;
}

void Cartridge::writeROM(uint16_t address, uint8_t value)
{
    switch (mapperType)
    {
        case MapperType::MBC1:
        {
            if (address <= 0x1FFF)
            {
                ramEnabled = ((value & 0x0F) == 0x0A);
            }
            else if (address <= 0x3FFF)
            {
                selectedROMBank = value & 0x1F;

                if (selectedROMBank == 0)
                    selectedROMBank = 1;
            }
            else if (address <= 0x5FFF)
            {
                selectedRAMBank = value & 0x03;
            }
            else if (address <= 0x7FFF)
            {
                bankingMode = value & 0x01;
            }

            break;
        }

        case MapperType::MBC3:
        {
            if (address <= 0x1FFF)
            {
                ramEnabled = ((value & 0x0F) == 0x0A);
            }
            else if (address <= 0x3FFF)
            {
                selectedROMBank = value & 0x7F;

                if (selectedROMBank == 0)
                    selectedROMBank = 1;
            }
            else if (address <= 0x5FFF)
            {
                selectedRAMBank = value & 0x0F;
            }
            else if (address <= 0x7FFF)
            {
                // RTC latch area for MBC3.
            }

            break;
        }

        case MapperType::MBC5:
        {
            if (address <= 0x1FFF)
            {
                ramEnabled = ((value & 0x0F) == 0x0A);
            }
            else if (address <= 0x2FFF)
            {
                selectedROMBank = (selectedROMBank & 0x100) | value;
            }
            else if (address <= 0x3FFF)
            {
                selectedROMBank = (selectedROMBank & 0x0FF) | ((value & 0x01) << 8);
            }
            else if (address <= 0x5FFF)
            {
                selectedRAMBank = value & 0x0F;
            }

            break;
        }

        default:
            break;
    }
}

uint8_t Cartridge::readRAM(uint16_t offset)
{
    if (!isRAMAccessible())
        return 0xFF;

    const size_t effectiveRAMOffset =
        size_t(selectedRAMBank) * 0x2000 + offset;

    if (effectiveRAMOffset >= cartridgeRAM.size())
        return 0xFF;

    return cartridgeRAM[effectiveRAMOffset];
}

void Cartridge::writeRAM(uint16_t offset, uint8_t value)
{
    if (!isRAMAccessible())
        return;

    const size_t effectiveRAMOffset =
        size_t(selectedRAMBank) * 0x2000 + offset;

    if (effectiveRAMOffset >= cartridgeRAM.size())
        return;

    cartridgeRAM[effectiveRAMOffset] = value;
}

bool Cartridge::loadFile(const std::string& path, std::vector<uint8_t>& buffer)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file.is_open())
        throw std::runtime_error("Unable to open Cartridge file!");

    std::streamsize size = file.tellg();

    if (size <= 0)
        throw std::runtime_error("Cartridge file is empty.");

    file.seekg(0, std::ios::beg);

    buffer.resize(static_cast<size_t>(size));

    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        throw std::runtime_error("Unable to read Cartridge file data!");

    return true;
}

void Cartridge::determineCartridgeType(uint8_t rawType)
{
    cartridgeType = static_cast<CartridgeType>(rawType);

    mapperType = MapperType::Unsupported;
    hasRAM = false;
    hasBattery = false;
    hasTimer = false;
    hasRumble = false;

    switch (rawType)
    {
        case 0x00:
            mapperType = MapperType::ROMOnly;
            break;

        case 0x01:
            mapperType = MapperType::MBC1;
            break;

        case 0x02:
            mapperType = MapperType::MBC1;
            hasRAM = true;
            break;

        case 0x03:
            mapperType = MapperType::MBC1;
            hasRAM = true;
            hasBattery = true;
            break;

        case 0x05:
            mapperType = MapperType::MBC2;
            break;

        case 0x06:
            mapperType = MapperType::MBC2;
            hasBattery = true;
            break;

        case 0x08:
            mapperType = MapperType::ROMOnly;
            hasRAM = true;
            break;

        case 0x09:
            mapperType = MapperType::ROMOnly;
            hasRAM = true;
            hasBattery = true;
            break;

        case 0x0B:
            mapperType = MapperType::MMM01;
            break;

        case 0x0C:
            mapperType = MapperType::MMM01;
            hasRAM = true;
            break;

        case 0x0D:
            mapperType = MapperType::MMM01;
            hasRAM = true;
            hasBattery = true;
            break;

        case 0x0F:
            mapperType = MapperType::MBC3;
            hasTimer = true;
            hasBattery = true;
            break;

        case 0x10:
            mapperType = MapperType::MBC3;
            hasTimer = true;
            hasRAM = true;
            hasBattery = true;
            break;

        case 0x11:
            mapperType = MapperType::MBC3;
            break;

        case 0x12:
            mapperType = MapperType::MBC3;
            hasRAM = true;
            break;

        case 0x13:
            mapperType = MapperType::MBC3;
            hasRAM = true;
            hasBattery = true;
            break;

        case 0x19:
            mapperType = MapperType::MBC5;
            break;

        case 0x1A:
            mapperType = MapperType::MBC5;
            hasRAM = true;
            break;

        case 0x1B:
            mapperType = MapperType::MBC5;
            hasRAM = true;
            hasBattery = true;
            break;

        case 0x1C:
            mapperType = MapperType::MBC5;
            hasRumble = true;
            break;

        case 0x1D:
            mapperType = MapperType::MBC5;
            hasRAM = true;
            hasRumble = true;
            break;

        case 0x1E:
            mapperType = MapperType::MBC5;
            hasRAM = true;
            hasBattery = true;
            hasRumble = true;
            break;

        default:
            mapperType = MapperType::Unsupported;
            break;
    }
}

size_t Cartridge::getROMSizeFromCode(uint8_t code) const
{
    switch (code)
    {
        case 0x00: return 32 * 1024;
        case 0x01: return 64 * 1024;
        case 0x02: return 128 * 1024;
        case 0x03: return 256 * 1024;
        case 0x04: return 512 * 1024;
        case 0x05: return 1024 * 1024;
        case 0x06: return 2 * 1024 * 1024;
        case 0x07: return 4 * 1024 * 1024;
        case 0x08: return 8 * 1024 * 1024;

        // Weird/rare official-ish sizes. You can support later.
        case 0x52: return 1152 * 1024;
        case 0x53: return 1280 * 1024;
        case 0x54: return 1536 * 1024;

        default:
            return 0;
    }
}

size_t Cartridge::getRAMSizeFromCode(uint8_t code) const
{
    switch (code)
    {
        case 0x00: return 0;
        case 0x01: return 2 * 1024;
        case 0x02: return 8 * 1024;
        case 0x03: return 32 * 1024;
        case 0x04: return 128 * 1024;
        case 0x05: return 64 * 1024;

        default:
            return 0;
    }
}

bool Cartridge::isRAMAccessible() const
{
    if (!hasRAM)
        return false;

    if (mapperType == MapperType::ROMOnly)
        return true;

    return ramEnabled;
}
