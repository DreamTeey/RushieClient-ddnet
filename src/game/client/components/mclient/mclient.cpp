#include "mclient.h"

#include <engine/console.h>
#include <engine/input.h>
#include <engine/shared/config.h>
#include <game/client/gameclient.h>
#include <game/gamecore.h>

// 定义静态成员变量
CMClient::SFriendState CMClient::m_aFriendStates[IFriends::MAX_FRIENDS];

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

	// 初始化复读功能
	m_HasLastMessage = false;
	mem_zero(m_aLastMessage, sizeof(m_aLastMessage));

	// 初始化自动加一功能
	m_HasPreviousMessage = false;
	m_LastPlusOneTime = 0.0f;
	mem_zero(m_aPreviousMessage, sizeof(m_aPreviousMessage));
	mem_zero(m_aLastRepeatedMessage, sizeof(m_aLastRepeatedMessage));

	// 初始化钩子角度辅助功能
	m_HookAngleHelperEnabled = false;
	m_BestHookAngle = 0.0f;
	m_HasBestAngle = false;

	// 初始化最后勾我的玩家
	m_LastHookedByClientId = -1;
	m_LastHookedByTime = 0;
	mem_zero(m_aLastHookedByName, sizeof(m_aLastHookedByName));
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

	// 注册复读命令（支持按键绑定和直接执行）
	Console()->Register("mc_repeat_last_message", "", CFGFLAG_CLIENT,
		ConRepeatLastMessageCallback, this, "Repeat last message");

	// 注册钩子角度辅助命令（支持按键绑定和直接执行）
	Console()->Register("mc_toggle_hook_angle_helper", "", CFGFLAG_CLIENT,
		ConToggleHookAngleHelperCallback, this, "Toggle hook angle helper");
	Console()->Register("mc_hook_angle_apply", "", CFGFLAG_CLIENT,
		ConHookAngleApplyCallback, this, "Apply best angle");
	Console()->Register("mc_hook_angle_reset", "", CFGFLAG_CLIENT,
		ConHookAngleResetCallback, this, "Reset helper state");
}

void CMClient::OnMessage(int MsgType, void *pRawMsg)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;
	// 处理SV_CHAT消息
	if(MsgType == NETMSGTYPE_SV_CHAT)
	{

		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		int ClientId = pMsg->m_ClientId;
		int Team = pMsg->m_Team;

		// 调试日志：ClientId和Team
		// dbg_msg("mclient", "ClientId=%d, Team=%d", ClientId, Team);

		// 安全检查：确保ClientId在有效范围内
		if(ClientId < 0 || ClientId > MAX_CLIENTS)
		{
			// dbg_msg("mclient", "Invalid ClientId, returning");
			return;
		}

		// 获取消息内容
		const char *pMessage = pMsg->m_pMessage;

		// 调试日志：消息内容
		// dbg_msg("mclient", "Message: %s", pMessage);

		// 记录最后一条消息（如果是自己发送的）
		if(ClientId != GameClient()->m_Snap.m_LocalClientId)
		{
			str_copy(m_aLastMessage, pMessage, sizeof(m_aLastMessage));
			m_HasLastMessage = true;
			// dbg_msg("mclient", "Saved last message: %s", m_aLastMessage);
			// 处理自动加一功能（只处理公屏消息，不处理私聊）
			if(Team == 0 && g_Config.m_McAutoPlusOneEnable)
			{
				ProcessAutoPlusOne(pMessage, ClientId);
			}
		}
	}
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

	// 钩子角度辅助功能
	if(g_Config.m_McHookAngleHelper)
	{
		UpdateHookAngleHelper();
		RenderHookAngleHelper();
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

	// 更新最后勾我的玩家
	if(g_Config.m_McWordLibraryEnable)
	{
		UpdateLastHookedBy();
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
           (unsigned int)g_Config.m_ClDummyColorBody != pTarget->m_ColorBody ||
           (unsigned int)g_Config.m_ClDummyColorFeet != pTarget->m_ColorFeet)
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
           (unsigned int)g_Config.m_ClPlayerColorBody != pTarget->m_ColorBody ||
           (unsigned int)g_Config.m_ClPlayerColorFeet != pTarget->m_ColorFeet)
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

