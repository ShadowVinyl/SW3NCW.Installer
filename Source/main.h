#pragma once

#include "resource.h"

class WindowClass
{
public:
	void ChangeLanguage();
	void EnableButtons(bool Flag);
	void WindowMenu(HWND hWnd);
	bool BrowseForFolder();
};

ATOM RegisterMainClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
BOOL WINAPI CnsHandler(DWORD dwCtrlType);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
INT CALLBACK BrowsePathProc(HWND hWnd, UINT message, LPARAM lParam, LPARAM pData);