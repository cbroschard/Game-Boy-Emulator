// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef MEMORYDUMPCOMMAND_H
#define MEMORYDUMPCOMMAND_H

#include "common/CommandUtils.h"
#include "debug/MonitorCommand.h"

class MemoryDumpCommand : public MonitorCommand
{
    public:
        MemoryDumpCommand();
        virtual ~MemoryDumpCommand();

        int order() const override;

        std::string name() const override;
        std::string category() const override;
        std::string shortHelp() const override;
        std::string help() const override;

        void execute(MLMonitor& mlMonitor, const std::vector<std::string>& args);


    protected:

    private:
};

#endif // MEMORYDUMPCOMMAND_H