// 武器切换功能
bool CMClient::HasWeapon(int Weapon) const
{
	int WeaponIndex = Weapon - 1;
	if(WeaponIndex < 0 || WeaponIndex >= NUM_WEAPONS)
		return false;

	int LocalClientId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalClientId < 0 || LocalClientId >= MAX_CLIENTS)
		return false;

	const CGameClient::CClientData &ClientData = GameClient()->m_aClients[LocalClientId];
	return ClientData.m_Predicted.m_aWeapons[WeaponIndex].m_Got;
}

void CMClient::SwitchToLastWeapon()
{
	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	int CurrentWeapon = GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_WantedWeapon;
	int LastWeapon = m_aLastWeapon[1 - m_LastWeaponIndex];

	if(LastWeapon <= 0 || LastWeapon > NUM_WEAPONS || !HasWeapon(LastWeapon))
	{
		LastWeapon = 1;
		if(!HasWeapon(LastWeapon))
			return;
	}

	GameClient()->m_Controls.m_aInputData[g_Config.m_ClDummy].m_WantedWeapon = LastWeapon;
	UpdateWeaponHistory(CurrentWeapon);
}

void CMClient::UpdateWeaponHistory(int CurrentWeapon)
{
	if(CurrentWeapon <= 0 || CurrentWeapon > NUM_WEAPONS)
		return;

	if(!HasWeapon(CurrentWeapon))
		return;

	if(m_aLastWeapon[m_LastWeaponIndex] != CurrentWeapon)
	{
		m_LastWeaponIndex = 1 - m_LastWeaponIndex;
		m_aLastWeapon[m_LastWeaponIndex] = CurrentWeapon;
	}
}

void CMClient::RepeatLastMessage()
{
	// 检查是否有记录的消息
	if(!m_HasLastMessage || m_aLastMessage[0] == '\0')
	{
		// dbg_msg("mclient", "No last message to repeat");
		return;
	}

	// 检查是否连接到服务器
	if(Client()->State() != IClient::STATE_ONLINE)
	{
		// dbg_msg("mclient", "Not connected to server");
		return;
	}

	// 发送消息
	GameClient()->m_Chat.SendChat(0, m_aLastMessage);

	// dbg_msg("mclient", "Repeated message: %s", m_aLastMessage);
}

void CMClient::ConRepeatLastMessageCallback(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pMClient = static_cast<CMClient *>(pUserData);
	pMClient->RepeatLastMessage();
}

void CMClient::ProcessAutoPlusOne(const char *pMessage, int ClientId)
{
	// 检查是否是命令（以/开头）
	if(pMessage[0] == '/')
		return;

	// 检查冷却时间
	float TimeDiff = Client()->LocalTime() - m_LastPlusOneTime;
	if(TimeDiff < g_Config.m_McAutoPlusOneCooldown)
	{
		// dbg_msg("mclient", "Auto plus one cooldown: %.2f seconds remaining", g_Config.m_McAutoPlusOneCooldown - TimeDiff);
		return;
	}

	// 检查是否已经复读过这条消息
	if(str_comp(pMessage, m_aLastRepeatedMessage) == 0)
	{
		// dbg_msg("mclient", "Already repeated this message: %s", pMessage);
		return;
	}

	// 检查是否有上一条消息
	if(!m_HasPreviousMessage)
	{
		// 保存当前消息作为上一条消息
		str_copy(m_aPreviousMessage, pMessage, sizeof(m_aPreviousMessage));
		m_HasPreviousMessage = true;
		// dbg_msg("mclient", "Saved previous message: %s", m_aPreviousMessage);
		return;
	}

	// 检查当前消息是否与上一条消息相同
	if(str_comp(pMessage, m_aPreviousMessage) == 0)
	{
		// 检查是否不是自己发的
		if(ClientId != GameClient()->m_Snap.m_LocalClientId)
		{
			// 发送消息完成加一
			GameClient()->m_Chat.SendChat(0, pMessage);

			// 更新状态
			str_copy(m_aLastRepeatedMessage, pMessage, sizeof(m_aLastRepeatedMessage));
			m_LastPlusOneTime = Client()->LocalTime();

			// dbg_msg("mclient", "Auto plus one: %s", pMessage);
		}
	}
	else
	{
		// 更新上一条消息
		str_copy(m_aPreviousMessage, pMessage, sizeof(m_aPreviousMessage));
		// dbg_msg("mclient", "Updated previous message: %s", m_aPreviousMessage);
	}
}

