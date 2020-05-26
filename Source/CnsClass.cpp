#include <Windows.h>
#include <cstdio>
#include "CnsClass.h"

void CnsClass::HideConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}
void CnsClass::ShowConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
}
void CnsClass::Print(const char* Colour, const char* format, ...)
{
	int ColourIndex = 15;
	if (Colour == "Green")
		ColourIndex = 2;
	else if (Colour == "Red")
		ColourIndex = 4;
	else if (Colour == "Yellow")
		ColourIndex = 14;

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdOut, (WORD)(ColourIndex));

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	printf("\n");
	va_end(args);

	// set to default
	SetConsoleTextAttribute(hStdOut, 15);
}
void CnsClass::Print(const char* Colour, const wchar_t* format, ...)
{
	int ColourIndex = 15;
	if (Colour == "Green")
		ColourIndex = 2;
	else if (Colour == "Red")
		ColourIndex = 4;
	else if (Colour == "Yellow")
		ColourIndex = 14;

	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hStdOut, (WORD)(ColourIndex));

	va_list args;
	va_start(args, format);
	vwprintf(format, args);
	wprintf(L"\n");
	va_end(args);

	// set to default
	SetConsoleTextAttribute(hStdOut, 15);
}