#include <Windows.h>
#include <Shlwapi.h>
#include <string>
#include <shlobj.h>
#include <cstdio>
#include <iso646.h>
#include <comdef.h>

#include "pugixml_read.h"
#include "main.h"
#include "json_read.h"

#define CURL_STATICLIB
#define NO_WIN32_LEAN_AND_MEAN

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

#pragma warning(disable : 4244)
#pragma warning(disable : 4302)

#pragma comment(lib,"Shlwapi.lib")

#if defined(_DEBUG)
#pragma comment(lib,"zlib.lib")
#pragma comment(lib,"pugixml.lib")
#pragma comment(lib,"libcurld.lib")
#else
#pragma comment(lib,"zlib.lib")
#pragma comment(lib,"pugixml.lib")
#pragma comment(lib,"libcurl.lib")
#endif

// Заставляем линкер генерировать манифест визуальных стилей окон
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WINDOW_ICON MAKEINTRESOURCE(IDI_ICON1)
#define WINDOW_BGND MAKEINTRESOURCE(IDB_BITMAP1)

// ID
#define ID_START	1
#define ID_PROGRESS 2
#define ID_FAQ		3
#define ID_ABOUT	4
#define ID_COMBOBOX 5

HINSTANCE hInstance;
HWND hWnd, hWndConsole, hButtonStart, hButtonInfo, hButtonAbout, hComboBox;
HFONT hFont;
HDC hdc;

LPCSTR FontName[]		= { "Verdana" };

#if defined(_WIN64) or defined(WIN64)
const wchar_t* WndName	= L"Star Wolves 3: New Civil War Installer";
#else
const wchar_t* WndName	= L"Star Wolves 3: New Civil War Installer (x86)";
#endif
const int WINDOW_SIZE_X = 640;
const int WINDOW_SIZE_Y = 480;
const char* FTP_URL		= "ftp://158.46.49.38/";
const wchar_t* WinClass	= L"Main window class";
const wchar_t* LogFile	= L"filelog.txt";
const wchar_t* cfgFile	= L"filecfg.xml";
const wchar_t* progver	= L"1.5";
wchar_t SavedPath[2048] = L"C:\\Program Files (x86)";
int InstallLang			= 0; // 0 - Russian, 1 - English
int CurrentLang			= 0;
bool QueueError			= FALSE;
bool Beginning			= FALSE;
LPCSTR Lang1;
LPCSTR Lang2;


