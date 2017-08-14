#include <windows.h>
#include <stdio.h>

/* Windows Visual Styles Manifest */
#pragma comment(linker, \
                "\"/manifestdependency:type='Win32' "\
                "name='Microsoft.Windows.Common-Controls' "\
                "version='6.0.0.0' "\
                "processorArchitecture='*' "\
                "publicKeyToken='6595b64144ccf1df' "\
                "language='*'\"")

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int ncmdshow)

{
	typedef short (CALLBACK* cbstruct)();
	BOOL runTimeLinkSuccess = FALSE;

	//Provide the DLL Name
	HINSTANCE dllHandle = LoadLibrary(L"libhmsutils.dll");

	// Get the function address
	if (nullptr != dllHandle) {

		// Lookup exported functions
		cbstruct libhmsutils_ptr = (cbstruct)GetProcAddress(dllHandle, "install_libmysql_dll");

		// Call the  DLL function. 
		if ((runTimeLinkSuccess = (nullptr != libhmsutils_ptr))) {
			libhmsutils_ptr();
		}

		//Free the library:
		FreeLibrary(dllHandle);
	}

	// Catch Errors
	if (!runTimeLinkSuccess)
		printf("Cannot runtime load libhmsutils.dll\n");
		
	return EXIT_SUCCESS;
}