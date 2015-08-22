
// indigolaunch -- UI: select screen

#include "../platform.h"
#include "select.h"
#include "../audio.h"

const float kSlideAnimMinTime = 0.25f;

/* static */ void Select::InputCallback(Input input, void *pContext)
{
	ASSERT(NULL != pContext);
	Select *pInst = reinterpret_cast<Select *>(pContext);

	if (kNone == pInst->m_action)
	{
		switch (input)
		{
		case kInputLeft:
		case kInputRight:
			if (false == pInst->m_tAnim.IsRunning() || kSlideAnimMinTime <= pInst->m_tAnim.Get())
				pInst->m_action = (kInputLeft == input) ? kLeft : kRight;
			break;

		case kInputStart:
			if (false == pInst->m_tAnim.IsRunning())
				pInst->m_action = kSelect;
			break;

		case kInputBack:
		case kInputKillSwitch:
			break;
		}
	}
}

Select::Select(Renderer &renderer) :
	View(renderer)
,	m_pBar(NULL)
,	m_pArrowL(NULL)
,	m_pArrowR(NULL)
,	m_pA(NULL)
,	m_pEnter(NULL)
,	m_pKeyboard(NULL)
,	m_pController(NULL)
,	m_theSans60(renderer, L"assets\\font\\TheSansOsF-ExtraBold.otf", L"TheSansOsF ExtraBold")
,	m_theSans48(renderer, L"assets\\font\\TheSansOsF-Plain.otf", L"TheSansOsF Plain")
,	m_theSans40(renderer, L"assets\\font\\TheSansOsF-Plain.otf", L"TheSansOsF Plain")
,	m_theSans28(renderer, L"assets\\font\\TheSansOsF-Plain.otf", L"TheSansOsF Plain")
,	m_yume40(renderer, L"assets\\font\\Yume.ttf", L"Yume")
,	m_textRect(renderer, 640, 1080)
,	m_pCollection(NULL), m_action(kNone)
{
	// hack: only reset m_iTitle once!
	m_iTitle = 0;
}

Select::~Select()
{
	SAFE_RELEASE(m_pBar);
	SAFE_RELEASE(m_pArrowL);
	SAFE_RELEASE(m_pArrowR);
	SAFE_RELEASE(m_pA);
	SAFE_RELEASE(m_pEnter);
	SAFE_RELEASE(m_pKeyboard);
	SAFE_RELEASE(m_pController);
}

bool Select::Create()
{
	m_pBar = m_renderer.CreateTextureFromFile(L"assets\\image\\select\\bar.png", true);
	if (NULL == m_pBar)
		return false;

	m_pArrowL = m_renderer.CreateTextureFromFile(L"assets\\image\\select\\left-arrow.png", true);
	if (NULL == m_pArrowL)
		return false;

	m_pArrowR = m_renderer.CreateTextureFromFile(L"assets\\image\\select\\right-arrow.png", true);
	if (NULL == m_pArrowR)
		return false;

	m_pA = m_renderer.CreateTextureFromFile(L"assets\\image\\select\\a-button.png", true);
	if (NULL == m_pA)
		return false;

	m_pEnter = m_renderer.CreateTextureFromFile(L"assets\\image\\select\\enter-button.png", true);
	if (NULL == m_pEnter)
		return false;

	m_pKeyboard = m_renderer.CreateTextureFromFile(L"assets\\icon\\keyboard.png", true);
	if (NULL == m_pKeyboard)
		return false;

	m_pController = m_renderer.CreateTextureFromFile(L"assets\\icon\\controller.png", true);
	if (NULL == m_pController)
		return false;

	return m_theSans60.Create(60, true)  &&
	       m_theSans48.Create(48, false) &&
	       m_theSans40.Create(40, false) &&
	       m_theSans28.Create(28, false) &&
	          m_yume40.Create(40, false) &&
	        m_textRect.Create();
}

