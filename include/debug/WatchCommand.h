// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef WATCHCOMMAND_H
#define WATCHCOMMAND_H

#include "debug/MonitorCommand.h"

class WatchCommand : public MonitorCommand
{
    public:
        WatchCommand();
        virtual ~WatchCommand();

        int order() const override;

        std::string name() const override;
        std::string category() const override;
        std::string shortHelp() const override;
        std::string help() const override;

        void execute(MLMonitor& mlMonitor, const std::vector<std::string>& args) override;

    protected:

    private:
};

#endif // WATCHCOMMAND_H
