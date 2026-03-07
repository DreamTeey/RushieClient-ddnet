#include "word_library.h"
#include "mclient.h"
#include <game/client/gameclient.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>
#include <engine/shared/config.h>
#include <engine/console.h>
#include <engine/input.h>
#include <base/system.h>
#include <base/secure.h>
#include <engine/shared/linereader.h>

CWordLibrary::CWordLibrary()
{
	m_LastSendTime = 0.0f;
}

CWordLibrary::~CWordLibrary()
{
	// 清理分组指针
	for(auto *pGroup : m_vGroups)
	{
		delete pGroup;
	}
	m_vGroups.clear();
	m_vMessages.clear();
}

void CWordLibrary::OnConsoleInit()
{
	// 注册控制台命令
	Console()->Register("mc_add_word_group", "s[id] s[name] ?r[removable=1]", CFGFLAG_CLIENT, ConAddWordGroup, this, "Add a new word group");
	Console()->Register("mc_add_word_message", "s[group_id] s[message]", CFGFLAG_CLIENT, ConAddWordMessage, this, "Add a message to a group");
	Console()->Register("mc_remove_word_group", "s[group_id]", CFGFLAG_CLIENT, ConRemoveWordGroup, this, "Remove a word group");
	Console()->Register("mc_remove_word_message", "s[group_id] s[message]", CFGFLAG_CLIENT, ConRemoveWordMessage, this, "Remove a message from a group");
	Console()->Register("mc_send_word", "s[group_id]", CFGFLAG_CLIENT, ConSendWord, this, "Send a random message from a group");
	Console()->Register("mc_send_word_index", "s[group_id] i[index]", CFGFLAG_CLIENT, ConSendWordIndex, this, "Send a message by index from a group");
	Console()->Register("mc_list_word_groups", "", CFGFLAG_CLIENT, ConListWordGroups, this, "List all word groups");
	Console()->Register("mc_list_group_messages", "s[group_id]", CFGFLAG_CLIENT, ConListGroupMessages, this, "List messages in a group");
	Console()->Register("mc_bind_word_key", "s[group_id] i[key]", CFGFLAG_CLIENT, ConBindWordKey, this, "Bind a key to a word group");
	Console()->Register("mc_set_word_group_atlasthooker", "s[group_id] i[value]", CFGFLAG_CLIENT, ConSetWordGroupAtLastHooker, this, "Set whether to @mention last hooker when sending messages");

	// 注册配置保存回调
	ConfigManager()->RegisterCallback(ConfigSaveCallback, this, ConfigDomain::MCLIENT);

	// 加载配置
	LoadConfig();
}

void CWordLibrary::OnNewSnapshot()
{
	// 检查所有绑定的快捷键
	if(Config()->m_McWordLibraryEnable)
	{
		// 确保按键状态向量大小与分组数量一致
		if(m_vKeyStates.size() != m_vGroups.size())
			m_vKeyStates.resize(m_vGroups.size(), false);

		for(size_t i = 0; i < m_vGroups.size(); ++i)
		{
			auto *pGroup = m_vGroups[i];
			if(pGroup->m_BoundKey != 0)
			{
				bool KeyPressed = Input()->KeyIsPressed(pGroup->m_BoundKey);
				// 只在按键刚刚按下时触发消息发送
				if(KeyPressed && !m_vKeyStates[i])
				{
					SendRandomMessage(pGroup->m_aId);
				}
				m_vKeyStates[i] = KeyPressed;
			}
		}
	}
}

// 分组管理
CWordGroup *CWordLibrary::AddGroup(const char *pId, const char *pDisplayName, bool Removable)
{
	if(!pId || !pDisplayName)
		return nullptr;

	// 检查是否已存在
	if(FindGroup(pId))
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group already exists");
		return nullptr;
	}

	// 创建新分组
	CWordGroup *pGroup = new CWordGroup(pId, pDisplayName, Removable);
	pGroup->m_Index = m_vGroups.size();
	m_vGroups.push_back(pGroup);

	// 同步更新按键状态向量
	m_vKeyStates.push_back(false);

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group added successfully");
	return pGroup;
}