void CMClient::CheckAutoPlusOne()
{
	// 这个方法保留用于可能的扩展
	// 目前自动加一逻辑在ProcessAutoPlusOne中处理
}

// ================== 钩子角度辅助功能 ==================

bool CMClient::CheckHookCollision(const vec2& Start, const vec2& End, vec2& OutCollision, vec2& OutBeforeCollision) const
{
	// 使用Collision系统的IntersectLineTeleHook方法检测碰撞
	// 这个方法考虑了钩子的特殊行为（如传送、nohook等）
	return Collision()->IntersectLineTeleHook(Start, End, &OutCollision, &OutBeforeCollision) != 0;
}

bool CMClient::IsHookPassable(const vec2& Start, const vec2& End) const
{
	vec2 CollisionPos, BeforeCollisionPos;
	
	// 使用更精确的碰撞检测
	// 先使用IntersectLineTeleHook检测碰撞
	int CollisionResult = Collision()->IntersectLineTeleHook(Start, End, &CollisionPos, &BeforeCollisionPos);

	if(CollisionResult == 0)
	{
		// 没有碰撞，可以通过
		return true;
	}

	// 检查碰撞点是否在终点附近（表示可以穿过缝隙）
	float DistanceToEnd = distance(CollisionPos, End);

	// 如果碰撞点距离终点非常近（小于1个像素），认为可以通过
	if(DistanceToEnd < 1.0f)
	{
		return true;
	}
	
	// 检查碰撞类型
	int TileIndex = Collision()->GetTile(round_to_int(CollisionPos.x), round_to_int(CollisionPos.y));
	
	// 某些特殊瓦片允许钩子通过
	// 例如：传送门、空气等
	if(Collision()->IsTeleport(TileIndex))
		return true;
	
	// 其他碰撞类型（墙壁、nohook等）不允许通过
	return false;
}

