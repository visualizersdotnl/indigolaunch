
#include "platform.h"
// #include "fileutil.h"

bool FileExists(const std::wstring &path)
{
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(path.c_str(), &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		return true;
	}

	return false;
}

uint8_t *ReadFile(const std::wstring &path, bool checkForTrailingZero /* = false */)
{
	HANDLE hFile = CreateFile(
		path.c_str(), 
		GENERIC_READ,
		0, 
		NULL, 
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		const DWORD fileSize = GetFileSize(hFile, NULL); // max. 4GB
		uint8_t *pData = new uint8_t[fileSize];
		
		DWORD numBytesRead = 0;
		ReadFile(hFile, pData, fileSize, &numBytesRead, NULL);
		CloseHandle(hFile);			

		if (fileSize == numBytesRead && (!checkForTrailingZero || pData[fileSize-1] == 0))
			return pData;
		else
			delete[] pData;
	}

	return NULL;
}
