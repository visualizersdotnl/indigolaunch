
// indigolaunch -- UI: splash screen

#include "../platform.h"
#include "splash.h"

/* static */ void Splash::InputCallback(Input input, void *pContext)
{
}

Splash::Splash(Renderer &renderer) :
	View(renderer)
,	m_pBackground(NULL)
,	m_pLeafL(NULL)
,	m_pLeafR(NULL)
,	m_pLogo(NULL)
,	m_tActivate(0.f)
{
}

Splash::~Splash()
{
	SAFE_RELEASE(m_pBackground);
	SAFE_RELEASE(m_pLeafL);
	SAFE_RELEASE(m_pLeafR);
	SAFE_RELEASE(m_pLogo);
}

bool Splash::Create()
{
	m_pBackground = m_renderer.CreateTextureFromFile(L"assets\\image\\splash\\background.png", true);
	if (NULL == m_pBackground)
		return false;

	m_pLeafL = m_renderer.CreateTextureFromFile(L"assets\\image\\splash\\left-leaf.png", true);
	if (NULL == m_pLeafL)
		return false;

	m_pLeafR = m_renderer.CreateTextureFromFile(L"assets\\image\\splash\\right-leaf.png", true);
	if (NULL == m_pLeafR)
		return false;

	m_pLogo = m_renderer.CreateTextureFromFile(L"assets\\image\\splash\\logo.png", true);
	if (NULL == m_pLogo)
		return false;

	return true;
}

bool Splash::Update(float time)
{
	const float tAnim = time - m_tActivate;

	// render frame
	m_renderer.BeginFrame(kColorBlack);
	{		
		// draw: background & leaves
		m_renderer.AddSprite(m_pBackground, Renderer::kOpaque, D3DXVECTOR2(0.f, 0.f), 0.f);
		const float leafY = m_pLeafL->GetHeight();
		m_renderer.AddSprite(m_pLeafL, Renderer::kAlpha, D3DXVECTOR2(0.f, 1080.f-leafY), 0.25f);
		m_renderer.AddSprite(m_pLeafR, Renderer::kAlpha, D3DXVECTOR2(1920.f-m_pLeafR->GetWidth(), 1080.f-leafY), 0.25f);

		// ribbon FX
//		m_renderer.DrawRibbon(0.5f, 1.f, time);

		// draw: splash logo
		m_renderer.AddSpriteCenter(m_pLogo, Renderer::kAlpha, D3DXVECTOR2(1920.f*0.5f, 1080.f*0.5f), 0.75f);

		// fade in
		const float fade = tAnim < 2.f ? tAnim*0.5f : 1.f;
		m_renderer.FadeSprite(1.f, 1.f-fade);
	}
	m_renderer.EndFrame();

	return tAnim < 3.f;
}
