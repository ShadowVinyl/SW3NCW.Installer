#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif

#include <Windows.h>
#include <Shlwapi.h>
#include <cstdio>
#include <fstream>
#include <comdef.h>
#include <curl/curl.h>

#include "pugixml_read.h"
#include "json_read.h"
#include "FileClass.h"
#include "CnsClass.h"
#include "LogClass.h"

extern HWND hWnd;
extern const wchar_t* WndName;
extern const char* FTP_URL;
extern bool QueueError;
extern bool Beginning;
extern int InstallLang;
extern int CurrentLang;

extern FileClass File;
extern LogClass LOG;
extern CnsClass Console;

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

	printf("Progress: %5.1f%% [", fractiondownloaded * 100);

	// create the "meter"

	int ii = 0;
	// how wide you want the progress meter to be
	int totaldotz = 30;
	// part of the progressmeter that's already "full"
	double dotz = round(fractiondownloaded * totaldotz);
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

std::wstring GetExePath()
{
	wchar_t selfdir[MAX_PATH];
	GetModuleFileNameW(NULL, selfdir, MAX_PATH);
	PathRemoveFileSpecW(selfdir);
	wcscat(selfdir, L"\\");
	return selfdir;
}

int  FileClass::FtpGetStatus()
{
	CURLcode result;
	curl_global_init(CURL_GLOBAL_ALL);
	CURL* curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, FTP_URL);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
		result = curl_easy_perform(curl);

		if (result != CURLE_OK)
		{
			Console.Print("Red", "Server is offline (%d)", result);
			LOG.LOG("(ERROR) [FileDownload] Server is offline (%d)", result);
		}
		curl_easy_cleanup(curl);
		curl_global_cleanup();
	}
	return result;
}
long FileClass::FileSize(const char* FileName)
{
	FILE* file = fopen(FileName, "r");
	if (!file)
	{
		Console.Print("Red", "FileSize: cant open a file (%d)", GetLastError());
		LOG.LOG("(ERROR) [FileSize] Cant open a file %s (%d)", FileName, GetLastError());
		return false;
	}
	fseek(file, 0L, SEEK_END);
	long size = ftell(file);
	fclose(file);

	return size;
}
bool FileClass::FileExists(const char* FileName)
{
	FILE* file = fopen(FileName, "rb");
	if (!file)
	{
		Console.Print("Red", "FileExists: cant find %s on this adress (%d)", GetLastError());
		LOG.LOG("(ERROR) [FileExists] cant find %s on this adress (%d)", FileName, GetLastError());
		return false;
	}
	else
	{
		fclose(file);
		return true;
	}
}
double FileClass::FtpGetFileSize(const char* FileName)
{
	CURLcode result;
	double filesize = 0.0;

	curl_global_init(CURL_GLOBAL_DEFAULT);

	CURL* curl = curl_easy_init();
	if (curl) {

		char Host[50];
		strcpy(Host, FTP_URL);
		strcat(Host, FileName);

		curl_easy_setopt(curl, CURLOPT_URL, Host);
		curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlGetSize);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L);

		result = curl_easy_perform(curl);

		if (result == CURLE_OK) {
			result = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
			if ((result == CURLE_OK) && (filesize > 0.0))
				Console.Print(NULL, "File size %s: %0.0f Kbytes", FileName, filesize / 1024);
		}
		else
		{
			Console.Print("Red", "Failed to get file size (%d)", result);
			LOG.LOG("(ERROR) Failed to get file size (%d)", result);
		}
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	return filesize;
}

bool FileClass::FileDownload(const char* FileName)
{
	Console.Print(NULL, "Load file %s from local server...\n", FileName);
	LOG.LOG("(INFO) [FileDownload] Load %s from local server.", FileName);

	if (FtpGetStatus() != CURLE_OK)
		return false;

	FtpGetFileSize(FileName);

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
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, CurlProgress);

			Console.Print(NULL, "\nDownload: %s", FileName);
			LOG.LOG("(INFO) [FileDownload] Download: %s", FileName);

			int result = curl_easy_perform(curl);
			if (result != CURLE_OK)
			{
				Console.Print("Red", "Error! Host is offline (%d).\nPlease try again later.", result);
				LOG.LOG("(ERROR) [FileDownload] Download is failed (%d)", result);
				fclose(ofile);
				remove(FileName);
				curl_easy_cleanup(curl);
				return false;
			}
			else
			{
				Console.Print(NULL, "Progress: 100.0%%");

				double total;
				result = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
				if (result == CURLE_OK)
					LOG.LOG("(INFO) [FileDownload] Download time (sec): %.1f", total);
				result = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &total);
				if (result == CURLE_OK)
					LOG.LOG("(INFO) [FileDownload] Download speed (Kbyte/s): %.1f", total / 1024);

				LOG.LOG("(INFO) [FileDownload] Download: Ok!");
			}
			fclose(ofile);
		}

		// always cleanup
		curl_easy_cleanup(curl);
	}

	Console.Print("Green", "Load file %s from local server: Ok!", FileName);

	return true;
}
bool FileClass::FileOpen(const char* FileName)
{
	STARTUPINFOA cif;
	ZeroMemory(&cif, sizeof(STARTUPINFOA));
	PROCESS_INFORMATION pi;

	Console.Print(NULL, "\nStarting extraction of %s...", FileName);
	LOG.LOG("(INFO) [FileOpen] Starting extraction of %s...", FileName);
	if (!CreateProcessA((LPCSTR)FileName, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &cif, &pi))
	{
		Console.Print("Red", "Cant open a file (%d)", GetLastError());
		LOG.LOG("(ERROR) [FileOpen] Cant open a %s:", FileName);
		return false;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	Console.Print("Green", "Extraction is done.\n");
	LOG.LOG("(INFO) [FileOpen] Extraction of %s is done!", FileName);

	return true;
}
int  FileClass::ShellMoveFiles(const wchar_t* srcPath, const wchar_t* newPath)
{
	const wchar_t* Src = srcPath;
	const wchar_t* Dest = newPath;

	SHFILEOPSTRUCTW fileOperation;
	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));

	fileOperation.wFunc  = FO_MOVE;
	fileOperation.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SIMPLEPROGRESS;
	fileOperation.pFrom  = Src;
	fileOperation.pTo    = Dest;
	fileOperation.hwnd   = hWnd;

	int result = true;
	int ShellResult = SHFileOperationW(&fileOperation);
	if (ShellResult != 0)
	{
		Console.Print("Red", "SHFileOperation error: %u", ShellResult);
		LOG.LOG("(ERROR) [Windows Shell] Error in SHFileOperation (%u)", ShellResult);
		result = false;
	}

	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));

	return result;
}

