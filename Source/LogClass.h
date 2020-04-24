
class LogClass
{
public:
	// we use 2 same functions with ANSI and UNICODE parameters
	static void InitLog();
	static void ReleaseLog();
	static void LogMessage(const char* message, bool ErrorFlag, const char* param1, double param2);
	static void LogMessage(const wchar_t* message, bool ErrorFlag, const wchar_t* param1, double param2);
};