void Window::WindowMenu(HWND hWnd)
{
	hButtonStart = CreateWindowExA(
		0,
		"BUTTON",
		ReadJSONLocTag("bInstallStart").c_str(),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, WINDOW_SIZE_Y - 160,
		165, 30,
		hWnd,
		(HMENU)ID_START,
		hInstance,
		NULL
	);

	
	hButtonInfo = CreateWindowExA(
		0,
		"BUTTON",
		ReadJSONLocTag("bModGuide").c_str(),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, WINDOW_SIZE_Y - 120,
		165, 30,
		hWnd,
		(HMENU)ID_FAQ,
		hInstance,
		NULL
	);

	hButtonAbout = CreateWindowExA(
		0,
		"BUTTON",
		ReadJSONLocTag("bAbout").c_str(),
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, WINDOW_SIZE_Y - 80,
		165, 30,
		hWnd,
		(HMENU)ID_ABOUT,
		hInstance,
		NULL
	);
	
	hComboBox = CreateWindowA(
		"COMBOBOX",
		"combobox",
		WS_CHILD | WS_VISIBLE | CBS_SORT | CBS_DROPDOWNLIST,
		10, 40,
		165, 20,
		hWnd,
		(HMENU)ID_COMBOBOX,
		hInstance,
		0
	);

	SendMessageA(hButtonStart, WM_SETFONT, (WCHAR)hFont, NULL);
	SendMessageA(hButtonInfo, WM_SETFONT, (WCHAR)hFont, NULL);
	SendMessageA(hButtonAbout, WM_SETFONT, (WCHAR)hFont, NULL); 
	SendMessageA(hComboBox, WM_SETFONT, (WCHAR)hFont, NULL);

	SendMessageA(hComboBox, CB_ADDSTRING, 0, (LPARAM)ReadJSONLocTag("cChLoc1").c_str());
	SendMessageA(hComboBox, CB_ADDSTRING, 0, (LPARAM)ReadJSONLocTag("cChLoc2").c_str());

	ShowWindow(hWnd, SW_SHOW);
	SendMessageA(hComboBox, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);

}
void Window::ChangeLanguage()
{
	SendMessageA(hButtonStart, WM_SETTEXT, 0, (LPARAM)ReadJSONLocTag("bInstallStart").c_str());
	SendMessageA(hButtonInfo,  WM_SETTEXT, 0, (LPARAM)ReadJSONLocTag("bModGuide").c_str());
	SendMessageA(hButtonAbout, WM_SETTEXT, 0, (LPARAM)ReadJSONLocTag("bAbout").c_str());
	SendMessageA(hComboBox, CB_ADDSTRING, 0, (LPARAM)ReadJSONLocTag("cChLoc1").c_str());
	SendMessageA(hComboBox, CB_ADDSTRING, 0, (LPARAM)ReadJSONLocTag("cChLoc2").c_str());

	LPCSTR str = ReadJSONLocTag("cChLoc").c_str();
	TextOutA(hdc, 10, 20, str, strlen(str));

	if (CurrentLang == 0)
	{
		CurrentLang = 1;
		LogClass::LOG("(INFO) [Main] UI language was changed to English");
		WriteXMLConfigTag("InterfaceLang", "English");
	}
	else
	{
		CurrentLang = 0;
		LogClass::LOG("(INFO) [Main] UI language was changed to Russian");
		WriteXMLConfigTag("InterfaceLang", "Russian");
	}
}
void Window::EnableButtons(bool Flag)
{
	EnableWindow(hButtonStart, Flag);
	EnableWindow(hButtonInfo, Flag);
}
bool Window::BrowseForFolder()
{
	wchar_t DestDir[2048];
	BOOL fRet;

	std::string titletxt = ReadJSONLocTag("cChInstallPath").c_str();

	BROWSEINFOA bi = { 0 };
	ZeroMemory(&bi, sizeof(bi));
	bi.hwndOwner	  = hWnd;
	bi.pidlRoot		  = NULL;
	bi.pszDisplayName = (LPSTR)titletxt.c_str();
	bi.lpszTitle	  = titletxt.c_str();
	bi.ulFlags		  = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
	bi.lpfn			  = &BrowsePathProc;
	bi.iImage		  = 0;
	bi.lParam		  = (LPARAM)SavedPath;

	LPITEMIDLIST pidl = ::SHBrowseForFolderA(&bi);

	if (!pidl)
		fRet = NULL;
	else
	{
		fRet = SHGetPathFromIDListW(pidl, (LPWSTR)DestDir);
		wcscat(DestDir, L"\\\0");
		FileClass::FileQueueSet(DestDir);
	}
	return fRet;
}

