#include "word_library.h"
#include <game/client/gameclient.h>
#include <game/client/components/chat.h>
#include <game/client/components/controls.h>
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

	// 加载配置
	LoadConfig();
}

void CWordLibrary::OnNewSnapshot()
{
	// 检查快捷键
	if(Input()->KeyIsPressed(KEY_F1))
	{
		CWordGroup *pGroup = GetGroupByKey(KEY_F1);
		if(pGroup && Config()->m_McWordLibraryEnable)
		{
			SendRandomMessage(pGroup->m_aId);
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

	// 随机选择一条消息
	int Index = secure_rand_below(vGroupMessages.size());
	CWordMessage *pMessage = vGroupMessages[Index];

	// 检查冷却时间
	float CurrentTime = Client()->LocalTime();
	if(CurrentTime - m_LastSendTime < Config()->m_McWordLibrarySendCooldown)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Cooldown active");
		return false;
	}

	// 发送消息
	GameClient()->m_Chat.SendChat(0, pMessage->m_aContent);
	m_LastSendTime = CurrentTime;
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
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Cooldown active");
		return false;
	}

	// 发送消息
	CWordMessage *pMessage = vGroupMessages[Index];
	GameClient()->m_Chat.SendChat(0, pMessage->m_aContent);
	m_LastSendTime = CurrentTime;
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

// 配置保存/加载
void CWordLibrary::SaveConfig()
{
	char aPath[IO_MAX_PATH_LENGTH];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, Config()->m_McWordLibraryConfig, aPath, sizeof(aPath));

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
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Configuration saved successfully");
}

void CWordLibrary::LoadConfig()
{
	char aPath[IO_MAX_PATH_LENGTH];
	Storage()->GetCompletePath(IStorage::TYPE_SAVE, Config()->m_McWordLibraryConfig, aPath, sizeof(aPath));

	// 清空现有数据
	for(auto *pGroup : m_vGroups)
	{
		delete pGroup;
	}
	m_vGroups.clear();
	m_vMessages.clear();

	// 读取配置文件
	CLineReader LineReader;
	if(!LineReader.OpenFile(Storage()->OpenFile(aPath, IOFLAG_READ, IStorage::TYPE_SAVE)))
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "No config file found, using defaults");
		return;
	}

	while(const char *pLine = LineReader.Get())
	{
		// 跳过注释和空行
		if(pLine[0] == '#' || pLine[0] == '\n' || pLine[0] == '\r' || str_length(pLine) == 0)
			continue;

		// 解析命令
		char aCommand[128];
		char aArgs[512];
		if(sscanf(pLine, "%s %[^\n]", aCommand, aArgs) == 2)
		{
			if(str_comp(aCommand, "mc_add_word_group") == 0)
			{
				char aId[MAX_WORD_GROUP_ID_LENGTH];
				char aName[MAX_WORD_GROUP_NAME_LENGTH];
				int Removable = 1;
				if(sscanf(aArgs, "\"%[^\"]\" \"%[^\"]\" %d", aId, aName, &Removable) >= 2)
				{
					AddGroup(aId, aName, Removable != 0);
				}
			}
			else if(str_comp(aCommand, "mc_add_word_message") == 0)
			{
				char aGroupId[MAX_WORD_GROUP_ID_LENGTH];
				char aMessage[MAX_WORD_MESSAGE_LENGTH];
				if(sscanf(aArgs, "\"%[^\"]\" \"%[^\"]\"", aGroupId, aMessage) == 2)
				{
					AddMessage(aGroupId, aMessage);
				}
			}
		}
	}

	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "word_library", "Configuration loaded successfully");
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

void CWordLibrary::ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData)
{
	CWordLibrary *pThis = static_cast<CWordLibrary*>(pUserData);
	pThis->SaveConfig();
}
