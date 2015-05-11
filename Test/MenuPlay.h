#pragma once

#include "Menu.h"
#include <Database.h>

class MenuPlay : public Menu
{
	protected:
		DatabaseWorkspace		m_ws;

	public:
		MenuPlay();
};
