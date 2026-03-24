#include "mclient.h"

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <base/system.h>
#include <game/client/gameclient.h>

void CMClient::UpdateFriendList()
{
	m_NumFriendStates = 0;
	mem_zero(m_aFriendStates, sizeof(m_aFriendStates));
	m_LastFriendRefreshTime = 0;
}

void CMClient::CheckFriendNotification()
{
	if(!g_Config.m_McFriendNotify)
		return;

	IServerBrowser *pServerBrowser = GameClient()->ServerBrowser();
	if(!pServerBrowser)
		return;

	float CurrentTime = Client()->LocalTime();

	// 1. 定时触发刷新
	if(g_Config.m_McFriendNotifyAutoRefresh)
	{
		if(CurrentTime - m_LastFriendRefreshTime >= g_Config.m_McFriendNotifyRefreshInterval)
		{
			pServerBrowser->Refresh(IServerBrowser::TYPE_INTERNET);
			m_LastFriendRefreshTime = CurrentTime;
		}
	}

	// 2. 关键检查：如果服务器列表正在刷新中，跳过本次状态检查
	//    避免在数据未完全加载时误判好友下线
	if(pServerBrowser->IsRefreshing() || pServerBrowser->IsGettingServerlist())
	{
		// dbg_msg("mclient-friend", "Skipping friend state check - server list is refreshing");
		return;
	}

	// 3. 标记阶段：先假设所有记录的好友都已离线
	for(int i = 0; i < m_NumFriendStates; i++)
		m_aFriendStates[i].m_IsStillOnline = false;

	// 4. 扫描阶段：遍历所有服务器查找好友
	ScanServersForFriends();

	// 5. 清理与下线提醒阶段
	ProcessOfflineFriends();
}

void CMClient::ScanServersForFriends()
{
	int NumServers = GameClient()->ServerBrowser()->NumServers();
	for(int i = 0; i < NumServers; i++)
	{
		const CServerInfo *pServerInfo = GameClient()->ServerBrowser()->Get(i);
		if(!pServerInfo || pServerInfo->m_FriendState == IFriends::FRIEND_NO)
			continue;

		for(int j = 0; j < pServerInfo->m_NumClients; j++)
		{
			const CServerInfo::CClient *pClient = &pServerInfo->m_aClients[j];
			if(!pClient || pClient->m_FriendState == IFriends::FRIEND_NO)
				continue;

			HandleFriendClient(pServerInfo, pClient);
		}
	}
}

void CMClient::HandleFriendClient(const CServerInfo *pServerInfo, const CServerInfo::CClient *pClient)
{
	// 在现有列表中查找该好友
	int FoundIndex = FindFriendInList(pClient);
	
	if(FoundIndex >= 0)
	{
		// 已在列表中，更新状态为在线
		m_aFriendStates[FoundIndex].m_IsStillOnline = true;
	}
	else if(m_NumFriendStates < IFriends::MAX_FRIENDS)
	{
		// 发现新上线的好友！
		OnFriendJoin(pServerInfo, pClient);
		
		int NewIdx = m_NumFriendStates++;
		str_copy(m_aFriendStates[NewIdx].m_aName, pClient->m_aName, sizeof(m_aFriendStates[NewIdx].m_aName));
		str_copy(m_aFriendStates[NewIdx].m_aClan, pClient->m_aClan, sizeof(m_aFriendStates[NewIdx].m_aClan));
		m_aFriendStates[NewIdx].m_IsStillOnline = true;
	}
}

int CMClient::FindFriendInList(const CServerInfo::CClient *pClient) const
{
	for(int k = 0; k < m_NumFriendStates; k++)
	{
		if(str_comp(m_aFriendStates[k].m_aName, pClient->m_aName) == 0 &&
		   str_comp(m_aFriendStates[k].m_aClan, pClient->m_aClan) == 0)
		{
			return k;
		}
	}
	return -1;
}

void CMClient::AddFriendToList(const CServerInfo::CClient *pClient)
{
	SFriendState& newState = m_aFriendStates[m_NumFriendStates++];
	str_copy(newState.m_aName, pClient->m_aName, sizeof(newState.m_aName));
	str_copy(newState.m_aClan, pClient->m_aClan, sizeof(newState.m_aClan));
	newState.m_IsStillOnline = true;
}

void CMClient::ProcessOfflineFriends()
{
	for(int i = 0; i < m_NumFriendStates; i++)
	{
		if(!m_aFriendStates[i].m_IsStillOnline)
		{
			// 触发下线提醒（如果启用）
			if(g_Config.m_McFriendNotifyOffline)
				OnFriendLeave(m_aFriendStates[i].m_aName, m_aFriendStates[i].m_aClan);

			// 从数组中移除（用末尾元素覆盖当前位置）
			RemoveFriendFromList(i);
			i--; // 重新检查当前索引位置
		}
	}
}

void CMClient::RemoveFriendFromList(int Index)
{
	if(m_NumFriendStates > 1)
		m_aFriendStates[Index] = m_aFriendStates[m_NumFriendStates - 1];
	
	m_NumFriendStates--;
}

void CMClient::OnFriendJoin(const CServerInfo *pServerInfo, const CServerInfo::CClient *pFriendClient)
{
	if(!pServerInfo || !pFriendClient)
		return;

	// 获取好友名称
	const char *pFriendName = pFriendClient->m_aName;

	// 发送通知到聊天框
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "[%s] 已上线", pFriendName);
	GameClient()->m_Chat.Echo(aBuf);

	// 注意：好友进入地图打招呼逻辑已移到 CheckFriendJoinMap() 中
	// 此处仅保留上线通知
}

bool CMClient::IsFriendInCurrentServer(const CServerInfo *pFriendServerInfo) const
{
	// 获取当前服务器信息
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	// 检查是否是当前服务器（比较地址）
	for(int i = 0; i < pFriendServerInfo->m_NumAddresses && i < CurrentServerInfo.m_NumAddresses; i++)
	{
		if(net_addr_comp(&pFriendServerInfo->m_aAddresses[i], &CurrentServerInfo.m_aAddresses[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

void CMClient::SendGreetingMessage(const char *pFriendName)
{
	// 准备打招呼文本
	char aGreetText[256];
	str_copy(aGreetText, g_Config.m_McFriendAutoGreetText, sizeof(aGreetText));

	// 替换 {name} 为好友名字
	const char *pPos = str_find(aGreetText, "{name}");
	if(pPos)
	{
		char aNewGreetText[256];
		int PrefixLen = pPos - aGreetText;
		str_copy(aNewGreetText, aGreetText, PrefixLen + 1);
		str_append(aNewGreetText, pFriendName, sizeof(aNewGreetText));
		str_append(aNewGreetText, pPos + str_length("{name}"), sizeof(aNewGreetText));
		str_copy(aGreetText, aNewGreetText, sizeof(aGreetText));
	}

	// 发送打招呼消息
	GameClient()->m_Chat.SendChat(0, aGreetText);
}

void CMClient::OnFriendLeave(const char *pName, const char *pClan)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "[%s] 已下线或离开服务器", pName);
	GameClient()->m_Chat.Echo(aBuf);
}
