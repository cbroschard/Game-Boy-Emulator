// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "debug/MLMonitor.h"
#include "debug/AssembleCommand.h"
#include "debug/CPUCommand.h"
#include "debug/DisassembleCommand.h"
#include "debug/GoCommand.h"
#include "debug/MemoryDumpCommand.h"
#include "debug/MemoryEditCommand.h"

MLMonitor::MLMonitor() :
    mlMonitorBackend(nullptr),
    running(false),
    outputFileEnabled(false)
{
    registerCommand(std::make_unique<AssembleCommand>());
    registerCommand(std::make_unique<CPUCommand>());
    registerCommand(std::make_unique<DisassembleCommand>());
    registerCommand(std::make_unique<GoCommand>());
    registerCommand(std::make_unique<MemoryDumpCommand>());
    registerCommand(std::make_unique<MemoryEditCommand>());
}

MLMonitor::~MLMonitor()
{
    if (outputFile.is_open())
        outputFile.close();
}

void MLMonitor::enterMonitor()
{
    if (mlMonitorBackend) mlMonitorBackend->enterMonitor();
}

std::string MLMonitor::getPrompt() const
{
    const auto iterator = commands.find("a");

    if (iterator != commands.end())
    {
        const AssembleCommand* assembleCommand =
            dynamic_cast<const AssembleCommand*>(
                iterator->second.get());

        if (assembleCommand != nullptr &&
            assembleCommand->isInteractiveActive())
        {
            return assembleCommand->currentPrompt();
        }
    }

    return "> ";
}

std::string MLMonitor::executeAndCapture(const std::string& cmdLine)
{
    std::ostringstream buffer;
    auto* old = std::cout.rdbuf(buffer.rdbuf()); // Redirect std::cout to buffer

    try
    {
        handleCommand(cmdLine);
    }
    catch (const std::exception& ex)
    {
        std::cout << "Error executing command: " << ex.what() << "\n";
    }
    catch (...)
    {
        std::cout << "Error executing command: unknown exception\n";
    }

    std::cout.rdbuf(old); // Restore std::cout

    const std::string out = buffer.str();

    // Tee monitor command + output to file if enabled.
    writeOutputFileBlock(cmdLine, out);

    return out;
}

std::vector<std::string> MLMonitor::drainAsyncLines()
{
    std::lock_guard<std::mutex> lock(asyncMutex);
    std::vector<std::string> out;
    out.swap(asyncLines);
    return out;
}