ATOM RegisterMainClass(HINSTANCE hInstance)
{
	WNDCLASSEX wc;

	wc.cbSize			= sizeof(WNDCLASSEX);
	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc		= WindowProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= hInstance;
	wc.hIcon			= LoadIcon(hInstance, WINDOW_ICON);
	wc.hCursor			= LoadCursor(NULL, MAKEINTRESOURCEW(32512));
	wc.hbrBackground	= CreatePatternBrush((HBITMAP)LoadImage(hInstance, WINDOW_BGND, IMAGE_BITMAP, WINDOW_SIZE_X, WINDOW_SIZE_Y, LR_COPYFROMRESOURCE));
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= WinClass;
	wc.hIconSm			= LoadIcon(hInstance, WINDOW_ICON);

	return RegisterClassEx(&wc);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hFont = CreateFontA(-13, 0, 0, 0,
		FW_NORMAL, 0,
		0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		*FontName);

	hWnd = CreateWindowExW(
		0L,
		WinClass,
		WndName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
		(GetSystemMetrics(SM_CXSCREEN) - WINDOW_SIZE_X) / 2, (GetSystemMetrics(SM_CYSCREEN) - WINDOW_SIZE_Y) / 2,
		WINDOW_SIZE_X, WINDOW_SIZE_Y,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd) {
		CnsClass::ShowConsole();
		CnsClass::Print("Red","Failure in window creation (%d)\n", GetLastError());
		LogClass::LOG("(ERROR) [Main] Failure in window creation (%d)", GetLastError());
		MessageBoxA(NULL, "Window creation is failed.", (LPCSTR)WndName, MB_OK | MB_ICONERROR);
		return FALSE;
	};

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HFONT hFont2;
	LPCSTR titletext1, titletext2;

	switch (message)
	{
		case WM_CREATE:
			Window::WindowMenu(hWnd);
			break;
		case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);

			static LOGFONTA lf;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfPitchAndFamily = DEFAULT_PITCH;
			strcpy(lf.lfFaceName, *FontName);
			lf.lfHeight = 15;
			lf.lfWidth = 6;
			lf.lfWeight = 5;
			lf.lfEscapement = 0; //шрифт без поворота

			hFont2 = CreateFontIndirectA(&lf);
			SelectObject(hdc, hFont2);
			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkMode(hdc, TRANSPARENT);

			titletext1 = ReadJSONLocTag("cChLoc").c_str();
			TextOutA(hdc, 10, 20, titletext1, strlen(titletext1));

			DeleteObject(hFont2); //выгрузим из памяти объект шрифта

			EndPaint(hWnd, &ps);
		}
		break;
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_ABOUT)
			{
				DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			}
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				char ListItem[256];
				int ItemIndex = SendMessage(hComboBox, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				SendMessage(hComboBox, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)ListItem);
				if (ItemIndex == 1)
					InstallLang = 0;
				else
					InstallLang = 1;
			}
			if (LOWORD(wParam) == ID_START)
				Window::BrowseForFolder();
			if (LOWORD(wParam) == ID_FAQ)
				ShellExecute(0, 0, L"https://steamcommunity.com/sharedfiles/filedetails/?id=766483986", 0, 0, SW_SHOW);
			break;
		}
		break;
		case WM_DISPLAYCHANGE:
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case WM_CLOSE:
		{
			titletext2 = ReadJSONLocTag("bOnExit").c_str();

			_bstr_t b(WndName);
			const char* newTitle = b;

			const int result = MessageBoxA(hWnd, titletext2, newTitle, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
			switch (result)
			{
				case IDYES:
					DestroyWindow(hWnd);
			}
		break;
		}
		case WM_DESTROY:
		{
			FileClass::FilesDelete();
			LogClass::ReleaseLog();
			PostQuitMessage(0);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
int CALLBACK BrowsePathProc(HWND hWnd, UINT message, LPARAM lParam, LPARAM pData)
{
	wchar_t szDir[2048];
	switch (message)
	{
		case BFFM_INITIALIZED:
		{
			LPCTSTR pszInitialPath = reinterpret_cast<LPCTSTR>(pData);
			SendMessage(hWnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(pszInitialPath));
		}
		break;
		case BFFM_SELCHANGED:
			if (SHGetPathFromIDListW((LPITEMIDLIST)lParam, szDir))
				SendMessage(hWnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)szDir);
		break;
	}
	return 0;
}
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
BOOL WINAPI CnsHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
		//this just disables Ctrl-C
	case CTRL_C_EVENT:
		return TRUE;
		// if user pressed "X" button on console window
	case CTRL_CLOSE_EVENT:
	{
		if (Beginning != FALSE)
		{
			FileClass::FilesDelete();
			LogClass::LOG("(WARNING) [Main] A process was terminated by user prematurely!");
			Beginning = FALSE;
		}
		LogClass::ReleaseLog();
		FreeConsole();
		return TRUE;
	}
	default:
		return FALSE;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LogClass::InitLog();
	LogClass::LOG("(INFO) [Main] InitWindow...");

	AllocConsole();
	FILE* CnsOpen = freopen("CONOUT$", "w", stdout);
	SetConsoleCtrlHandler(CnsHandler, TRUE);
	CnsClass::HideConsole();

	std::string xmltag = ReadXMLConfigTag("InterfaceLang");
	if (xmltag == "English")
		CurrentLang = 1;

	Lang1 = ReadJSONLocTag("cChLoc1").c_str();
	Lang2 = ReadJSONLocTag("cChLoc2").c_str();

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!RegisterMainClass(hInstance)) {
		CnsClass::ShowConsole();
		CnsClass::Print("Red", "Failure in class registration (%d)", GetLastError());
		LogClass::LOG("(ERROR) [Main] Failure in class registration (%d)", GetLastError());
		return FALSE;
	};

	if (!InitInstance(hInstance, nCmdShow))
	{
		CnsClass::ShowConsole();
		CnsClass::Print("Red", "Failure in init application(%d)", GetLastError());
		LogClass::LOG("(ERROR) [Main] Failure in init application (%d)", GetLastError());
		MessageBoxA(NULL, "Init application is failed.", (LPCSTR)WndName, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	LogClass::LOG("(INFO) [Main] InitWindow: Ok!");

	// Message loop
	MSG msg = { 0 };

	BOOL bRet;
	while (bRet = GetMessage(&msg, NULL, 0, 0) != 0)
	{
		if (bRet != -1)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}