void CWordLibrary::RemoveGroup(const char *pId)
{
	if(!pId)
		return;

	CWordGroup *pGroup = FindGroup(pId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return;
	}

	if(!pGroup->m_Removable)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Cannot remove this group");
		return;
	}

	// 删除该分组的所有消息
	auto it = m_vMessages.begin();
	while(it != m_vMessages.end())
	{
		if(it->m_pGroup == pGroup)
			it = m_vMessages.erase(it);
		else
			++it;
	}

	// 从分组列表中移除
	for(auto it = m_vGroups.begin(); it != m_vGroups.end(); ++it)
	{
		if(*it == pGroup)
		{
			delete *it;
			m_vGroups.erase(it);
			break;
		}
	}

	// 重新索引分组
	for(size_t i = 0; i < m_vGroups.size(); ++i)
	{
		m_vGroups[i]->m_Index = i;
	}

	// 同步更新按键状态向量
	m_vKeyStates.resize(m_vGroups.size(), false);

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group removed successfully");
}

CWordGroup *CWordLibrary::FindGroup(const char *pId)
{
	if(!pId)
		return nullptr;

	for(auto *pGroup : m_vGroups)
	{
		if(str_comp(pGroup->m_aId, pId) == 0)
			return pGroup;
	}
	return nullptr;
}

bool CWordLibrary::UpdateGroup(const char *pId, const char *pNewDisplayName)
{
	if(!pId || !pNewDisplayName)
		return false;

	CWordGroup *pGroup = FindGroup(pId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return false;
	}

	str_copy(pGroup->m_aDisplayName, pNewDisplayName);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group updated successfully");
	return true;
}

// 消息管理
CWordMessage *CWordLibrary::AddMessage(const char *pGroupId, const char *pContent)
{
	if(!pGroupId || !pContent)
		return nullptr;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return nullptr;
	}

	// 去重检测：检查消息是否已存在
	CWordMessage *pExistingMessage = FindMessage(pGroupId, pContent);
	if(pExistingMessage)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Message already exists in this group");
		return pExistingMessage;
	}

	// 创建新消息
	m_vMessages.emplace_back(pGroup, pContent);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Message added successfully");
	return &m_vMessages.back();
}

void CWordLibrary::RemoveMessage(const char *pGroupId, const char *pContent)
{
	if(!pGroupId || !pContent)
		return;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return;
	}

	// 查找并删除消息
	for(auto it = m_vMessages.begin(); it != m_vMessages.end(); ++it)
	{
		if(it->m_pGroup == pGroup && str_comp(it->m_aContent, pContent) == 0)
		{
			m_vMessages.erase(it);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Message removed successfully");
			return;
		}
	}

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Message not found");
}

CWordMessage *CWordLibrary::FindMessage(const char *pGroupId, const char *pContent)
{
	if(!pGroupId || !pContent)
		return nullptr;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
		return nullptr;

	for(auto &Message : m_vMessages)
	{
		if(Message.m_pGroup == pGroup && str_comp(Message.m_aContent, pContent) == 0)
			return &Message;
	}
	return nullptr;
}

bool CWordLibrary::UpdateMessage(const char *pGroupId, const char *pOldContent, const char *pNewContent)
{
	if(!pGroupId || !pOldContent || !pNewContent)
		return false;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return false;
	}

	for(auto &Message : m_vMessages)
	{
		if(Message.m_pGroup == pGroup && str_comp(Message.m_aContent, pOldContent) == 0)
		{
			str_copy(Message.m_aContent, pNewContent);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Message updated successfully");
			return true;
		}
	}

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Message not found");
	return false;
}

