
#include "platform.h"
#include "../3rdparty/bass24/c/bass.h"
#include "audio.h"

const std::wstring kSamplePaths[kNumSamples] =
{
	L"assets\\sound\\footfall_click.wav",
	L"assets\\sound\\echo_alert.wav",
	L"assets\\sound\\echo_affirm.wav",
	L"assets\\sound\\echo_affirm.wav"
};

static HSAMPLE s_hSamples[kNumSamples] = { NULL };

bool Audio_Create(unsigned int iDevice, HWND hWnd)
{
	ASSERT(iDevice == -1); // || iDevice < Audio_GetDeviceCount());
	ASSERT(hWnd != NULL);

	// Bass device IDs:
	//  0 = No sound (causes functionality to be limited, so -1 is the better pick).
	// -1 = Default.
	// >0 = As enumerated.
	if (!BASS_Init((iDevice == -1) ? -1 : iDevice + 1, 44100, BASS_DEVICE_LATENCY, hWnd, NULL))
	{ 
		switch (BASS_ErrorGetCode())
		{
		case BASS_ERROR_DEVICE:
		case BASS_ERROR_ALREADY:
		case BASS_ERROR_NO3D:
		case BASS_ERROR_UNKNOWN:
		case BASS_ERROR_MEM:
			ASSERT(0);

		case BASS_ERROR_DRIVER:
		case BASS_ERROR_FORMAT:
			SetLastError(L"Can not initialize BASS audio library @ 44.1 kHz.");
			return false;
		}
	}

	for (int iSample = 0; iSample < kNumSamples; ++iSample)
	{
		const std::wstring &path = kSamplePaths[iSample];
		s_hSamples[iSample] = BASS_SampleLoad(FALSE, path.c_str(), 0, 0, 1, BASS_UNICODE);
		if (NULL == s_hSamples[iSample])
		{
			switch (BASS_ErrorGetCode())
			{
			case BASS_ERROR_INIT:
			case BASS_ERROR_NOTAVAIL:
			case BASS_ERROR_ILLPARAM:
			case BASS_ERROR_NO3D:
			case BASS_ERROR_FILEFORM:
			case BASS_ERROR_CODEC:
			case BASS_ERROR_FORMAT:
			case BASS_ERROR_SPEAKER:
			case BASS_ERROR_MEM:
				ASSERT(0);

			case BASS_ERROR_FILEOPEN:
			case BASS_ERROR_UNKNOWN:			
				SetLastError(L"Can not load sample: " + path);
				return false;
			}
		}
	}

	return true;
}

void Audio_Destroy()
{
	BASS_Free();
}

void Audio_PlaySample(Sample sample)
{
	ASSERT(NULL != s_hSamples[sample]);
	BASS_ChannelPlay(BASS_SampleGetChannel(s_hSamples[sample], false), true);
}
