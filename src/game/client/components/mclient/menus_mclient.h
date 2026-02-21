
#ifndef GAME_CLIENT_COMPONENTS_MCLIENT_MENUS_MCLIENT_H
#define GAME_CLIENT_COMPONENTS_MCLIENT_MENUS_MCLIENT_H

#include <game/client/ui.h>

class CMenus
{
public:
	void RenderMClientSettings(CUIRect MainView);
	void RenderMClientFunSettings(CUIRect View);

	int m_ActiveMClientTab = 0;
	CLineInput m_McRainbowSpeedInput;
};

#endif
