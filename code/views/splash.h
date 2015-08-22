
// indigolaunch -- UI: splash screen

#pragma once

#include "view.h"

class Splash : public View
{
public:
	static void InputCallback(Input input, void *pContext);

	Splash(Renderer &renderer);
	~Splash();
	
	bool Create();
	bool Update(float time);

	void OnActivate(float time) { m_tActivate = time; }

private:
	Renderer::Texture2D *m_pBackground, *m_pLeafL, *m_pLeafR;
	Renderer::Texture2D *m_pLogo;

	float m_tActivate;
};