bool CMClient::SimulateHookFlight(const vec2& Start, float Angle, vec2& OutHitPos, float& OutDistance, bool& OutHitTeleport) const
{
	// 参考DDNet官方实现，模拟钩子的实际飞行过程
	
	// 获取钩子参数
	float HookFireSpeed = GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_Predicted.m_Tuning.m_HookFireSpeed;
	float HookLength = GameClient()->m_aClients[GameClient()->m_aLocalIds[g_Config.m_ClDummy]].m_Predicted.m_Tuning.m_HookLength;
	
	if(HookFireSpeed <= 0.0f || HookLength <= 0.0f)
	{
		OutHitPos = Start;
		OutDistance = 0.0f;
		OutHitTeleport = false;
		return false;
	}
	
	// 钩子起始距离（与官方实现一致）
	static constexpr float HOOK_START_DISTANCE = CCharacterCore::PhysicalSize() * 1.5f;
	
	// 计算方向并量化（与官方实现一致）
	vec2 Direction = direction(Angle);
	vec2 QuantizedDirection = Direction;
	QuantizedDirection.x = round_to_int(QuantizedDirection.x * 256.0f) / 256.0f;
	QuantizedDirection.y = round_to_int(QuantizedDirection.y * 256.0f) / 256.0f;
	
	// 计算起始位置
	vec2 BasePos = Start;
	vec2 StartOffset = QuantizedDirection * HOOK_START_DISTANCE;
	vec2 SegmentStartPos = BasePos + StartOffset;
	SegmentStartPos.x = round_to_int(SegmentStartPos.x);
	SegmentStartPos.y = round_to_int(SegmentStartPos.y);
	
	// 最大tick数（与官方实现一致）
	const int MaxHookTicks = 5 * Client()->GameTickSpeed();
	
	// 模拟钩子飞行
	OutHitTeleport = false;
	
	for(int HookTick = 0; HookTick < MaxHookTicks; ++HookTick)
	{
		int Tele;
		vec2 HitPos;
		vec2 SegmentEndPos = SegmentStartPos + QuantizedDirection * HookFireSpeed;
		
		// 检查是否超过最大长度
		if(distance(BasePos, SegmentEndPos) > HookLength)
		{
			// 钩子到达最大长度，没有碰撞
			OutHitPos = BasePos + normalize(SegmentEndPos - BasePos) * HookLength;
			OutDistance = distance(Start, OutHitPos);
			return false;
		}
		
		// 检查地图碰撞
		int Hit = Collision()->IntersectLineTeleHook(SegmentStartPos, SegmentEndPos, &HitPos, nullptr, &Tele);
		
		// 没有碰撞，继续
		if(!Hit)
		{
			SegmentStartPos = SegmentEndPos;
			SegmentStartPos.x = round_to_int(SegmentStartPos.x);
			SegmentStartPos.y = round_to_int(SegmentStartPos.y);
			continue;
		}
		
		// 碰撞到钩子传送门
		if(Hit == TILE_TELEINHOOK)
		{
			OutHitTeleport = true;
			
			// 检查传送出口
			const std::vector<vec2>& vTeleOuts = Collision()->TeleOuts(Tele - 1);
			if(vTeleOuts.empty())
			{
				// 钩子卡住
				OutHitPos = HitPos;
				OutDistance = distance(Start, HitPos);
				return true;
			}
			else if(vTeleOuts.size() > 1)
			{
				// 多个出口，不确定
				OutHitPos = HitPos;
				OutDistance = distance(Start, HitPos);
				return true;
			}
			
			// 通过传送门，更新位置继续
			BasePos = vTeleOuts[0];
			SegmentStartPos = BasePos + QuantizedDirection * HOOK_START_DISTANCE;
			SegmentStartPos.x = round_to_int(SegmentStartPos.x);
			SegmentStartPos.y = round_to_int(SegmentStartPos.y);
			continue;
		}
		
		// 碰撞到其他物体（墙壁、nohook等）
		OutHitPos = HitPos;
		OutDistance = distance(Start, HitPos);
		return true;
	}
	
	// 超过最大tick数，没有碰撞
	OutHitPos = BasePos + QuantizedDirection * HookLength;
	OutDistance = distance(Start, OutHitPos);
	return false;
}
void CMClient::ConToggleHookAngleHelperCallback(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	pThis->m_HookAngleHelperEnabled = !pThis->m_HookAngleHelperEnabled;
	g_Config.m_McHookAngleHelper = pThis->m_HookAngleHelperEnabled;
	dbg_msg("mclient", "Hook angle helper %s", pThis->m_HookAngleHelperEnabled ? "enabled" : "disabled");
}

void CMClient::ConHookAngleApplyCallback(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	pThis->ApplyBestAngle();
}

void CMClient::ConHookAngleResetCallback(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	pThis->m_HasBestAngle = false;
	pThis->m_BestHookAngle = 0.0f;
	pThis->m_vAngleResults.clear();
	dbg_msg("mclient", "Hook angle helper reset");
}


