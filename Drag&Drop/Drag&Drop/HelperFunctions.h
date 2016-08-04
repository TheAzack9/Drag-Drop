#ifndef HELPERFUNCTIONS
#define HELPERFUNCTIONS

#include <vector>
#include <string>
#include <Windows.h>
#include <set>
#include <stdio.h>
#include <tchar.h>

class HelperFunctions
{
public:
	static std::vector<std::wstring> CustomTokenize(const std::wstring& str, const std::wstring& delimiters)
	{
		std::vector<std::wstring> tokens;

		size_t lastPos, pos = 0;
		do
		{
			lastPos = str.find_first_not_of(delimiters, pos);
			if (lastPos == std::wstring::npos) break;

			pos = str.find_first_of(delimiters, lastPos + 1);
			std::wstring token = str.substr(lastPos, pos - lastPos);

			size_t pos2 = token.find_first_not_of(L" \t\r\n");
			//If a token fix was needed, then are the pos one token off, just needed to keep it right
			bool offsett = false;
			if (pos2 != std::wstring::npos)
			{
				//Check wether it's even needed to try combine mathematic expressions
				if (token.find(L'(') != std::wstring::npos || token.find(L'[') != std::wstring::npos)
				{
					//Add the next token while the order of ( and ), and [ and ] are still uneven 
					while (std::count(token.begin(), token.end(), L'(') != std::count(token.begin(), token.end(), L')') &&
						std::count(token.begin(), token.end(), L'[') != std::count(token.begin(), token.end(), L']') &&
						pos != std::wstring::npos && lastPos != std::wstring::npos)
					{
						lastPos = str.find_first_not_of(delimiters, pos);
						if (lastPos == std::wstring::npos) break;
						pos = str.find_first_of(delimiters, lastPos + 1);
						token += str.at(lastPos - 1) + str.substr(lastPos, pos - lastPos);
						if (pos == std::wstring::npos) break;
						++pos;
						offsett = true;
					}
				}
				//End of addition
				size_t lastPos2 = token.find_last_not_of(L" \t\r\n");
				if (pos2 != 0 || lastPos2 != (token.size() - 1))
				{
					// Trim white-space
					token.assign(token, pos2, lastPos2 - pos2 + 1);
				}
				tokens.push_back(token);
			}

			if (pos == std::wstring::npos) break;
			if (!offsett)
				++pos;
		} while (true);

		return tokens;
	}

	static std::wstring wstringReplace(std::wstring wstr, std::wstring oldstr, std::wstring newstr)
	{
		size_t pos = 0;
		while ((pos = wstr.find(oldstr, pos)) != std::wstring::npos) {
			wstr.replace(pos, oldstr.length(), newstr);
			pos += newstr.length();
		}
		return wstr;
	}

	static std::vector<std::wstring> wstringSplit(const std::wstring& str, TCHAR delimeter)
	{
		std::vector<std::wstring> result;
		TCHAR const* pch = str.c_str();
		TCHAR const* start = pch;
		for (; *pch; ++pch)
		{
			if (delimeter == *pch)
			{
				if (start != pch)
				{
					std::wstring str(start, pch);
					result.push_back(str);
				}
				else
				{
					result.push_back(L"");
				}
				start = pch + 1;
			}
		}
		result.push_back(start);
		return result;
	}
	static int CopyDirectory(const std::wstring &refcstrSourceDirectory,
		const std::wstring &refcstrDestinationDirectory, bool replaceifexists)
	{
		std::wstring     strSource;               // Source file
		std::wstring     strDestination;          // Destination file
		std::wstring     strPattern;              // Pattern
		HANDLE          hFile;                   // Handle to file
		WIN32_FIND_DATA FileInformation;         // File information

		int error = 0;
		strPattern = refcstrSourceDirectory + L"\\*.*";

		// Create destination directory
		if (::CreateDirectory(refcstrDestinationDirectory.c_str(), 0) == FALSE)
			return ::GetLastError();

		hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (FileInformation.cFileName[0] != '.')
				{
					strSource.erase();
					strDestination.erase();

					strSource = refcstrSourceDirectory + L"\\" + FileInformation.cFileName;
					strDestination = refcstrDestinationDirectory + L"\\" + FileInformation.cFileName;

					if (FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						// Copy subdirectory
						error = CopyDirectory(strSource, strDestination, replaceifexists);
						if (error)
							return error;
					}
					else
					{
						// Copy file
						if (::CopyFile(strSource.c_str(), strDestination.c_str(), replaceifexists) == FALSE)
							return ::GetLastError();
					}
				}
			} while (::FindNextFile(hFile, &FileInformation) == TRUE);

			// Close handle
			::FindClose(hFile);

			DWORD dwError = ::GetLastError();
			if (dwError != ERROR_NO_MORE_FILES)
				return dwError;
		}

		return error;
	}
};


#endif