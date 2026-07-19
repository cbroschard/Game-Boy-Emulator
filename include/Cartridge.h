// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "common/HardwareMode.h"
#include "StateReader.h"
#include "StateWriter.h"

class Cartridge
{
    public:
        Cartridge();
        virtual ~Cartridge();

        struct CartridgeInfo
        {
            bool loaded = false;

            std::string title;
            std::string cartridgeType;
            std::string mapperType;

            uint8_t cartridgeTypeCode = 0;
            uint8_t romSizeCode = 0;
            uint8_t ramSizeCode = 0;

            uint8_t cgbFlag = 0;
            CartridgeColorSupport colorSupport = CartridgeColorSupport::DMGOnly;

            std::size_t romSizeBytes = 0;
            std::size_t ramSizeBytes = 0;

            std::size_t romBankCount = 0;
            std::size_t ramBankCount = 0;

            bool hasRAM = false;
            bool ramEnabled = false;
            bool hasBattery = false;
            bool hasTimer = false;
            bool hasRumble = false;

            uint16_t selectedROMBank = 0;
            uint8_t selectedRAMBank = 0;
            uint8_t bankingMode = 0;

            uint8_t sgbFlag = 0;
            uint8_t destinationCode = 0;
            uint8_t maskROMVersion = 0;
            uint8_t headerChecksum = 0;
        };

        void reset();

        void saveState(StateWriter& wrtr) const;
        bool loadState(const StateReader::Chunk& chunk, StateReader& rdr);

        bool loadCartridge(const std::string& path);
        void unloadCartridge();

        uint8_t readROM(uint16_t offset);
        void writeROM(uint16_t offset, uint8_t value);

        uint8_t readRAM(uint16_t offset);
        void writeRAM(uint16_t offset, uint8_t value);

        inline uint8_t getCGBFlag() const { return cartridgeHeader.cgbFlag; }
        CartridgeColorSupport getColorSupport() const;
        CartridgeInfo getCartridgeInfo() const;

        // Persistence
        inline bool cartridgeHasBattery() const { return hasBattery; }
        bool loadBatterySave(const std::filesystem::path& path);
        bool saveBatterySave(const std::filesystem::path& path) const;
        std::string makePersistencePath(const std::string& romPath) const;

        inline bool isBatterySaveDirty() const { return batterySaveDirty; }
        inline void clearBatterySaveDirty() { batterySaveDirty = false; }
        inline uint64_t getBatteryModificationCounter() const { return batteryModificationCounter; }

        inline void markBatterySaveDirty()
        {
            if (!hasBattery)
                return;

            batterySaveDirty = true;
            ++batteryModificationCounter;
        }

    protected:

    private:
        std::vector<uint8_t> cartridgeROM;
        std::vector<uint8_t> cartridgeRAM;

        static constexpr uint16_t HEADER_START      = 0x0100;
        static constexpr uint16_t CARTRIDGE_TYPE    = 0x0147;
        static constexpr uint16_t ROM_SIZE_CODE     = 0x0148;
        static constexpr uint16_t RAM_SIZE_CODE     = 0x0149;
        static constexpr uint16_t HEADER_CHECKSUM   = 0x014D;

        #pragma pack(push, 1)
        struct CartridgeHeader
        {
            uint8_t entryPoint[4];       // $0100-$0103
            uint8_t nintendoLogo[48];    // $0104-$0133
            char title[15];              // $0134-$0142
            uint8_t cgbFlag;             // $0143

            uint16_t newLicenseeCode;    // $0144-$0145
            uint8_t sgbFlag;             // $0146
            uint8_t cartridgeType;       // $0147
            uint8_t romSizeCode;         // $0148
            uint8_t ramSizeCode;         // $0149
            uint8_t destinationCode;     // $014A
            uint8_t oldLicenseeCode;     // $014B
            uint8_t maskROMVersion;      // $014C
            uint8_t headerChecksum;      // $014D
            uint16_t globalChecksum;     // $014E-$014F
        } cartridgeHeader;
        #pragma pack(pop)

        enum class CartridgeType : uint8_t
        {
            ROMOnly             = 0x00,

            MBC1                = 0x01,
            MBC1RAM             = 0x02,
            MBC1RAMBattery      = 0x03,

            MBC2                = 0x05,
            MBC2Battery         = 0x06,

            ROMRAM              = 0x08,
            ROMRAMBattery       = 0x09,

            MMM01               = 0x0B,
            MMM01RAM            = 0x0C,
            MMM01RAMBattery     = 0x0D,

            MBC3TimerBattery    = 0x0F,
            MBC3TimerRAMBattery = 0x10,
            MBC3                = 0x11,
            MBC3RAM             = 0x12,
            MBC3RAMBattery      = 0x13,

            MBC5                = 0x19,
            MBC5RAM             = 0x1A,
            MBC5RAMBattery      = 0x1B,
            MBC5Rumble          = 0x1C,
            MBC5RumbleRAM       = 0x1D,
            MBC5RumbleRAMBattery= 0x1E,

            MBC6                = 0x20,
            MBC7SensorRumbleRAMBattery = 0x22,

            PocketCamera        = 0xFC,
            BandaiTAMA5         = 0xFD,
            HuC3                = 0xFE,
            HuC1RAMBattery      = 0xFF
        };

        enum class MapperType
        {
            ROMOnly,
            MBC1,
            MBC2,
            MMM01,
            MBC3,
            MBC5,
            MBC6,
            MBC7,
            HuC1,
            HuC3,
            Camera,
            TAMA5,
            Unsupported
        };

        CartridgeType cartridgeType;
        MapperType mapperType;

        bool hasRAM;
        bool ramEnabled;
        bool hasBattery;
        bool hasTimer;
        bool hasRumble;

        bool batterySaveDirty;
        uint64_t batteryModificationCounter;

        uint16_t selectedROMBank;
        uint8_t selectedRAMBank;
        uint8_t bankingMode;

        // Helpers
        bool loadFile(const std::string& path, std::vector<uint8_t>& buffer);

        void determineCartridgeType(uint8_t rawType);

        size_t getROMSizeFromCode(uint8_t code) const;
        size_t getRAMSizeFromCode(uint8_t code) const;

        bool isRAMAccessible() const;

        std::string getCartridgeTypeName() const;
        std::string getMapperTypeName() const;
        std::string getCartridgeTitle() const;
};

#endif // CARTRIDGE_H
