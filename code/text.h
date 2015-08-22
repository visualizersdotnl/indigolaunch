
// indigolaunch -- D3DX text implementation (hack)

#pragma once

#include "renderer.h"

class TextRect;

class Font
{
	friend class TextRect;

public:
	Font(Renderer &renderer, const std::wstring &path, const std::wstring &name);
	~Font();

	bool Create(unsigned int size, bool bold);

private:
	Renderer &m_renderer;
	const std::wstring m_path;
	const std::wstring m_name;

	bool m_isRegistered;
	ID3DX10Font *m_pFont;
};

class TextRect
{
public:
	TextRect(Renderer &renderer, unsigned int width, unsigned int height);
	~TextRect();

	bool Create();

	void Begin();
	void End();

	// - Format: Google 'MSDN DrawText()'
	void Write(const Font &font, const std::wstring &text, const RECT &position, UINT Format);

	const Renderer::Texture2D *GetTexture() const
	{ 
		ASSERT(NULL != m_pTarget);
		return m_pTarget; 
	}

private:
	Renderer &m_renderer;
	const unsigned int m_width;
	const unsigned int m_height;

	Renderer::Texture2D *m_pTarget;

	bool m_canWrite;
};
