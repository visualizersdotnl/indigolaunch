
// indigolaunch -- UI: select screen

#pragma once

#include "view.h"
#include "../collection.h"
#include "../timer.h"
#include "../text.h"

class Select : public View
{
public:
	static void InputCallback(Input input, void *pContext);

	Select(Renderer &renderer);
	~Select();
	
	bool Create();
	bool Update(float time);

	void OnActivate(const Collection &collection);
	const Title &GetSelectedTitle() const;

private:
	Renderer::Texture2D *m_pBar;
	Renderer::Texture2D *m_pArrowL, *m_pArrowR;
	Renderer::Texture2D *m_pA, *m_pEnter;
	Renderer::Texture2D *m_pKeyboard, *m_pController;

	Font m_theSans60;
	Font m_theSans48;
	Font m_theSans40;
	Font m_theSans28;
	Font m_yume40;
	TextRect m_textRect;

	// set by OnActivate()
	const Collection *m_pCollection;
	size_t m_iTitle;

	// set by InputCallback()
	enum Action
	{
		kNone,
		kLeft,
		kRight,
		kSelect
	} m_action;

	AnimTimer m_tAnim;
	float m_animDir;
	size_t m_iPrevTitle;
};
