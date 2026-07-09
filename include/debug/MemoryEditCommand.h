#ifndef MEMORYEDITCOMMAND_H
#define MEMORYEDITCOMMAND_H

#include "debug/MonitorCommand.h"

class MemoryEditCommand : public MonitorCommand
{
    public:
        MemoryEditCommand();
        virtual ~MemoryEditCommand();

        int order() const override;

        std::string name() const override;
        std::string category() const override;
        std::string shortHelp() const override;
        std::string help() const override;

        void execute(MLMonitor& mlMonitor, const std::vector<std::string>& args);

    protected:

    private:
};

#endif // MEMORYEDITCOMMAND_H