void CMClient::UpdateHookAngleHelper()
{
	// 获取本地玩家
	const CNetObj_PlayerInfo *pLocalInfo = GameClient()->m_Snap.m_pLocalInfo;
	if(!pLocalInfo)
	{
		// dbg_msg("mclient", "Hook angle helper: No local player info");
		return;
	}

	// 获取本地玩家位置
	vec2 LocalPos = GameClient()->m_Snap.m_pLocalCharacter ?
				vec2(GameClient()->m_Snap.m_pLocalCharacter->m_X, GameClient()->m_Snap.m_pLocalCharacter->m_Y) :
				vec2(0, 0);
	if(LocalPos.x == 0 && LocalPos.y == 0)
	{
		// dbg_msg("mclient", "Hook angle helper: Invalid local pos");
		return;
	}

	// 获取当前鼠标角度
	vec2 MousePos = GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy];

	// 考虑鼠标距离缩放
	if(g_Config.m_TcScaleMouseDistance && !GameClient()->m_Snap.m_SpecInfo.m_Active)
	{
		const int MaxDistance = g_Config.m_ClDyncam ? g_Config.m_ClDyncamMaxDistance : g_Config.m_ClMouseMaxDistance;
		if(MaxDistance > 5 && MaxDistance < 1000) // Don't scale if angle bind or reduces precision
			MousePos *= 1000.0f / (float)MaxDistance;
	}

	float CurrentAngle = angle(MousePos);

	// 将角度转换为度数
	float CurrentAngleDeg = CurrentAngle * 180.0f / pi;

	// 扫描范围和步进
	float ScanRange = g_Config.m_McHookAngleScanRange;
	float ScanStep = g_Config.m_McHookAngleScanStep / 4.0;

	// 从当前角度开始扫描
	float StartAngle = CurrentAngleDeg - ScanRange / 2.0f;
	float EndAngle = CurrentAngleDeg + ScanRange / 2.0f;

	// 扫描寻找最佳角度
	m_HasBestAngle = false;
	m_BestHookAngle = 0.0f;

	// 清空之前的扫描结果
	m_vAngleResults.clear();

	// 存储可通过的角度范围
	struct SAngleRange
	{
		float StartAngle;
		float EndAngle;
	};
	std::vector<SAngleRange> vPassableRanges;

	for(float Angle = StartAngle; Angle <= EndAngle; Angle += ScanStep)
	{
		// 将角度转换为弧度
		float AngleRad = Angle * pi / 180.0f;

		// 使用新的模拟函数检测钩子碰撞
		vec2 HitPos;
		float Distance;
		bool HitTeleport;
		bool bHit = SimulateHookFlight(LocalPos, AngleRad, HitPos, Distance, HitTeleport);

		// 记录扫描结果
		SAngleResult Result;
		Result.m_Angle = AngleRad;
		Result.m_Distance = Distance;
		Result.m_Passable = !bHit;
		Result.m_CollisionPos = HitPos;
		m_vAngleResults.push_back(Result);
	}

	// 分析扫描结果，找出可通过的角度范围
	if(!m_vAngleResults.empty())
	{
		// 找出所有可通过的角度范围
		bool bInPassableRange = m_vAngleResults[0].m_Passable;
		float RangeStart = m_vAngleResults[0].m_Angle;

		for(const auto &Result : m_vAngleResults)
		{
			if(Result.m_Passable && !bInPassableRange)
			{
				// 进入可通过区域
				bInPassableRange = true;
				RangeStart = Result.m_Angle;
			}
			else if(!Result.m_Passable && bInPassableRange)
			{
				// 离开可通过区域，记录范围
				bInPassableRange = false;
				SAngleRange Range;
				Range.StartAngle = RangeStart;
				Range.EndAngle = Result.m_Angle;
				vPassableRanges.push_back(Range);
			}
		}

		// 如果最后还在可通过区域，记录这个范围
		if(bInPassableRange)
		{
			SAngleRange Range;
			Range.StartAngle = RangeStart;
			Range.EndAngle = m_vAngleResults.back().m_Angle;
			vPassableRanges.push_back(Range);
		}

		// 找到最佳角度（最接近鼠标角度的可通过范围的中心）
		float BestDiff = 999999.0f;
		for(const auto &Range : vPassableRanges)
		{
			// 计算范围的中心角度
			float CenterAngle = (Range.StartAngle + Range.EndAngle) / 2.0f;

			// 计算与鼠标角度的差异
			float Diff = fabs(CenterAngle - CurrentAngle);

			// 处理角度环绕
			if(Diff > pi)
			{
				Diff = 2.0f * pi - Diff;
			}

			// 找到最接近鼠标角度的中心角度
			if(Diff < BestDiff)
			{
				BestDiff = Diff;
				m_BestHookAngle = CenterAngle;
				m_HasBestAngle = true;
			}
		}

		// 如果没有找到可通过角度，检查是否有碰撞点
		if(!m_HasBestAngle)
		{
			// 找出所有有碰撞的角度
			for(const auto &Result : m_vAngleResults)
			{
				if(!Result.m_Passable && Result.m_Distance > 50.0f)
				{
					// 有碰撞，使用这个角度
					float Diff = fabs(Result.m_Angle - CurrentAngle);

					// 处理角度环绕
					if(Diff > pi)
					{
						Diff = 2.0f * pi - Diff;
					}

					// 找到最接近鼠标角度的碰撞角度
					if(Diff < BestDiff)
					{
						BestDiff = Diff;
						m_BestHookAngle = Result.m_Angle;
						m_HasBestAngle = true;
					}
				}
			}
		}
	}
	// 自动应用最佳角度
	if(m_HasBestAngle && g_Config.m_McHookAngleAutoApply)
	{
		ApplyBestAngle();
	}

}

