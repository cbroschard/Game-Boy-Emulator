// Copyright (c) 2026 Christopher Broschard
// All rights reserved.
//
// This source code is provided for personal, educational, and
// non-commercial use only. Redistribution, modification, or use
// of this code in whole or in part for any other purpose is
// strictly prohibited without the prior written consent of the author.
#include "EmulatorUI.h"

EmulatorUI::EmulatorUI()
{
    fileDlg.open = false;
    fileDlg.currentDir = std::filesystem::current_path();
}

EmulatorUI::~EmulatorUI() = default;

void EmulatorUI::draw()
{
    installMenu();
    drawFileDialog();
}

std::vector<UICommand> EmulatorUI::consumeCommands()
{
    std::lock_guard<std::mutex> lock(outMutex_);
    auto tmp = std::move(out_);
    out_.clear();
    return tmp;
}

void EmulatorUI::installMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Quit", "Alt+F4"))
                push(UICommand::Type::Quit);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("System"))
        {
            if (ImGui::MenuItem("Save Emulator State to File...", "Ctrl+S"))
                startSaveFileDialog("Save Emulator State (.sav)", { ".sav" }, UICommand::Type::SaveState, true);

            if (ImGui::MenuItem("Load Emulator State from file...", "Ctrl+L"))
                startFileDialog("Select SAV image to load", { ".sav" }, UICommand::Type::LoadState);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EmulatorUI::startFileDialog(const char* title, std::initializer_list<const char*> exts, UICommand::Type type)
{
    fileDlg.title = title ? title : "";

    fileDlg.allowedExtensions.clear();
    for (auto e : exts)
        fileDlg.allowedExtensions.emplace_back(e);

    fileDlg.selectedEntry.clear();
    fileDlg.fileName.clear();
    fileDlg.error.clear();

    fileDlg.allowOverwrite = false;
    fileDlg.mode = FileDialog::Mode::OpenExisting;
    fileDlg.open = true;

    pendingType_ = type;
}

void EmulatorUI::startSaveFileDialog(const char* title, std::initializer_list<const char*> exts, UICommand::Type type, bool allowOverwrite)
{
    fileDlg.title = title ? title : "";

    fileDlg.allowedExtensions.clear();
    for (auto e : exts)
        fileDlg.allowedExtensions.emplace_back(e);

    fileDlg.selectedEntry.clear();
    fileDlg.fileName.clear();
    fileDlg.error.clear();

    fileDlg.allowOverwrite = allowOverwrite;
    fileDlg.mode = FileDialog::Mode::SaveAs;
    fileDlg.open = true;

    pendingType_ = type;
}

void EmulatorUI::drawFileDialog()
{
    if (!fileDlg.open)
        return;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImVec2 workPos  = vp->WorkPos;
    ImVec2 workSize = vp->WorkSize;

    const float margin = 24.0f;

    ImVec2 desired(760.0f, 520.0f);
    desired.x = std::min(desired.x, workSize.x - margin * 2.0f);
    desired.y = std::min(desired.y, workSize.y - margin * 2.0f);

    ImVec2 center(workPos.x + workSize.x * 0.5f,
                  workPos.y + workSize.y * 0.5f);

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(desired, ImGuiCond_Appearing);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(420, 260),
        ImVec2(workSize.x - margin * 2.0f,
               workSize.y - margin * 2.0f)
    );

    const char* windowTitle =
        fileDlg.title.empty() ? "Select File" : fileDlg.title.c_str();

    if (!ImGui::Begin(windowTitle, &fileDlg.open))
    {
        ImGui::End();
        return;
    }

    namespace fs = std::filesystem;

    auto openPath = [this](const fs::path& path)
    {
        try
        {
            if (fs::is_directory(path))
            {
                fileDlg.currentDir = path;
                fileDlg.selectedEntry.clear();
                fileDlg.fileName.clear();
                fileDlg.error.clear();
                return;
            }

            if (!isAllowedByExtension(path))
            {
                fileDlg.error = "File type not allowed for this action.";
                return;
            }

            emitChosenPath(path);
        }
        catch (const std::exception& e)
        {
            fileDlg.error = e.what();
        }
    };

    auto chooseSavePath = [this](const fs::path& path)
    {
        try
        {
            if (fs::is_directory(path))
                return;

            fileDlg.fileName = path.filename().string();
            fileDlg.error.clear();

            if (!isAllowedByExtension(path))
            {
                fileDlg.error = "File type not allowed for this action.";
                return;
            }

            if (!fileDlg.allowOverwrite)
                return;

            emitChosenPath(path);
        }
        catch (const std::exception& e)
        {
            fileDlg.error = e.what();
        }
    };

    auto goUp = [this]()
    {
        auto parent = fileDlg.currentDir.parent_path();

        if (!parent.empty())
        {
            fileDlg.currentDir = parent;
            fileDlg.selectedEntry.clear();
            fileDlg.fileName.clear();
            fileDlg.error.clear();
        }
    };

    ImGui::TextUnformatted(fileDlg.currentDir.string().c_str());
    ImGui::Separator();

    float reserve = 0.0f;
    reserve += ImGui::GetFrameHeightWithSpacing();
    reserve += ImGui::GetTextLineHeightWithSpacing();

    if (fileDlg.mode == FileDialog::Mode::SaveAs)
    {
        reserve += ImGui::GetTextLineHeightWithSpacing();
        reserve += ImGui::GetFrameHeightWithSpacing();
        reserve += ImGui::GetStyle().ItemSpacing.y;
    }

    ImGui::BeginChild("##file_list", ImVec2(0, -reserve), true);

    std::vector<fs::directory_entry> entries;

    try
    {
        for (const auto& entry : fs::directory_iterator(fileDlg.currentDir))
            entries.push_back(entry);

        std::sort(entries.begin(), entries.end(),
                  [](const fs::directory_entry& a,
                     const fs::directory_entry& b)
                  {
                      bool ad = a.is_directory();
                      bool bd = b.is_directory();

                      if (ad != bd)
                          return ad;

                      return a.path().filename().string() <
                             b.path().filename().string();
                  });
    }
    catch (const std::exception& e)
    {
        fileDlg.error = e.what();
    }

    {
        ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;

        bool clicked = ImGui::Selectable("..", false, flags);
        bool doubleClicked =
            ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        if (clicked)
        {
            fileDlg.selectedEntry.clear();
            fileDlg.error.clear();
        }

        if (doubleClicked)
            goUp();
    }

    for (const auto& entry : entries)
    {
        const fs::path& path = entry.path();
        std::string name = path.filename().string();
        bool isDir = entry.is_directory();

        if (!isDir && fileDlg.mode == FileDialog::Mode::OpenExisting)
        {
            if (!isAllowedByExtension(path))
                continue;
        }

        std::string label = isDir ? (name + "/") : name;
        bool selected = fileDlg.selectedEntry == name;

        ImGuiSelectableFlags flags = ImGuiSelectableFlags_AllowDoubleClick;

        bool clicked = ImGui::Selectable(label.c_str(), selected, flags);
        bool doubleClicked =
            ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

        if (clicked)
        {
            fileDlg.selectedEntry = name;
            fileDlg.error.clear();

            if (fileDlg.mode == FileDialog::Mode::SaveAs && !isDir)
                fileDlg.fileName = name;
        }

        if (doubleClicked)
        {
            fileDlg.selectedEntry = name;
            fileDlg.error.clear();

            if (isDir)
            {
                openPath(path);
            }
            else if (fileDlg.mode == FileDialog::Mode::SaveAs)
            {
                chooseSavePath(path);
            }
            else
            {
                openPath(path);
            }
        }
    }

    ImGui::EndChild();

    if (fileDlg.mode == FileDialog::Mode::SaveAs)
    {
        ImGui::Separator();
        ImGui::TextUnformatted("File name:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##save_name", &fileDlg.fileName);
    }

    if (!fileDlg.error.empty())
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", fileDlg.error.c_str());

    if (ImGui::Button("Up"))
        goUp();

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        fileDlg.open = false;
        fileDlg.selectedEntry.clear();
        fileDlg.fileName.clear();
        fileDlg.error.clear();

        ImGui::End();
        return;
    }

    ImGui::SameLine();

    if (fileDlg.mode == FileDialog::Mode::OpenExisting)
    {
        bool hasSelection = !fileDlg.selectedEntry.empty();

        if (!hasSelection)
            ImGui::BeginDisabled();

        if (ImGui::Button("Open"))
        {
            try
            {
                fs::path p = fileDlg.currentDir / fileDlg.selectedEntry;
                openPath(p);
            }
            catch (const std::exception& e)
            {
                fileDlg.error = e.what();
            }
        }

        if (!hasSelection)
            ImGui::EndDisabled();
    }
    else
    {
        bool hasName = !fileDlg.fileName.empty();

        if (!hasName)
            ImGui::BeginDisabled();

        if (ImGui::Button("Save"))
        {
            try
            {
                fs::path outPath = fileDlg.currentDir / fileDlg.fileName;

                if (!outPath.has_extension() &&
                    fileDlg.allowedExtensions.size() == 1)
                {
                    outPath += fileDlg.allowedExtensions[0];
                }

                if (!isAllowedByExtension(outPath))
                {
                    fileDlg.error = "File type not allowed for this action.";
                }
                else if (fs::exists(outPath) && !fileDlg.allowOverwrite)
                {
                    fileDlg.error = "File already exists. Choose a different name.";
                }
                else
                {
                    emitChosenPath(outPath);
                }
            }
            catch (const std::exception& e)
            {
                fileDlg.error = e.what();
            }
        }

        if (!hasName)
            ImGui::EndDisabled();
    }

    ImGui::End();
}

void EmulatorUI::push(UICommand::Type t, std::string path)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    UICommand c;
    c.type = t;
    c.path = std::move(path);
    out_.push_back(std::move(c));
}

bool EmulatorUI::isAllowedByExtension(const std::filesystem::path& path) const
{
    if (fileDlg.allowedExtensions.empty())
        return true;

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    for (const auto& aRaw : fileDlg.allowedExtensions)
    {
        std::string a = aRaw;
        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        if (ext == a)
            return true;
    }
    return false;
}

void EmulatorUI::emitChosenPath(const std::filesystem::path& path)
{

    push(pendingType_, path.string());

    fileDlg.open = false;
    fileDlg.selectedEntry.clear();
    fileDlg.fileName.clear();
    fileDlg.error.clear();
}
