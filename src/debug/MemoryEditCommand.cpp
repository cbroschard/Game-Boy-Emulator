#include "debug/MemoryEditCommand.h"
#include "debug/MLMonitor.h"

MemoryEditCommand::MemoryEditCommand() = default;

MemoryEditCommand::~MemoryEditCommand() = default;

int MemoryEditCommand::order() const
{
    return 1;
}

std::string MemoryEditCommand::name() const
{
    return "e";
}

std::string MemoryEditCommand::category() const
{
    return "Memory";
}

std::string MemoryEditCommand::shortHelp() const
{
    return "e         - Edit memory";
}

std::string MemoryEditCommand::help() const
{
    return
        "e <address> <value> [value...]\n"
        "    Edit memory starting at the specified address.\n"
        "\n"
        "Arguments:\n"
        "    <address>   Starting memory address to edit.\n"
        "                Hex examples: $7000, 0x7000, 7000.\n"
        "    <value>     One or more byte values to write.\n"
        "                Hex examples: $01, 0x01, 01.\n"
        "\n"
        "Examples:\n"
        "    e help          Show this help.\n"
        "    e $7000 01      Write $01 to $7000.\n"
        "    e $7000 01 02   Write $01 to $7000 and $02 to $7001.\n"
        "    e $72DD 01      Write $01 to $72DD.\n"
        "\n";
}

void MemoryEditCommand::execute(MLMonitor& mlMonitor, const std::vector<std::string>& args)
{
    MLMonitorBackend* backend = mlMonitor.getMLMonitorBackend();

    if (!backend)
    {
        std::cout << "Monitor backend is not attached." << std::endl;
        return;
    }

    if (args.size() >= 2 && isHelp(args[1]))
    {
        std::cout << help();
        return;
    }

    if (args.size() < 3)
    {
        std::cout << "Usage:\n" << help();
        return;
    }

    try
    {
        uint16_t address = parseAddress(args[1]);

        for (size_t i = 2; i < args.size(); ++i)
        {
            const uint16_t parsedValue = parseAddress(args[i]);

            if (parsedValue > 0xFF)
            {
                std::cout << "Invalid byte value: " << args[i] << std::endl;
                return;
            }

            const uint8_t value = static_cast<uint8_t>(parsedValue);
            const uint8_t oldValue = backend->readRAM(address);

            backend->writeRAM(address, value);

            const uint8_t newValue = backend->readRAM(address);

            std::cout
                << std::uppercase << std::hex << std::setfill('0')
                << "$" << std::setw(4) << address
                << ": $" << std::setw(2) << static_cast<int>(oldValue)
                << " -> $" << std::setw(2) << static_cast<int>(newValue);

            if (newValue != value)
                std::cout << "  write did not change memory";

            std::cout
                << std::dec << std::setfill(' ')
                << std::nouppercase
                << std::endl;

            address = static_cast<uint16_t>(address + 1);
        }
    }
    catch (const std::exception&)
    {
        std::cout << "Error: Invalid address or value. Usage:\n" << help();
    }
}
