#include <Windows.h>
#include "CnsClass.h"

void CnsClass::HideConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}
void CnsClass::ShowConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}
void CnsClass::SetTextColor(int ColourIndex)
{
	// 2 - green, 4 - red, 14 - yellow, 15 - white
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdOut, (WORD)(ColourIndex));
}