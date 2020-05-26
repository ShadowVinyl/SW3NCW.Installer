
class LogClass
{
public:
	// we use 2 same functions with ANSI and UNICODE parameters
	static void InitLog();
	static void ReleaseLog();
	static void LOG(const char* format, ...);
	static void LOG(const wchar_t* format, ...);
};