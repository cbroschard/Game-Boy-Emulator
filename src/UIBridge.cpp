// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "UIBridge.h"

UIBridge::UIBridge(EmulatorUI& ui,
                   std::atomic<bool>& uiPaused,
                   std::atomic<bool>& running,
                   StringFn saveState,
                   StringFn loadState) :
                       ui_(ui),
                       uiPaused_(uiPaused),
                       running_(running),
                       saveState_(saveState),
                       loadState_(loadState)
{

}

UIBridge::~UIBridge() = default;

void UIBridge::processCommands()
{
    for (const auto& cmd : ui_.consumeCommands())
    {
        switch (cmd.type)
        {
            case UICommand::Type::SaveState:
            {
                if (saveState_)
                    saveState_(cmd.path);
                break;
            }

            case UICommand::Type::LoadState:
            {
                if (loadState_)
                    loadState_(cmd.path);
                break;
            }

            case UICommand::Type::Quit:
            {
                running_ = false;
                break;
            }

            default:
                break;
        }
    }
}
