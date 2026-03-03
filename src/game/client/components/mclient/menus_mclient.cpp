#include <base/math.h>
#include <base/system.h>
#include <base/str.h>
#include <base/types.h>

#include <algorithm>
#include <cctype>

#include <engine/font_icons.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/localization.h>
#include <generated/protocol.h>

#include <game/client/components/menu_background.h>
#include <game/client/components/menus.h>
#include <game/client/components/binds.h>
#include <game/client/components/key_binder.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/client/components/mclient/word_library.h>

#include <vector>

// 好友打招呼文本输入框
static CLineInputBuffered<128> s_FriendGreetInput;

// 词库输入框
static CLineInputBuffered<MAX_WORD_GROUP_ID_LENGTH> s_WordGroupIdInput;
static CLineInputBuffered<MAX_WORD_GROUP_NAME_LENGTH> s_WordGroupNameInput;
static CLineInputBuffered<MAX_WORD_MESSAGE_LENGTH> s_WordMessageInput;

enum
{
	MCLIENT_TAB_SETTINGS = 0,
	MCLIENT_TAB_INFO,
	MCLIENT_TAB_WORD_LIBRARY,
	NUMBER_OF_MCLIENT_TABS
};

static float s_Time = 0.0f;
static bool s_StartedTime = false;

const float FontSize = 14.0f;
const float EditBoxFontSize = 12.0f;
const float LineSize = 20.0f;

const float ColorPickerLineSize = 25.0f;
const float HeadlineFontSize = 20.0f;

const float HeadlineHeight = HeadlineFontSize + 0.0f;
const float Margin = 10.0f;
const float MarginSmall = 5.0f;
const float MarginExtraSmall = 2.5f;
const float MarginBetweenSections = 30.0f;
const float MarginBetweenViews = 30.0f;

const float ColorPickerLabelSize = 13.0f;
const float ColorPickerLineSpacing = 5.0f;

static void SetFlag(int32_t &Flags, int n, bool Value)
{
	if(Value)
		Flags |= (1 << n);
	else
		Flags &= ~(1 << n);
}

static bool IsFlagSet(int32_t Flags, int n)
{
	return (Flags & (1 << n)) != 0;
}

void CMenus::RenderSettingsMClient(CUIRect MainView)
{
	s_Time += Client()->RenderFrameTime() * (1.0f / 100.0f);
	if(!s_StartedTime)
	{
		s_StartedTime = true;
		s_Time = (float)rand() / (float)RAND_MAX;
	}

	static int s_CurCustomTab = 0;

	CUIRect TabBar, Button;
	int TabCount = NUMBER_OF_MCLIENT_TABS;

	MainView.HSplitTop(LineSize, &TabBar, &MainView);
	const float TabWidth = TabBar.w / TabCount;
	static CButtonContainer s_aPageTabs[NUMBER_OF_MCLIENT_TABS] = {};
	const char *apTabNames[] = {
		MCLocalize("Settings"),
		MCLocalize("Info"),
		MCLocalize("Word Library")};

	for(int Tab = 0; Tab < NUMBER_OF_MCLIENT_TABS; ++Tab)
	{
		TabBar.VSplitLeft(TabWidth, &Button, &TabBar);
		const int Corners = Tab == 0 ? IGraphics::CORNER_L : Tab == NUMBER_OF_MCLIENT_TABS - 1 ? IGraphics::CORNER_R : IGraphics::CORNER_NONE;
		if(DoButton_MenuTab(&s_aPageTabs[Tab], apTabNames[Tab], s_CurCustomTab == Tab, &Button, Corners, nullptr, nullptr, nullptr, nullptr, 4.0f))
			s_CurCustomTab = Tab;
	}

	MainView.HSplitTop(Margin, nullptr, &MainView);

	if(s_CurCustomTab == MCLIENT_TAB_SETTINGS)
	{
		RenderSettingsMClientSettings(MainView);
	}

	if(s_CurCustomTab == MCLIENT_TAB_INFO)
	{
		RenderSettingsMClientInfo(MainView);
	}

	if(s_CurCustomTab == MCLIENT_TAB_WORD_LIBRARY)
	{
		RenderSettingsMClientWordLibrary(MainView);
	}
}

