
#pragma once

#include "../3rdparty/pugixml-1.2/src/pugixml.hpp"
#include "renderer.h"

// a Title instance handles all relevant details for a single title
class Title
{
public:
	Title(const std::wstring &path);
	~Title();
	
	bool Create(Renderer &renderer);

	const std::wstring &GetPath() const { return m_path; }

	// title XML node IDs
	enum TitleNode
	{
		kName = 0,
		kDeveloper,
//		kYear,
		kGenre,
//		kPublisher,
		kNumPlayers,
		kKeyboard,
		kController,
		kSummary,
		kNumTitleNodes
	};

	const std::wstring &GetTitleInfo(TitleNode node) const { return m_titleInfo[node]; }
	const std::wstring &GetName() const { return GetTitleInfo(kName); }

	// hack: remove after XML redesign
	bool UsesKeyboard() const { return m_titleInfo[kKeyboard] == L"TRUE"; }
	bool UsesController() const { return m_titleInfo[kController] == L"TRUE"; }

	// launch XML node IDs
	enum LaunchNode
	{
		kExecutable = 0,
		kWorkDirectory,
		kCommandLine,
		kNumLaunchNodes
	};

	const std::wstring &GetLaunchInfo(LaunchNode node) const { return m_launchInfo[node]; }

	enum ExecutableType
	{
		kNative,
		kBrowserLocal,
		kBrowserURL
	};

	ExecutableType GetExecutableType() const { return m_exeType; }
	
	const Renderer::Texture2D *GetBackground()    const { return m_pBackground; }
	const Renderer::Texture2D *GetDeveloperLogo() const { return m_pDevLogo; }
	
	const Renderer::Texture2D *GetPlatformIcon(size_t index) const
	{
		if (index < m_platforms.size())
			return m_platforms[index];
		else
			return NULL;
	}

	const Renderer::Texture2D *GetAwardIcon(size_t index) const
	{
		if (index < m_awards.size())
			return m_awards[index];
		else
			return NULL;
	}
	
private:
	bool LoadTitleXML(Renderer &renderer);
	bool LoadLaunchXML();

	const std::wstring m_path;
	bool m_isCreated;

	pugi::xml_document m_titleXML;
	pugi::xml_document m_launchXML;
	
	std::wstring m_titleInfo[kNumTitleNodes];
	std::wstring m_launchInfo[kNumLaunchNodes];

	ExecutableType m_exeType;

	Renderer::Texture2D *m_pBackground;
	Renderer::Texture2D *m_pDevLogo;
	std::vector<Renderer::Texture2D *> m_platforms;
	std::vector<Renderer::Texture2D *> m_awards;
};
