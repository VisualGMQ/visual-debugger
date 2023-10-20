#include "file_dialog.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

std::vector<std::string> OpenFileDialog(const std::string& title) {
    OPENFILENAME ofn;
    char szFile[MAX_PATH] = {0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR |
                OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_ENABLESIZING;
    ofn.lpstrTitle = title.c_str();

    if (GetOpenFileName(&ofn) == TRUE) {
        return {std::string{szFile}};
    } else {
        return {};
    }
}