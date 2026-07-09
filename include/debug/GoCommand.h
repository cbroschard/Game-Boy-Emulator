#ifndef GOCOMMAND_H
#define GOCOMMAND_H

#include "debug/MonitorCommand.h"

class GoCommand : public MonitorCommand
{
    public:
        GoCommand();
        virtual ~GoCommand();

        int order() const override;

        std::string name() const override;
        std::string category() const override;
        std::string shortHelp() const override;
        std::string help() const override;

        void execute(MLMonitor& mlMonitor, const std::vector<std::string>& args);

    protected:

    private:
};

#endif // GOCOMMAND_H
