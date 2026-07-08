// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "Debug/MLMonitor.h"
#include "Debug/CPUCommand.h"

MLMonitor::MLMonitor() :
    mlMonitorBackend(nullptr),
    running(false),
    outputFileEnabled(false)
{
    registerCommand(std::make_unique<CPUCommand>());
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
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;

    // Blank command outside interactive assembler mode does nothing.
    if (cmd.empty())
        return;

    // Normalize to lowercase
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    if (cmd == "exit" || cmd == "q" || cmd == "quit")
    {
        running = false;
        return;
    }

    std::vector<std::string> args;
    args.push_back(cmd);

    std::string token;
    while (iss >> token)
        args.push_back(token);

    if (cmd == "out" || cmd == "capture")
    {
        handleOutputFileCommand(args);
        return;
    }

        if (cmd == "help" || cmd == "h" || cmd == "?")
    {
        // If user typed: help <command>
        if (args.size() >= 2)
        {
            std::string topic = args[1];

            std::transform(topic.begin(), topic.end(), topic.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            auto it = commands.find(topic);
            if (it != commands.end())
            {
                const std::string txt = it->second->help();
                std::cout << txt;

                if (!txt.empty() && txt.back() != '\n')
                    std::cout << "\n";
            }
            else
            {
                std::cout << "Unknown command: " << topic << "\n";
            }

            return;
        }

        // Plain "help" => main help
        std::map<std::string, std::vector<const MonitorCommand*>> grouped;

        for (const auto& kv : commands)
        {
            grouped[kv.second->category()].push_back(kv.second.get());
        }

        std::vector<std::string> categories;

        for (const auto& kv : grouped)
        {
            categories.push_back(kv.first);
        }

        std::sort(categories.begin(), categories.end(),
            [&grouped](const std::string& a, const std::string& b)
            {
                const auto& aCommands = grouped[a];
                const auto& bCommands = grouped[b];

                const auto aMin = std::min_element(aCommands.begin(), aCommands.end(),
                    [](const MonitorCommand* lhs, const MonitorCommand* rhs)
                    {
                        return lhs->order() < rhs->order();
                    });

                const auto bMin = std::min_element(bCommands.begin(), bCommands.end(),
                    [](const MonitorCommand* lhs, const MonitorCommand* rhs)
                    {
                        return lhs->order() < rhs->order();
                    });

                const int aOrder = (*aMin)->order();
                const int bOrder = (*bMin)->order();

                if (aOrder != bOrder)
                    return aOrder < bOrder;

                return a < b;
            });

        std::cout << "Available commands:\n";

        for (const std::string& cat : categories)
        {
            auto& cmds = grouped[cat];

            std::sort(cmds.begin(), cmds.end(),
                [](const MonitorCommand* a, const MonitorCommand* b)
                {
                    if (a->order() != b->order())
                        return a->order() < b->order();

                    return a->name() < b->name();
                });

            std::cout << "  " << cat << ":\n";

            for (const MonitorCommand* command : cmds)
            {
                std::cout << "    " << command->shortHelp() << "\n";
            }
        }

        return;
    }

    auto it = commands.find(cmd);
    if (it != commands.end())
    {
        it->second->execute(*this, args);
    }
    else
    {
        std::cout << "Unknown command: " << cmd << "\n";
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
