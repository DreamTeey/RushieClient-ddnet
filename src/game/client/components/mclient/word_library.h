#ifndef GAME_CLIENT_COMPONENTS_MCLIENT_WORD_LIBRARY_H
#define GAME_CLIENT_COMPONENTS_MCLIENT_WORD_LIBRARY_H

#include <game/client/component.h>
#include <engine/console.h>
#include <engine/config.h>
#include <engine/shared/config.h>
#include <base/vmath.h>
#include <vector>
#include <string>

enum
{
	MAX_WORD_GROUP_ID_LENGTH = 32,      // 分组ID最大长度
	MAX_WORD_GROUP_NAME_LENGTH = 64,    // 分组显示名称最大长度
	MAX_WORD_MESSAGE_LENGTH = 512,      // 消息内容最大长度
};

// 词库分组
class CWordGroup
{
public:
	char m_aId[MAX_WORD_GROUP_ID_LENGTH] = "";       // 内部标识符（如"greeting"）
	char m_aDisplayName[MAX_WORD_GROUP_NAME_LENGTH] = "";  // 显示名称（如"问候"）
	ColorRGBA m_Color = ColorRGBA(1.0f, 1.0f, 1.0f, 1.0f);  // 分组颜色
	bool m_Removable = true;       // 是否可删除
	bool m_Imported = false;       // 是否导入的
	int m_Index = 0;               // 分组索引
	int m_BoundKey = 0;            // 绑定的快捷键

	CWordGroup(const char *pId, const char *pDisplayName, bool Removable = true)
	{
		str_copy(m_aId, pId);
		str_copy(m_aDisplayName, pDisplayName);
		m_Removable = Removable;
	}

	bool operator==(const CWordGroup &Other) const
	{
		return str_comp(m_aId, Other.m_aId) == 0;
	}
};

// 单条消息
class CWordMessage
{
public:
	char m_aContent[MAX_WORD_MESSAGE_LENGTH] = "";  // 消息内容
	CWordGroup *m_pGroup = nullptr;                 // 所属分组
	int m_UsageCount = 0;                           // 使用次数
	int64_t m_LastUsed = 0;                         // 最后使用时间

	CWordMessage(CWordGroup *pGroup, const char *pContent)
	{
		m_pGroup = pGroup;
		str_copy(m_aContent, pContent);
	}
};

// 词库管理器
class CWordLibrary : public CComponent
{
private:
	std::vector<CWordGroup *> m_vGroups;  // 分组列表
	std::vector<CWordMessage> m_vMessages;  // 消息列表
	float m_LastSendTime = 0.0f;          // 最后发送时间
	char m_aLastSentMessage[MAX_WORD_MESSAGE_LENGTH] = "";  // 最后发送的消息内容

	// 配置保存回调
	static void ConfigSaveCallback(IConfigManager *pConfigManager, void *pUserData);

	// 控制台命令
	static void ConAddWordGroup(IConsole::IResult *pResult, void *pUserData);
	static void ConAddWordMessage(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveWordGroup(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveWordMessage(IConsole::IResult *pResult, void *pUserData);
	static void ConUpdateWordGroup(IConsole::IResult *pResult, void *pUserData);
	static void ConUpdateWordMessage(IConsole::IResult *pResult, void *pUserData);
	static void ConSendWord(IConsole::IResult *pResult, void *pUserData);
	static void ConSendWordIndex(IConsole::IResult *pResult, void *pUserData);
	static void ConListWordGroups(IConsole::IResult *pResult, void *pUserData);
	static void ConListGroupMessages(IConsole::IResult *pResult, void *pUserData);
	static void ConBindWordKey(IConsole::IResult *pResult, void *pUserData);

public:
	CWordLibrary();
	~CWordLibrary() override;

	// 分组管理
	CWordGroup *AddGroup(const char *pId, const char *pDisplayName, bool Removable = true);
	void RemoveGroup(const char *pId);
	CWordGroup *FindGroup(const char *pId);
	bool UpdateGroup(const char *pId, const char *pNewDisplayName);

	// 消息管理
	CWordMessage *AddMessage(const char *pGroupId, const char *pContent);
	void RemoveMessage(const char *pGroupId, const char *pContent);
	CWordMessage *FindMessage(const char *pGroupId, const char *pContent);
	bool UpdateMessage(const char *pGroupId, const char *pOldContent, const char *pNewContent);

	// 消息发送
	bool SendRandomMessage(const char *pGroupId);
	bool SendMessage(const char *pGroupId, int Index);

	// 列表功能
	void ListGroups();
	void ListGroupMessages(const char *pGroupId);

	// 快捷键
	bool BindKey(const char *pGroupId, int Key);
	bool UnbindKey(const char *pGroupId);
	CWordGroup *GetGroupByKey(int Key);

	// 配置保存/加载
	void SaveConfig();
	void LoadConfig();

	// 获取列表
	const std::vector<CWordGroup *>& GetGroups() const { return m_vGroups; }
	const std::vector<CWordMessage>& GetMessages() const { return m_vMessages; }

	// 组件接口
	int Sizeof() const override { return sizeof(*this); }
	void OnConsoleInit() override;
	void OnNewSnapshot() override;
};

#endif // GAME_CLIENT_COMPONENTS_MCLIENT_WORD_LIBRARY_H
