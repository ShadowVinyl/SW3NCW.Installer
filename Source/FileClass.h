
class FileClass
{
public:
	int  FtpGetStatus();
	long FileSize(const char* FileName);
	bool FileExists(const char* FileName);
	double FtpGetFileSize(const char* FileName);
	void FileQueueSet(wchar_t* DestDir);
	void FilesDelete();

private:
	bool FileDownload(const char* FileName);
	bool FileOpen(const char* FileName);
	int  ShellMoveFiles(const wchar_t* SrcDir, const wchar_t* DestDir);
};


std::wstring GetExePath();