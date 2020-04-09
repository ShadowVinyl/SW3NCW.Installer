#include <Windows.h>
#include <Shlwapi.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <shlobj.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <Tlhelp32.h>
#include <ctime>
#include <curl/curl.h>

#include "main.h"

#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#pragma warning(disable : 4302)

// Заставляем линкер генерировать манифест визуальных стилей окон
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"Shlwapi.lib")
#pragma comment(lib,"libcurl.lib")

// Имя окна
#define WINDOW_NAME L"Star Wolves 3: New Civil War Installer"

#define WINDOW_CLASS L"Main window class"	

#define WINDOW_ICON MAKEINTRESOURCE(IDI_ICON1)
#define WINDOW_BGND MAKEINTRESOURCE(IDB_BITMAP1)

// Разрешение создаваемого окна (картинку тоже регулирует)
#define WINDOW_SIZE_X 640
#define WINDOW_SIZE_Y 480

#define NO_WIN32_LEAN_AND_MEAN

// ID
#define ID_START 1
#define ID_PROGRESS 2
#define ID_FAQ 3
#define ID_ABOUT 4
#define ID_COMBOBOX 5

//#define CONSOLEAPP

HINSTANCE hInstance;
HWND hWnd, hWndConsole, hButtonStart, hButtonInfo, hButtonAbout, hComboBox;
HFONT hFont;

LPCWSTR FontName[] = { L"Verdana" };

const char* FTP_URL		= "ftp://158.46.49.38/";
const wchar_t* version	= L"1.5";
const wchar_t* Lang1	= L"Русский (Russian)";
const wchar_t* Lang2	= L"Английский (English)";
const wchar_t* LogFile	= L"logfile.txt";
int CurrentLang			= 1; // 1 - Russian, 2 - English
int InstallLang			= 1; // 1 - Russian, 2 - English
bool QueueError			= FALSE;
bool Beginning			= FALSE;
FILE* CnsOpen;

static size_t CurlWriteData(void* ptr, size_t size, const size_t nmemb, FILE* stream)
{
	return fwrite(ptr, size, nmemb, stream);
}
static size_t CurlGetSize(void* ptr, size_t size, size_t nmemb, void* data)
{
	(void)ptr;
	(void)data;
	// return size only
	return (size_t)(size * nmemb);
}
int CurlProgress(void* ptr, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded)
{
	// ensure that the file to be downloaded is not empty
	// because that would cause a division by zero error later on
	if (TotalToDownload <= 0.0) 
		return 0;
	
	double fractiondownloaded = NowDownloaded / TotalToDownload;

	printf("Progress: %3.0f%% [", fractiondownloaded * 100);

	// create the "meter"
	
	int ii = 0;
	// how wide you want the progress meter to be
	int totaldotz = 30;
	// part of the progressmeter that's already "full"
	int dotz = round(fractiondownloaded * totaldotz);
	// part  that's full already
	for (; ii < dotz; ii++) {
		printf("=");
	}
	// remaining part (spaces)
	for (; ii < totaldotz; ii++) {
		printf(" ");
	}
	// and back to line begin - do not forget the fflush to avoid output buffering problems!
	printf("]\r");
	
	
	fflush(stdout);
	// if you don't return 0, the transfer will be aborted - see the documentation
	return 0;
}

void Console::HideConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}
void Console::ShowConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}
void Console::SetTextColor(int ColourIndex)
{
	// 2 - green, 4 - red, 14 - yellow, 15 - white
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdOut, (WORD)(ColourIndex));
}

