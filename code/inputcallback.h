
#pragma once

enum Input
{
	kInputLeft,
	kInputRight,
	kInputStart,
	kInputBack,
	kInputKillSwitch
};

typedef void (*InputCallback)(Input input, void *pContext);