void FileClass::FileQueueSet(wchar_t* DestDir)
{
	Beginning = true;
	Console.ShowConsole();
	ShowWindow(hWnd, SW_HIDE);
	LOG.LOG("(INFO) [Main] A process was started.");

	std::string langstr;
	if (InstallLang == 0)
		langstr = "Russian";
	else
		langstr = "English";
	LOG.LOG("(INFO) [Main] Used %s version of mod.", langstr.c_str());

	// ”станавливаем заданную директорию "по умолчанию"
	wchar_t Destination[2048];
	wcscpy(Destination, DestDir);
	wcscat(Destination, L"DataTemp\\");
	CreateDirectoryW(Destination, NULL);
	SetCurrentDirectoryW(Destination);
	LOG.LOG(L"(INFO) [Main] Destination path: %s", (LPCWSTR)Destination);

	char* FileName;
	char files[4][40] = {
		"1_temp.exe",
		"2_temp.exe",
		"3_temp.exe",
	};

	for (int i = 0; i < 4; i++)
	{
		switch (i)
		{
			case 3:
			{
				if (InstallLang == 0)
					FileName = (char*)"4_temp_rus.exe";
				else
					FileName = (char*)"4_temp_eng.exe";
			}
			break;
			default:
				FileName = files[i];
			break;
		}

		Console.Print("Yellow", L"[%d/4] Destination: %s\n", i+1, Destination);

		if (!FileDownload(FileName))
		{
			QueueError = true;
			Sleep(2000);
			break;
		}

		Sleep(2000);
		if (!FileOpen(FileName))
		{
			Console.Print("Red", "Error! File %s is not found in the destination directory! Installation is failed!", FileName);
			LOG.LOG("(ERROR) [Main] %s is not found in the destination directory!", FileName);
			QueueError = true;
			Sleep(2000);
			break;
		}

		Sleep(1000);
		DeleteFileA(FileName);
	}

	SetCurrentDirectoryW(DestDir);
	if (!QueueError)
	{
		LOG.LOG("(INFO) [Windows Shell] Moving files into DATA folder...");
		Console.Print("Yellow", "Moving files into DATA folder...\nPlease, don't terminate the process to avoid mistakes!");
		if (!ShellMoveFiles(L"DataTemp\\Data\\*\0", L"Data\0"))
		{
			QueueError = true;
			Sleep(2000);
		}
		else
		{
			Console.Print("Green", "Moving files into DATA folder: Ok!");
		}
	}

	RemoveDirectoryW(L"DataTemp\\Data");
	RemoveDirectoryW(L"DataTemp");

	Sleep(2000);
	ShowWindow(hWnd, SW_SHOW);
	Console.HideConsole();
	if (!QueueError)
	{
		LPCSTR titletext = ReadJSONLocTag("cInstallDone").c_str();
		_bstr_t b(WndName);
		const char* newTitle = b;
		MessageBoxA(hWnd, titletext, newTitle, MB_OK | MB_ICONQUESTION);
		LOG.LOG("(INFO) [Main] Process was done successfully.\n");
	}
	else
	{
		QueueError = false;
		LPCSTR titletext = ReadJSONLocTag("cInstallFailed").c_str();
		_bstr_t b(WndName);
		const char* newTitle = b;
		MessageBoxA(hWnd, titletext, newTitle, MB_OK | MB_ICONERROR);
		LOG.LOG("(INFO) [Main] Process was done with errors.\n");
	}
	Beginning = false;
}

void FileClass::FilesDelete()
{
	DeleteFileA("1_temp.exe");
	DeleteFileA("2_temp.exe");
	DeleteFileA("3_temp.exe");
	DeleteFileA("4_temp.exe");
}