void CMenus::RenderSettingsMClientSettings(CUIRect MainView)
{
	// 初始化好友打招呼文本输入框
	static bool s_FriendGreetInitialized = false;
	if(!s_FriendGreetInitialized)
	{
		s_FriendGreetInput.Set(g_Config.m_McFriendAutoGreetText);
		s_FriendGreetInitialized = true;
	}

    static CScrollRegion s_ScrollRegion;
    vec2 ScrollOffset(0.0f, 0.0f);
    CScrollRegionParams ScrollParams;
    ScrollParams.m_ScrollUnit = 60.0f;
    ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
    ScrollParams.m_ScrollbarMargin = 5.0f;
    s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);

    MainView.y += ScrollOffset.y;

    MainView.VSplitRight(5.0f, &MainView, nullptr); // Padding for scrollbar
    MainView.VSplitLeft(5.0f, nullptr, &MainView); // Padding for scrollbar

    CUIRect LeftColumn, RightColumn;
    MainView.Margin(MarginSmall, &MainView);
    MainView.VSplitMid(&LeftColumn, &RightColumn, MarginBetweenViews);

    // --- 核心辅助函数：渲染一个功能块 ---
    auto DoSettingGroup = [&](CUIRect* pColumn, const char* pTitle, auto&& RenderContent) {
        // 固定高度：设定一个足够大的值，容纳所有选项展开后的状态
        // 趣味面板：22行选项 + 标题 = 22 * 25.0f + 20.0f = 570.0f
        // 实用功能：约10行选项 + 标题 = 10 * 25.0f + 20.0f = 270.0f
        const float FixedGroupHeight = 570.0f; 
        
        // 1. 准备背景矩形 (在处理 pColumn 之前)
        CUIRect Background = *pColumn;
        Background.h = FixedGroupHeight;
        
        // 应用出血量：向外扩张 8 像素
        Background.Margin(-8.0f, &Background);
        
        // 2. [关键] 先画背景，它会处于最底层
        Background.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);

        // 3. 渲染标题
        CUIRect Label;
        pColumn->HSplitTop(HeadlineHeight, &Label, pColumn);
        Ui()->DoLabel(&Label, pTitle, HeadlineFontSize, TEXTALIGN_MC);
        pColumn->HSplitTop(MarginSmall, nullptr, pColumn);

        // 4. 渲染内容 (此时内容会覆盖在背景之上)
        RenderContent(pColumn);

        // 5. 强制将 pColumn 的游标移到固定高度之后，确保下一个 Group 位置正确
        float UsedHeight = Background.y + FixedGroupHeight - pColumn->y; 
        // 如果实际内容没超过固定高度，就补齐间距
        pColumn->HSplitTop(FixedGroupHeight - (Label.h + MarginSmall), nullptr, pColumn);
        
        // 组间距
        pColumn->HSplitTop(MarginBetweenSections, nullptr, pColumn);
    };

    // 子选项渲染
    auto DoSubOption = [&](CUIRect* pColumn, auto&& RenderFunc) {
        CUIRect Row, SubRow;
        pColumn->HSplitTop(LineSize, &Row, pColumn);
        Row.VSplitLeft(25.0f, nullptr, &SubRow);
        RenderFunc(&SubRow);
        pColumn->HSplitTop(MarginSmall, nullptr, pColumn);
    };

    // ================== 左侧渲染 ==================
    DoSettingGroup(&LeftColumn, MCLocalize("趣味面板"), [&](CUIRect* pCol) {

        // 随机皮肤
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRandomSkinRotate, MCLocalize("随机皮肤轮换"), &g_Config.m_McRandomSkinRotate, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McRandomSkinRotate)
        {
            DoSubOption(pCol, [&](CUIRect* pRect) {
                Ui()->DoScrollbarOption(&g_Config.m_McRandomSkinRotateInterval, &g_Config.m_McRandomSkinRotateInterval, pRect, MCLocalize("轮换间隔(秒)"), 1, 60);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRandomSkinRotateOnlyLeftClick, MCLocalize("仅在左键时生效"), &g_Config.m_McRandomSkinRotateOnlyLeftClick, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRandomSkinRotateMain, MCLocalize("主体更换"), &g_Config.m_McRandomSkinRotateMain, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRandomSkinRotateDummy, MCLocalize("分身更换"), &g_Config.m_McRandomSkinRotateDummy, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRandomSkinRotateMainColor, MCLocalize("主体随机颜色"), &g_Config.m_McRandomSkinRotateMainColor, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRandomSkinRotateDummyColor, MCLocalize("分身随机颜色"), &g_Config.m_McRandomSkinRotateDummyColor, pRect, LineSize);
            });
        }

        // 克隆人
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McClonePlayer, MCLocalize("克隆人"), &g_Config.m_McClonePlayer, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McClonePlayer)
        {
            const char* apLabels[] = { "复制名字", "勾住生效", "锤击生效", "距离生效" };
            int* apConfigs[] = { &g_Config.m_McCloneCopyName, &g_Config.m_McCloneOnHook, &g_Config.m_McCloneOnHammer, &g_Config.m_McCloneOnDistance };
            for(int i = 0; i < 4; ++i) {
                DoSubOption(pCol, [&](CUIRect* pRect) {
                    DoButton_CheckBoxAutoVMarginAndSet(apConfigs[i], MCLocalize(apLabels[i]), apConfigs[i], pRect, LineSize);
                });
            }
        }

        // 彩虹Tee
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRainbowTee, MCLocalize("同步彩虹Tee"), &g_Config.m_McRainbowTee, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McRainbowTee)
        {
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRainbowTeeBody, MCLocalize("身体"), &g_Config.m_McRainbowTeeBody, pRect, LineSize);
            });
            if(g_Config.m_McRainbowTeeBody) {
                DoSubOption(pCol, [&](CUIRect* pRect) {
                    Ui()->DoScrollbarOption(&g_Config.m_McRainbowTeeBodySpeed, &g_Config.m_McRainbowTeeBodySpeed, pRect, MCLocalize("速度"), 0, 1000);
                });
            }
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRainbowTeeFeet, MCLocalize("脚"), &g_Config.m_McRainbowTeeFeet, pRect, LineSize);
            });
            if(g_Config.m_McRainbowTeeFeet) {
                DoSubOption(pCol, [&](CUIRect* pRect) {
                    Ui()->DoScrollbarOption(&g_Config.m_McRainbowTeeFeetSpeed, &g_Config.m_McRainbowTeeFeetSpeed, pRect, MCLocalize("速度"), 0, 1000);
                });
            }
        }

        // 消息复读
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McRepeaterEnable, MCLocalize("消息复读"), &g_Config.m_McRepeaterEnable, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McRepeaterEnable)
        {
            DoSubOption(pCol, [&](CUIRect* pRect) {
                static CButtonContainer ReaderButton, ClearButton;
                DoLine_KeyReader(*pRect, ReaderButton, ClearButton, MCLocalize("复读按键"), "mc_repeat_last_message");
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                Ui()->DoScrollbarOption(&g_Config.m_McRepeaterCooldown, &g_Config.m_McRepeaterCooldown, pRect, MCLocalize("冷却时间(秒)"), 1, 10);
            });
        }

        // 自动加一
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McAutoPlusOneEnable, MCLocalize("自动加一"), &g_Config.m_McAutoPlusOneEnable, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McAutoPlusOneEnable)
        {
            DoSubOption(pCol, [&](CUIRect* pRect) {
                Ui()->DoScrollbarOption(&g_Config.m_McAutoPlusOneCooldown, &g_Config.m_McAutoPlusOneCooldown, pRect, MCLocalize("冷却时间(秒)"), 1, 10);
            });
        }
    });

    // ================== 右侧渲染 ==================
    DoSettingGroup(&RightColumn, MCLocalize("实用功能"), [&](CUIRect* pCol) {
        // 钩子角度辅助
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McHookAngleHelper, MCLocalize("钩子角度辅助"), &g_Config.m_McHookAngleHelper, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McHookAngleHelper)
        {
            DoSubOption(pCol, [&](CUIRect* pRect) {
                static CButtonContainer ReaderButton, ClearButton;
                DoLine_KeyReader(*pRect, ReaderButton, ClearButton, MCLocalize("应用角度按键"), "mc_hook_angle_apply");
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                static CButtonContainer ReaderButton, ClearButton;
                DoLine_KeyReader(*pRect, ReaderButton, ClearButton, MCLocalize("重置按键"), "mc_hook_angle_reset");
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                Ui()->DoScrollbarOption(&g_Config.m_McHookAngleScanRange, &g_Config.m_McHookAngleScanRange, pRect, MCLocalize("扫描范围(度)"), 30, 180);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                Ui()->DoScrollbarOption(&g_Config.m_McHookAngleScanStep, &g_Config.m_McHookAngleScanStep, pRect, MCLocalize("扫描步进(度)"), 1, 10);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McHookAngleShowScanRange, MCLocalize("显示扫描范围"), &g_Config.m_McHookAngleShowScanRange, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McHookAngleShowAllAngles, MCLocalize("显示所有测试角度"), &g_Config.m_McHookAngleShowAllAngles, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McHookAngleShowCollision, MCLocalize("显示碰撞点"), &g_Config.m_McHookAngleShowCollision, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McHookAngleAutoApply, MCLocalize("自动应用最佳角度"), &g_Config.m_McHookAngleAutoApply, pRect, LineSize);
            });
        }

        // 武器快捷切换
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McWeaponSwitch, MCLocalize("武器快捷切换"), &g_Config.m_McWeaponSwitch, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McWeaponSwitch)
        {
            DoSubOption(pCol, [&](CUIRect* pRect) {
                static CButtonContainer ReaderButton, ClearButton;
                DoLine_KeyReader(*pRect, ReaderButton, ClearButton, MCLocalize("切换按键"), "mc_switch_last_weapon");
            });
        }

        // 好友上线提醒
        DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McFriendNotify, MCLocalize("好友上线提醒"), &g_Config.m_McFriendNotify, pCol, LineSize);
        pCol->HSplitTop(MarginSmall, nullptr, pCol);
        if(g_Config.m_McFriendNotify)
        {
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McFriendNotifyAutoRefresh, MCLocalize("自动刷新服务器列表"), &g_Config.m_McFriendNotifyAutoRefresh, pRect, LineSize);
            });
            if(g_Config.m_McFriendNotifyAutoRefresh) {
                DoSubOption(pCol, [&](CUIRect* pRect) {
                    Ui()->DoScrollbarOption(&g_Config.m_McFriendNotifyRefreshInterval, &g_Config.m_McFriendNotifyRefreshInterval, pRect, MCLocalize("刷新间隔(秒)"), 10, 300);
                });
            }
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McFriendNotifyOffline, MCLocalize("好友下线提醒"), &g_Config.m_McFriendNotifyOffline, pRect, LineSize);
            });
            DoSubOption(pCol, [&](CUIRect* pRect) {
                DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McFriendAutoGreet, MCLocalize("好友进图自动打招呼"), &g_Config.m_McFriendAutoGreet, pRect, LineSize);
            });
            if(g_Config.m_McFriendAutoGreet) {
                DoSubOption(pCol, [&](CUIRect* pRect) {
                    CUIRect Label;
                    pRect->HSplitTop(LineSize, &Label, pRect);
                    Label.VSplitLeft(100.0f, &Label, pRect);
                    Ui()->DoLabel(&Label, MCLocalize("打招呼文本"), 12.0f, TEXTALIGN_ML);
                    if(Ui()->DoEditBox(&s_FriendGreetInput, pRect, 12.0f))
                    {
                        str_copy(g_Config.m_McFriendAutoGreetText, s_FriendGreetInput.GetString(), sizeof(g_Config.m_McFriendAutoGreetText));
                    }
                });
            }
        }
    });

    // ================== 滚动条计算 ==================
    CUIRect ScrollRect;
    ScrollRect.x = MainView.x;
    ScrollRect.y = maximum(LeftColumn.y, RightColumn.y) + MarginSmall * 2.0f;
    ScrollRect.w = MainView.w;
    ScrollRect.h = 0.0f;

    s_ScrollRegion.AddRect(ScrollRect);
    s_ScrollRegion.End();
}

