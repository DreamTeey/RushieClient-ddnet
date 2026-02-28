#include "mclient.h"

#include <engine/console.h>
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
	m_BodyHue = 0.0f;
	m_FeetHue = 0.0f;
	mem_zero(m_aFriendStates, sizeof(m_aFriendStates));

	// 初始化武器历史
	m_aLastWeapon[0] = 1; // 默认锤子
	m_aLastWeapon[1] = 2; // 默认枪
	m_LastWeaponIndex = 0;
}

void CMClient::OnInit()
{
	UpdateFriendList();
}

void CMClient::OnConsoleInit()
{
	// 注册武器切换命令（支持按键绑定和直接执行）
	Console()->Register("mc_switch_last_weapon", "", CFGFLAG_CLIENT,
		ConSwitchLastWeaponCallback, this, "Switch to last used weapon");
}

void CMClient::OnRender()
{
	// 性能优化：提前返回检查
	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	// 检查克隆人皮肤复制功能
	if(g_Config.m_McClonePlayer)
	{
		CheckCloneSkin();
	}

	// 更新彩虹Tee颜色
	if(g_Config.m_McRainbowTee)
	{
		UpdateRainbow();
	}

	// 检查好友上线提醒
	if(g_Config.m_McFriendNotify)
	{
		CheckFriendNotification();
	}

	// 修复：使用正确的配置项作为条件判断
	if(g_Config.m_McRandomSkinRotate && ShouldRotateSkin())
	{
		HandleSkinRotation();
		m_LastRotateTime = Client()->LocalTime();
	}

	// 更新武器历史（只在武器真正改变时更新）
	static int s_LastWeapon = 0;
	if(Client()->State() == IClient::STATE_ONLINE)
	{
		int CurrentWeapon = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_WantedWeapon;
		if(CurrentWeapon > 0 && CurrentWeapon <= NUM_WEAPONS && CurrentWeapon != s_LastWeapon)
		{
			UpdateWeaponHistory(CurrentWeapon);
			s_LastWeapon = CurrentWeapon;
		}
	}
}

bool CMClient::CanCloneSkin() const
{
	return g_Config.m_McClonePlayer && Client()->State() == IClient::STATE_ONLINE;
}

bool CMClient::ShouldRotateSkin() const
{
	// 修复：确保时间计算正确，避免负数时间差
	float TimeDiff = Client()->LocalTime() - m_LastRotateTime;

	// 如果时间差为负数（可能由于时间重置），重置计时器
	if(TimeDiff < 0)
	{
		return true; // 立即允许轮换
	}

	// 检查左键限制
	if(g_Config.m_McRandomSkinRotateOnlyLeftClick)
	{
		// 修复：使用更可靠的鼠标状态检测
		if(!Input()->KeyIsPressed(KEY_MOUSE_1))
		{
			return false;
		}
	}

	// 检查时间间隔
	return TimeDiff >= g_Config.m_McRandomSkinRotateInterval;
}

void CMClient::HandleSkinRotation()
{
	bool Updated = false;

	// 主体皮肤轮换
	if(g_Config.m_McRandomSkinRotateMain || g_Config.m_McRandomSkinRotateMainColor)
	{
		g_Config.m_ClPlayerUseCustomColor = g_Config.m_McRandomSkinRotateMainColor;
		GameClient()->m_Skins.RandomizeSkin(0);
		GameClient()->SendInfo(false);
		Updated = true;
	}

	// 分身皮肤轮换
	if(g_Config.m_McRandomSkinRotateDummy || g_Config.m_McRandomSkinRotateDummyColor)
	{
		g_Config.m_ClDummyUseCustomColor = g_Config.m_McRandomSkinRotateDummyColor;
		GameClient()->m_Skins.RandomizeSkin(1);
		GameClient()->SendDummyInfo(false);
		Updated = true;
	}

	// 调试输出
	if(Updated)
	{
		/*
		dbg_msg("mclient", "Skin rotation completed - MainColor: %d, DummyColor: %d",
			g_Config.m_ClPlayerUseCustomColor,
			g_Config.m_ClDummyUseCustomColor);
		*/
	}
}

