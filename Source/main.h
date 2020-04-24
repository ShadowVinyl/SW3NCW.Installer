#pragma once

#include "resource.h"
#include "pugixml_read.h"
#include "FileClass.h"
#include "CnsClass.h"
#include "LogClass.h"

class Window
{
public:
	static void ChangeLanguage();
	static void EnableButtons(bool Flag);
	static void WindowMenu(HWND hWnd);
	static bool BrowseForFolder();
};

ATOM RegisterMainClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
BOOL WINAPI CnsHandler(DWORD dwCtrlType);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
int CALLBACK BrowsePathProc(HWND hWnd, UINT message, LPARAM lParam, LPARAM pData);