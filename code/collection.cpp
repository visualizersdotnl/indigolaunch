
#include "platform.h"
#include "collection.h"
#include "fileutil.h"

Collection::Collection(const std::wstring &rootPath) :
	m_rootPath(rootPath),
	m_isBuilt(false)
{
}

Collection::~Collection()
{
	// delete Title instances
	for (std::vector<Title *>::iterator iTitle = m_titles.begin(); iTitle != m_titles.end(); ++iTitle)
		delete *iTitle;
}

bool Collection::Create(Renderer &renderer)
{
	ASSERT(!m_isBuilt);

	const std::wstring searchPath = m_rootPath + L"\\*";

	// scan collection root path for titles
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(searchPath.c_str(), &findData);
	while (hFind != INVALID_HANDLE_VALUE)
	{
		if (FALSE == FindNextFile(hFind, &findData))
		{
//			GetLastError() != ERROR_NO_MORE_FILES;
			FindClose(hFind);
			hFind = INVALID_HANDLE_VALUE;
			break;
		}
		
		// (sub)directory?
		if (0 != (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			// an actual one?
			if (0 != wcscmp(findData.cFileName, L".."))
			{
				// probe required XML metadata files
				const std::wstring titlePath = m_rootPath + L"\\" + findData.cFileName;
				if (FileExists(titlePath + L"\\ib_title.xml") && FileExists(titlePath + L"\\ib_launch.xml"))
				{
					// title found!
					Title *pTitle = new Title(titlePath);
					m_titles.push_back(pTitle);
				}
			}
		}
	}

	if (!m_titles.empty())
	{
		// create Title instances
		for (std::vector<Title *>::iterator iTitle = m_titles.begin(); iTitle != m_titles.end(); ++iTitle)
		{
			if (false == (*iTitle)->Create(renderer))
			{
				return false;
			}
		}

		// sort them by genre (quick solution, perhaps move predicate to Title?)
		struct compareTitles {
			bool operator ()(Title *LHS, Title *RHS) {
				return LHS->GetTitleInfo(Title::kGenre) < RHS->GetTitleInfo(Title::kGenre); }
		};
		std::sort(m_titles.begin(), m_titles.end(), compareTitles());

		// done!
		m_isBuilt = true;
		return true;
	}
	else
	{
		SetLastError(L"Can not find any valid titles in directory: '" + m_rootPath + L"'");
		return false;
	}
}
