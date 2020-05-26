
class FileClass
{
public:

	static int  FtpGetStatus();
	static long FileSize(const char* FileName);
	static bool FileExists(const char* FileName);
	static double FtpGetFileSize(const char* FileName);
	static void FileQueueSet(wchar_t* DestDir);
	static void FilesDelete();

private:
	static bool FileDownload(const char* FileName);
	static bool FileOpen(const char* FileName);
	static int  ShellMoveFiles(const wchar_t* srcPath, const wchar_t* newPath);
};
