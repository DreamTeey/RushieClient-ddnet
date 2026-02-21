#include "mclient.h"

#include <engine/input.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>

CMClient::CMClient()
{
	m_LastRotateTime = 0.0f;
}

void CMClient::OnInit()
{
	// 初始化逻辑
}

void CMClient::OnConsoleInit()
{
	// 控制台初始化逻辑
}

void CMClient::OnRender()
{
	if(!g_Config.m_McRandomSkinRotate || Client()->State() != IClient::STATE_ONLINE)
		return;

	if(g_Config.m_McRandomSkinRotateOnlyLeftClick && !Input()->KeyIsPressed(KEY_MOUSE_1))
		return;

	float CurrentTime = Client()->LocalTime();

	// 处理主体
	if(CurrentTime - m_LastRotateTime >= g_Config.m_McRandomSkinRotateInterval)
	{
		if(g_Config.m_McRandomSkinRotateMain || g_Config.m_McRandomSkinRotateMainColor)
		{
			if(g_Config.m_McRandomSkinRotateMain)
				g_Config.m_ClPlayerUseCustomColor = false;
			if(g_Config.m_McRandomSkinRotateMainColor)
				g_Config.m_ClPlayerUseCustomColor = true;
			GameClient()->m_Skins.RandomizeSkin(0);
			GameClient()->SendInfo(false);
		}
		if(g_Config.m_McRandomSkinRotateDummy || g_Config.m_McRandomSkinRotateDummyColor)
		{
			if(g_Config.m_McRandomSkinRotateDummy)
				g_Config.m_ClDummyUseCustomColor = false;
			if(g_Config.m_McRandomSkinRotateDummyColor)
				g_Config.m_ClDummyUseCustomColor = true;
			GameClient()->m_Skins.RandomizeSkin(1);
			GameClient()->SendDummyInfo(false);
		}
		m_LastRotateTime = CurrentTime;
	}

}