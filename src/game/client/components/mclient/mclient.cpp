#include "mclient.h"

#include <engine/input.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>
#include <game/gamecore.h>

CMClient::CMClient()
{
	m_LastRotateTime = 0.0f;
	m_LastCloneTick = 0;
	m_LastClonedClientId = -1;
	m_RainbowDelay = 0;
	m_LastFriendRefreshTime = 0.0f;
	m_NumFriendStates = 0;
	mem_zero(m_aFriendStates, sizeof(m_aFriendStates));
}

void CMClient::OnInit()
{
	UpdateFriendList();
}

void CMClient::OnConsoleInit()
{
	// 控制台初始化逻辑
}

void CMClient::OnRender()
{
	// 检查克隆人皮肤复制功能
	CheckCloneSkin();

	// 更新彩虹Tee颜色
	UpdateRainbow();

	// 检查好友上线提醒
	CheckFriendNotification();

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

void CMClient::CheckCloneSkin()
{
	if(!g_Config.m_McClonePlayer || Client()->State() != IClient::STATE_ONLINE)
		return;

	int LocalId = GameClient()->m_Snap.m_LocalClientId;
	bool IsDummy = g_Config.m_ClDummy;
	if(LocalId < 0 || LocalId >= MAX_CLIENTS)
		return;

	vec2 LocalPos = vec2(0, 0);

	if(!GameClient()->m_Snap.m_aCharacters[LocalId].m_Active)
		return;

	LocalPos = vec2(
		GameClient()->m_Snap.m_aCharacters[LocalId].m_Cur.m_X,
		GameClient()->m_Snap.m_aCharacters[LocalId].m_Cur.m_Y
	);
	
	bool ShouldClone = false;
	int TargetId = -1;

	// 勾住生效
	if(g_Config.m_McCloneOnHook)
	{
		// 检查本地玩家是否正在使用钩子
		if(GameClient()->m_aClients[LocalId].m_Predicted.HookedPlayer() != -1)
		{
			TargetId = GameClient()->m_aClients[LocalId].m_Predicted.HookedPlayer();
			ShouldClone = true;
		}
	}

	// 锤击生效
	if(g_Config.m_McCloneOnHammer && !ShouldClone)
	{
		const CNetObj_PlayerInput *pInput = &GameClient()->m_aClients[LocalId].m_Predicted.m_Input;
		if(!pInput)
			return;
		
		if(pInput->m_WantedWeapon == WEAPON_HAMMER)
		{
			if(pInput->m_Fire & 1)
			{
				vec2 Dir = normalize(vec2(pInput->m_TargetX, pInput->m_TargetY));
				if(Dir.x == 0.0f && Dir.y == 0.0f)
					Dir = vec2(0.0f, -1.0f);

				const float Radius = 28.0f;
				const vec2 ProjStartPos = LocalPos + Dir * Radius * 0.75f;

				float MinDistance = 9999.0f;
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(i == LocalId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
						continue;

					vec2 OtherPos = vec2(
						GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_X,
						GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_Y
					);

					float Distance = distance(ProjStartPos, OtherPos);
					if(Distance < MinDistance && Distance < Radius * 0.5f)
					{
						MinDistance = Distance;
						TargetId = i;
					}
				}

				if(TargetId != -1 && TargetId != m_LastClonedClientId)
				{
					ShouldClone = true;
				}
			}
		}
	}

	if(g_Config.m_McCloneOnDistance && !ShouldClone)
	{
		float MinDistance = 9999.0f;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(i == LocalId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
				continue;

			vec2 OtherPos = vec2(
				GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_X,
				GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_Y
			);

			float Distance = distance(LocalPos, OtherPos);
			if(Distance < MinDistance)
			{
				MinDistance = Distance;
				TargetId = i;
			}
		}

		if(MinDistance < 50.0f && TargetId != m_LastClonedClientId)
		{
			ShouldClone = true;
		}
	}

	if(ShouldClone && TargetId != -1 && TargetId != m_LastClonedClientId)
	{
		CGameClient::CClientData *pTarget = &GameClient()->m_aClients[TargetId];
		TargetId = GameClient()->m_aClients[LocalId].m_Predicted.HookedPlayer();
		if (IsDummy) 
		{
			if (g_Config.m_McCloneCopyName) 
			{
				str_copy(g_Config.m_ClDummyName, pTarget->m_aName, sizeof(g_Config.m_ClDummyName));//名字
				g_Config.m_ClDummyCountry = pTarget->m_Country;//国家
				str_copy(g_Config.m_ClDummyClan, pTarget->m_aClan, sizeof(g_Config.m_ClDummyClan));	//战队
			}
			str_copy(g_Config.m_ClDummySkin, pTarget->m_aSkinName, sizeof(g_Config.m_ClDummySkin));
			g_Config.m_ClDummyUseCustomColor = pTarget->m_UseCustomColor;
			g_Config.m_ClDummyColorBody = pTarget->m_ColorBody;
			g_Config.m_ClDummyColorFeet = pTarget->m_ColorFeet;
			GameClient()->SendDummyInfo(false);
		}
		else 
		{
			if (g_Config.m_McCloneCopyName) 
			{
				str_copy(g_Config.m_PlayerName, pTarget->m_aName, sizeof(g_Config.m_PlayerName));//名字
				g_Config.m_PlayerCountry = pTarget->m_Country;//国家
				str_copy(g_Config.m_PlayerClan, pTarget->m_aClan, sizeof(g_Config.m_PlayerClan));	//战队
			}
			str_copy(g_Config.m_ClPlayerSkin, pTarget->m_aSkinName, sizeof(g_Config.m_ClPlayerSkin));
			g_Config.m_ClPlayerUseCustomColor = pTarget->m_UseCustomColor;
			g_Config.m_ClPlayerColorBody = pTarget->m_ColorBody;
			g_Config.m_ClPlayerColorFeet = pTarget->m_ColorFeet;
			GameClient()->SendInfo(false);
		}
		m_LastClonedClientId = TargetId;
	}
}

void CMClient::UpdateRainbow()
{
    if(!g_Config.m_McRainbowTee)
		return;

    // 从原本的颜色中提取饱和度和明度
    int OriginalBodyColor = g_Config.m_ClPlayerColorBody;
    float BodySat = ((OriginalBodyColor >> 8) & 0xFF) / 255.0f;
    float BodyLht = (OriginalBodyColor & 0xFF) / 255.0f;

    int OriginalFeetColor = g_Config.m_ClPlayerColorFeet;
    float FeetSat = ((OriginalFeetColor >> 8) & 0xFF) / 255.0f;
    float FeetLht = (OriginalFeetColor & 0xFF) / 255.0f;

    static float BodyHue = 0.0f;
    static float FeetHue = 0.0f;
    
    // 更新身体色相
    if(g_Config.m_McRainbowTeeBody)
    {
        float BodySpeed = g_Config.m_McRainbowTeeBodySpeed * Client()->FrameTimeAverage() * 0.01f;
        BodyHue += BodySpeed;
        if(BodyHue > 1.0f) BodyHue -= 1.0f;
    }
    
    // 更新脚部色相
    if(g_Config.m_McRainbowTeeFeet)
    {
        float FeetSpeed = g_Config.m_McRainbowTeeFeetSpeed * Client()->FrameTimeAverage() * 0.01f;
        FeetHue += FeetSpeed;
        if(FeetHue > 1.0f) FeetHue -= 1.0f;
    }

    if(g_Config.m_McRainbowTeeBody)
    {
        int BodyRainbowColor = getIntFromColor(BodyHue, BodySat, BodyLht);
        g_Config.m_ClPlayerColorBody = BodyRainbowColor;
        g_Config.m_ClDummyColorBody = BodyRainbowColor;
    }

    if(g_Config.m_McRainbowTeeFeet)
    {
        int FeetRainbowColor = getIntFromColor(FeetHue, FeetSat, FeetLht);
        g_Config.m_ClPlayerColorFeet = FeetRainbowColor;
        g_Config.m_ClDummyColorFeet = FeetRainbowColor;
    }

    if(Client()->State() == IClient::STATE_ONLINE)
    {
        if(m_RainbowDelay < time_get())
        {
			if(g_Config.m_ClPlayerUseCustomColor)
            	GameClient()->SendInfo(false);	
        	if(g_Config.m_ClDummyUseCustomColor)
            	GameClient()->SendDummyInfo(false);
            m_RainbowDelay = time_get() + time_freq() * g_Config.m_SvInfoChangeDelay;
        }
    }
}