void Window::WindowMenu(HWND hWnd)
{
	hButtonStart = CreateWindowEx(
		0,
		L"BUTTON",
		L"Начать установку",
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
		L"Руководство по моду",
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
		L"О программе",
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
	if (CurrentLang == 1)
	{
		CurrentLang = 0;
		SendMessage(hButtonStart, WM_SETTEXT, 0, (LPARAM)L"Start installation");
		SendMessage(hButtonInfo, WM_SETTEXT, 0, (LPARAM)L"Руководство");
		SendMessage(hButtonAbout, WM_SETTEXT, 0, (LPARAM)L"About");
		LOG::LogMessage("[Main] Change UI language: English", 0, 0, 0);
	}
	else
	{
		CurrentLang = 1;
		SendMessage(hButtonStart, WM_SETTEXT, 0, (LPARAM)L"Начать установку");
		SendMessage(hButtonInfo, WM_SETTEXT, 0, (LPARAM)L"Guide");
		SendMessage(hButtonAbout, WM_SETTEXT, 0, (LPARAM)L"О программе");
		LOG::LogMessage("[Main] Change UI language: Russian", 0, 0, 0);
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
	if (CurrentLang == 1)
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
		File::FileQueueSet(DestDir);
	}
	return fRet;
}

int  File::FtpGetStatus()
{
	CURL* curl;
	CURLcode result;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, FTP_URL);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		result = curl_easy_perform(curl);

		if (result == CURLE_OK)
		{
			Console::SetTextColor(2);
			printf("Server is online!\n\n");
			LOG::LogMessage("[FileDownload] Server is online.", 0, 0, 0);
			Console::SetTextColor(15);
		}
		else
		{
			Console::SetTextColor(4);
			printf("Server is offline (CURL code: %d)\n", result);
			Console::SetTextColor(15);
			LOG::LogMessage("[FileDownload] Server is offline:", 1, 0, result);
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return result;
}
int  File::FileSize(const char* FileName)
{
	std::fstream file(FileName);
	file.open(FileName);
	if (file.bad())
	{
		printf("FileSize: cant open a file (C++ error code: %d)", GetLastError());
		LOG::LogMessage("[FileSize] Cant open a file", 1, FileName, GetLastError());
		return 0;
	}

	int size = 0;
	file.seekg(0, std::ios::end);
	size = file.tellg();
	file.close();
	return size;
}
bool File::FileExists(const char* FileName)
{
	std::ifstream file;
	file.open(FileName);
	if (!file) return 0;
	file.close();
	return 1;
}
double File::FtpGetFileSize(char* FileName)
{
	CURL* curl;
	CURLcode res;
	//long filetime = -1;
	double filesize = 0.0;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl) {

		char Host[50];
		strcpy(Host, FTP_URL);
		strcat(Host, FileName);

		curl_easy_setopt(curl, CURLOPT_URL, Host);
		/* No download if the file */
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
		/* Ask for filetime */
		//curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlGetSize);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
		/* Switch on full protocol/debug output */
		/* curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L); */

		res = curl_easy_perform(curl);

		if (res == CURLE_OK) {
			/*
			res = curl_easy_getinfo(curl, CURLINFO_FILETIME, &filetime);
			if ((CURLE_OK == res) && (filetime >= 0)) {
				time_t file_time = (time_t)filetime;
				printf("File time %s: %s", FileName, ctime(&file_time));
			}
			*/
			res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD,
				&filesize);
			if ((CURLE_OK == res) && (filesize > 0.0))
				printf("File size %s: %0.0f Kbytes\n", FileName, filesize / 1024);
		}
		else {
			/* we failed */
			fprintf(stderr, "Failed to get file size (CURL code: %d)\n", res);
			LOG::LogMessage("Failed to get file size:", 1, 0, res);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	return filesize;
}
//
void File::FileQueueSet(wchar_t* DestDir)
{
	Beginning = TRUE;
	Console::ShowConsole();
	ShowWindow(hWnd, SW_HIDE);
	LOG::LogMessage("[Main] A process was started.", 0, 0, 0);
	if (InstallLang == 1)
		LOG::LogMessage("[Main] Use Russian version of mod.", 0, 0, 0);
	else
		LOG::LogMessage("[Main] Use English version of mod.", 0, 0, 0);

	// Устанавливаем заданную директорию "по умолчанию"
	wchar_t Destination[2048];
	wcscpy(Destination, DestDir);
	wcscat(Destination, L"DataTemp\\");
	CreateDirectoryW(Destination, NULL);
	SetCurrentDirectoryW(Destination);
	LOG::LogMessage(L"[Main] Destination path:", 0, (LPCWSTR)Destination, 0);

	char files_exe[][40] = {
		"1_temp.exe",
		"2_temp.exe",
		"3_temp.exe",
		"4_temp.exe"
	};

	for (int i = 0; i < 4; i++)
	{
		system("cls");
		Console::SetTextColor(14);
		wprintf(L"Destination: %s\n", Destination);
		printf("File name: %s\n\n", files_exe[i]);
		Console::SetTextColor(15);

		if (!FileDownload(files_exe[i]))
		{
			QueueError = TRUE;
			Sleep(2000);
			break;
		}

		Sleep(2000);
		if (!FileOpen(files_exe[i]))
		{
			Console::SetTextColor(4);
			printf("Error! File %s is not found in the destination directory! Installation is failed!\n", files_exe[i]);
			LOG::LogMessage("[Main] File is not found in the destination directory:", 1, files_exe[i], 0);
			Console::SetTextColor(15);
			QueueError = TRUE;
			Sleep(2000);
			break;
		}

		Sleep(1000);
		DeleteFileA(files_exe[i]);
	}

	system("cls");
	SetCurrentDirectoryW(DestDir);
	if (!QueueError)
	{
		LOG::LogMessage("[Windows Shell] Moving files into DATA folder...", 0, 0, 0);
		wprintf(L"Moving files into DATA folder...\n");
		Console::SetTextColor(14);
		wprintf(L"Please, don't terminate the process to avoid mistakes!\n");
		Console::SetTextColor(15);
		if (!ShellMoveFiles(L"DataTemp\\Data\\*\0", L"Data\0"))
		{
			QueueError = TRUE;
			Sleep(2000);
		}
		else
		{
			Console::SetTextColor(2);
			wprintf(L"Moving files into DATA folder: Ok!\n");
			Console::SetTextColor(15);
		}
	}

	RemoveDirectoryW(L"DataTemp\\Data");
	RemoveDirectoryW(L"DataTemp");

	Sleep(2000);
	ShowWindow(hWnd, SW_SHOW);
	Console::HideConsole();
	if (!QueueError)
	{
		wchar_t text[25];
		if (CurrentLang == 1)
			wcscpy(text, L"Установка завершена.");
		else
			wcscpy(text, L"Installation is done.");
		MessageBoxW(hWnd, text, WINDOW_NAME, MB_OK | MB_ICONQUESTION);
		LOG::LogMessage("[Main] Process was done successfully.\n", 0, 0, 0);
	}
	else
	{
		QueueError = FALSE;
		wchar_t text[80];
		if (CurrentLang == 1)
			wcscpy(text, L"Установка завершена с ошибками. Проверьте logfile.txt для поиска проблемы.");
		else
			wcscpy(text, L"Installation is failed. See logfile.txt for searching problems.");
		MessageBoxW(hWnd, text, WINDOW_NAME, MB_OK | MB_ICONERROR);
		LOG::LogMessage("[Main] Process was done with errors.\n", 0, 0, 0);
	}
	Beginning = FALSE;
}
bool File::FileDownload(char* FileName)
{
	printf("Load file %s from local server...\n", FileName);
	LOG::LogMessage("[FileDownload] Load file from local server:", 0, FileName, 0);

	printf("Connect to local server...\n");
	LOG::LogMessage("[FileDownload] Connect to local server...", 0, 0, 0);
	int result = FtpGetStatus();
	if (result != CURLE_OK)
		return 0;

	double size = FtpGetFileSize(FileName);

	CURL* curl = curl_easy_init();

	if (curl)
	{
		char Host[50];
		strcpy(Host, FTP_URL);
		strcat(Host, FileName);

		FILE* ofile = fopen(FileName, "wb");

		if (ofile)
		{
			curl_easy_setopt(curl, CURLOPT_URL, Host);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, ofile);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteData);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
			// Install the callback function
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, CurlProgress);
			printf("\nDownload: %s\n", FileName);
			LOG::LogMessage("[FileDownload] Download:", 0, FileName, 0);

			int result = curl_easy_perform(curl);
			if (result != CURLE_OK)
			{
				Console::SetTextColor(4);
				printf("Error! Host is offline (CURL code: %d).\nPlease try again later.\n", result);
				Console::SetTextColor(15);
				LOG::LogMessage("[FileDownload] Host is offline. Download is failed:", 1, 0, result);
				fclose(ofile);
				remove(FileName);
				curl_easy_cleanup(curl);
				return 0;
			}
			else
			{
				printf("Progress: 100%%\n");
				Console::SetTextColor(2);
				printf("\nDownload: Ok!\n");
				LOG::LogMessage("[FileDownload] Download: Ok!", 0, 0, 0);
			}
			fclose(ofile);
		}

		// always cleanup
		curl_easy_cleanup(curl);
	}

	printf("Load file %s from local server: Ok!\n", FileName);
	Console::SetTextColor(15);
	return 1;
}
bool File::FileOpen(char* FileName)
{
	STARTUPINFOA cif;
	ZeroMemory(&cif, sizeof(STARTUPINFOA));
	PROCESS_INFORMATION pi;

	printf("\nStarting extraction...\n");
	LOG::LogMessage("[FileOpen] Starting extraction:", 0, FileName, 0);
	if (!CreateProcessA((LPCSTR)FileName, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &cif, &pi))
	{
		Console::SetTextColor(4);
		printf("Cant open a file (C++ error code: %d)\n", GetLastError());
		LOG::LogMessage("[FileOpen] Cant open a file:", 1, FileName, GetLastError());
		Console::SetTextColor(15);
		return 0;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	Console::SetTextColor(2);
	printf("Extraction is done.\n\n");
	LOG::LogMessage("[FileOpen] Extraction is done:", 0, FileName, 0);
	Console::SetTextColor(15);
	return 1;
}
int  File::ShellMoveFiles(const wchar_t* srcPath, const wchar_t* newPath)
{
	const wchar_t* Src  = srcPath;
	const wchar_t* Dest = newPath;

	SHFILEOPSTRUCTW fileOperation;
	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));

	fileOperation.wFunc  = FO_MOVE;
	fileOperation.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SIMPLEPROGRESS;
	fileOperation.pFrom  = Src;
	fileOperation.pTo    = Dest;
	fileOperation.hwnd   = hWnd;

	int result = 1;
	int ShellResult = SHFileOperationW(&fileOperation);
	if (ShellResult != 0)
	{
		LOG::LogMessage("[Windows Shell] Error code:", 1, 0, ShellResult);
		Console::SetTextColor(4);
		printf("SHFileOperation Failure: %u\n", ShellResult);
		Console::SetTextColor(15);
		result = 0;
	}

	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));

	return result;
}

