#include <Windows.h>
#include <Shlwapi.h>
#include <string>
#include <shlobj.h>
#include <stdio.h>

#include "pugixml_read.h"
#include "main.h"

#pragma warning(disable : 4244)
#pragma warning(disable : 4302)

#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"zlib.lib")
#pragma comment(lib,"libcurl.lib")
#pragma comment(lib,"pugixml.lib")

// Заставляем линкер генерировать манифест визуальных стилей окон
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WINDOW_CLASS L"Main window class"	

#define WINDOW_ICON MAKEINTRESOURCE(IDI_ICON1)
#define WINDOW_BGND MAKEINTRESOURCE(IDB_BITMAP1)

// Разрешение создаваемого окна (картинку тоже регулирует)
#define WINDOW_SIZE_X 640
#define WINDOW_SIZE_Y 480

#define NO_WIN32_LEAN_AND_MEAN

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

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

LPCWSTR FontName[]		= { L"Verdana" };

const char* FTP_URL		= "ftp://158.46.49.38/";
const wchar_t* WndName  = L"Star Wolves 3: New Civil War Installer";
const wchar_t* progver	= L"1.5";
const wchar_t* Lang1	= L"Русский (Russian)";
const wchar_t* Lang2	= L"Английский (English)";
const wchar_t* LogFile	= L"logfile.txt";
int InstallLang			= 0; // 0 - Russian, 1 - English
bool QueueError			= FALSE;
bool Beginning			= FALSE;
int CurrentLang;
FILE* CnsOpen;