// 消息发送
bool CWordLibrary::SendRandomMessage(const char *pGroupId)
{
	if(!pGroupId)
		return false;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return false;
	}

	// 收集该分组的所有消息
	std::vector<CWordMessage*> vGroupMessages;
	for(auto &Message : m_vMessages)
	{
		if(Message.m_pGroup == pGroup)
			vGroupMessages.push_back(&Message);
	}

	if(vGroupMessages.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "No messages in this group");
		return false;
	}

	// 随机选择一条消息，避免重复
	CWordMessage *pMessage = nullptr;
	int Index = 0;
	
	// 如果只有一条消息，直接使用
	if(vGroupMessages.size() == 1)
	{
		pMessage = vGroupMessages[0];
	}
	else
	{
		// 如果有多条消息，尝试选择不重复的消息
		int Attempts = 0;
		const int MaxAttempts = 10; // 最多尝试10次
		
		while(Attempts < MaxAttempts)
		{
			Index = secure_rand_below(vGroupMessages.size());
			pMessage = vGroupMessages[Index];
			
			// 如果最后发送的消息为空，或者当前消息不等于最后发送的消息，则使用
			if(m_aLastSentMessage[0] == 0 || str_comp(pMessage->m_aContent, m_aLastSentMessage) != 0)
			{
				break;
			}
			
			Attempts++;
		}
	}

	// 检查冷却时间
	float CurrentTime = Client()->LocalTime();
	if(CurrentTime - m_LastSendTime < Config()->m_McWordLibrarySendCooldown)
	{
		return false;
	}

	// 准备消息内容
	char aMessage[MAX_WORD_MESSAGE_LENGTH];
	str_copy(aMessage, pMessage->m_aContent, sizeof(aMessage));

	// 如果分组启用了@最后勾我的玩家
	if(pGroup->m_AtLastHooker)
	{
		const char *pHookerName = GetLastHookerName();
		if(pHookerName && pHookerName[0])
		{
			// 将消息格式化为 "@玩家名 消息内容"
			char aTemp[MAX_WORD_MESSAGE_LENGTH];
			str_format(aTemp, sizeof(aTemp), "%s: %s", pHookerName, pMessage->m_aContent);
			str_copy(aMessage, aTemp, sizeof(aMessage));
		}
	}

	// 发送消息
	GameClient()->m_Chat.SendChat(0, aMessage);
	m_LastSendTime = CurrentTime;
	str_copy(m_aLastSentMessage, pMessage->m_aContent);
	pMessage->m_UsageCount++;
	pMessage->m_LastUsed = time_get();
	return true;
}

bool CWordLibrary::SendMessage(const char *pGroupId, int Index)
{
	if(!pGroupId)
		return false;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return false;
	}

	// 收集该分组的所有消息
	std::vector<CWordMessage*> vGroupMessages;
	for(auto &Message : m_vMessages)
	{
		if(Message.m_pGroup == pGroup)
			vGroupMessages.push_back(&Message);
	}

	if(Index < 0 || Index >= (int)vGroupMessages.size())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Invalid message index");
		return false;
	}

	// 检查冷却时间
	float CurrentTime = Client()->LocalTime();
	if(CurrentTime - m_LastSendTime < Config()->m_McWordLibrarySendCooldown)
	{
		return false;
	}

	// 获取要发送的消息
	CWordMessage *pMessage = vGroupMessages[Index];

	// 准备消息内容
	char aMessage[MAX_WORD_MESSAGE_LENGTH];
	str_copy(aMessage, pMessage->m_aContent, sizeof(aMessage));

	// 如果分组启用了@最后勾我的玩家
	if(pGroup->m_AtLastHooker)
	{
		const char *pHookerName = GetLastHookerName();
		if(pHookerName && pHookerName[0])
		{
			// 将消息格式化为 "@玩家名 消息内容"
			char aTemp[MAX_WORD_MESSAGE_LENGTH];
			str_format(aTemp, sizeof(aTemp), "%s: %s", pHookerName, pMessage->m_aContent);
			str_copy(aMessage, aTemp, sizeof(aMessage));
		}
	}

	// 发送消息
	GameClient()->m_Chat.SendChat(0, aMessage);
	m_LastSendTime = CurrentTime;
	str_copy(m_aLastSentMessage, pMessage->m_aContent);
	pMessage->m_UsageCount++;
	pMessage->m_LastUsed = time_get();
	return true;
}

// 列表功能
void CWordLibrary::ListGroups()
{
	if(m_vGroups.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "No groups found");
		return;
	}

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "=== Word Groups ===");
	for(auto *pGroup : m_vGroups)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "[%d] %s (%s) - %d messages", 
			pGroup->m_Index, pGroup->m_aDisplayName, pGroup->m_aId, 
			(int)std::count_if(m_vMessages.begin(), m_vMessages.end(), 
				[pGroup](const CWordMessage& msg) { return msg.m_pGroup == pGroup; }));
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", aBuf);
	}
}

