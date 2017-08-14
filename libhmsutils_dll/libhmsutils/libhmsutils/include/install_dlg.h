#ifndef INSTALL_DLG_H
#define INSTALL_DLG_H

#include <windows.h>
#include <string>
#include <ShlDisp.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <tchar.h>
#include <vector>
#include <comutil.h>
#include <Wbemidl.h>
#include <conio.h>

using namespace std;

extern LRESULT show_install_dlg(HWND hwndApp, LPVOID pvData);

std::wstring s2ws(const std::string& str);
std::string ws2s(const std::wstring& wstr);
std::wstring  getTempPath();
bool Unzip2Folder(BSTR lpZipFile, BSTR lpFolder);
BOOL download(std::wstring extractfilePath);
std::string ExtractPathFromWindowsService(std::string servicepath);

#endif