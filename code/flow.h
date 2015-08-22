
// indigolaunch -- app. flow

#pragma once

// #include "inputcallback.h"
// #include "collection.h"
#include "views/splash.h"
#include "views/select.h"

// hack: hardcoded top-level UI manager
class Flow
{
public:
	static void InputCallback(Input input, void *pContext);

	Flow(const Collection &collection, Renderer &renderer);
	~Flow();

	bool Create();
	bool Update(bool &renderFrame, float time);

	::InputCallback GetInputCallback(void **ppContext);
	bool InLaunchMode() const { return m_curMode == kLaunch; }

private:
	bool LaunchTitle(const Title &title);
	BOOL LaunchExecutable(const std::wstring &exePath, std::wstring cmdLine, const std::wstring &workPath);

	// the Collection is currently the root set (used by Coverflow)
	const Collection &m_collection;

	enum CurMode
	{
		kSplash,
		kSelect,
		kLaunch
	} m_curMode;

	Splash m_splash;
	Select m_select;

	STARTUPINFO m_startupInfo;
	PROCESS_INFORMATION m_processInfo;
};
