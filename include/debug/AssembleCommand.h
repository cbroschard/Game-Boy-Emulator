// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef ASSEMBLECOMMAND_H
#define ASSEMBLECOMMAND_H

#include "debug/MonitorCommand.h"

class AssembleCommand : public MonitorCommand
{
    public:
        AssembleCommand();
        virtual ~AssembleCommand();

        int order() const override;

        std::string name() const override;
        std::string category() const override;
        std::string shortHelp() const override;
        std::string help() const override;

        void execute(MLMonitor& mlMonitor, const std::vector<std::string>& args) override;

        bool isInteractiveActive() const;
        bool handleInteractiveLine(MLMonitor& mlMonitor, const std::string& line);

        std::string currentPrompt() const;

    protected:

    private:
        bool interactiveActive;
        uint16_t interactiveAddress;

        bool assembleAndWrite(MLMonitor& mlMonitor, uint16_t address, const std::string& line, uint16_t& nextAddress);
};

#endif // ASSEMBLECOMMAND_H
