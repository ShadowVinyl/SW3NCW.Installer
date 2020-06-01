
class LogClass
{
public:
	void InitLog();
	void ReleaseLog();
	void LOG(const char* format, ...);
	void LOG(const wchar_t* format, ...);
};