void Window::WindowMenu(HWND hWnd)
{
	struct texts {
		LPCWSTR text1;
		LPCWSTR text2;
		LPCWSTR text3;
	} text;
	if (CurrentLang == 0)
	{
		text.text1 = L"Начать установку";
		text.text2 = L"Руководство по моду";
		text.text3 = L"О программе";
	}
	else
	{
		text.text1 = L"Start installation";
		text.text2 = L"Guide";
		text.text3 = L"About";
	}

	hButtonStart = CreateWindowEx(
		0,
		L"BUTTON",
		text.text1,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, WINDOW_SIZE_Y - 160,
		165, 30,
		hWnd,
		(HMENU)ID_START,
		hInstance,
		NULL
	);

	
	hButtonInfo = CreateWindowEx(
		0,
		L"BUTTON",
		text.text2,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, WINDOW_SIZE_Y - 120,
		165, 30,
		hWnd,
		(HMENU)ID_FAQ,
		hInstance,
		NULL
	);

	hButtonAbout = CreateWindowEx(
		0,
		L"BUTTON",
		text.text3,
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

	SendMessage(hButtonStart, WM_SETFONT, (WCHAR)hFont, NULL);
	SendMessage(hButtonInfo, WM_SETFONT, (WCHAR)hFont, NULL);
	SendMessage(hButtonAbout, WM_SETFONT, (WCHAR)hFont, NULL); 
	SendMessage(hComboBox, WM_SETFONT, (WCHAR)hFont, NULL);

	SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)Lang1);
	SendMessage(hComboBox, CB_ADDSTRING, 0, (LPARAM)Lang2);

	ShowWindow(hWnd, SW_SHOW);
	SendMessage(hComboBox, CB_SETCURSEL, (WPARAM)1, (LPARAM)0);

}
void Window::ChangeLanguage()
{
	LPCWSTR str;
	if (CurrentLang == 0)
	{
		CurrentLang = 1;
		SendMessage(hButtonStart, WM_SETTEXT, 0, (LPARAM)L"Start installation");
		SendMessage(hButtonInfo, WM_SETTEXT, 0, (LPARAM)L"Guide");
		SendMessage(hButtonAbout, WM_SETTEXT, 0, (LPARAM)L"About");
		str = L"Choose the localization:\0";
		TextOutW(hdc, 10, 20, str, wcslen(str));
		LogClass::LogMessage("(INFO) [Main] Change UI language: English", 0, 0, 0);
		WriteXMLConfigTag("InterfaceLang", "English");
	}
	else
	{
		CurrentLang = 0;
		SendMessage(hButtonStart, WM_SETTEXT, 0, (LPARAM)L"Начать установку");
		SendMessage(hButtonInfo, WM_SETTEXT, 0, (LPARAM)L"Руководство по моду");
		SendMessage(hButtonAbout, WM_SETTEXT, 0, (LPARAM)L"О программе");
		str = L"Выберите локализацию:\0";
		TextOutW(hdc, 10, 20, str, wcslen(str));
		LogClass::LogMessage("(INFO) [Main] Change UI language: Russian", 0, 0, 0);
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
	LPITEMIDLIST pidl;
	BOOL fRet;
	BROWSEINFO bi = { 0 };
	ZeroMemory(&bi, sizeof(bi));

	wchar_t text[52];
	if (CurrentLang == 0)
		wcscpy(text, L"Выберите папку с игрой Star Wolves 3: Civil War");
	else
		wcscpy(text, L"Choose a game folder with Star Wolves 3: Civil War");
	
	bi.hwndOwner	  = hWnd;
	bi.pidlRoot		  = NULL;
	bi.pszDisplayName = (LPWSTR)DestDir;
	bi.lpszTitle	  = text;
	bi.ulFlags		  = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
	bi.lpfn			  = &BrowsePathProc;
	bi.iImage		  = 0;
	bi.lParam		  = (LPARAM)L"C:\\Program Files (x86)";

	pidl = ::SHBrowseForFolder(&bi);

	if (!pidl)
		fRet = NULL;
	else
	{
		fRet = SHGetPathFromIDList(pidl, (LPWSTR)DestDir);
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
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= CreatePatternBrush((HBITMAP)LoadImage(hInstance, WINDOW_BGND, IMAGE_BITMAP, WINDOW_SIZE_X, WINDOW_SIZE_Y, LR_COPYFROMRESOURCE));
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= WINDOW_CLASS;
	wc.hIconSm			= LoadIcon(hInstance, WINDOW_ICON);

	return RegisterClassEx(&wc);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hFont = CreateFont(-13, 0, 0, 0,
		FW_NORMAL, 0,
		0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		*FontName);

	hWnd = CreateWindowEx(
		WS_EX_CONTROLPARENT,
		WINDOW_CLASS,
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
		printf("InitInstance: Window creation is failed (C++ error code: %d)\n", GetLastError());
		LogClass::LogMessage("(ERROR) [Main] Window creation is failed:", 1, 0, 0);
		MessageBox(NULL, L"Window creation is failed.", WndName, MB_OK | MB_ICONERROR);
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
	LPCWSTR str;
	if (CurrentLang == 0)
		str = L"Выберите локализацию:\0";
	else
		str = L"Choose the localization:\0";
	switch (message)
	{
		case WM_CREATE:
			Window::WindowMenu(hWnd);
			break;
		case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);

			static LOGFONT lf;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfPitchAndFamily = DEFAULT_PITCH;
			wcscpy(lf.lfFaceName, *FontName);
			lf.lfHeight = 15;
			lf.lfWidth = 6;
			lf.lfWeight = 5;
			lf.lfEscapement = 0; //шрифт без поворота

			hFont2 = CreateFontIndirect(&lf);
			SelectObject(hdc, hFont2);
			SetTextColor(hdc, RGB(255, 255, 255));
			SetBkMode(hdc, TRANSPARENT);
			TextOutW(hdc, 10, 20, str, wcslen(str));

			DeleteObject(hFont2); //выгрузим из памяти объект шрифта

			EndPaint(hWnd, &ps);
		}
		break;
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_ABOUT)
			{
				wchar_t text[200];
				if (CurrentLang == 0)
				{
					wcscpy(text, L"Версия установщика: ");
					wcscat(text, progver);
					wcscat(text, L"\nИспользуются библиотеки:\ncURL, PugiXML, zlib, Shlwapi.\n\n");
					wcscat(text, L"Автор: Алексей 'Aleksey_SR' Петрачков");
				}
				else
				{
					wcscpy(text, L"Version: ");
					wcscat(text, progver);
					wcscat(text, L"\nUsing libraries:\ncURL, PugiXML, zlib, Shlwapi.\n\n");
					wcscat(text, L"Author: Alex 'Aleksey_SR' Petrachkov");
				}

				MessageBoxW(hWnd, text, WndName, MB_OK | MB_ICONQUESTION);
			}
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int ItemIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				TCHAR  ListItem[256];
				(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)ListItem);
				wcscat((LPWSTR)ListItem, L"\0");
				if (wcslen((LPWSTR)ListItem) == wcslen(Lang2))
					InstallLang = 2;
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
			wchar_t text[55];
			if (CurrentLang == 0)
				wcscpy(text, L"Вы уверены, что хотите выйти из программы установки?");
			else
				wcscpy(text, L"Are you sure to close the installer?");

			const int result = MessageBox(hWnd, text, WndName, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
			switch (result)
			{
				case IDYES:
					DestroyWindow(hWnd);
			}
		break;
		}
		case WM_DESTROY:
		{
			if (FileClass::FileExists("1_temp.exe") == 1)
				DeleteFileA("1_temp.exe");
			if (FileClass::FileExists("2_temp.exe") == 1)
				DeleteFileA("2_temp.exe");
			if (FileClass::FileExists("3_temp.exe") == 1)
				DeleteFileA("3_temp.exe");
			if (FileClass::FileExists("4_temp.exe") == 1)
				DeleteFileA("4_temp.exe");
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
			if (Beginning)
				LogClass::LogMessage("(WARNING) [Main] A process was terminated by user prematurely!", 0, 0, 0);
			Beginning = FALSE;
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
	LogClass::LogMessage("(INFO) [Main] InitWindow...", 0, 0, 0);

	AllocConsole();
	CnsOpen = freopen("CONOUT$", "w", stdout);
	SetConsoleCtrlHandler(CnsHandler, TRUE);
	CnsClass::HideConsole();

	string xmltag = ReadXMLConfigTag("InterfaceLang");
	if (xmltag == "Russian" || xmltag != "English")
		CurrentLang = 0;
	else
		CurrentLang = 1;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!RegisterMainClass(hInstance)) {
		CnsClass::ShowConsole();
		printf("RegisterMainClass: Critical error (C++ error code: %d)\n", GetLastError());
		LogClass::LogMessage("(ERROR) [Main] Critical error in application (cannot register class) :", 1, 0, 0);
		return FALSE;
	};

	if (!InitInstance(hInstance, nCmdShow))
	{
		CnsClass::ShowConsole();
		printf("InitInstance: Cannot init application (C++ error code: %d)\n", GetLastError());
		LogClass::LogMessage("(ERROR) [Main] Init application is failed:", 1, 0, 0);
		MessageBox(NULL, L"Init application is failed.", WndName, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	LogClass::LogMessage("(INFO) [Main] InitWindow: Ok!", 0, 0, 0);

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