void MLMonitor::handleCommand(const std::string& line)
{
    /*
     * Interactive assembler input must be handled before normal monitor
     * command parsing.
     *
     * This allows lines such as:
     *
     *     LD A,$01
     *     BIT 7,A
     *     JR NZ,$C020
     *
     * It also allows a blank line or "." to exit interactive assembly.
     */
    auto assembleIt = commands.find("a");

    if (assembleIt != commands.end())
    {
        AssembleCommand* assembleCommand =
            dynamic_cast<AssembleCommand*>(
                assembleIt->second.get());

        if (assembleCommand != nullptr &&
            assembleCommand->isInteractiveActive())
        {
            assembleCommand->handleInteractiveLine(
                *this,
                line);

            return;
        }
    }

    std::istringstream iss(line);

    std::string cmd;
    iss >> cmd;

    // Blank commands outside interactive assembly mode do nothing.
    if (cmd.empty())
        return;

    // Normalize the monitor command name to lowercase.
    std::transform(
        cmd.begin(),
        cmd.end(),
        cmd.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(
                std::tolower(c));
        });

    if (cmd == "exit" ||
        cmd == "q" ||
        cmd == "quit")
    {
        running = false;
        return;
    }

    std::vector<std::string> args;
    args.push_back(cmd);

    std::string token;

    while (iss >> token)
        args.push_back(token);

    if (cmd == "out" ||
        cmd == "capture")
    {
        handleOutputFileCommand(args);
        return;
    }

    if (cmd == "help" ||
        cmd == "h" ||
        cmd == "?")
    {
        // help <command>
        if (args.size() >= 2)
        {
            std::string topic = args[1];

            std::transform(
                topic.begin(),
                topic.end(),
                topic.begin(),
                [](unsigned char c)
                {
                    return static_cast<char>(
                        std::tolower(c));
                });

            auto commandIt = commands.find(topic);

            if (commandIt != commands.end())
            {
                const std::string text =
                    commandIt->second->help();

                std::cout << text;

                if (!text.empty() &&
                    text.back() != '\n')
                {
                    std::cout << "\n";
                }
            }
            else
            {
                std::cout
                    << "Unknown command: "
                    << topic
                    << "\n";
            }

            return;
        }

        // Plain help: group commands by category.
        std::map<
            std::string,
            std::vector<const MonitorCommand*>>
            grouped;

        for (const auto& entry : commands)
        {
            grouped[entry.second->category()]
                .push_back(entry.second.get());
        }

        std::vector<std::string> categories;

        for (const auto& entry : grouped)
            categories.push_back(entry.first);

        std::sort(
            categories.begin(),
            categories.end(),
            [&grouped](
                const std::string& left,
                const std::string& right)
            {
                const auto& leftCommands =
                    grouped[left];

                const auto& rightCommands =
                    grouped[right];

                const auto leftMin =
                    std::min_element(
                        leftCommands.begin(),
                        leftCommands.end(),
                        [](const MonitorCommand* first,
                           const MonitorCommand* second)
                        {
                            return first->order() <
                                   second->order();
                        });

                const auto rightMin =
                    std::min_element(
                        rightCommands.begin(),
                        rightCommands.end(),
                        [](const MonitorCommand* first,
                           const MonitorCommand* second)
                        {
                            return first->order() <
                                   second->order();
                        });

                const int leftOrder =
                    (*leftMin)->order();

                const int rightOrder =
                    (*rightMin)->order();

                if (leftOrder != rightOrder)
                    return leftOrder < rightOrder;

                return left < right;
            });

        std::cout << "Available commands:\n";

        for (const std::string& category : categories)
        {
            auto& categoryCommands =
                grouped[category];

            std::sort(
                categoryCommands.begin(),
                categoryCommands.end(),
                [](const MonitorCommand* left,
                   const MonitorCommand* right)
                {
                    if (left->order() != right->order())
                    {
                        return left->order() <
                               right->order();
                    }

                    return left->name() <
                           right->name();
                });

            std::cout
                << "  "
                << category
                << ":\n";

            for (const MonitorCommand* command :
                 categoryCommands)
            {
                std::cout
                    << "    "
                    << command->shortHelp()
                    << "\n";
            }
        }

        return;
    }

    auto commandIt = commands.find(cmd);

    if (commandIt != commands.end())
    {
        commandIt->second->execute(
            *this,
            args);
    }
    else
    {
        std::cout
            << "Unknown command: "
            << cmd
            << "\n";
    }
}

void MLMonitor::writeOutputFileBlock(const std::string& cmdLine, const std::string& output)
{
    if (!outputFileEnabled || !outputFile.is_open())
        return;

    outputFile << "> " << cmdLine << "\n";

    if (!output.empty())
    {
        outputFile << output;

        if (output.back() != '\n')
            outputFile << "\n";
    }

    outputFile << "\n";
    outputFile.flush();
}

void MLMonitor::handleOutputFileCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2)
    {
        std::cout <<
            "Usage:\n"
            "  out on <file>\n"
            "  out off\n"
            "  out status\n";
        return;
    }

    std::string sub = args[1];
    std::transform(sub.begin(), sub.end(), sub.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (sub == "on")
    {
        std::string path = "mlmonitor_output.txt";

        if (args.size() >= 3)
            path = args[2];

        // Close previous file if one is already open.
        if (outputFile.is_open())
            outputFile.close();

        outputFile.open(path, std::ios::out | std::ios::app);

        if (!outputFile.is_open())
        {
            outputFileEnabled = false;
            outputFilePath.clear();
            std::cout << "Unable to open output file: " << path << "\n";
            return;
        }

        outputFileEnabled = true;
        outputFilePath = path;

        std::cout << "Monitor output file enabled: " << outputFilePath << "\n";
        return;
    }

    if (sub == "off")
    {
        if (outputFile.is_open())
            outputFile.close();

        outputFileEnabled = false;

        std::cout << "Monitor output file disabled";

        if (!outputFilePath.empty())
            std::cout << ": " << outputFilePath;

        std::cout << "\n";
        return;
    }

    if (sub == "status")
    {
        if (outputFileEnabled && outputFile.is_open())
            std::cout << "Monitor output file is ON: " << outputFilePath << "\n";
        else
            std::cout << "Monitor output file is OFF\n";

        return;
    }

    std::cout << "Unknown out command: " << sub << "\n";
}