bool Select::Update(float time)
{
	ASSERT(NULL != m_pCollection);

	// handle action
	switch (m_action)
	{
	case kNone:
		break;

	case kLeft:		
	case kRight:
		m_iPrevTitle = m_iTitle;

		if (kLeft == m_action)
		{
			m_iTitle = (m_iTitle > 0) ? m_iTitle-1 : m_pCollection->GetNumTitles()-1;
			m_animDir = -1.f;
		}
		else
		{
			m_iTitle = (m_iTitle < m_pCollection->GetNumTitles()-1) ? m_iTitle+1 : 0;
			m_animDir = 1.f;
		}
		
		if (kSlideAnimMinTime <= m_tAnim.Get())
		{
			m_tAnim.Run(kSlideAnimMinTime);
		}
		else
			m_tAnim.Run(1.f);
		
		Audio_PlaySample(kSampleLR);
		m_action = kNone;
		break;

	case kSelect:
		Audio_PlaySample(kSampleSelect);
		break;
	}

	// render frame
	m_renderer.BeginFrame(kColorBlack);
	{
		// Z order
		const float kBackgroundZ = 0.f;
		const float kIconBackZ = 0.04f;
		const float kIconZ = 0.05f;
		const float kBoxZ = 0.1f;
		const float kBoxTextZ = 0.11f;
		const float kControlIconZ = 0.12f;
		const float kVertBarZ = 0.2f;
		const float kArrowZ = 0.3f;
		const float kBarZ = 0.4f;
		const float kButtonZ = 0.5f;
		const float kSliderZ = 0.5f;

		// title, box anchor & icon anim.
		const Title *pTitle = &m_pCollection->GetTitle(m_iTitle);
		float boxAnchor = 0.f;
		float iconAnim = 0.f;
		if (m_tAnim.IsRunning())
		{
			float tAnim = m_tAnim.Get()*2.f;
			
			if (tAnim < 1.f)
			{
				// show prev. title during first half of anim.
				pTitle = &m_pCollection->GetTitle(m_iPrevTitle);

				// slide out
				tAnim = SmoothStep(tAnim);
				boxAnchor = -640.f*tAnim;
				iconAnim = tAnim;		
			}
			else
			{
				// slide in
				tAnim = SmoothStep(tAnim-1.f);
				boxAnchor = -640.f*(1.f-tAnim);
				iconAnim = 1.f-tAnim;		
			}
		}

		// draw: title background
		const Renderer::Texture2D *pBackground = m_pCollection->GetTitle(m_iTitle).GetBackground();
		if (false == m_tAnim.IsRunning())
		{
			m_renderer.AddSprite(pBackground, Renderer::kOpaque, D3DXVECTOR2(0.f, 0.f), kBackgroundZ);
		}
		else
		{
			// animate (slide)
			const float xOut = m_animDir*SmoothStep(m_tAnim.Get())*1920.f;
			const float xIn = -m_animDir*1920.f + xOut;
			const Renderer::Texture2D *pPrevBackground = m_pCollection->GetTitle(m_iPrevTitle).GetBackground();
			m_renderer.AddSprite(pPrevBackground, Renderer::kOpaque, D3DXVECTOR2(xOut, 0.f), kBackgroundZ);
			m_renderer.AddSprite(pBackground, Renderer::kOpaque, D3DXVECTOR2(xIn, 0.f), kBackgroundZ);
		}

		// draw: platform (8) & award (2) icons
		for (size_t iPlatform = 0; iPlatform < 8; ++iPlatform)
		{
			const Renderer::Texture2D *pIcon = pTitle->GetPlatformIcon(iPlatform);
			if (NULL == pIcon)
				break;
			
			const float iconX = 640.f+15.f + boxAnchor*0.3f;
			const float iconY = 100.f + pIcon->GetHeight()*iPlatform + boxAnchor*0.9f;		
			const float alpha = 1.f-iconAnim;
//			const float backRot = -iconAnim*m_animDir*0.10f*3.14f;
//			m_renderer.AddSprite(NULL, Renderer::kAlpha, D3DXCOLOR(0.1f, 0.1f, 0.1f, alpha*0.49f), D3DXVECTOR2(iconX-2.f, iconY-1.f), D3DXVECTOR2(pIcon->GetWidth(), pIcon->GetHeight()), kIconBackZ, backRot);
			m_renderer.AddSprite(pIcon, Renderer::kAlpha, D3DXVECTOR2(iconX, iconY), kIconZ, alpha*0.8f);
		}

		for (size_t iAward = 0; iAward < 2; ++iAward)
		{
			const Renderer::Texture2D *pIcon = pTitle->GetAwardIcon(iAward);
			if (NULL == pIcon)
				break;
			
			const float iconX = 1800.f - (30.f+pIcon->GetWidth())*iAward;
			const float iconY = -pIcon->GetHeight()*iconAnim;		
			m_renderer.AddSprite(pIcon, Renderer::kAlpha, D3DXVECTOR2(iconX, iconY), kIconZ);
		}
		
		// draw: transparent box
		m_renderer.AddSprite(
			NULL, 
			Renderer::kAlpha,
			D3DXCOLOR(0.f, 0.f, 0.f, 0.6f),
			D3DXVECTOR2(boxAnchor, 0.f),
			D3DXVECTOR2(640.f, 1080.f), 
			kBoxZ);

		// draw: vertical bars
		m_renderer.AddSprite(NULL, Renderer::kOpaque, kColorWhite, D3DXVECTOR2(0.f, 0.f), D3DXVECTOR2(10.f, 1080.f), kVertBarZ);
		m_renderer.AddSprite(NULL, Renderer::kOpaque, kColorWhite, D3DXVECTOR2(1920.f-10.f, 0.f), D3DXVECTOR2(10.f, 1080.f), kVertBarZ);

		// draw: L/R arrows
		const D3DXVECTOR2 arrPosL(0.f, 1080.f*0.5f - m_pArrowL->GetHeight()*0.5f);
		const D3DXVECTOR2 arrPosR(1920.f-m_pArrowR->GetWidth(), 1080.f*0.5f - m_pArrowR->GetHeight()*0.5f);
		m_renderer.AddSprite(m_pArrowL, Renderer::kAlpha, arrPosL, kArrowZ);
		m_renderer.AddSprite(m_pArrowR, Renderer::kAlpha, arrPosR, kArrowZ);

		// draw: (INDIGO) bar (incl. leaves)
		m_renderer.AddSprite(m_pBar, Renderer::kAlpha, D3DXVECTOR2(0.f, 1080.f-m_pBar->GetHeight()), kBarZ);

		// draw: button
		const Renderer::Texture2D *pButton = (g_padConnected) ? m_pA : m_pEnter;
		m_renderer.AddSprite(pButton, Renderer::kAlpha, D3DXVECTOR2(1350.f, 990.f), kButtonZ);

		// draw: control icons
		float iconX = boxAnchor + 90.f;
		const float iconY = 730.f;
		
		if (true == pTitle->UsesKeyboard())
		{
			m_renderer.AddSprite(m_pKeyboard, Renderer::kAlpha, D3DXVECTOR2(iconX, iconY), kControlIconZ);
			iconX += m_pKeyboard->GetWidth() + 20.f;
		}
		
		if (true == pTitle->UsesController())
		{
			m_renderer.AddSprite(m_pController, Renderer::kAlpha, D3DXVECTOR2(iconX, iconY), kControlIconZ);
			iconX += m_pKeyboard->GetWidth() + 20.f;
		}

		// draw: slider
		const float sliderStart = 125.f;
		const float sliderEnd = 1920.f-125.f;
		const float sliderLen = (1920.f-250.f) / m_pCollection->GetNumTitles();

		float sliderX = sliderStart;
		const float sliderY = 946.f;
		float sliderAlpha = 1.f;

		if (m_tAnim.IsRunning())
		{
			sliderX += sliderLen*m_iPrevTitle;
			sliderX += m_animDir*m_tAnim.Get()*sliderLen;
			
			// hack: fade at edges
			if (sliderX < sliderStart)
			{
				sliderX = 125.f;
				sliderAlpha = 1.f-m_tAnim.Get();
			}
			else if (sliderX > sliderEnd-sliderLen)
			{
				sliderX = sliderEnd-sliderLen;
				sliderAlpha = 1.f-m_tAnim.Get();
			}	
		}
		else 
		{
			sliderX += sliderLen*m_iTitle;
		}

		const D3DXCOLOR sliderColor(75.f/256.f, 221.f/256.f, 205.f/256.f, sliderAlpha);
		m_renderer.AddSprite(NULL, Renderer::kAlpha, sliderColor, D3DXVECTOR2(sliderX, sliderY), D3DXVECTOR2(sliderLen, 10.f), kSliderZ);

		// draw text
		m_textRect.Begin();
		{
			// anchors (box)
			const int textL = 90;
			const int textR = 640-45;
			int textY = 100;
			
			// title & developer
			const RECT titlePos = { textL, textY, textR, textY+60 };
			m_textRect.Write(m_theSans60, pTitle->GetName(), titlePos, DT_LEFT);
			textY = titlePos.bottom;
			const RECT devPos = { textL, textY, textR, textY+48 };
			m_textRect.Write(m_theSans48, L"by " + pTitle->GetTitleInfo(Title::kDeveloper), devPos, DT_LEFT);
			textY = devPos.bottom;
			
			// summary (max. 14 lines)
			const RECT descPos = { textL, textY, textR, textY + 14*28 };
			m_textRect.Write(m_theSans28, pTitle->GetTitleInfo(Title::kSummary), descPos, DT_LEFT | DT_VCENTER | DT_WORDBREAK);
			textY = descPos.bottom;
			
			// num. players
			const RECT numPlayerPos = { textL, textY, textR, textY+40 };
			m_textRect.Write(m_yume40, L"PLAYERS:", numPlayerPos, DT_LEFT);
			m_textRect.Write(m_theSans40, pTitle->GetTitleInfo(Title::kNumPlayers), numPlayerPos, DT_RIGHT);
			textY = numPlayerPos.bottom;

			// genre
			const RECT genrePos = { textL, textY, textR, textY+40 };
			m_textRect.Write(m_yume40, L"GENRE:", genrePos, DT_LEFT);
			m_textRect.Write(m_theSans40, pTitle->GetTitleInfo(Title::kGenre), genrePos, DT_RIGHT);
			textY = genrePos.bottom;
		}
		m_textRect.End();

		// draw: box text
		m_renderer.AddSprite(m_textRect.GetTexture(), Renderer::kAlpha, D3DXVECTOR2(boxAnchor, 0.f), kBoxTextZ);
	}
	m_renderer.EndFrame();

	return m_action != kSelect;
}

void Select::OnActivate(const Collection &collection)
{
	m_pCollection = &collection;
	m_action = kNone;

	// hack: do not reset m_iTitle
//	m_iTitle = 0;
}

const Title &Select::GetSelectedTitle() const 
{ 
	ASSERT(NULL != m_pCollection); 
	return m_pCollection->GetTitle(m_iTitle);
}
