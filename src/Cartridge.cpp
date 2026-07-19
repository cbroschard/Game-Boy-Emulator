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
#include <system_error>
#include "Cartridge.h"

Cartridge::Cartridge() :
    cartridgeType(CartridgeType::ROMOnly),
    mapperType(MapperType::ROMOnly),
    hasRAM(false),
    ramEnabled(false),
    hasBattery(false),
    hasTimer(false),
    hasRumble(false),
    batterySaveDirty(false),
    batteryModificationCounter(0),
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

    batterySaveDirty = false;
    batteryModificationCounter = 0;

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

    size_t expectedRAMSize = getRAMSizeFromCode(ramSizeCode);

    if (mapperType == MapperType::MBC2)
    {
        expectedRAMSize = 512;
        hasRAM = true;
    }

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

            case MapperType::MBC2:
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

        case MapperType::MBC2:
        {
            if (address <= 0x3FFF)
            {
                if ((address & 0x0100) == 0)
                {
                    ramEnabled = ((value & 0x0F) == 0x0A);
                }
                else
                {
                    selectedROMBank = value & 0x0F;

                    if (selectedROMBank == 0)
                        selectedROMBank = 1;
                }
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

    if (mapperType == MapperType::MBC2)
    {
        const size_t index = offset & 0x01FF;
        return 0xF0 | (cartridgeRAM[index] & 0x0F);
    }

    const size_t effectiveRAMOffset = size_t(selectedRAMBank) * 0x2000 + offset;

    if (effectiveRAMOffset >= cartridgeRAM.size())
        return 0xFF;

    return cartridgeRAM[effectiveRAMOffset];
}

void Cartridge::writeRAM(uint16_t offset, uint8_t value)
{
    if (!isRAMAccessible())
        return;

    size_t effectiveRAMOffset = 0;
    uint8_t storedValue = value;

    if (mapperType == MapperType::MBC2)
    {
        effectiveRAMOffset = offset & 0x01FF;
        storedValue = value & 0x0F;
    }
    else
    {
        effectiveRAMOffset = size_t(selectedRAMBank) * 0x2000 + offset;
    }

    if (effectiveRAMOffset >= cartridgeRAM.size())
        return;

    if (cartridgeRAM[effectiveRAMOffset] == storedValue)
        return;

    cartridgeRAM[effectiveRAMOffset] = storedValue;

    if (hasBattery)
    {
        batterySaveDirty = true;
        ++batteryModificationCounter;
    }
}

CartridgeColorSupport Cartridge::getColorSupport() const
{
    if (cartridgeROM.empty())
        return CartridgeColorSupport::DMGOnly;

    switch (cartridgeHeader.cgbFlag)
    {
        case 0x80:
            return CartridgeColorSupport::CGBCompatible;

        case 0xC0:
            return CartridgeColorSupport::CGBOnly;

        default:
            return CartridgeColorSupport::DMGOnly;
    }
}

Cartridge::CartridgeInfo Cartridge::getCartridgeInfo() const
{
    CartridgeInfo info;

    info.loaded = !cartridgeROM.empty();

    if (!info.loaded)
        return info;

    info.title = getCartridgeTitle();
    info.cartridgeType = getCartridgeTypeName();
    info.mapperType = getMapperTypeName();

    info.cartridgeTypeCode = cartridgeHeader.cartridgeType;

    info.romSizeCode = cartridgeHeader.romSizeCode;

    info.ramSizeCode = cartridgeHeader.ramSizeCode;

    info.cgbFlag = cartridgeHeader.cgbFlag;

    info.colorSupport = getColorSupport();

    info.romSizeBytes = cartridgeROM.size();

    info.ramSizeBytes = cartridgeRAM.size();

    info.romBankCount = cartridgeROM.empty() ? 0 : cartridgeROM.size() / 0x4000;

    if (cartridgeRAM.empty())
    {
        info.ramBankCount = 0;
    }
    else if (cartridgeRAM.size() <= 0x2000)
    {
        info.ramBankCount = 1;
    }
    else
    {
        info.ramBankCount =
            cartridgeRAM.size() / 0x2000;
    }

    info.hasRAM = hasRAM;
    info.ramEnabled = ramEnabled;
    info.hasBattery = hasBattery;
    info.hasTimer = hasTimer;
    info.hasRumble = hasRumble;

    info.selectedROMBank = selectedROMBank;

    info.selectedRAMBank = selectedRAMBank;

    info.bankingMode = bankingMode;

    info.sgbFlag = cartridgeHeader.sgbFlag;

    info.destinationCode = cartridgeHeader.destinationCode;

    info.maskROMVersion = cartridgeHeader.maskROMVersion;

    info.headerChecksum = cartridgeHeader.headerChecksum;

    return info;
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

        case 0x20:
        {
            mapperType = MapperType::MBC6;
            hasRAM = true;
            hasBattery = true;
            break;
        }

        case 0x22:
        {
            mapperType = MapperType::MBC7;
            hasRAM = true;
            hasBattery = true;
            hasRumble = true;
            break;
        }

        case 0xFC:
        {
            mapperType = MapperType::Camera;
            hasRAM = true;
            hasBattery = true;
            break;
        }

        case 0xFD:
        {
            mapperType = MapperType::TAMA5;
            hasRAM = true;
            hasBattery = true;
            hasTimer = true;
            break;
        }

        case 0xFE:
        {
            mapperType = MapperType::HuC3;
            hasRAM = true;
            hasBattery = true;
            hasTimer = true;
            break;
        }

        case 0xFF:
        {
            mapperType = MapperType::HuC1;
            hasRAM = true;
            hasBattery = true;
            break;
        }

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

std::string Cartridge::getCartridgeTitle() const
{
    if (cartridgeROM.empty())
        return "";

    std::string title;

    for (std::size_t index = 0;
         index < sizeof(cartridgeHeader.title);
         ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(
                cartridgeHeader.title[index]);

        if (character == 0x00)
            break;

        if (character >= 0x20 &&
            character <= 0x7E)
        {
            title.push_back(
                static_cast<char>(character));
        }
    }

    while (!title.empty() &&
           title.back() == ' ')
    {
        title.pop_back();
    }

    return title;
}

std::string Cartridge::getMapperTypeName() const
{
    switch (mapperType)
    {
        case MapperType::ROMOnly:
            return "ROM Only";

        case MapperType::MBC1:
            return "MBC1";

        case MapperType::MBC2:
            return "MBC2";

        case MapperType::MMM01:
            return "MMM01";

        case MapperType::MBC3:
            return "MBC3";

        case MapperType::MBC5:
            return "MBC5";

        case MapperType::MBC6:
            return "MBC6";

        case MapperType::MBC7:
            return "MBC7";

        case MapperType::HuC1:
            return "HuC1";

        case MapperType::HuC3:
            return "HuC3";

        case MapperType::Camera:
            return "Pocket Camera";

        case MapperType::TAMA5:
            return "Bandai TAMA5";

        case MapperType::Unsupported:
            return "Unsupported";
    }

    return "Unknown";
}

std::string Cartridge::getCartridgeTypeName() const
{
    switch (cartridgeType)
    {
        case CartridgeType::ROMOnly:
            return "ROM Only";

        case CartridgeType::MBC1:
            return "MBC1";

        case CartridgeType::MBC1RAM:
            return "MBC1 + RAM";

        case CartridgeType::MBC1RAMBattery:
            return "MBC1 + RAM + Battery";

        case CartridgeType::MBC2:
            return "MBC2";

        case CartridgeType::MBC2Battery:
            return "MBC2 + Battery";

        case CartridgeType::ROMRAM:
            return "ROM + RAM";

        case CartridgeType::ROMRAMBattery:
            return "ROM + RAM + Battery";

        case CartridgeType::MMM01:
            return "MMM01";

        case CartridgeType::MMM01RAM:
            return "MMM01 + RAM";

        case CartridgeType::MMM01RAMBattery:
            return "MMM01 + RAM + Battery";

        case CartridgeType::MBC3TimerBattery:
            return "MBC3 + Timer + Battery";

        case CartridgeType::MBC3TimerRAMBattery:
            return "MBC3 + Timer + RAM + Battery";

        case CartridgeType::MBC3:
            return "MBC3";

        case CartridgeType::MBC3RAM:
            return "MBC3 + RAM";

        case CartridgeType::MBC3RAMBattery:
            return "MBC3 + RAM + Battery";

        case CartridgeType::MBC5:
            return "MBC5";

        case CartridgeType::MBC5RAM:
            return "MBC5 + RAM";

        case CartridgeType::MBC5RAMBattery:
            return "MBC5 + RAM + Battery";

        case CartridgeType::MBC5Rumble:
            return "MBC5 + Rumble";

        case CartridgeType::MBC5RumbleRAM:
            return "MBC5 + Rumble + RAM";

        case CartridgeType::MBC5RumbleRAMBattery:
            return "MBC5 + Rumble + RAM + Battery";

        case CartridgeType::MBC6:
            return "MBC6";

        case CartridgeType::MBC7SensorRumbleRAMBattery:
            return "MBC7 + Sensor + Rumble + RAM + Battery";

        case CartridgeType::PocketCamera:
            return "Pocket Camera";

        case CartridgeType::BandaiTAMA5:
            return "Bandai TAMA5";

        case CartridgeType::HuC3:
            return "HuC3";

        case CartridgeType::HuC1RAMBattery:
            return "HuC1 + RAM + Battery";
    }

    return "Unknown";
}

bool Cartridge::loadBatterySave(const std::filesystem::path& path)
{
    if (cartridgeRAM.empty())
        return true;

    std::error_code error;

    if (!std::filesystem::exists(path, error))
    {
        if (error)
        {
            std::cerr
                << "Unable to check cartridge persistence file: "
                << error.message()
                << "\n";

            return false;
        }

        // No existing persistence file. This is normal for a new game.
        return true;
    }

    if (!std::filesystem::is_regular_file(path, error) || error)
        return false;

    const std::uintmax_t fileSize = std::filesystem::file_size(path, error);

    if (error)
        return false;

    if (fileSize != cartridgeRAM.size())
    {
        std::cerr
            << "Persistence file size mismatch. Expected "
            << cartridgeRAM.size()
            << " bytes but found "
            << fileSize
            << " bytes.\n";

        return false;
    }

    std::ifstream file(path, std::ios::binary);

    if (!file)
        return false;

    file.read(reinterpret_cast<char*>(cartridgeRAM.data()), static_cast<std::streamsize>(cartridgeRAM.size()));

    return file.good();
}

bool Cartridge::saveBatterySave(const std::filesystem::path& path) const
{
    if (cartridgeRAM.empty())
        return true;

    std::filesystem::path temporaryPath = path;
    temporaryPath += ".tmp";

    {
        std::ofstream file(temporaryPath, std::ios::binary | std::ios::trunc);

        if (!file)
            return false;

        file.write(reinterpret_cast<const char*>(cartridgeRAM.data()), static_cast<std::streamsize>(cartridgeRAM.size()));

        file.flush();

        if (!file.good())
            return false;
    }

    std::error_code error;

    std::filesystem::remove(path, error);
    error.clear();

    std::filesystem::rename(
        temporaryPath,
        path,
        error
    );

    if (error)
    {
        std::filesystem::remove(temporaryPath);
        return false;
    }

    return true;
}

std::string Cartridge::makePersistencePath(const std::string& romPath) const
{
    std::filesystem::path p(romPath);
    p.replace_extension(".cartpersist");
    return p.string();
}
