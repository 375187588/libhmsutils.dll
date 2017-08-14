#include "../include/install_dlg.h"

using namespace std;

// Globals
HWND gHwnd;
std::wstring locate_libmysql = L"Unknown";
std::wstring download_server = L"dev.mysql.com";
std::string  webspace_url    = "/get/Downloads/Connector-C/";
std::string  ZipArchiveName  = "mysql-connector-c-6.1.11-win32.zip";

#define DLGTITLE  L"Libmysql.dll installer for hMailServer"
#define DLGFONT   L"MS Sans Serif"
#define DLGAPPLY  L"&Progress"
#define DLGCANCEL L"&Finish"
#define bt_download L"Start"
#define NUMCHARS(aa) (sizeof(aa)/sizeof((aa)[0]))
#define ed_listbox L"No Data"
#define IDC_ed_ListBox 1050
#define IDC_bt_download 1048




std::wstring tempfile = L"";
std::wstring tempdir = L"";

#pragma pack(push, 8)                 
static struct { // dltt 

        DWORD  style;
        DWORD  dwExtendedStyle;
        WORD   ccontrols;
        short  x;
        short  y;
        short  cx;
        short  cy;
        WORD   menu;         // name or ordinal of a menu resource
        WORD   windowClass;  // name or ordinal of a window class
        WCHAR  wszTitle[NUMCHARS(DLGTITLE)]; // title string of the dialog box
        short  pointsize;       // only if DS_SETFONT flag is set
        WCHAR  wszFont[NUMCHARS(DLGFONT)];   // typeface name, if DS_SETFONT is set

        // control info
        //
        struct {
                DWORD  style;
                DWORD  exStyle;
                short  x;
                short  y;
                short  cx;
                short  cy;
                WORD   id;
                WORD   sysClass;       // 0xFFFF identifies a system window class
                WORD   idClass;        // ordinal of a system window class
                WCHAR  wszTitle[NUMCHARS(DLGAPPLY)];
                WORD   cbCreationData; // bytes of following creation data
                //       WORD   wAlign;         // align next control to DWORD boundry.
        } apply;

        struct {
                DWORD  style;
                DWORD  exStyle;
                short  x;
                short  y;
                short  cx;
                short  cy;
                WORD   id;
                WORD   sysClass;       // 0xFFFF identifies a system window class
                WORD   idClass;        // ordinal of a system window class
                WCHAR  wszTitle[NUMCHARS(DLGCANCEL)];
                WORD   cbCreationData; // bytes of following creation data
        } cancel;

        struct {
                DWORD  style;
                DWORD  exStyle;
                short  x;
                short  y;
                short  cx;
                short  cy;
                WORD   id;
                WORD   sysClass;       // 0xFFFF identifies a system window class
                WORD   idClass;        // ordinal of a system window class
                WCHAR  wszTitle[NUMCHARS(bt_download)];
                WORD   cbCreationData; // bytes of following creation data
        } download;

        struct {
			DWORD  style;
			DWORD  exStyle;
			short  x;
			short  y;
			short  cx;
			short  cy;
			WORD   id;
			WORD   sysClass;       // 0xFFFF identifies a system window class
			WORD   idClass;        // ordinal of a system window class
			WCHAR  wszTitle[NUMCHARS(ed_listbox)];
			WORD   cbCreationData; // bytes of following creation data
        } MyListBox;

} 
g_DlgTemplate = {

        WS_POPUP | WS_VISIBLE | WS_CAPTION   // style  0x94c800c4
        | DS_MODALFRAME | DS_3DLOOK | WS_BORDER
        | DS_SETFONT,
        0x0,        // exStyle;
        4,          // ccontrols
        80, 20, 270, 180,
        0,                       // menu: none
        0,                       // window class: none
        DLGTITLE,                // Window caption
        9,                       // font pointsize
        DLGFONT,

        {
                WS_CHILD | WS_TABSTOP | WS_VISIBLE | WS_GROUP | BS_GROUPBOX,   // 0x50030001
                WS_EX_NOPARENTNOTIFY, // 0x4
                20, 20, 235, 114,
                IDOK,
                0xFFFF, 0x0080, // button
                DLGAPPLY, 0,
        },

        {
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,    // 0x50010000
                WS_EX_NOPARENTNOTIFY, // 0x4
                204, 147, 50, 14,
                IDCANCEL,
                0xFFFF, 0x0080, // button
                DLGCANCEL, 0,
        },

        {
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,    // 0x50010000
                WS_EX_NOPARENTNOTIFY, // 0x4
                134, 147, 50, 14,
                IDC_bt_download,
                0xFFFF, 0x0080, // button
                bt_download, 0,
        },

        {
			WS_CHILD | WS_VISIBLE | LBS_NOSEL | BS_MULTILINE |  BS_LEFT ,    // 0x50010000
			0, // 0x4
			34, 37, 210, 84,
			IDC_ed_ListBox,
			0xFFFF, 0x0083, // button
			ed_listbox, 0,
        },
};

