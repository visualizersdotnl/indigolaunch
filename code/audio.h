
#pragma once

// 'iDevice' - valid device index or -1 for system default
bool Audio_Create(unsigned int iDevice, HWND hWnd);
void Audio_Destroy();

// hack: fixed set of samples
enum Sample
{
	kSampleLR,
	kSampleBack,
	kSampleSelect,
	kSampleLaunch,
	kNumSamples
};

void Audio_PlaySample(Sample sample);
