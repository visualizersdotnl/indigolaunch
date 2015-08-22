
#pragma once

#include "title.h"

// a Collection builds an amount of Title instances from a (relative) root path
class Collection
{
public:
	Collection(const std::wstring &rootPath);
	~Collection();

	bool Create(Renderer &renderer);

	size_t GetNumTitles() const { return m_titles.size(); }

	const Title &GetTitle(unsigned int iTitle) const 
	{ 
		ASSERT(iTitle < GetNumTitles()); 
		return *m_titles[iTitle]; 
	}

private:
	const std::wstring m_rootPath;
	bool m_isBuilt;

	std::vector<Title *> m_titles;
};
