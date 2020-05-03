
class FileClass
{
public:
	static int  FtpGetStatus();
	static int  FileSize(const char* FileName);
	static bool FileExists(const char* FileName);
	static double FtpGetFileSize(char* FileName);
	static void FileQueueSet(wchar_t* DestDir);
	static void FilesDelete();

private:
	static bool FileDownload(char* FileName);
	static bool FileOpen(char* FileName);
	static int  ShellMoveFiles(const wchar_t* srcPath, const wchar_t* newPath);
};