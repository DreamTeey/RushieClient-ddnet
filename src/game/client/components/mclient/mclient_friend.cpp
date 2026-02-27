#include "mclient.h"

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <base/system.h>
#include <game/client/gameclient.h>

void CMClient::UpdateFriendList()
{
    m_NumFriendStates = 0;
    m_LastFriendRefreshTime = 0;
}

void CMClient::CheckFriendNotification()
{
    if(!g_Config.m_McFriendNotify)
        return;

    float CurrentTime = Client()->LocalTime();

    // 1. 定时触发服务器浏览器刷新
    if(g_Config.m_McFriendNotifyAutoRefresh)
    {
        if(CurrentTime - m_LastFriendRefreshTime >= g_Config.m_McFriendNotifyRefreshInterval)
        {
            GameClient()->ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
            m_LastFriendRefreshTime = CurrentTime;
            // 注意：这里不再手动重置 m_aFriendStates，交给下方的扫描逻辑处理
        }
    }

    // 2. 标记阶段：先假设所有记录的好友都下线了
    for(int i = 0; i < m_NumFriendStates; i++)
        m_aFriendStates[i].m_IsStillOnline = false;

    // 3. 扫描阶段：遍历当前服务器浏览器缓存的所有数据
    int NumServers = GameClient()->ServerBrowser()->NumServers();
    int NumFriendsFound = 0;

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

            NumFriendsFound++;

            // 检查该好友是否已在我们的记录列表中
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
                // 已存在，更新其在线状态标记
                m_aFriendStates[FoundIndex].m_IsStillOnline = true;
            }
            else if(m_NumFriendStates < IFriends::MAX_FRIENDS)
            {
                // 发现新上线（或新加入列表）的好友
                OnFriendJoin(pServerInfo, pClient);

                int NewIdx = m_NumFriendStates++;
                str_copy(m_aFriendStates[NewIdx].m_aName, pClient->m_aName, sizeof(m_aFriendStates[NewIdx].m_aName));
                str_copy(m_aFriendStates[NewIdx].m_aClan, pClient->m_aClan, sizeof(m_aFriendStates[NewIdx].m_aClan));
                m_aFriendStates[NewIdx].m_IsStillOnline = true;
            }
        }
    }

    // 4. 清理阶段：移除那些在本次扫描中未被发现的好友（即已离线或离开服务器）
    for(int i = 0; i < m_NumFriendStates; i++)
    {
        if(!m_aFriendStates[i].m_IsStillOnline)
        {
            // 如果你想加“好友下线通知”，可以在这里调用
            // 移除逻辑：用最后一个元素覆盖当前元素，然后减小计数
            m_aFriendStates[i] = m_aFriendStates[m_NumFriendStates - 1];
            m_NumFriendStates--;
            i--; // 抵消自增，重新检查当前位置
        }
    }

    // 调试输出
    static float s_LastDebugTime = 0;
    if(CurrentTime - s_LastDebugTime >= 10.0f)
    {
        s_LastDebugTime = CurrentTime;
        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "[FriendNotify] 监控中: 在线好友 %d, 记录数 %d", NumFriendsFound, m_NumFriendStates);
        // GameClient()->m_Chat.Echo(aBuf);
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
	str_format(aBuf, sizeof(aBuf), "好友 %s 已上线于 %s!", pFriendName, pServerInfo->m_aName);
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