#pragma pack(pop)
std::string ConvertWCSToMBS(const wchar_t* pstr, long wslen)
{
	int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

	std::string dblstr(len, '\0');
	len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
		pstr, wslen /* not necessary NULL-terminated */,
		&dblstr[0], len,
		NULL, NULL /* no default char */);

	return dblstr;
}

std::string ConvertBSTRToMBS(BSTR bstr)
{
	int wslen = ::SysStringLen(bstr);
	return ConvertWCSToMBS((wchar_t*)bstr, wslen);
}

// Check Windows the Username for the Service Status
std::string GetServicePathName()
{
	HRESULT hres;

	hres = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hres)) {}

	hres = CoInitializeSecurity(
		nullptr,
		-1,
		nullptr,
		nullptr,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		nullptr,
		EOAC_NONE,
		nullptr
	);

	if (FAILED(hres)) {
		CoUninitialize();
	}

	IWbemLocator *pLoc = nullptr;

	hres = CoCreateInstance(
		CLSID_WbemLocator,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&pLoc);

	if (FAILED(hres)) {
		CoUninitialize();
	}

	IWbemServices *pSvc = nullptr;

	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
		nullptr,                 // User name. NULL = current user
		nullptr,                 // User password. NULL = current
		nullptr,                 // Locale. NULL indicates current
		0,                       // Security flags.
		nullptr,                 // Authority (for example, Kerberos)
		nullptr,                 // Context object 
		&pSvc                    // pointer to IWbemServices proxy
	);

	if (FAILED(hres)) {
		pLoc->Release();
		CoUninitialize();
	}

	hres = CoSetProxyBlanket(
		pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		nullptr,                     // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		nullptr,                     // client identity
		EOAC_NONE                    // proxy capabilities 
	);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}

	IEnumWbemClassObject* pEnumerator = nullptr;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT PathName FROM Win32_Service WHERE (name='hMailServer')"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		nullptr,
		&pEnumerator);

	if (FAILED(hres)) {
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
	}

	IWbemClassObject *pclsObj = nullptr;
	ULONG uReturn = 0;

	pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

	VARIANT vtProp;
	pclsObj->Get(L"PathName", 0, &vtProp, nullptr, nullptr);
	std::string result = ConvertBSTRToMBS(vtProp.bstrVal);
	VariantClear(&vtProp);
	pclsObj->Release();

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();
	return result;
}

// Needs the Service PathName attribute string to work
std::string ExtractPathFromWindowsService(std::string servicepath)
{
	std::string str = servicepath;
	std::string mystr = str.substr(0, str.find("hMailServer.exe", 0));
	mystr = mystr.erase(0, 1);
	return mystr;
}