void CMClient::CheckCloneSkin()
{
	if(!CanCloneSkin())
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
		ShouldClone = CheckHammerClone(LocalPos, LocalId, TargetId);
	}

	// 距离生效
	if(g_Config.m_McCloneOnDistance && !ShouldClone)
	{
		ShouldClone = CheckDistanceClone(LocalPos, TargetId);
	}

	// if(ShouldClone && TargetId != -1 && TargetId != m_LastClonedClientId)
	if(ShouldClone && TargetId != -1)
	{
		CopyPlayerSkin(TargetId, IsDummy);
		m_LastClonedClientId = TargetId;
	}
}

bool CMClient::CheckHammerClone(const vec2& LocalPos, int LocalId, int& TargetId)
{
	const CNetObj_PlayerInput *pInput = &GameClient()->m_aClients[LocalId].m_Predicted.m_Input;
	if(!pInput || pInput->m_WantedWeapon != WEAPON_HAMMER || !(pInput->m_Fire & 1))
		return false;

	const vec2 Dir = normalize(vec2(pInput->m_TargetX, pInput->m_TargetY));
	const vec2 ProjStartPos = LocalPos + Dir * HAMMER_RADIUS * 0.75f;

	float MinDistance = 9999.0f;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == LocalId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
			continue;

		const vec2 OtherPos = vec2(
			GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_X,
			GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_Y
		);

		const float Distance = distance(ProjStartPos, OtherPos);
		if(Distance < MinDistance && Distance < HAMMER_RADIUS * 0.5f)
		{
			MinDistance = Distance;
			TargetId = i;
		}
	}

	// return TargetId != -1 && TargetId != m_LastClonedClientId;
	return TargetId != -1;
}

bool CMClient::CheckDistanceClone(const vec2& LocalPos, int& TargetId)
{
	float MinDistance = 9999.0f;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == GameClient()->m_Snap.m_LocalClientId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
			continue;

		const vec2 OtherPos = vec2(
			GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_X,
			GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_Y
		);

		const float Distance = distance(LocalPos, OtherPos);
		if(Distance < MinDistance)
		{
			MinDistance = Distance;
			TargetId = i;
		}
	}

	return MinDistance < MAX_CLONE_DISTANCE;
	// return MinDistance < MAX_CLONE_DISTANCE && TargetId != m_LastClonedClientId;
}

void CMClient::CopyPlayerSkin(int TargetId, bool IsDummy)
{
    CGameClient::CClientData *pTarget = &GameClient()->m_aClients[TargetId];
    bool Changed = false;

    // 1. 判断并复制基本信息 (Name, Country, Clan)
    if(g_Config.m_McCloneCopyName)
    {
        if(IsDummy)
        {
            if(str_comp(g_Config.m_ClDummyName, pTarget->m_aName) != 0 ||
               g_Config.m_ClDummyCountry != pTarget->m_Country ||
               str_comp(g_Config.m_ClDummyClan, pTarget->m_aClan) != 0)
            {
                str_copy(g_Config.m_ClDummyName, pTarget->m_aName, sizeof(g_Config.m_ClDummyName));
                g_Config.m_ClDummyCountry = pTarget->m_Country;
                str_copy(g_Config.m_ClDummyClan, pTarget->m_aClan, sizeof(g_Config.m_ClDummyClan));
                Changed = true;
            }
        }
        else
        {
            if(str_comp(g_Config.m_PlayerName, pTarget->m_aName) != 0 ||
               g_Config.m_PlayerCountry != pTarget->m_Country ||
               str_comp(g_Config.m_PlayerClan, pTarget->m_aClan) != 0)
            {
                str_copy(g_Config.m_PlayerName, pTarget->m_aName, sizeof(g_Config.m_PlayerName));
                g_Config.m_PlayerCountry = pTarget->m_Country;
                str_copy(g_Config.m_PlayerClan, pTarget->m_aClan, sizeof(g_Config.m_PlayerClan));
                Changed = true;
            }
        }
    }

    // 2. 判断并复制皮肤信息 (Skin, Colors)
    if(IsDummy)
    {
        if(str_comp(g_Config.m_ClDummySkin, pTarget->m_aSkinName) != 0 ||
           g_Config.m_ClDummyUseCustomColor != pTarget->m_UseCustomColor ||
           g_Config.m_ClDummyColorBody != pTarget->m_ColorBody ||
           g_Config.m_ClDummyColorFeet != pTarget->m_ColorFeet)
        {
            str_copy(g_Config.m_ClDummySkin, pTarget->m_aSkinName, sizeof(g_Config.m_ClDummySkin));
            g_Config.m_ClDummyUseCustomColor = pTarget->m_UseCustomColor;
            g_Config.m_ClDummyColorBody = pTarget->m_ColorBody;
            g_Config.m_ClDummyColorFeet = pTarget->m_ColorFeet;
            Changed = true;
        }
    }
    else
    {
        if(str_comp(g_Config.m_ClPlayerSkin, pTarget->m_aSkinName) != 0 ||
           g_Config.m_ClPlayerUseCustomColor != pTarget->m_UseCustomColor ||
           g_Config.m_ClPlayerColorBody != pTarget->m_ColorBody ||
           g_Config.m_ClPlayerColorFeet != pTarget->m_ColorFeet)
        {
            str_copy(g_Config.m_ClPlayerSkin, pTarget->m_aSkinName, sizeof(g_Config.m_ClPlayerSkin));
            g_Config.m_ClPlayerUseCustomColor = pTarget->m_UseCustomColor;
            g_Config.m_ClPlayerColorBody = pTarget->m_ColorBody;
            g_Config.m_ClPlayerColorFeet = pTarget->m_ColorFeet;
            Changed = true;
        }
    }

    // 3. 只有当信息确实发生变化时才发送更新
    if(Changed)
    {
        SendSkinUpdate(IsDummy);
    }
}

