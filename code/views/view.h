
#pragma once

#include "../renderer.h"
#include "../inputcallback.h"

class View
{
public:
//	static void InputCallback(Input input, void *pContext);

	View(Renderer &renderer) : m_renderer(renderer) {}
	virtual ~View() {}

	virtual bool Create() = 0;
	virtual bool Update(float time) = 0;
		
protected:
	Renderer &m_renderer;
};