void LOG::InitLog()
{
	const wchar_t* str = version;
	char buf[32];
	size_t len = wcstombs(buf, str, wcslen(str));
	if (len > 0u)
		buf[len] = '\0';
	puts(buf);

	time_t t = time(0);

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"w");
	fprintf(out, "Log started: %s\n", ctime(&t));
	fprintf(out, "Version: %s\n\n", buf);
	fclose(out);
}
void LOG::LogMessage(const char* message, int ErrorCode, const char* param1, int param2)
{
	const int MAXLEN = 80;
	char s[MAXLEN];
	time_t t = time(0);
	strftime(s, MAXLEN, "%H:%M:%S", localtime(&t));

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"a");
	if (out)
	{
		if (!param1) param1 = "";
		if (!param2) param2 = 0;
		if (ErrorCode == 1)
			fprintf(out, "(%s) (ERROR) %s %s %d\n", s, message, param1, param2);
		else if (ErrorCode == 2)
			fprintf(out, "(%s) (WARNING) %s %s\n", s, message, param1);
		else
			fprintf(out, "(%s) (INFO) %s %s\n", s, message, param1);
		fclose(out);
	}
}
void LOG::LogMessage(const wchar_t* message, int ErrorCode, const wchar_t* param1, int param2)
{
	const int MAXLEN = 80;
	wchar_t s[MAXLEN];
	time_t t = time(0);
	wcsftime(s, MAXLEN, L"%H:%M:%S", localtime(&t));

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"a");
	if (out)
	{
		if (!param1) param1 = L"";
		if (!param2) param2 = 0;
		if (ErrorCode == 1)
			fwprintf(out, L"(%s) (ERROR) %s %s %d\n", s, message, param1, param2);
		else if (ErrorCode == 2)
			fwprintf(out, L"(%s) (WARNING) %s %s\n", s, message, param1);
		else
			fwprintf(out, L"(%s) (INFO) %s %s\n", s, message, param1);
		fclose(out);
	}
}
void LOG::ReleaseLog()
{
	const wchar_t* str = version;
	char buf[32];
	size_t len = wcstombs(buf, str, wcslen(str));
	if (len > 0u)
		buf[len] = '\0';
	puts(buf);

	time_t t = time(0);

	wchar_t selfdir[MAX_PATH] = { 0 };
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\\0");
	wcscat(selfdir, LogFile);

	FILE* out = _wfopen(selfdir, L"a");
	fprintf(out, "\nLog stopped: %s\n", ctime(&t));
	fclose(out);
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
		WINDOW_NAME,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
		(GetSystemMetrics(SM_CXSCREEN) - WINDOW_SIZE_X) / 2, (GetSystemMetrics(SM_CYSCREEN) - WINDOW_SIZE_Y) / 2,
		WINDOW_SIZE_X, WINDOW_SIZE_Y,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hWnd) {
		Console::ShowConsole();
		printf("InitInstance: Window creation is failed (C++ error code: %d)\n", GetLastError());
		LOG::LogMessage("[Main] Window creation is failed:", 1, 0, GetLastError());
		MessageBox(NULL, L"Window creation is failed.", WINDOW_NAME, MB_OK | MB_ICONERROR);
		return FALSE;
	};

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	HFONT hFont2;
	const wchar_t* str = L"Выберите локализацию:\0";
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
				if (CurrentLang == 1)
				{
					wcscpy(text, L"Версия установщика: ");
					wcscat(text, version);
					wcscat(text, L"\nДля связи по сети используется библиотека CURL.\n\n");
					wcscat(text, L"Автор: Алексей 'Aleksey_SR' Петрачков");
				}
				else
				{
					wcscpy(text, L"Version: ");
					wcscat(text, version);
					wcscat(text, L"\nThe programm use CURL library for network connections.\n\n");
					wcscat(text, L"Author: Alex 'Aleksey_SR' Petrachkov");
				}

				MessageBoxW(hWnd, text, WINDOW_NAME, MB_OK | MB_ICONQUESTION);
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
			if (CurrentLang == 1)
				wcscpy(text, L"Вы уверены, что хотите выйти из программы установки?");
			else
				wcscpy(text, L"Are you sure to close the installer?");

			const int result = MessageBox(hWnd, text, WINDOW_NAME, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
			switch (result)
			{
				case IDYES:
					DestroyWindow(hWnd);
			}
		break;
		}
		case WM_DESTROY:
		{
			if (File::FileExists("1_temp.exe") == 1)
				DeleteFileA("1_temp.exe");
			if (File::FileExists("2_temp.exe") == 1)
				DeleteFileA("2_temp.exe");
			if (File::FileExists("3_temp.exe") == 1)
				DeleteFileA("3_temp.exe");
			if (File::FileExists("4_temp.exe") == 1)
				DeleteFileA("4_temp.exe");
			LOG::ReleaseLog();
			PostQuitMessage(0);
			// for console app (not for windows app)
			#ifdef CONSOLEAPP
				PostMessage(hWndConsole, WM_CLOSE, 0, 0);
			#endif
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
			::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(pszInitialPath));
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
	case CTRL_C_EVENT:
		return TRUE;    //this just disables Ctrl-C
	// if user pressed "X" button on console window
	case CTRL_CLOSE_EVENT:
	{
		if (Beginning)
			LOG::LogMessage("[Main] A process was terminated by user prematurely!", 2, 0, 0);
		LOG::ReleaseLog();
		FreeConsole();
		return TRUE;
	}
	default:
		return FALSE;
	}
}