void CMenus::RenderSettingsMClientInfo(CUIRect MainView)
{
	CUIRect LeftView, RightView, Button, Label, LowerLeftView;
	MainView.HSplitTop(MarginSmall, nullptr, &MainView);

	MainView.VSplitMid(&LeftView, &RightView, MarginBetweenViews);
	LeftView.VSplitLeft(MarginSmall, nullptr, &LeftView);
	RightView.VSplitRight(MarginSmall, &RightView, nullptr);
	LeftView.HSplitMid(&LeftView, &LowerLeftView, 0.0f);

	LeftView.HSplitTop(HeadlineHeight, &Label, &LeftView);
	Ui()->DoLabel(&Label, MCLocalize("MClient Links"), HeadlineFontSize, TEXTALIGN_ML);
	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);

	static CButtonContainer s_WebsiteButton;
	LeftView.HSplitTop(LineSize * 2.0f, &Button, &LeftView);
	if(DoButton_Menu(&s_WebsiteButton, MCLocalize("Website"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		Client()->ViewLink("https://github.com");

	LeftView = LowerLeftView;
	LeftView.HSplitBottom(LineSize * 4.0f + MarginSmall * 2.0f + HeadlineFontSize, nullptr, &LeftView);
	LeftView.HSplitTop(HeadlineHeight, &Label, &LeftView);
	Ui()->DoLabel(&Label, MCLocalize("Config Files"), HeadlineFontSize, TEXTALIGN_ML);
	LeftView.HSplitTop(MarginSmall, nullptr, &LeftView);

	char aBuf[128 + IO_MAX_PATH_LENGTH];
	CUIRect MClientConfig;

	LeftView.HSplitTop(LineSize * 2.0f, &Button, &LeftView);

	static CButtonContainer s_Config;
	if(DoButton_Menu(&s_Config, MCLocalize("MClient Settings"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, "settings_mclient.cfg", aBuf, sizeof(aBuf));
		Client()->ViewFile(aBuf);
	}

	// =======RIGHT VIEW========

	RightView.HSplitTop(HeadlineHeight, &Label, &RightView);
	Ui()->DoLabel(&Label, MCLocalize("MClient Developer"), HeadlineFontSize, TEXTALIGN_ML);
	RightView.HSplitTop(MarginSmall, nullptr, &RightView);
	RightView.HSplitTop(MarginSmall, nullptr, &RightView);

	const float TeeSize = 50.0f;
	const float CardSize = TeeSize + MarginSmall;
	CUIRect TeeRect, DevCardRect;
	{
		RightView.HSplitTop(CardSize, &DevCardRect, &RightView);
		DevCardRect.VSplitLeft(CardSize, &TeeRect, &Label);
		Label.VSplitLeft(TextRender()->TextWidth(12.0f, "Developer"), &Label, &Button);
		Button.VSplitLeft(MarginSmall, nullptr, &Button);
		Button.w = LineSize, Button.h = LineSize, Button.y = Label.y + (Label.h / 2.0f - Button.h / 2.0f);
		Ui()->DoLabel(&Label, "Developer", 12.0f, TEXTALIGN_ML);
		// RenderDevSkin(TeeRect.Center(), 50.0f, "default", "default", false, 0, 0, 0, false, true);
	}
}

void CMenus::RenderSettingsMClientWordLibrary(CUIRect MainView)
{
	MainView.HSplitTop(MarginSmall, nullptr, &MainView);

	CUIRect LeftView, RightView, Column1, Column2, Column3, Column4, Button, Label;
	MainView.VSplitMid(&LeftView, &RightView, Margin);
	LeftView.VSplitLeft(MarginSmall, nullptr, &LeftView);
	RightView.VSplitRight(MarginSmall, &RightView, nullptr);

	// 词库将使用4列布局
	// [词库分组] - [分组编辑] - [消息列表] - [消息编辑]

	// 静态变量用于存储选中状态和输入内容
	static CWordGroup *s_pSelectedGroup = nullptr;
	static CWordMessage *s_pSelectedMessage = nullptr;
	static char s_aGroupId[MAX_WORD_GROUP_ID_LENGTH] = "";
	static char s_aGroupName[MAX_WORD_GROUP_NAME_LENGTH] = "";
	static char s_aMessageContent[MAX_WORD_MESSAGE_LENGTH] = "";

	LeftView.VSplitMid(&Column1, &Column2, Margin);
	RightView.VSplitMid(&Column3, &Column4, Margin);

	// ====== 词库分组列表 ======
	{
		Column1.HSplitTop(HeadlineHeight, &Label, &Column1);
		Label.VSplitRight(25.0f, &Label, &Button);
		Ui()->DoLabel(&Label, MCLocalize("词库分组"), HeadlineFontSize, TEXTALIGN_ML);
		Column1.HSplitTop(MarginSmall, nullptr, &Column1);

		// 搜索框
		CUIRect SearchRect;
		Column1.HSplitBottom(25.0f, &Column1, &SearchRect);
		SearchRect.HSplitTop(MarginSmall, nullptr, &SearchRect);

		static CLineInputBuffered<128> s_GroupFilterInput;
		std::vector<CWordGroup *> vpFilteredGroups;
		for(CWordGroup *pGroup : GameClient()->m_WordLibrary.GetGroups())
		{
			if(str_find_nocase(pGroup->m_aId, s_GroupFilterInput.GetString()))
				vpFilteredGroups.push_back(pGroup);
			else if(str_find_nocase(pGroup->m_aDisplayName, s_GroupFilterInput.GetString()))
				vpFilteredGroups.push_back(pGroup);
		}

		int SelectedOldGroup = -1;
		static CListBox s_GroupListBox;
		s_GroupListBox.DoStart(35.0f, vpFilteredGroups.size(), 1, 2, SelectedOldGroup, &Column1);

		static std::vector<unsigned char> s_vGroupItemIds;
		static std::vector<CButtonContainer> s_vGroupDeleteButtons;

		const int MaxGroups = GameClient()->m_WordLibrary.GetGroups().size();
		s_vGroupItemIds.resize(MaxGroups);
		s_vGroupDeleteButtons.resize(MaxGroups);

		for(size_t i = 0; i < vpFilteredGroups.size(); i++)
		{
			CWordGroup *pGroup = vpFilteredGroups[i];

			if(s_pSelectedGroup && pGroup == s_pSelectedGroup)
				SelectedOldGroup = i;

			const CListboxItem Item = s_GroupListBox.DoNextItem(&s_vGroupItemIds[i], SelectedOldGroup >= 0 && (size_t)SelectedOldGroup == i);
			if(!Item.m_Visible)
				continue;

			CUIRect GroupRect, DeleteButton, GroupLabel;
			Item.m_Rect.Margin(0.0f, &GroupRect);
			GroupRect.VSplitLeft(26.0f, &DeleteButton, &GroupRect);
			DeleteButton.HMargin(7.5f, &DeleteButton);
			DeleteButton.VSplitLeft(MarginSmall, nullptr, &DeleteButton);
			DeleteButton.VSplitRight(MarginExtraSmall, &DeleteButton, nullptr);

			// 删除按钮
			if(pGroup->m_Removable)
			{
				if(Ui()->DoButton_FontIcon(&s_vGroupDeleteButtons[i], FontIcon::TRASH, 0, &DeleteButton, IGraphics::CORNER_ALL))
				{
					GameClient()->m_WordLibrary.RemoveGroup(pGroup->m_aId);
					if(s_pSelectedGroup == pGroup)
						s_pSelectedGroup = nullptr;
				}
			}

			// 分组名称
			GroupRect.HMargin(MarginExtraSmall, &GroupRect);
			Ui()->DoLabel(&GroupRect, pGroup->m_aDisplayName, FontSize, TEXTALIGN_ML);
		}

		const int NewSelectedGroup = s_GroupListBox.DoEnd();
		if(SelectedOldGroup != NewSelectedGroup || (SelectedOldGroup >= 0 && Ui()->HotItem() == &s_vGroupItemIds[NewSelectedGroup] && Ui()->MouseButtonClicked(0)))
		{
			s_pSelectedGroup = vpFilteredGroups[NewSelectedGroup];
			if(!Ui()->LastMouseButton(1) && !Ui()->LastMouseButton(2))
			{
				str_copy(s_aGroupId, s_pSelectedGroup->m_aId);
				str_copy(s_aGroupName, s_pSelectedGroup->m_aDisplayName);
			}
		}

		Ui()->DoEditBox_Search(&s_GroupFilterInput, &SearchRect, 14.0f, !Ui()->IsPopupOpen() && !GameClient()->m_GameConsole.IsActive());
	}

	// ====== 分组编辑 ======
	{
		Column2.HSplitTop(HeadlineHeight, &Label, &Column2);
		Label.VSplitRight(25.0f, &Label, &Button);
		Ui()->DoLabel(&Label, MCLocalize("分组编辑"), HeadlineFontSize, TEXTALIGN_ML);
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);

		// 启用词库功能
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McWordLibraryEnable, MCLocalize("启用词库功能"), &g_Config.m_McWordLibraryEnable, &Column2, LineSize);
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);

		// 发送冷却时间
		CUIRect CooldownRow, CooldownLabel, CooldownSlider;
		Column2.HSplitTop(LineSize, &CooldownRow, &Column2);
		CooldownRow.VSplitLeft(100.0f, &CooldownLabel, &CooldownSlider);
		Ui()->DoLabel(&CooldownLabel, MCLocalize("发送冷却时间 (秒)"), EditBoxFontSize, TEXTALIGN_ML);
		// 不设置固定宽度，使用剩余空间
		Ui()->DoScrollbarOption(&g_Config.m_McWordLibrarySendCooldown, &g_Config.m_McWordLibrarySendCooldown, &CooldownSlider, "", 1, 10);
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);

		// 分组ID
		static CLineInput s_GroupIdInput;
		s_GroupIdInput.SetBuffer(s_aGroupId, sizeof(s_aGroupId));
		s_GroupIdInput.SetEmptyText(MCLocalize("Group ID"));
		Column2.HSplitTop(HeadlineFontSize, &Button, &Column2);
		Ui()->DoEditBox(&s_GroupIdInput, &Button, 12.0f);

		// 分组名称
		static CLineInput s_GroupNameInput;
		s_GroupNameInput.SetBuffer(s_aGroupName, sizeof(s_aGroupName));
		s_GroupNameInput.SetEmptyText(MCLocalize("Group Name"));
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);
		Column2.HSplitTop(HeadlineFontSize, &Button, &Column2);
		Ui()->DoEditBox(&s_GroupNameInput, &Button, 12.0f);

		// 添加和更新按钮
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);
		CUIRect AddUpdateButtons;
		Column2.HSplitTop(LineSize * 2.0f, &AddUpdateButtons, &Column2);
		CUIRect AddButton, UpdateButton;
		AddUpdateButtons.VSplitMid(&AddButton, &UpdateButton, MarginSmall);

		static CButtonContainer s_AddGroupButton, s_UpdateGroupButton;
		if(DoButtonLineSize_Menu(&s_AddGroupButton, MCLocalize("添加分组"), 0, &AddButton, LineSize))
		{
			if(s_aGroupId[0] != 0)
			{
				GameClient()->m_WordLibrary.AddGroup(s_aGroupId, s_aGroupName);
				s_aGroupId[0] = 0;
				s_aGroupName[0] = 0;
			}
		}
		if(DoButtonLineSize_Menu(&s_UpdateGroupButton, MCLocalize("更新分组"), 0, &UpdateButton, LineSize) && s_pSelectedGroup)
		{
			GameClient()->m_WordLibrary.UpdateGroup(s_pSelectedGroup->m_aId, s_aGroupName);
		}

		// 保存和加载按钮
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);
		CUIRect SaveLoadButtons;
		Column2.HSplitTop(LineSize * 2.0f, &SaveLoadButtons, &Column2);
		CUIRect SaveButton, LoadButton;
		SaveLoadButtons.VSplitMid(&SaveButton, &LoadButton, MarginSmall);

		static CButtonContainer s_SaveButton, s_LoadButton;
		if(DoButtonLineSize_Menu(&s_SaveButton, MCLocalize("保存配置"), 0, &SaveButton, LineSize))
		{
			ConfigManager()->Save();
		}
		if(DoButtonLineSize_Menu(&s_LoadButton, MCLocalize("加载配置"), 0, &LoadButton, LineSize))
		{
			// 先清空现有词库数据
			GameClient()->m_WordLibrary.LoadConfig();
		}

		// 发送按钮
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);
		Column2.HSplitTop(LineSize * 2.0f, &Button, &Column2);
		static CButtonContainer s_SendButton;
		if(DoButtonLineSize_Menu(&s_SendButton, MCLocalize("发送消息"), 0, &Button, LineSize) && s_pSelectedGroup)
		{
			GameClient()->m_WordLibrary.SendRandomMessage(s_pSelectedGroup->m_aId);
		}

		// 快捷键绑定
		Column2.HSplitTop(MarginSmall, nullptr, &Column2);
		CUIRect KeyBindRect, ClearKeyButton;
		Column2.HSplitTop(LineSize * 2.0f, &KeyBindRect, &Column2);
		KeyBindRect.VSplitRight(LineSize * 2.0f, &KeyBindRect, &ClearKeyButton);

		if(s_pSelectedGroup)
		{
			// 使用 CKeyBinder 组件处理按键绑定
			static CButtonContainer s_KeyBindButton, s_ClearKeyButton;
			CBindSlot CurrentBind(s_pSelectedGroup->m_BoundKey, KeyModifier::NONE);
			CKeyBinder::CKeyReaderResult KeyResult = GameClient()->m_KeyBinder.DoKeyReader(&s_KeyBindButton, &s_ClearKeyButton, &KeyBindRect, CurrentBind, false);

			if(!KeyResult.m_Aborted && KeyResult.m_Bind.m_Key != CurrentBind.m_Key)
			{
				if(KeyResult.m_Bind.m_Key == KEY_UNKNOWN)
				{
					// 清除绑定
					GameClient()->m_WordLibrary.UnbindKey(s_pSelectedGroup->m_aId);
				}
				else
				{
					// 绑定新按键
					GameClient()->m_WordLibrary.BindKey(s_pSelectedGroup->m_aId, KeyResult.m_Bind.m_Key);
				}
			}
		}
	}

	// ====== 消息列表 ======
	{
		Column3.HSplitTop(HeadlineHeight, &Label, &Column3);
		Label.VSplitRight(25.0f, &Label, &Button);
		Ui()->DoLabel(&Label, MCLocalize("消息列表"), HeadlineFontSize, TEXTALIGN_ML);
		Column3.HSplitTop(MarginSmall, nullptr, &Column3);

		// 搜索框
		CUIRect SearchRect;
		Column3.HSplitBottom(25.0f, &Column3, &SearchRect);
		SearchRect.HSplitTop(MarginSmall, nullptr, &SearchRect);

		static CLineInputBuffered<128> s_MessageFilterInput;
		std::vector<CWordMessage *> vpFilteredMessages;
		for(const CWordMessage &Message : GameClient()->m_WordLibrary.GetMessages())
		{
			if(!s_pSelectedGroup || Message.m_pGroup == s_pSelectedGroup)
			{
				if(str_find_nocase(Message.m_aContent, s_MessageFilterInput.GetString()))
					vpFilteredMessages.push_back(const_cast<CWordMessage *>(&Message));
			}
		}

		int SelectedOldMessage = -1;
		static CListBox s_MessageListBox;
		s_MessageListBox.DoStart(35.0f, vpFilteredMessages.size(), 1, 2, SelectedOldMessage, &Column3);

		static std::vector<unsigned char> s_vMessageItemIds;
		static std::vector<CButtonContainer> s_vMessageDeleteButtons;

		const int MaxMessages = GameClient()->m_WordLibrary.GetMessages().size();
		s_vMessageItemIds.resize(MaxMessages);
		s_vMessageDeleteButtons.resize(MaxMessages);

		for(size_t i = 0; i < vpFilteredMessages.size(); i++)
		{
			CWordMessage *pMessage = vpFilteredMessages[i];

			if(s_pSelectedMessage && pMessage == s_pSelectedMessage)
				SelectedOldMessage = i;

			const CListboxItem Item = s_MessageListBox.DoNextItem(&s_vMessageItemIds[i], SelectedOldMessage >= 0 && (size_t)SelectedOldMessage == i);
			if(!Item.m_Visible)
				continue;

			CUIRect MessageRect, DeleteButton, MessageLabel;
			Item.m_Rect.Margin(0.0f, &MessageRect);
			MessageRect.VSplitLeft(26.0f, &DeleteButton, &MessageRect);
			DeleteButton.HMargin(7.5f, &DeleteButton);
			DeleteButton.VSplitLeft(MarginSmall, nullptr, &DeleteButton);
			DeleteButton.VSplitRight(MarginExtraSmall, &DeleteButton, nullptr);

			// 删除按钮
			if(pMessage->m_pGroup && pMessage->m_pGroup->m_Removable)
			{
				if(Ui()->DoButton_FontIcon(&s_vMessageDeleteButtons[i], FontIcon::TRASH, 0, &DeleteButton, IGraphics::CORNER_ALL))
				{
					GameClient()->m_WordLibrary.RemoveMessage(pMessage->m_pGroup->m_aId, pMessage->m_aContent);
					if(s_pSelectedMessage == pMessage)
						s_pSelectedMessage = nullptr;
				}
			}

			// 消息内容
			MessageRect.HMargin(MarginExtraSmall, &MessageRect);
			Ui()->DoLabel(&MessageRect, pMessage->m_aContent, FontSize, TEXTALIGN_ML);
		}

		const int NewSelectedMessage = s_MessageListBox.DoEnd();
		if(SelectedOldMessage != NewSelectedMessage || (SelectedOldMessage >= 0 && Ui()->HotItem() == &s_vMessageItemIds[NewSelectedMessage] && Ui()->MouseButtonClicked(0)))
		{
			s_pSelectedMessage = vpFilteredMessages[NewSelectedMessage];
			if(!Ui()->LastMouseButton(1) && !Ui()->LastMouseButton(2))
			{
				str_copy(s_aMessageContent, s_pSelectedMessage->m_aContent);
			}
		}

		Ui()->DoEditBox_Search(&s_MessageFilterInput, &SearchRect, 14.0f, !Ui()->IsPopupOpen() && !GameClient()->m_GameConsole.IsActive());
	}

	// ====== 消息编辑 ======
	{
		Column4.HSplitTop(HeadlineHeight, &Label, &Column4);
		Label.VSplitRight(25.0f, &Label, &Button);
		Ui()->DoLabel(&Label, MCLocalize("消息编辑"), HeadlineFontSize, TEXTALIGN_ML);
		Column4.HSplitTop(MarginSmall, nullptr, &Column4);

		// 显示当前选中的分组
		Column4.HSplitTop(LineSize, &Button, &Column4);
		if(s_pSelectedGroup)
		{
			Ui()->DoLabel(&Button, s_pSelectedGroup->m_aDisplayName, EditBoxFontSize, TEXTALIGN_ML);
		}
		else
		{
			Ui()->DoLabel(&Button, MCLocalize("未选择分组"), EditBoxFontSize, TEXTALIGN_ML);
		}

		// 消息内容输入框
		static CLineInput s_MessageInput;
		s_MessageInput.SetBuffer(s_aMessageContent, sizeof(s_aMessageContent));
		s_MessageInput.SetEmptyText(MCLocalize("Message Content"));
		Column4.HSplitTop(MarginSmall, nullptr, &Column4);
		Column4.HSplitTop(HeadlineFontSize * 2.0f, &Button, &Column4);
		Ui()->DoEditBox(&s_MessageInput, &Button, 12.0f);

		// 添加和更新按钮
		Column4.HSplitTop(MarginSmall, nullptr, &Column4);
		CUIRect AddUpdateButtons;
		Column4.HSplitTop(LineSize * 2.0f, &AddUpdateButtons, &Column4);
		CUIRect AddButton, UpdateButton;
		AddUpdateButtons.VSplitMid(&AddButton, &UpdateButton, MarginSmall);

		static CButtonContainer s_AddMessageButton, s_UpdateMessageButton;
		if(DoButtonLineSize_Menu(&s_AddMessageButton, MCLocalize("添加消息"), 0, &AddButton, LineSize) && s_pSelectedGroup)
		{
			if(s_aMessageContent[0] != 0)
			{
				GameClient()->m_WordLibrary.AddMessage(s_pSelectedGroup->m_aId, s_aMessageContent);
				s_aMessageContent[0] = 0;
			}
		}
		if(DoButtonLineSize_Menu(&s_UpdateMessageButton, MCLocalize("更新消息"), 0, &UpdateButton, LineSize) && s_pSelectedMessage)
		{
			GameClient()->m_WordLibrary.UpdateMessage(s_pSelectedMessage->m_pGroup->m_aId, s_pSelectedMessage->m_aContent, s_aMessageContent);
		}
	}
}