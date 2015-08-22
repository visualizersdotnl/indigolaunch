
// indigolaunch -- app. flow

#include "platform.h"
#include "flow.h"
#include "timer.h"

/* static */ void Flow::InputCallback(Input input, void *pContext)
{
	ASSERT(NULL != pContext);
	Flow *pInst = reinterpret_cast<Flow *>(pContext);

	if (pInst->m_curMode == kLaunch)
	{
		switch (input)
		{
		case kInputStart:
		case kInputBack:
		case kInputLeft:
		case kInputRight:
			break;

		case kInputKillSwitch:
			{
				HANDLE hProcess = pInst->m_processInfo.hProcess;
				if (NULL != hProcess)
				{
					// hack: give any process a second to comply (do this properly in Flow::Update())
					DWORD ExitCode;
					Timer timer;
					while (STILL_ACTIVE == GetExitCodeProcess(hProcess, &ExitCode)) 
					{
						if (timer.Get() > 1.f)
						{
							ExitCode = -1;
							break;
						}
					}
					
					TerminateProcess(hProcess, ExitCode);
				}
			}
		
			break;
		}
	}
}

Flow::Flow(const Collection &collection, Renderer &renderer) :
	m_collection(collection)
,	m_curMode(kSplash)
,	m_splash(renderer)
,	m_select(renderer)
{
}

Flow::~Flow() {}

bool Flow::Create()
{
	return m_splash.Create() && m_select.Create();
}

bool Flow::Update(bool &renderFrame, float time)
{
	if (m_curMode != kLaunch)
	{
		if (renderFrame)
		{
			switch (m_curMode)
			{
			case kSplash:
				if (!m_splash.Update(time))
				{
					// enter select view
					m_curMode = kSelect;
					m_select.OnActivate(m_collection);
				}

				break;

			case kSelect:
				if (!m_select.Update(time))
				{
					// launch selected title
					if (false == LaunchTitle(m_select.GetSelectedTitle()))
						return false;

					m_curMode = kLaunch;
				}

				break;
			
			default:
				ASSERT(0);
				break;
			}
		}
	}
	else // m_curMode == kLaunch
	{
		ASSERT(NULL != m_processInfo.hProcess);
		ASSERT(NULL != m_processInfo.hThread);

		const DWORD dwExitCode = WaitForSingleObject(m_processInfo.hProcess, 0);
		switch (dwExitCode)
		{
		case WAIT_ABANDONED:
		case WAIT_OBJECT_0:
		case WAIT_FAILED:
			// close handles
			CloseHandle(m_processInfo.hThread);
			CloseHandle(m_processInfo.hProcess);

			// return app. window to original size & position
			ShowWindow(g_hWnd, SW_SHOWNORMAL);
			
			// force focus (just to be sure)
			SetForegroundWindow(g_hWnd);
			
			// return to splash screen
			// hack: little grace period to account for possible display mode switch
			m_curMode = kSplash;
			m_splash.OnActivate(time + 2.f); 
			
			break;
		
		default:
			Sleep(10); // sleep for a while
			break;
		}
		
		// indicate that no frame was rendered
		renderFrame = false;
	} 
	
	return true;
}

InputCallback Flow::GetInputCallback(void **ppContext)
{
	switch (m_curMode)
	{
	case kSplash:
		*ppContext = &m_splash;
		return Splash::InputCallback;

	case kSelect:
		*ppContext = &m_select;
		return Select::InputCallback;

	case kLaunch:
		*ppContext = this;
		return InputCallback;

	default:
		ASSERT(0);
		return NULL;
	}
}

bool Flow::LaunchTitle(const Title &title)
{
	// get app. directory & set title directory
	TCHAR curPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, curPath);
	VERIFY(TRUE == SetCurrentDirectory(title.GetPath().c_str()));

	// launch
	BOOL bResult;
	if (Title::kNative == title.GetExecutableType())
	{
		bResult = LaunchExecutable(
			title.GetLaunchInfo(Title::kExecutable).c_str(),
			title.GetLaunchInfo(Title::kCommandLine).c_str(),
			title.GetLaunchInfo(Title::kWorkDirectory));
	}
	else
	{
		// hack: must grab default browser path from registry!
		bResult = LaunchExecutable(
			L"C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe",
			title.GetLaunchInfo(Title::kExecutable).c_str(),
			(Title::kBrowserURL == title.GetExecutableType())
				? std::wstring(L"") 
				: title.GetLaunchInfo(Title::kWorkDirectory));
	}

	// set app. directory
	SetCurrentDirectory(curPath);

	if (TRUE == bResult)
	{
		// minimize app. window
		ShowWindow(g_hWnd, SW_MINIMIZE);

		return true;
	}
	else
	{
		SetLastError(L"Can't launch title: " + title.GetName() + L"\r\nReview XML metadata!" );
		return false;
	}
}

BOOL Flow::LaunchExecutable(const std::wstring &exePath, std::wstring cmdLine, const std::wstring &workPath)
{
	ASSERT(!exePath.empty());

	memset(&m_startupInfo, 0, sizeof(STARTUPINFO));
	m_startupInfo.cb = sizeof(STARTUPINFO);
	memset(&m_processInfo, 0, sizeof(PROCESS_INFORMATION));

	// hack: Firefox only!
	cmdLine = L"-new-window " + cmdLine;

	wchar_t *pCmdLineCopy = NULL;
	if (!cmdLine.empty())
	{
		pCmdLineCopy = new WCHAR[cmdLine.size()+1];
		wcscpy_s(pCmdLineCopy, cmdLine.size()+1 , cmdLine.c_str());
	}

	const BOOL bResult = CreateProcess(
		exePath.c_str(),
		pCmdLineCopy,
		NULL,
		NULL,
		FALSE,
		DETACHED_PROCESS | NORMAL_PRIORITY_CLASS | CREATE_DEFAULT_ERROR_MODE,
		NULL,
		workPath.empty() ? NULL : workPath.c_str(),
		&m_startupInfo,
		&m_processInfo);
		
	delete[] pCmdLineCopy;

	return bResult;
}