void CMClient::RenderHookAngleHelper()
{
	if(!m_HasBestAngle)
	{
		// dbg_msg("mclient", "Hook angle helper: No best angle to render");
		return;
	}

	// 获取本地玩家位置
	const CNetObj_PlayerInfo *pLocalInfo = GameClient()->m_Snap.m_pLocalInfo;
	if(!pLocalInfo)
	{
		// dbg_msg("mclient", "Hook angle helper: No local player info for render");
		return;
	}

	vec2 LocalPos = GameClient()->m_Snap.m_pLocalCharacter ?
				vec2(GameClient()->m_Snap.m_pLocalCharacter->m_X, GameClient()->m_Snap.m_pLocalCharacter->m_Y) :
				vec2(0, 0);
	if(LocalPos.x == 0 && LocalPos.y == 0)
	{
		// dbg_msg("mclient", "Hook angle helper: Invalid local pos for render");
		return;
	}

	// 绘制扫描范围（扇形）
	if(g_Config.m_McHookAngleShowScanRange && !m_vAngleResults.empty())
	{
		
		Graphics()->TextureClear();
		Graphics()->LinesBegin();
		Graphics()->SetColor(0.3f, 0.3f, 0.3f, 0.3f);

		// 绘制扇形边界
		vec2 StartDir = vec2(cos(m_vAngleResults.front().m_Angle), sin(m_vAngleResults.front().m_Angle));
		vec2 EndDir = vec2(cos(m_vAngleResults.back().m_Angle), sin(m_vAngleResults.back().m_Angle));

		IEngineGraphics::CLineItem BoundaryLines[] = {
			{LocalPos.x, LocalPos.y, LocalPos.x + StartDir.x * 800.0f, LocalPos.y + StartDir.y * 800.0f},
			{LocalPos.x, LocalPos.y, LocalPos.x + EndDir.x * 800.0f, LocalPos.y + EndDir.y * 800.0f}};
		Graphics()->LinesDraw(BoundaryLines, 2);
		Graphics()->LinesEnd();
	}

	// 绘制所有测试角度
	if(g_Config.m_McHookAngleShowAllAngles)
	{
		for(const auto &Result : m_vAngleResults)
		{
			vec2 Direction = vec2(cos(Result.m_Angle), sin(Result.m_Angle));
			vec2 EndPos = Result.m_CollisionPos;

			// 根据是否可通设置颜色
			if(Result.m_Passable)
			{
				// 可通：绿色半透明
				Graphics()->TextureClear();
				Graphics()->LinesBegin();
				Graphics()->SetColor(0.0f, 1.0f, 0.0f, 0.2f);
				IEngineGraphics::CLineItem Line(LocalPos.x, LocalPos.y, EndPos.x, EndPos.y);
				Graphics()->LinesDraw(&Line, 1);
				Graphics()->LinesEnd();
			}
			else
			{
				// 不可通：红色半透明
				Graphics()->TextureClear();
				Graphics()->LinesBegin();
				Graphics()->SetColor(1.0f, 0.0f, 0.0f, 0.2f);
				IEngineGraphics::CLineItem Line(LocalPos.x, LocalPos.y, EndPos.x, EndPos.y);
				Graphics()->LinesDraw(&Line, 1);
				Graphics()->LinesEnd();

				// 绘制碰撞点
				if(g_Config.m_McHookAngleShowCollision && !Result.m_Passable)
				{
					Graphics()->TextureClear();
					Graphics()->QuadsBegin();
					Graphics()->SetColor(1.0f, 0.5f, 0.0f, 0.8f);
					IGraphics::CQuadItem CollisionQuad(EndPos.x - 2.0f, EndPos.y - 2.0f, 4.0f, 4.0f);
					Graphics()->QuadsDraw(&CollisionQuad, 1);
					Graphics()->QuadsEnd();
				}
			}
		}

		// 计算最佳角度的终点
		vec2 BestDirection = vec2(cos(m_BestHookAngle), sin(m_BestHookAngle));

		
		// 检测碰撞点
		vec2 CollisionPos, BeforeCollisionPos;
		vec2 BestEnd;
		if(CheckHookCollision(LocalPos, LocalPos + BestDirection * 800.0f, CollisionPos, BeforeCollisionPos))
		{
			// 有碰撞，显示到碰撞点的线
			BestEnd = CollisionPos;
		}
		else
		{
			// 无碰撞，显示最大长度
			BestEnd = LocalPos + BestDirection * 800.0f;
		}

		// 绘制最佳角度线（绿色，锁定时为黄色）
		Graphics()->TextureClear();
		Graphics()->LinesBegin();

		Graphics()->SetColor(0.0f, 1.0f, 0.0f, 1.0f); // 绿色表示未锁定

		IEngineGraphics::CLineItem Line(LocalPos.x, LocalPos.y, BestEnd.x, BestEnd.y);
		Graphics()->LinesDraw(&Line, 1);
		Graphics()->LinesEnd();

		// 绘制起点和终点
		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
		IGraphics::CQuadItem StartQuad(LocalPos.x - 3.0f, LocalPos.y - 3.0f, 6.0f, 6.0f);
		Graphics()->QuadsDraw(&StartQuad, 1);
		Graphics()->QuadsEnd();

		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
		IGraphics::CQuadItem EndQuad(BestEnd.x - 3.0f, BestEnd.y - 3.0f, 6.0f, 6.0f);
		Graphics()->QuadsDraw(&EndQuad, 1);
		Graphics()->QuadsEnd();
	}
}