void CWordLibrary::ListGroupMessages(const char *pGroupId)
{
	if(!pGroupId)
		return;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return;
	}

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "=== Messages ===");
	int Index = 0;
	for(auto &Message : m_vMessages)
	{
		if(Message.m_pGroup == pGroup)
		{
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "[%d] %s (used %d times)", 
				Index++, Message.m_aContent, Message.m_UsageCount);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", aBuf);
		}
	}

	if(Index == 0)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "No messages in this group");
	}
}

// 快捷键
bool CWordLibrary::BindKey(const char *pGroupId, int Key)
{
	if(!pGroupId)
		return false;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return false;
	}

	// 检查是否已被其他分组绑定
	for(auto *pOtherGroup : m_vGroups)
	{
		if(pOtherGroup != pGroup && pOtherGroup->m_BoundKey == Key)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Key already bound to another group");
			return false;
		}
	}

	pGroup->m_BoundKey = Key;
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Key bound successfully");
	return true;
}

bool CWordLibrary::UnbindKey(const char *pGroupId)
{
	if(!pGroupId)
		return false;

	CWordGroup *pGroup = FindGroup(pGroupId);
	if(!pGroup)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Group not found");
		return false;
	}

	pGroup->m_BoundKey = 0;
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Key unbound successfully");
	return true;
}

CWordGroup *CWordLibrary::GetGroupByKey(int Key)
{
	for(auto *pGroup : m_vGroups)
	{
		if(pGroup->m_BoundKey == Key)
			return pGroup;
	}
	return nullptr;
}

const char *CWordLibrary::GetLastHookerName()
{
	// 直接访问CMClient的公共成员变量
	return GameClient()->m_MClient.m_aLastHookedByName;
}

// 配置保存/加载
void CWordLibrary::SaveConfig()
{
	char aPath[IO_MAX_PATH_LENGTH];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, Config()->m_McWordLibraryConfig, aPath, sizeof(aPath));

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", aPath);

	IOHANDLE File = Storage()->OpenFile(aPath, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Failed to open config file for writing");
		return;
	}

	char aBuf[512];
	str_copy(aBuf, "# Word Library Configuration\n", sizeof(aBuf));
	io_write(File, aBuf, str_length(aBuf));

	// 保存分组
	for(auto *pGroup : m_vGroups)
	{
		str_format(aBuf, sizeof(aBuf), "mc_add_word_group \"%s\" \"%s\" %d\n", 
			pGroup->m_aId, pGroup->m_aDisplayName, pGroup->m_Removable ? 1 : 0);
		io_write(File, aBuf, str_length(aBuf));

		// 保存分组设置（@最后勾我玩家）
		if(pGroup->m_AtLastHooker)
		{
			str_format(aBuf, sizeof(aBuf), "mc_set_word_group_atlasthooker \"%s\" 1\n", 
				pGroup->m_aId);
			io_write(File, aBuf, str_length(aBuf));
		}
	}

	// 保存消息
	for(auto &Message : m_vMessages)
	{
		if(Message.m_pGroup)
		{
			str_format(aBuf, sizeof(aBuf), "mc_add_word_message \"%s\" \"%s\"\n", 
				Message.m_pGroup->m_aId, Message.m_aContent);
			io_write(File, aBuf, str_length(aBuf));
		}
	}

	io_close(File);
	str_format(aBuf, sizeof(aBuf), "Saved %d groups and %d messages", (int)m_vGroups.size(), (int)m_vMessages.size());
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", aBuf);
}

void CWordLibrary::LoadConfig()
{
	// 清空现有数据
	for(auto *pGroup : m_vGroups)
	{
		delete pGroup;
	}
	m_vGroups.clear();
	m_vMessages.clear();
	m_vKeyStates.clear();

	// 重新加载词库配置 - 参考ExecuteFile的实现
	CLineReader LineReader;
	if(LineReader.OpenFile(Storage()->OpenFile("settings_mclient.cfg", IOFLAG_READ, IStorage::TYPE_SAVE)))
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "loading word library from settings_mclient.cfg");

		while(const char *pLine = LineReader.Get())
		{
			if(str_startswith(pLine, "mc_add_word_group") ||
				str_startswith(pLine, "mc_add_word_message") ||
				str_startswith(pLine, "mc_bind_word_key") ||
				str_startswith(pLine, "mc_set_word_group_atlasthooker"))
			{
				Console()->ExecuteLineFlag(pLine, CFGFLAG_CLIENT, IConsole::CLIENT_ID_UNSPECIFIED);
			}
		}
	}
}