std::wstring find_files(std::wstring wrkdir)
{
	std::wstring gaga;

	std::wstring temp;
	WIN32_FIND_DATA file_data = { 0 };
	temp = wrkdir + L"\\" + L"*";
	HANDLE fHandle = FindFirstFile(temp.c_str(), &file_data);

	if (fHandle == INVALID_HANDLE_VALUE)
	{
		//return 0;
	}
	else
	{
		while (FindNextFile(fHandle, &file_data))
		{
			if (file_data.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY &&
				wcscmp(file_data.cFileName, L".") != 0 &&
				wcscmp(file_data.cFileName, L"..") != 0)
			{
				find_files(wrkdir + L"" + file_data.cFileName);
			}
			else if (file_data.dwFileAttributes != FILE_ATTRIBUTE_HIDDEN &&
				file_data.dwFileAttributes != FILE_ATTRIBUTE_SYSTEM  &&
				wcscmp(file_data.cFileName, L".") != 0 &&
				wcscmp(file_data.cFileName, L"..") != 0
				)
			{

				if (0 == _tcscmp(file_data.cFileName, L"libmysql.dll")) {
					
					LPCWSTR foo1 = wrkdir.c_str();
					LPCWSTR foo2 = file_data.cFileName;
					LPCWSTR sep = L"\\";

					std::wstring dfx = std::wstring(sep) + foo2;
					LPCWSTR result = dfx.c_str();
				
					std::wstring df = std::wstring(foo1) + result;
					LPCWSTR caststr = df.c_str(); 
					locate_libmysql = caststr;
				}
			}
		}
	}
	return locate_libmysql;
}

// Create new GUID and return as string
std::string GetNewGUID()
{
	UUID uuid = { 0 };
	std::string guid;

	// Create uuid or load from a string by UuidFromString() function
	::UuidCreate(&uuid);

	// If you want to convert uuid to string, use UuidToString() function
	RPC_CSTR szUuid = NULL;
	if (::UuidToStringA(&uuid, &szUuid) == RPC_S_OK)
	{
		guid = (char*)szUuid;
		::RpcStringFreeA(&szUuid);
	}

	return guid;
}

int StringToWString(std::wstring &ws, const std::string &s)
{
	std::wstring wsTmp(s.begin(), s.end());
	ws = wsTmp;
	return 0;
}

/* Eventhandler in this Dialog */
INT_PTR CALLBACK install_dlg_proc(
        HWND   hwnd,
        UINT   uMsg,
        WPARAM wParam,
        LPARAM lParam)
{
	

	gHwnd = hwnd;
        switch (uMsg)
        {
        case WM_INITDIALOG:
        {
		
			
			HWND hWndListBox = GetDlgItem(gHwnd, IDC_ed_ListBox);

		

			//wchar_t data[] = L"Ready to install Libmysql. Press start.";
			//SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data);
        }
        break;

        case WM_COMMAND:
        {
                UINT wId = LOWORD(wParam);
                if (wId == IDCANCEL) {
                        EndDialog(hwnd, wId);
                }

                UINT wId2 = LOWORD(wParam);
                if (wId2 == IDC_bt_download)
                {   									
					std::wstring w(getTempPath());
					std::string s(w.begin(), w.end());

					std::string newdir = GetNewGUID();
					std::string workdir = s;

					// New GUID Dir String
					std::string result_dir = s + newdir + "\\";
					
					// Create new GUID Dir
					CreateDirectoryA(result_dir.c_str(), NULL);								
									
					// Extend new GUID dir plus download filename
					std::string download_filename = result_dir + "mysql.zip";
					
					// Cast strings
					std::wstring dl;
					StringToWString(dl, download_filename);
					std::wstring extractDir;

					// removing the substring '.zip' 
					string ExtractFolderName  = ZipArchiveName.substr(0, ZipArchiveName.length() - 4) +"\\";

					// Cast from std::string to std::wstring
					StringToWString(extractDir, result_dir + ExtractFolderName);
					
					// Status
					wcout << "Downloading file: " << dl << endl;

					//Downloadling file	
					download(dl);
					BSTR mytempdir = SysAllocString((_bstr_t)result_dir.c_str());
					BSTR mytempfile = SysAllocString((_bstr_t)dl.c_str());
					
					cout << "Extracting ZIP-Archive.. " ;
					
					HWND hWndListBox = GetDlgItem(gHwnd, IDC_ed_ListBox);		
					wchar_t data1[] = L"Extracting Archive...";	
					SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data1);

					Unzip2Folder(mytempfile, mytempdir);
					cout << "- [OK]" << endl;				

					wchar_t data2[] = L"All files extracted from Zip-Archive...";
					SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data2);

					wcout << "Locating libmysql.dll in unzipped Folder " << endl;		

					wchar_t data3[] = L"Locating libmysql.dll in unzipped Folder...";
					SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data3);
					
					wstring found = find_files(extractDir);					
					wcout << "Found at " << found << endl;
					wchar_t data4[] = L"Found 'libmysql.dll in extracted folder...";
					SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data4);


					std::string  servicePath = ExtractPathFromWindowsService(GetServicePathName());
					cout << "hMailServer Servicepath: " << servicePath << endl;
					wcout << "Copying libmysql.dll into hMailServer\\bin folder..." <<  endl;

					wchar_t data5[] = L"Copying to \\hMailServer\\bin Folder...";
					SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data5);

									
					// hMailServers\bin folder +"libmysql.dll" added to it
					servicePath = servicePath + "libmysql.dll";

					std::wstring spath;
					StringToWString(spath, servicePath);

					wcout << "Copypath: " << spath << endl;;


						
					// Now everything is prepared, copy the the file to its final destination	
					BOOL b = CopyFile(found.c_str(), spath.c_str(), 0);
					if (!b) {
						cout << "Error: - Cannot copy file, access Problem? " << GetLastError() << endl;
					}
					else {
						wchar_t data6[] = L"Success! - Click finish";

						SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data6);
						//cout << "libmysql.dll was placed into hMailServer\\bin Folder [OK]" << endl;
					}

                }				
        }
        break;

        case WM_CLOSE:
                EndDialog(hwnd, IDCANCEL);
                break;
        default: ;
        }

	return FALSE;
}