// Custom Entry Point for console/window app
// WARNING: use different types of linker options (console app or windows app or set it to null) to prevent linker error
#ifdef CONSOLEAPP
int main(int argc, char** argv)
{
	LOG::InitLog();
	LOG::LogMessage("[Main] InitWindow...", 0, 0, 0);

	char title[500];
	GetConsoleTitleA(title, 500);
	SetConsoleCtrlHandler(CnsHandler, TRUE);

	hWndConsole = FindWindowA(NULL, title);
	ShowWindow(hWndConsole, SW_SHOWNORMAL);
	hInstance = GetModuleHandle(NULL);
	Console::HideConsole();
	//MoveWindow(hWndConsole, 20, 20, 500, 500, true);  // test hwnd

	if (!RegisterMainClass(hInstance)) {
		Console::ShowConsole();
		printf("RegisterMainClass: Class registation is failed (C++ error code: %d)\n", GetLastError());
		LOG::LogMessage("[Main] Critical error in application (cannot register class) :", 1, 0, GetLastError());
		MessageBox(NULL, L"Class registation is failed.", WINDOW_NAME, MB_OK | MB_ICONERROR);
		return FALSE;
	};
	
	if (!InitInstance(hInstance, SW_SHOWNORMAL))
	{
		Console::ShowConsole();
		printf("InitInstance: Cannot init application (C++ error code: %d)\n", GetLastError());
		LOG::LogMessage("[Main] Init application is failed:", 1, 0, GetLastError());
		MessageBox(NULL, L"Init application is failed.", WINDOW_NAME, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	LOG::LogMessage("[Main] InitWindow: Ok!", 0, 0, 0);

	// Цикл обработки сообщений
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

	LOG::ReleaseLog();
	
	return msg.wParam;
}
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	LOG::InitLog();
	LOG::LogMessage("[Main] InitWindow...", 0, 0, 0);

	AllocConsole();
	CnsOpen = freopen("CONOUT$", "w", stdout);
	SetConsoleCtrlHandler(CnsHandler, TRUE);
	Console::HideConsole();

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (!RegisterMainClass(hInstance)) {
		Console::ShowConsole();
		printf("RegisterMainClass: Critical error (C++ error code: %d)\n", GetLastError());
		LOG::LogMessage("[Main] Critical error in application (cannot register class) :", 1, 0, GetLastError());
		return FALSE;
	};

	if (!InitInstance(hInstance, nCmdShow))
	{
		Console::ShowConsole();
		printf("InitInstance: Cannot init application (C++ error code: %d)\n", GetLastError());
		LOG::LogMessage("[Main] Init application is failed:", 1, 0, GetLastError());
		MessageBox(NULL, L"Init application is failed.", WINDOW_NAME, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	LOG::LogMessage("[Main] InitWindow: Ok!", 0, 0, 0);

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
#endif