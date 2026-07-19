// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#ifndef EMULATORUI_H
#define EMULATORUI_H

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <mutex>
#include <string>
#include <vector>
#include "imgui/imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "UICommand.h"
#include "Version.h"

class EmulatorUI
{
    public:
        EmulatorUI();
        virtual ~EmulatorUI();

        inline bool isDialogOpen() const { return fileDlg.open; }

        void draw();

        std::vector<UICommand> consumeCommands();

    protected:

    private:
        struct FileDialog
        {
            enum class Mode
            {
                OpenExisting,
                SaveAs
            };

            bool open = false;
            bool allowOverwrite = false;
            Mode mode = Mode::OpenExisting;

            std::string title;
            std::filesystem::path currentDir;
            std::vector<std::string> allowedExtensions;

            std::string selectedEntry;
            std::string fileName;
            std::string error;
        };
        FileDialog fileDlg;

        std::vector<UICommand> out_;
        mutable std::mutex outMutex_;

        bool fileDialogOpen_;
        std::string pendingPath_;
        UICommand::Type pendingType_;

        void installMenu();
        void startFileDialog(const char* title, std::initializer_list<const char*> exts, UICommand::Type type);
        void startSaveFileDialog(const char* title, std::initializer_list<const char*> exts, UICommand::Type type, bool allowOverwrite = false);
        void drawFileDialog();

        void push(UICommand::Type t, std::string path = {});

        bool isAllowedByExtension(const std::filesystem::path& path) const;
        void emitChosenPath(const std::filesystem::path& path);
};

#endif // EMULATORUI_H
