
#include "platform.h"
#include "title.h"
#include "xmlutil.h"
#include "fileutil.h"

Title::Title(const std::wstring &path) :
	m_path(path)
,	m_isCreated(false)
,	m_pBackground(NULL)
,	m_pDevLogo(NULL)
{
}

Title::~Title()
{
	SAFE_RELEASE(m_pBackground);
	SAFE_RELEASE(m_pDevLogo);

	for (auto iPlatform = m_platforms.begin(); iPlatform != m_platforms.end(); ++iPlatform)
		SAFE_RELEASE(*iPlatform);

	for (auto iAward = m_awards.begin(); iAward != m_awards.end(); ++iAward)
		SAFE_RELEASE(*iAward);
}

bool Title::Create(Renderer &renderer)
{
	ASSERT(!m_isCreated);

	// load metadata XML
	if (!LoadTitleXML(renderer) || !LoadLaunchXML())
	{
		return false;
	}

	// load background art
	m_pBackground = renderer.CreateTextureFromFile(m_path + L"\\ib_background.png", true);
	if (NULL == m_pBackground)
		return false;

	// load developer logo (optional)
	const std::wstring devLogoPath = m_path + L"\\ib_developer.png";
	if (FileExists(devLogoPath))
	{
		m_pDevLogo = renderer.CreateTextureFromFile(devLogoPath, true);
		if (NULL == m_pDevLogo)
			return false;
	}

	m_isCreated = true;
	return true;
}

bool Title::LoadTitleXML(Renderer &renderer)
{
	// XML node names
	const wchar_t *kNodeNames[kNumTitleNodes] = {
		L"Name",
		L"Developer",
//		L"Year",
		L"Genre",
//		L"Publisher",
		L"NumPlayers",
		L"Keyboard",
		L"Controller",
		L"Summary"};

	// load XML & parse (flat part)
	const std::wstring path = m_path + L"\\ib_title.xml";
	if (!LoadAndParseFlatXML(m_titleXML, path, kNodeNames, kNumTitleNodes, m_titleInfo))
		return false;

	// parse icons (optional)
	const pugi::xml_node platforms = m_titleXML.child(L"Platforms");
	if (!platforms.empty())
	{
		pugi::xml_node node = platforms.first_child();
		while (!node.empty())
		{
			if (std::wstring(L"Icon") == node.name())
			{
				// node found: file exists?
				const std::wstring path = L"assets\\icon\\platform\\" + std::wstring(node.child_value());
				if (FileExists(path))
				{
					// load icon
					Renderer::Texture2D *pIcon = renderer.CreateTextureFromFile(path, true);
					if (NULL == pIcon)
						return false;

					m_platforms.push_back(pIcon);
				}
			}
		
			node = node.next_sibling();
		}
	}

	const pugi::xml_node awards = m_titleXML.child(L"Awards");
	if (!awards.empty())
	{
		pugi::xml_node node = awards.first_child();
		while (!node.empty())
		{
			if (std::wstring(L"Icon") == node.name())
			{
				// node found: file exists?
				const std::wstring path = L"assets\\icon\\award\\" + std::wstring(node.child_value());
				if (FileExists(path))
				{
					// load icon
					Renderer::Texture2D *pIcon = renderer.CreateTextureFromFile(path, true);
					if (NULL == pIcon)
						return false;

					m_awards.push_back(pIcon);
				}
			}
		
			node = node.next_sibling();
		}
	}

	return true;
}

bool Title::LoadLaunchXML()
{
	// XML node names
	const wchar_t *kNodeNames[kNumLaunchNodes] = {
		L"Executable",
		L"WorkDir",
		L"CommandLineParams" };

	// load (flat) XML
	const std::wstring path = m_path + L"\\ib_launch.xml";
	if (!LoadAndParseFlatXML(m_launchXML, path, kNodeNames, kNumLaunchNodes, m_launchInfo))
		return false;

	// executable type: detect URL (http://...)
	const std::wstring &exePath = m_launchInfo[kExecutable];
	const size_t iPrefix = exePath.find_first_of(L"http://");
	if (iPrefix == 0)
	{
		m_exeType = kBrowserURL;
	}
	else
	{
		// it's no URL: look for extension
		const size_t iLastDot = exePath.find_last_of(L".");
		if (iLastDot != std::wstring::npos)
		{
			// strip it
			std::wstring Ext = exePath.substr(iLastDot+1);
			std::transform(Ext.begin(), Ext.end(), Ext.begin(), ::tolower);
			
			// HTML or Flash?
			if (Ext == L"html" || Ext == L"htm" || Ext == L"swf")
			{
				// yes: browser content
				m_exeType = kBrowserLocal;
			}
			else
			{
				// no: assume it's a native executable
				m_exeType = kNative;
			}
		}
		else
		{
			SetLastError(L"Title in '" + m_path + L"' specifies invalid executable path (non-URL requires extension!): '" + exePath + L"'");
			return false;
		}
	}

	// verify executable & work directory (local only)
	if (kBrowserURL != m_exeType)
	{
		if (!FileExists(m_path + L"\\" + exePath))
		{
			SetLastError(L"Title in '" + m_path + L"' specifies invalid executable path: '" + exePath + L"'");
			return false;
		}

		const std::wstring &workDir = m_launchInfo[kWorkDirectory];
		if (!m_launchInfo[kWorkDirectory].empty() && !FileExists(m_path + L"\\" + workDir))
		{
			SetLastError(L"Title in '" + m_path + L"' specifies invalid work directory: '" + workDir + L"'");
			return false;
		}
	}
	
	return true;
}
