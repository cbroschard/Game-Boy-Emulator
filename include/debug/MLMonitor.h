// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MLMONITOR_H
#define MLMONITOR_H

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Debug/MLMonitorBackend.h"
#include "Debug/MonitorCommand.h"

class MLMonitor
{
    public:
        MLMonitor();
        virtual ~MLMonitor();

        inline void attachMLMonitorBackendInstance(MLMonitorBackend* mlMonitorBackend) { this->mlMonitorBackend = mlMonitorBackend; }

        std::string executeAndCapture(const std::string& cmdLine);

        inline void setRunningFlag(bool flag) { running = flag; }
        inline bool getRunningFlag() const { return running; }

        inline MLMonitorBackend* getMLMonitorBackend() const { return mlMonitorBackend; }

        void enterMonitor();
        std::string getPrompt() const;

        // Breakpoint management
        inline void addBreakpoint(uint16_t bp) { breakpoints.insert(bp); }
        inline void clearAllBreakpoints() { breakpoints.clear(); }
        void clearBreakpoint(uint16_t bp);
        void listBreakpoints() const;

        // std::cout queuing/draining
        void queueAsyncLine(const std::string& s);
        std::vector<std::string> drainAsyncLines();

        // Helpers
        inline bool breakpointsEmpty() const { return breakpoints.empty(); }
        inline bool hasBreakpoint(uint16_t pc) { return (breakpoints.find(pc) != breakpoints.end()); }

    protected:

    private:
        // Non-owning pointers
        MLMonitorBackend* mlMonitorBackend;

        bool running;

        // Breakpoint
        std::unordered_set<uint16_t> breakpoints;

        // std::cout queue
        std::mutex asyncMutex;
        std::vector<std::string> asyncLines;

        std::unordered_map<std::string, std::unique_ptr<MonitorCommand>> commands;

        // Console output to file
        std::ofstream outputFile;
        std::string outputFilePath;
        bool outputFileEnabled;

        // Command registration
        inline void registerCommand(std::unique_ptr<MonitorCommand> cmd) { commands[cmd->name()] = std::move(cmd); }

        // Process commands
        void handleCommand(const std::string& line);
        void handleOutputFileCommand(const std::vector<std::string>& args);
        void writeOutputFileBlock(const std::string& cmdLine, const std::string& output);
};

#endif // MLMONITOR_H
