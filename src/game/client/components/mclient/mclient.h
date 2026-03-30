#ifndef GAME_CLIENT_COMPONENTS_MCLIENT_MCLIENT_H
#define GAME_CLIENT_COMPONENTS_MCLIENT_MCLIENT_H

#include <game/client/component.h>
#include <engine/console.h>
#include <engine/friends.h>
#include <engine/serverbrowser.h>
#include <engine/client/enums.h>
#include <engine/shared/protocol.h>

class CMClient : public CComponent
{
private:
	// 常量定义
	static constexpr int MAX_CLONE_DISTANCE = 50;
	static constexpr int HAMMER_RADIUS = 28;
	static constexpr float DEFAULT_HUE_SPEED = 0.01f;

public:
	CMClient();

	// DDNet 组件标准方法
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnConsoleInit() override;
	virtual void OnRender() override;
	virtual void OnMessage(int MsgType, void *pRawMsg) override;
	virtual void OnStateChange(int NewState, int OldState) override;
	
	// 最后勾我的玩家相关（公开给其他组件访问）
	int m_LastHookedByClientId; // 最后勾我的玩家的客户端ID
	int64_t m_LastHookedByTime; // 最后被勾的时间戳
	char m_aLastHookedByName[MAX_NAME_LENGTH]; // 最后勾我的玩家的名称
	
	// 更新最后勾我的玩家
	void UpdateLastHookedBy();

	// 获取最后勾我的玩家ID
	int GetLastHookedByClientId() const { return m_LastHookedByClientId; }

	// 获取最后勾我的玩家名称
	const char *GetLastHookedByName() const { return m_aLastHookedByName; }

	// 克隆人皮肤复制方法
	void CheckCloneSkin();
	void UpdateRainbow();

	// 好友上线提醒方法
	void CheckFriendNotification();
	void UpdateFriendList();
	void OnFriendJoin(const CServerInfo *pServerInfo, const CServerInfo::CClient *pFriendClient);	
	void OnFriendLeave(const char *pName, const char *pClan);

	// 武器切换方法
	void SwitchToLastWeapon();
	void UpdateWeaponHistory(int CurrentWeapon);
	bool HasWeapon(int Weapon) const;
	static void ConSwitchLastWeaponCallback(IConsole::IResult *pResult, void *pUserData);

	// 复读功能方法
	void RepeatLastMessage();
	static void ConRepeatLastMessageCallback(IConsole::IResult *pResult, void *pUserData);

	// 自动加一方法
	void CheckAutoPlusOne();
	void ProcessAutoPlusOne(const char *pMessage, int ClientId);

	// 钩子角度辅助方法
	void UpdateHookAngleHelper();
	void RenderHookAngleHelper();
	void ApplyBestAngle(); // 应用最佳角度
	static void ConToggleHookAngleHelperCallback(IConsole::IResult *pResult, void *pUserData);
	static void ConHookAngleApplyCallback(IConsole::IResult *pResult, void *pUserData);
	static void ConHookAngleResetCallback(IConsole::IResult *pResult, void *pUserData);

	// 钩子角度辅助碰撞检测
	bool CheckHookCollision(const vec2& Start, const vec2& End, vec2& OutCollision, vec2& OutBeforeCollision) const;
	bool IsHookPassable(const vec2& Start, const vec2& End) const;
	bool SimulateHookFlight(const vec2& Start, float Angle, vec2& OutHitPos, float& OutDistance, bool& OutHitTeleport) const;

	// 颜色转换函数
	static int getIntFromColor(float Hue, float Sat, float LhT)
	{
		int R = (int)(round(255 * Hue));
		int G = (int)(round(255 * Sat));
		int B = (int)(round(255 * LhT));
		R = (R << 16) & 0x00FF0000;
		G = (G << 8) & 0x0000FF00;
		B = B & 0x000000FF;
		return 0xFF000000 | R | G | B;
	}

private:
	// Helper方法
	bool CanCloneSkin() const;
	bool ShouldRotateSkin() const;
	void HandleSkinRotation();
	void CopyPlayerSkin(int TargetId, bool IsDummy);
	void SendSkinUpdate(bool IsDummy);
	bool CheckHammerClone(const vec2& LocalPos, int LocalId, int& TargetId);
	bool CheckDistanceClone(const vec2& LocalPos, int& TargetId);
	
	// 反向锤击检测：检测是否有其他玩家正在用锤子攻击本地角色
	bool CheckBeingHammered(const vec2& LocalPos, int LocalId, int& AttackerId);

	// 被锤击时随机表情功能
	void CheckRandomEmoteOnHammer();
	void SendRandomEmote(int DummyIndex);

	// 好友功能辅助方法
	void ScanServersForFriends();
	void HandleFriendClient(const CServerInfo *pServerInfo, const CServerInfo::CClient *pClient);
	int FindFriendInList(const CServerInfo::CClient *pClient) const;
	void AddFriendToList(const CServerInfo::CClient *pClient);
	void ProcessOfflineFriends();
	void RemoveFriendFromList(int Index);
	bool IsFriendInCurrentServer(const CServerInfo *pFriendServerInfo) const;
	void SendGreetingMessage(const char *pFriendName);
	
	// 逻辑计时器
	float m_LastRotateTime;

	// 克隆人皮肤复制计时器
	int m_LastCloneTick;

	// 最后复制的玩家ID
	int m_LastClonedClientId;
	int64_t m_RainbowDelay;

	// 彩虹tee颜色状态
	float m_BodyHue;
	float m_FeetHue;

	// 好友上线提醒相关
	float m_LastFriendRefreshTime;
	struct SFriendState
	{
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		bool m_IsStillOnline;
	};
	static SFriendState m_aFriendStates[IFriends::MAX_FRIENDS];
	int m_NumFriendStates;

	// 好友进入地图打招呼相关
	bool m_aPrevPlayerActive[MAX_CLIENTS];  // 上一帧玩家是否在当前地图
	void CheckFriendJoinMap();  // 检测好友进入地图

	// 武器切换相关
	int m_aLastWeapon[2]; // 记录最近使用的两个武器
	int m_LastWeaponIndex;

	// 复读功能相关
	char m_aLastMessage[512]; // 记录最后一条消息
	bool m_HasLastMessage; // 是否有记录的消息

	// 自动加一相关
	char m_aPreviousMessage[512]; // 记录上一条公屏消息
	char m_aLastRepeatedMessage[512]; // 记录最后一条复读的消息
	bool m_HasPreviousMessage; // 是否有上一条消息
	float m_LastPlusOneTime; // 最后一次加一的时间

	// 被锤击时随机表情相关
	int m_aaLastRandomEmoteAttackTick[NUM_DUMMIES][MAX_CLIENTS]; // 记录每个角色对每个攻击者的最后攻击帧数

	// 钩子角度辅助相关
	bool m_HookAngleHelperEnabled; // 是否启用钩子角度辅助
	float m_BestHookAngle; // 最佳钩子角度
	bool m_HasBestAngle; // 是否找到最佳角度
	
	// 可视化增强
	struct SAngleResult
	{
		float m_Angle;
		float m_Distance;
		bool m_Passable;
		vec2 m_CollisionPos;
	};
	std::vector<SAngleResult> m_vAngleResults;
};

#endif
