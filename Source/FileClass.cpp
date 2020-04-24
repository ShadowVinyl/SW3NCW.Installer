#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)
#endif
#define CURL_STATICLIB

#include <Windows.h>
#include <stdio.h>
#include <fstream>
#include <curl/curl.h>

#include "pugixml_read.h"
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

int  FileClass::FtpGetStatus()
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
			CnsClass::SetTextColor(2);
			printf("Server is online!\n\n");
			LogClass::LogMessage("(INFO) [FileDownload] Server is online.", 0, 0, 0);
			CnsClass::SetTextColor(15);
			double connect;
			result = curl_easy_getinfo(curl, CURLINFO_CONNECT_TIME, &connect);
			if (CURLE_OK == result)
				LogClass::LogMessage("(INFO) [FileDownload] Ping (ms):", 0, 0, connect / 1000);
		}
		else
		{
			CnsClass::SetTextColor(4);
			printf("Server is offline (CURL code: %d)\n", result);
			CnsClass::SetTextColor(15);
			LogClass::LogMessage("(ERROR) [FileDownload] Server is offline:", 0, 0, result);
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return result;
}
int  FileClass::FileSize(const char* FileName)
{
	std::fstream file;
	file.open(FileName);
	if (!file || file.bad())
	{
		printf("FileSize: cant open a file (C++ error code: %d)", GetLastError());
		LogClass::LogMessage("(ERROR) [FileSize] Cant open a file", 1, FileName, 0);
		return 0;
	}

	int size = 0;
	file.seekg(0, std::ios::end);
	size = file.tellg();
	file.close();
	return size;
}
bool FileClass::FileExists(const char* FileName)
{
	std::ifstream file;
	file.open(FileName);
	if (!file || file.bad()) return 0;
	file.close();
	return 1;
}
double FileClass::FtpGetFileSize(char* FileName)
{
	CURL* curl;
	CURLcode res;
	//long filetime = -1;
	double filesize = 0.0;
	double speed = 0.0;

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
			res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);
			if ((CURLE_OK == res) && (filesize > 0.0))
				printf("File size %s: %0.0f Kbytes\n", FileName, filesize / 1024);
		}
		else {
			/* we failed */
			fprintf(stderr, "Failed to get file size (CURL code: %d)\n", res);
			LogClass::LogMessage("(ERROR) Failed to get file size:", 0, 0, res);
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

	return filesize;
}
//
void FileClass::FileQueueSet(wchar_t* DestDir)
{
	Beginning = TRUE;
	CnsClass::ShowConsole();
	ShowWindow(hWnd, SW_HIDE);
	LogClass::LogMessage("(INFO) [Main] A process was started.", 0, 0, 0);
	if (InstallLang == 1)
		LogClass::LogMessage("(INFO) [Main] Use Russian version of mod.", 0, 0, 0);
	else
		LogClass::LogMessage("(INFO) [Main] Use English version of mod.", 0, 0, 0);

	// Устанавливаем заданную директорию "по умолчанию"
	wchar_t Destination[2048];
	wcscpy(Destination, DestDir);
	wcscat(Destination, L"DataTemp\\");
	CreateDirectoryW(Destination, NULL);
	SetCurrentDirectoryW(Destination);
	LogClass::LogMessage(L"(INFO) [Main] Destination path:", 0, (LPCWSTR)Destination, 0);

	char* FileName;
	char files_exe[][40] = {
		"1_temp.exe",
		"2_temp.exe",
		"3_temp.exe",
	};

	for (int i = 0; i < 4; i++)
	{
		if (i != 3)
			FileName = files_exe[i];
		else
		{
			if (InstallLang == 1)
				FileName = (char*)"4_temp_rus.exe";
			else
				FileName = (char*)"4_temp_eng.exe";
		}

		system("cls");
		CnsClass::SetTextColor(14);
		wprintf(L"Destination: %s\n", Destination);
		printf("File name: %s\n\n", FileName);
		CnsClass::SetTextColor(15);

		if (!FileDownload(FileName))
		{
			QueueError = TRUE;
			Sleep(2000);
			break;
		}

		Sleep(2000);
		if (!FileOpen(FileName))
		{
			CnsClass::SetTextColor(4);
			printf("Error! File %s is not found in the destination directory! Installation is failed!\n", FileName);
			LogClass::LogMessage("(ERROR) [Main] File is not found in the destination directory:", 1, FileName, 0);
			CnsClass::SetTextColor(15);
			QueueError = TRUE;
			Sleep(2000);
			break;
		}

		Sleep(1000);
		DeleteFileA(FileName);
	}

	system("cls");
	SetCurrentDirectoryW(DestDir);
	if (!QueueError)
	{
		LogClass::LogMessage("(INFO) [Windows Shell] Moving files into DATA folder...", 0, 0, 0);
		wprintf(L"Moving files into DATA folder...\n");
		CnsClass::SetTextColor(14);
		wprintf(L"Please, don't terminate the process to avoid mistakes!\n");
		CnsClass::SetTextColor(15);
		if (!ShellMoveFiles(L"DataTemp\\Data\\*\0", L"Data\0"))
		{
			QueueError = TRUE;
			Sleep(2000);
		}
		else
		{
			CnsClass::SetTextColor(2);
			wprintf(L"Moving files into DATA folder: Ok!\n");
			CnsClass::SetTextColor(15);
		}
	}

	RemoveDirectoryW(L"DataTemp\\Data");
	RemoveDirectoryW(L"DataTemp");

	Sleep(2000);
	ShowWindow(hWnd, SW_SHOW);
	CnsClass::HideConsole();
	if (!QueueError)
	{
		wchar_t text[25];
		if (CurrentLang == 0)
			wcscpy(text, L"Установка завершена.");
		else
			wcscpy(text, L"Installation is done.");
		MessageBoxW(hWnd, text, WndName, MB_OK | MB_ICONQUESTION);
		LogClass::LogMessage("(INFO) [Main] Process was done successfully.\n", 0, 0, 0);
	}
	else
	{
		QueueError = FALSE;
		wchar_t text[80];
		if (CurrentLang == 0)
			wcscpy(text, L"Установка завершена с ошибками. Проверьте logfile.txt для поиска проблемы.");
		else
			wcscpy(text, L"Installation is failed. See logfile.txt for searching problems.");
		MessageBoxW(hWnd, text, WndName, MB_OK | MB_ICONERROR);
		LogClass::LogMessage("(INFO) [Main] Process was done with errors.\n", 0, 0, 0);
	}
	Beginning = FALSE;
}
bool FileClass::FileDownload(char* FileName)
{
	printf("Load file %s from local server...\n", FileName);
	LogClass::LogMessage("(INFO) [FileDownload] Load file from local server:", 0, FileName, 0);

	printf("Connect to local server...\n");
	LogClass::LogMessage("(INFO) [FileDownload] Connect to local server...", 0, 0, 0);
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

			LogClass::LogMessage("(INFO) [FileDownload] Download:", 0, FileName, 0);

			int result = curl_easy_perform(curl);
			if (result != CURLE_OK)
			{
				CnsClass::SetTextColor(4);
				printf("Error! Host is offline (CURL code: %d).\nPlease try again later.\n", result);
				CnsClass::SetTextColor(15);
				LogClass::LogMessage("(ERROR) [FileDownload] Host is offline. Download is failed:", 0, 0, result);
				fclose(ofile);
				remove(FileName);
				curl_easy_cleanup(curl);
				return 0;
			}
			else
			{
				printf("Progress: 100%%\n");
				CnsClass::SetTextColor(2);
				printf("\nDownload: Ok!\n");

				double total;
				result = curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total);
				if (result == CURLE_OK)
					LogClass::LogMessage("(INFO) [FileDownload] Download time (sec):", 0, 0, total);
				result = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &total);
				if (result == CURLE_OK)
					LogClass::LogMessage("(INFO) [FileDownload] Download speed (Kbyte/s):", 0, 0, total / 1024);

				LogClass::LogMessage("(INFO) [FileDownload] Download: Ok!", 0, 0, 0);
			}
			fclose(ofile);
		}

		// always cleanup
		curl_easy_cleanup(curl);
	}

	printf("Load file %s from local server: Ok!\n", FileName);
	CnsClass::SetTextColor(15);
	return 1;
}
bool FileClass::FileOpen(char* FileName)
{
	STARTUPINFOA cif;
	ZeroMemory(&cif, sizeof(STARTUPINFOA));
	PROCESS_INFORMATION pi;

	printf("\nStarting extraction...\n");
	LogClass::LogMessage("(INFO) [FileOpen] Starting extraction:", 0, FileName, 0);
	if (!CreateProcessA((LPCSTR)FileName, NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &cif, &pi))
	{
		CnsClass::SetTextColor(4);
		printf("Cant open a file (C++ error code: %d)\n", GetLastError());
		LogClass::LogMessage("(ERROR) [FileOpen] Cant open a file:", 1, FileName, 0);
		CnsClass::SetTextColor(15);
		return 0;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	CnsClass::SetTextColor(2);
	printf("Extraction is done.\n\n");
	LogClass::LogMessage("(INFO) [FileOpen] Extraction is done:", 0, FileName, 0);
	CnsClass::SetTextColor(15);
	return 1;
}
int  FileClass::ShellMoveFiles(const wchar_t* srcPath, const wchar_t* newPath)
{
	const wchar_t* Src = srcPath;
	const wchar_t* Dest = newPath;

	SHFILEOPSTRUCTW fileOperation;
	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));

	fileOperation.wFunc = FO_MOVE;
	fileOperation.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_SIMPLEPROGRESS;
	fileOperation.pFrom = Src;
	fileOperation.pTo = Dest;
	fileOperation.hwnd = hWnd;

	int result = 1;
	int ShellResult = SHFileOperationW(&fileOperation);
	if (ShellResult != 0)
	{
		LogClass::LogMessage("[Windows Shell] Error code:", 1, 0, ShellResult);
		CnsClass::SetTextColor(4);
		printf("SHFileOperation Failure: %u\n", ShellResult);
		CnsClass::SetTextColor(15);
		result = 0;
	}

	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));

	return result;
}