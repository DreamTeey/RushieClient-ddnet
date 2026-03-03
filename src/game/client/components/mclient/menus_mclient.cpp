#include <base/math.h>
#include <base/system.h>
#include <base/str.h>

#include <algorithm>
#include <cctype>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/localization.h>
#include <generated/protocol.h>

#include <game/client/components/menu_background.h>
#include <game/client/components/menus.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
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
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, "mclient_config.cfg", aBuf, sizeof(aBuf));
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
	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 60.0f;
	ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
	ScrollParams.m_ScrollbarMargin = 5.0f;
	s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);

	MainView.y += ScrollOffset.y;
	MainView.VSplitRight(5.0f, &MainView, nullptr);
	MainView.VSplitLeft(5.0f, nullptr, &MainView);

	CUIRect LeftColumn, RightColumn;
	MainView.Margin(MarginSmall, &MainView);
	MainView.VSplitMid(&LeftColumn, &RightColumn, MarginBetweenViews);

	// ================== 左侧：分组管理 ==================
	auto DoSettingGroup = [&](CUIRect* pColumn, const char* pTitle, auto&& RenderContent) {
		const float FixedGroupHeight = 600.0f;
		CUIRect Background = *pColumn;
		Background.h = FixedGroupHeight;
		Background.Margin(-8.0f, &Background);
		Background.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);

		CUIRect Label;
		pColumn->HSplitTop(HeadlineHeight, &Label, pColumn);
		Ui()->DoLabel(&Label, pTitle, HeadlineFontSize, TEXTALIGN_MC);
		pColumn->HSplitTop(MarginSmall, nullptr, pColumn);

		RenderContent(pColumn);

		pColumn->HSplitTop(FixedGroupHeight - (Label.h + MarginSmall), nullptr, pColumn);
		pColumn->HSplitTop(MarginBetweenSections, nullptr, pColumn);
	};

	DoSettingGroup(&LeftColumn, MCLocalize("分组管理"), [&](CUIRect* pCol) {
		// 启用词库功能
		DoButton_CheckBoxAutoVMarginAndSet(&g_Config.m_McWordLibraryEnable, MCLocalize("启用词库功能"), &g_Config.m_McWordLibraryEnable, pCol, LineSize);
		pCol->HSplitTop(MarginSmall, nullptr, pCol);

		// 发送冷却时间
		Ui()->DoScrollbarOption(&g_Config.m_McWordLibrarySendCooldown, &g_Config.m_McWordLibrarySendCooldown, pCol, MCLocalize("发送冷却时间(秒)"), 1, 10);
		pCol->HSplitTop(MarginSmall, nullptr, pCol);

		// 添加分组
		CUIRect Row, Label, Input, Button;
		pCol->HSplitTop(LineSize, &Label, pCol);
		Ui()->DoLabel(&Label, MCLocalize("分组ID:"), EditBoxFontSize, TEXTALIGN_ML);
		Label.VSplitRight(5.0f, &Label, &Input);
		Input.w = 150.0f;
		if(Ui()->DoEditBox(&s_WordGroupIdInput, &Input, EditBoxFontSize))
		{
			// 输入框内容更新
		}
		Input.VSplitRight(5.0f, &Input, &Button);
		Button.w = 80.0f;
		static CButtonContainer s_AddGroupButton;
		if(DoButton_Menu(&s_AddGroupButton, MCLocalize("添加"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			if(s_WordGroupIdInput.GetString()[0] != 0)
			{
				Console()->ExecuteLineFormatted("mc_add_word_group %s %s", s_WordGroupIdInput.GetString(), s_WordGroupIdInput.GetString());
				s_WordGroupIdInput.Clear();
			}
		}
		pCol->HSplitTop(MarginSmall, nullptr, pCol);

		// 分组列表
		CUIRect ListRect;
		pCol->HSplitTop(300.0f, &ListRect, pCol);
		ListRect.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.15f), IGraphics::CORNER_ALL, 5.0f);
		ListRect.Margin(5.0f, &ListRect);

		// 显示分组列表
		CWordLibrary *pWordLibrary = &GameClient()->m_WordLibrary;
		if(pWordLibrary)
		{
			CUIRect GroupRow;
			for(size_t i = 0; i < pWordLibrary->m_vGroups.size(); ++i)
			{
				CWordGroup *pGroup = pWordLibrary->m_vGroups[i];
				if(!pGroup)
					continue;

				ListRect.HSplitTop(LineSize, &GroupRow, &ListRect);
				GroupRow.VSplitLeft(20.0f, &Button, &Label);

				// 删除按钮
				if(pGroup->m_Removable)
				{
					static CButtonContainer s_DeleteGroupButtons[100];
					if(DoButton_Menu(&s_DeleteGroupButtons[i], "×", 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 3.0f, 0.0f, ColorRGBA(1.0f, 0.0f, 0.0f, 0.25f)))
					{
						Console()->ExecuteLineFormatted("mc_remove_word_group %s", pGroup->m_aId);
					}
				}

				// 分组信息
				Label.VSplitLeft(5.0f, nullptr, &Label);
				Ui()->DoLabel(&Label, pGroup->m_aDisplayName, EditBoxFontSize, TEXTALIGN_ML);
				Label.VSplitRight(80.0f, &Label, &Button);

				// 发送按钮
				static CButtonContainer s_SendGroupButtons[100];
				if(DoButton_Menu(&s_SendGroupButtons[i], MCLocalize("发送"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 3.0f, 0.0f, ColorRGBA(0.0f, 1.0f, 0.0f, 0.25f)))
				{
					Console()->ExecuteLineFormatted("mc_send_word %s", pGroup->m_aId);
				}

				ListRect.HSplitTop(MarginSmall, nullptr, &ListRect);
			}
		}
	});

	// ================== 右侧：消息管理 ==================
	DoSettingGroup(&RightColumn, MCLocalize("消息管理"), [&](CUIRect* pCol) {
		// 添加消息
		CUIRect Row, Label, Input, Button;
		pCol->HSplitTop(LineSize, &Label, pCol);
		Ui()->DoLabel(&Label, MCLocalize("选择分组:"), EditBoxFontSize, TEXTALIGN_ML);
		Label.VSplitRight(5.0f, &Label, &Input);
		Input.w = 150.0f;
		if(Ui()->DoEditBox(&s_WordGroupIdInput, &Input, EditBoxFontSize))
		{
			// 输入框内容更新
		}
		pCol->HSplitTop(MarginSmall, nullptr, pCol);

		pCol->HSplitTop(LineSize, &Label, pCol);
		Ui()->DoLabel(&Label, MCLocalize("消息内容:"), EditBoxFontSize, TEXTALIGN_ML);
		Label.VSplitRight(5.0f, &Label, &Input);
		Input.w = 300.0f;
		if(Ui()->DoEditBox(&s_WordMessageInput, &Input, EditBoxFontSize))
		{
			// 输入框内容更新
		}
		Input.VSplitRight(5.0f, &Input, &Button);
		Button.w = 80.0f;
		static CButtonContainer s_AddMessageButton;
		if(DoButton_Menu(&s_AddMessageButton, MCLocalize("添加"), 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
		{
			if(s_WordGroupIdInput.GetString()[0] != 0 && s_WordMessageInput.GetString()[0] != 0)
			{
				Console()->ExecuteLineFormatted("mc_add_word_message %s %s", s_WordGroupIdInput.GetString(), s_WordMessageInput.GetString());
				s_WordMessageInput.Clear();
			}
		}
		pCol->HSplitTop(MarginSmall, nullptr, pCol);

		// 消息列表
		CUIRect ListRect;
		pCol->HSplitTop(300.0f, &ListRect, pCol);
		ListRect.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.15f), IGraphics::CORNER_ALL, 5.0f);
		ListRect.Margin(5.0f, &ListRect);

		// 显示消息列表
		CWordLibrary *pWordLibrary = &GameClient()->m_WordLibrary;
		if(pWordLibrary)
		{
			CUIRect MessageRow;
			for(size_t i = 0; i < pWordLibrary->m_vMessages.size(); ++i)
			{
				CWordMessage *pMessage = &pWordLibrary->m_vMessages[i];
				if(!pMessage || !pMessage->m_pGroup)
					continue;

				ListRect.HSplitTop(LineSize, &MessageRow, &ListRect);
				MessageRow.VSplitLeft(20.0f, &Button, &Label);

				// 删除按钮
				if(pMessage->m_pGroup->m_Removable)
				{
					static CButtonContainer s_DeleteMessageButtons[1000];
					if(DoButton_Menu(&s_DeleteMessageButtons[i], "×", 0, &Button, BUTTONFLAG_LEFT, nullptr, IGraphics::CORNER_ALL, 3.0f, 0.0f, ColorRGBA(1.0f, 0.0f, 0.0f, 0.25f)))
					{
						Console()->ExecuteLineFormatted("mc_remove_word_message %s %s", pMessage->m_pGroup->m_aId, pMessage->m_aContent);
					}
				}

				// 消息内容
				Label.VSplitLeft(5.0f, nullptr, &Label);
				Ui()->DoLabel(&Label, pMessage->m_aContent, EditBoxFontSize, TEXTALIGN_ML);

				ListRect.HSplitTop(MarginSmall, nullptr, &ListRect);
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