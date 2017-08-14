#ifndef LIBMAIN_H
#define LIBMAIN_H

#include <windows.h>
#include <cstdio>

extern "C" __declspec(dllexport) int install_libmysql_dll();
extern "C" __declspec(dllexport) void showFrm();

#endif