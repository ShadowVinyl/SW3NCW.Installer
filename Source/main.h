#pragma once

#include "resource.h"
#include "FileClass.h"
#include "CnsClass.h"
#include "LogClass.h"

class Window
{
public:
	void ChangeLanguage();
	void EnableButtons(bool Flag);
	static void WindowMenu(HWND hWnd);
	static bool BrowseForFolder();
};

ATOM RegisterMainClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
BOOL WINAPI CnsHandler(DWORD dwCtrlType);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int CALLBACK BrowsePathProc(HWND hWnd, UINT message, LPARAM lParam, LPARAM pData);