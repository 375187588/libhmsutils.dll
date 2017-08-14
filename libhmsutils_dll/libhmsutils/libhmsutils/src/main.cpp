#include <windows.h>
#include <windowsx.h>
#include <ole2.h>
#include <shlwapi.h>
#include <stdio.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <direct.h>

#include "../include/main.h"
#include "../include/install_dlg.h"


extern "C" __declspec(dllexport) int install_libmysql_dll() {

	printf("libhmsutils.dll - was called");
	showFrm();
	return 0;

}

void showFrm()
{
	show_install_dlg(nullptr, nullptr);
}