void CMClient::ApplyBestAngle()
{
	if(!m_HasBestAngle)
	{
		dbg_msg("mclient", "Cannot apply angle: No best angle found");
		return;
	}
	
	// 计算鼠标位置
	float Angle = m_BestHookAngle;
	vec2 Direction = vec2(cos(Angle), sin(Angle));
	
	// 设置鼠标位置（使用最大距离）
	float Distance = 800.0f;
	vec2 MousePos = Direction * Distance;
	
	// 应用到输入
	GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy] = MousePos;
	
	dbg_msg("mclient", "Applied angle %.2f deg", Angle * 180.0f / pi);
}

void CMClient::UpdateLastHookedBy()
{
	// 确保在游戏状态
	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalId < 0 || LocalId >= MAX_CLIENTS)
		return;

	// 遍历所有客户端，寻找当前正在勾本地玩家的玩家
	int64_t CurrentTime = time_get();
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// 跳过本地玩家和无效客户端
		if(i == LocalId)
			continue;

		// 检查客户端是否活跃
		if(!GameClient()->m_Snap.m_aCharacters[i].m_Active)
			continue;

		// 检查这个玩家是否正在勾本地玩家（使用m_Cur，即当前快照状态）
		int HookedPlayer = GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_HookedPlayer;
		if(HookedPlayer == LocalId)
		{
			// 更新最后勾我的玩家信息
			m_LastHookedByClientId = i;
			m_LastHookedByTime = CurrentTime;

			// 获取玩家名称
			const char *pName = GameClient()->m_aClients[i].m_aName;
			if(pName[0])
			{
				str_copy(m_aLastHookedByName, pName, sizeof(m_aLastHookedByName));
			}
			return;
		}
	}
}