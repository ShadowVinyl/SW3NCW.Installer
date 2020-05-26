
class CnsClass
{
public:
	static void HideConsole();
	static void ShowConsole();
	static void Print(const char* Colour, const char* format, ...);
	static void Print(const char* Colour, const wchar_t* format, ...);
};

