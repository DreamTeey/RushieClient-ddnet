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

	float CurrentTime = Client()->LocalTime();

	// 1. 定时触发刷新
	if(g_Config.m_McFriendNotifyAutoRefresh)
	{
		if(CurrentTime - m_LastFriendRefreshTime >= g_Config.m_McFriendNotifyRefreshInterval)
		{
			GameClient()->ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			m_LastFriendRefreshTime = CurrentTime;
		}
	}

	// 2. 标记阶段：先假设所有记录的好友都已离线
	for(int i = 0; i < m_NumFriendStates; i++)
		m_aFriendStates[i].m_IsStillOnline = false;

	// 3. 扫描阶段：遍历所有服务器查找好友
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

			// 在现有列表中查找该好友
			int FoundIndex = -1;
			for(int k = 0; k < m_NumFriendStates; k++)
			{
				if(str_comp(m_aFriendStates[k].m_aName, pClient->m_aName) == 0 &&
				   str_comp(m_aFriendStates[k].m_aClan, pClient->m_aClan) == 0)
				{
					FoundIndex = k;
					break;
				}
			}

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
	}

	// 4. 清理与下线提醒阶段
	for(int i = 0; i < m_NumFriendStates; i++)
	{
		if(!m_aFriendStates[i].m_IsStillOnline)
		{
			// 触发下线提醒
			OnFriendLeave(m_aFriendStates[i].m_aName, m_aFriendStates[i].m_aClan);

			// 从数组中移除（用末尾元素覆盖当前位置）
			if(m_NumFriendStates > 1)
				m_aFriendStates[i] = m_aFriendStates[m_NumFriendStates - 1];
			
			m_NumFriendStates--;
			i--; // 重新检查当前索引位置
		}
	}
}
void CMClient::OnFriendJoin(const CServerInfo *pServerInfo, const CServerInfo::CClient *pFriendClient)
{
	if(!pServerInfo || !pFriendClient)
		return;

	// 获取好友名称
	const char *pFriendName = pFriendClient->m_aName;

	// 发送通知到聊天框
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "+++ 好友 [%s] 已上线 | 位置: %s", pFriendName, pServerInfo->m_aName);
	GameClient()->m_Chat.Echo(aBuf);

	// 如果启用了自动打招呼功能
	if(g_Config.m_McFriendAutoGreet)
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
}

void CMClient::OnFriendLeave(const char *pName, const char *pClan)
{
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "--- 好友 [%s] 已下线或离开服务器", pName);
	GameClient()->m_Chat.Echo(aBuf);
}