void CMClient::SendSkinUpdate(bool IsDummy)
{
	if(IsDummy)
		GameClient()->SendDummyInfo(false);
	else
		GameClient()->SendInfo(false);
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

    // 更新身体色相
    if(g_Config.m_McRainbowTeeBody)
    {
        float BodySpeed = g_Config.m_McRainbowTeeBodySpeed * Client()->FrameTimeAverage() * 0.01f;
        m_BodyHue += BodySpeed;
        if(m_BodyHue > 1.0f) m_BodyHue -= 1.0f;
    }

    // 更新脚部色相
    if(g_Config.m_McRainbowTeeFeet)
    {
        float FeetSpeed = g_Config.m_McRainbowTeeFeetSpeed * Client()->FrameTimeAverage() * 0.01f;
        m_FeetHue += FeetSpeed;
        if(m_FeetHue > 1.0f) m_FeetHue -= 1.0f;
    }

    if(g_Config.m_McRainbowTeeBody)
    {
        int BodyRainbowColor = getIntFromColor(m_BodyHue, BodySat, BodyLht);
        g_Config.m_ClPlayerColorBody = BodyRainbowColor;
        g_Config.m_ClDummyColorBody = BodyRainbowColor;
    }

    if(g_Config.m_McRainbowTeeFeet)
    {
        int FeetRainbowColor = getIntFromColor(m_FeetHue, FeetSat, FeetLht);
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

void CMClient::ConSwitchLastWeaponCallback(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	pThis->SwitchToLastWeapon();
}

void CMClient::SwitchToLastWeapon()
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	// 获取当前武器
	int WantedWeapon = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_WantedWeapon;

	// 切换到上一个武器
	int LastWeapon = m_aLastWeapon[1 - m_LastWeaponIndex];
	if(LastWeapon <= 0 || LastWeapon > NUM_WEAPONS)
		LastWeapon = 1; // 默认锤子

	// 直接设置武器
	GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_WantedWeapon = LastWeapon;

	// 更新武器历史
	UpdateWeaponHistory(WantedWeapon);
}

void CMClient::UpdateWeaponHistory(int CurrentWeapon)
{
	if(CurrentWeapon <= 0 || CurrentWeapon > NUM_WEAPONS)
		return;

	// 如果当前武器与上一个记录不同，则更新历史
	if(m_aLastWeapon[m_LastWeaponIndex] != CurrentWeapon)
	{
		m_LastWeaponIndex = 1 - m_LastWeaponIndex;
		m_aLastWeapon[m_LastWeaponIndex] = CurrentWeapon;
	}
}