bool Unzip2Folder(BSTR lpZipFile, BSTR lpFolder);

bool Unzip2Folder(BSTR lpZipFile, BSTR lpFolder)
{
	IShellDispatch *pISD;

	Folder  *pZippedFile = nullptr;
	Folder  *pDestination = nullptr;

	long FilesCount = 0;
	IDispatch* pItem = nullptr;
	FolderItems *pFilesInside = nullptr;

	VARIANT Options, Item;

	VARIANT OutFolder = { VT_NULL };
	VARIANT InZipFile = { VT_NULL };
	
	CoInitialize(nullptr);
	__try {
		if (CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&pISD) != S_OK)
			return true;

		InZipFile.vt = VT_BSTR;
		InZipFile.bstrVal = lpZipFile;
		pISD->NameSpace(InZipFile, &pZippedFile);
		if (!pZippedFile)
		{
			pISD->Release();
			return true;
		}
				
		OutFolder.vt = VT_BSTR;
		OutFolder.bstrVal = lpFolder;
		pISD->NameSpace(OutFolder, &pDestination);
		if (!pDestination)
		{
			pZippedFile->Release();
			pISD->Release();
			return true;
		}

		pZippedFile->Items(&pFilesInside);
		if (!pFilesInside)
		{
			pDestination->Release();
			pZippedFile->Release();
			pISD->Release();
			return true;
		}

		pFilesInside->get_Count(&FilesCount);
		if (FilesCount < 1)
		{
			pFilesInside->Release();
			pDestination->Release();
			pZippedFile->Release();
			pISD->Release();
			return false;
		}

		pFilesInside->QueryInterface(IID_IDispatch, (void**)&pItem);


		Item.vt = VT_DISPATCH;
		Item.pdispVal = pItem;

		Options.vt = VT_I4;
		Options.lVal = 1024 | 512 | 16 | 4;

		bool retval = pDestination->CopyHere(Item, Options) == S_OK;

		pItem->Release(); pItem = nullptr;
		pFilesInside->Release(); pFilesInside = nullptr;
		pDestination->Release(); pDestination = nullptr;
		pZippedFile->Release(); pZippedFile = nullptr;
		pISD->Release(); pISD = nullptr;

		return retval;

	}
	__finally
	{
		CoUninitialize();
	}
	return false;
}

LRESULT show_install_dlg(HWND hwndApp, LPVOID pvData)
{
	
        HINSTANCE hinst = hwndApp ? (HINSTANCE)(LONG_PTR)GetWindowLongPtr(hwndApp, GWLP_HINSTANCE)
                : (HINSTANCE)GetModuleHandle(nullptr);
		


        return DialogBoxIndirectParamW(hinst, (LPCDLGTEMPLATEW)&g_DlgTemplate, hwndApp,
			install_dlg_proc, (LPARAM)pvData);



}

