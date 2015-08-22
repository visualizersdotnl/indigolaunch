
// indigolaunch -- D3DX text implementation (hack)

#include "platform.h"
#include "text.h"

Font::Font(Renderer &renderer, const std::wstring &path, const std::wstring &name) :
	m_renderer(renderer)
,	m_path(path)
,	m_name(name)
,	m_isRegistered(false)
,	m_pFont(NULL)
{
}

Font::~Font()
{
	SAFE_RELEASE(m_pFont);
	
	if (m_isRegistered) 
		RemoveFontResourceEx(m_path.c_str(), FR_PRIVATE, NULL);
}

bool Font::Create(unsigned int size, bool bold)
{
	m_isRegistered = 0 != AddFontResourceEx(m_path.c_str(), FR_PRIVATE, NULL);
	if (m_isRegistered)
	{
		if SUCCEEDED(D3DX10CreateFont(
			&m_renderer.m_D3D,
			size,
			0,
			(!bold) ? FW_NORMAL : FW_BOLD,
			0,
			false,
			DEFAULT_CHARSET,
			OUT_TT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			ANTIALIASED_QUALITY,
			m_name.c_str(),
			&m_pFont))
		{
			return true;
		}
	}

	SetLastError(L"Can not load font: '" + m_path + L"'");
	return false;
}

TextRect::TextRect(Renderer &renderer, unsigned int width, unsigned int height) :
	m_renderer(renderer)
,	m_width(width)
,	m_height(height)
,	m_pTarget(NULL)
,	m_canWrite(false)
{
}

TextRect::~TextRect()
{
	SAFE_RELEASE(m_pTarget);
}

bool TextRect::Create()
{
	m_pTarget = m_renderer.CreateRenderTarget(m_width, m_height);
	return NULL != m_pTarget;
}

void TextRect::Begin()
{
	ASSERT(false == m_canWrite);
	ASSERT(NULL != m_pTarget);

	// bind render target with full viewport
	m_renderer.m_D3D.OMSetRenderTargets(1, &m_pTarget->m_pRTV, NULL);

	D3D10_VIEWPORT fullVP;
	fullVP.TopLeftX = 0;
	fullVP.TopLeftY = 0;
	fullVP.Width = m_width;
	fullVP.Height = m_height;
	fullVP.MinDepth = 0.f;
	fullVP.MaxDepth = 1.f;
	m_renderer.m_D3D.RSSetViewports(1, &fullVP);

	// clear
	m_renderer.m_D3D.ClearRenderTargetView(m_pTarget->m_pRTV, D3DXCOLOR((UINT) 0));

	m_canWrite = true;
}

void TextRect::End()
{
	ASSERT(true == m_canWrite);

	// restore back buffer with adj. viewport
	m_renderer.m_D3D.OMSetRenderTargets(1, &m_renderer.m_pBackBufferRTV, NULL);
	m_renderer.m_D3D.RSSetViewports(1, &m_renderer.m_adjVP);

	m_canWrite = false;
}

void TextRect::Write(const Font &font, const std::wstring &text, const RECT &position, UINT Format)
{
	ASSERT(true == m_canWrite);
	font.m_pFont->DrawText(NULL, text.c_str(), -1, LPRECT(&position), Format, kColorWhite);	
}