// 控制台命令实现
void CWordLibrary::ConAddWordGroup(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pId = pResult->GetString(0);
	const char *pDisplayName = pResult->GetString(1);
	bool Removable = pResult->NumArguments() > 2 ? pResult->GetInteger(2) != 0 : true;
	pThis->AddGroup(pId, pDisplayName, Removable);
}

void CWordLibrary::ConAddWordMessage(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	const char *pMessage = pResult->GetString(1);
	pThis->AddMessage(pGroupId, pMessage);
}

void CWordLibrary::ConRemoveWordGroup(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	pThis->RemoveGroup(pGroupId);
}

void CWordLibrary::ConRemoveWordMessage(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	const char *pMessage = pResult->GetString(1);
	pThis->RemoveMessage(pGroupId, pMessage);
}

void CWordLibrary::ConSendWord(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	pThis->SendRandomMessage(pGroupId);
}

void CWordLibrary::ConSendWordIndex(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	int Index = pResult->GetInteger(1);
	pThis->SendMessage(pGroupId, Index);
}

void CWordLibrary::ConListWordGroups(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	pThis->ListGroups();
}

void CWordLibrary::ConListGroupMessages(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	pThis->ListGroupMessages(pGroupId);
}

void CWordLibrary::ConBindWordKey(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	int Key = pResult->GetInteger(1);
	pThis->BindKey(pGroupId, Key);
}

void CWordLibrary::ConSetWordGroupAtLastHooker(IConsole::IResult *pResult, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	const char *pGroupId = pResult->GetString(0);
	int Value = pResult->GetInteger(1);

	CWordGroup *pGroup = pThis->FindGroup(pGroupId);
	if(pGroup)
	{
		pGroup->m_AtLastHooker = Value;
	}
}

void CWordLibrary::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);

	char aBuf[512];

	// 保存分组
	for(auto *pGroup : pThis->m_vGroups)
	{
		// Imported groups don't get saved
		if(pGroup->m_Imported)
			continue;

		str_format(aBuf, sizeof(aBuf), "mc_add_word_group \"%s\" \"%s\" %d",
			pGroup->m_aId, pGroup->m_aDisplayName, pGroup->m_Removable ? 1 : 0);
		pConfigManager->WriteLine(aBuf, ConfigDomain::MCLIENT);
	}

	// 保存消息
	for(auto &Message : pThis->m_vMessages)
	{
		if(Message.m_pGroup && !Message.m_pGroup->m_Imported)
		{
			str_format(aBuf, sizeof(aBuf), "mc_add_word_message \"%s\" \"%s\"",
				Message.m_pGroup->m_aId, Message.m_aContent);
			pConfigManager->WriteLine(aBuf, ConfigDomain::MCLIENT);
		}
	}

	// 保存快捷键绑定
	for(auto *pGroup : pThis->m_vGroups)
	{
		if(pGroup->m_Imported)
			continue;
		if(pGroup->m_BoundKey != 0)
		{
			str_format(aBuf, sizeof(aBuf), "mc_bind_word_key \"%s\" %d",
				pGroup->m_aId, pGroup->m_BoundKey);
			pConfigManager->WriteLine(aBuf, ConfigDomain::MCLIENT);
		}
	}

	// 保存@最后勾我玩家设置
	for(auto *pGroup : pThis->m_vGroups)
	{
		if(pGroup->m_Imported)
			continue;
		if(pGroup->m_AtLastHooker)
		{
			str_format(aBuf, sizeof(aBuf), "mc_set_word_group_atlasthooker \"%s\" %d",
				pGroup->m_aId, pGroup->m_AtLastHooker);
			pConfigManager->WriteLine(aBuf, ConfigDomain::MCLIENT);
		}
	}
}