std::wstring s2ws(const std::string& str)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

std::string ws2s(const std::wstring& wstr)
{
	int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), nullptr, 0, nullptr, nullptr);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), int(wstr.length() + 1), &strTo[0], size_needed, nullptr, nullptr);
	return strTo;
}

std::wstring  getTempPath()
{
	wchar_t wchPath[MAX_PATH];
	GetTempPathW(MAX_PATH, wchPath);
	std::wstring strTempPath = wchPath;
	return strTempPath.c_str();
}	



BOOL download(std::wstring extractfilePath)
{
	//SendMessage(gHwnd, WM_SETREDRAW, true, 0);

	HWND hWndListBox = GetDlgItem(gHwnd, IDC_ed_ListBox);		
	wchar_t data1[] = L"Connecting to Downloadserver...";	
	SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data1);
	
	tempdir = getTempPath();
		
	HWND hWndEdit = GetDlgItem(gHwnd, IDC_bt_download);
	SetWindowText(hWndEdit, TEXT("Pending"));
	EnableWindow(hWndEdit, FALSE);
	

	// Use WinHttpOpen to obtain a session handle
	HINTERNET hInternet = WinHttpOpen(L"Download files",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		nullptr,
		nullptr,	
		0);
	
	wchar_t data2[] = L"Downloading files...";
	SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data2);

	


	if (hInternet == nullptr)
	{
		printf("Failed to initialize http session.\n");
		return 0;
	}

	// Specify an HTTP server

	

	HINTERNET hConnected = WinHttpConnect(hInternet, download_server.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);

	if (hConnected == nullptr)
	{
		WinHttpCloseHandle(hInternet);
		return 0;
	}

	string download_url = webspace_url + ZipArchiveName;
	wstring endpoint;
	StringToWString(endpoint, download_url);

	wcout << "download_url " << endpoint << endl;

	// Create an HTTP Request handle
	HINTERNET hRequest = WinHttpOpenRequest(hConnected,
		L"GET",
		endpoint.c_str(),
		nullptr,
		nullptr,
		nullptr,
		WINHTTP_FLAG_REFRESH);

	if (hRequest == nullptr)
	{
		::WinHttpCloseHandle(hConnected);
		::WinHttpCloseHandle(hInternet);
		return 0;
	}

	// Send a Request
	if (WinHttpSendRequest(hRequest,
		nullptr, 0,
		nullptr, 0,
		0, 0) == FALSE)
	{
		//DWORD err = GetLastError();
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnected);
		WinHttpCloseHandle(hInternet);
		return 0;
	}

	// Receive a Response
	if (WinHttpReceiveResponse(hRequest, nullptr) == FALSE)
	{
		//DWORD err = GetLastError();
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnected);
		WinHttpCloseHandle(hInternet);
		return 0;
	}

	// Create a sample binary file 
	HANDLE hFile = CreateFile(extractfilePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	DWORD dwSize = 0;
	//bool bRetValue = false;
	printf("Downloading ........");

	do
	{
		// Check for available data.
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
		{
			printf("Data not available\n");
		}

		// Allocate space for the buffer.
		BYTE *pchOutBuffer = new BYTE[dwSize + 1];
		if (!pchOutBuffer)
		{
			printf("Http Request is out of memory.\n");
			dwSize = 0;
		}
		else
		{
			// Read the Data.
			DWORD dwDownloaded = 0;
			ZeroMemory(pchOutBuffer, dwSize + 1);
			if (!WinHttpReadData(hRequest, (LPVOID)pchOutBuffer, dwSize, &dwDownloaded))
			{
				printf("Http read data error.\n");
			}
			else
			{
				// Write buffer to sample binary file
				DWORD wmWritten;
				WriteFile(hFile, pchOutBuffer, dwSize, &wmWritten, nullptr);
			}

			delete[] pchOutBuffer;
		}

	} while (dwSize>0);
		
	wchar_t data3[] = L"Download complete.";
	SendMessage(hWndListBox, LB_ADDSTRING, 0, (LPARAM)data3);

	
	// Housekeeping
	CloseHandle(hFile);
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnected);
	WinHttpCloseHandle(hInternet);
	
	return false;
}

