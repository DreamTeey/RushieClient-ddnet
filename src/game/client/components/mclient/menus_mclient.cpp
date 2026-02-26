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
#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_scrollregion.h>

#include <vector>

enum
{
	MCLIENT_TAB_SETTINGS = 0,
	MCLIENT_TAB_INFO,
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
		MCLocalize("Info")};

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
}

void CMenus::RenderSettingsMClientSettings(CUIRect MainView)
{
    static CScrollRegion s_ScrollRegion;
    vec2 ScrollOffset(0.0f, 0.0f);
    CScrollRegionParams ScrollParams;
    ScrollParams.m_ScrollUnit = 120.0f;
    ScrollParams.m_Flags = CScrollRegionParams::FLAG_CONTENT_STATIC_WIDTH;
    ScrollParams.m_ScrollbarMargin = 5.0f;
    s_ScrollRegion.Begin(&MainView, &ScrollOffset, &ScrollParams);

    // 应用偏移
    MainView.y += ScrollOffset.y;

    CUIRect LeftColumn, RightColumn;
    MainView.Margin(MarginSmall, &MainView);
    MainView.VSplitMid(&LeftColumn, &RightColumn, MarginBetweenViews);

    // --- 核心辅助函数：渲染一个功能块 ---
    auto DoSettingGroup = [&](CUIRect* pColumn, const char* pTitle, auto&& RenderContent) {
        // 固定高度：设定一个足够大的值，容纳所有选项展开后的状态
        const float FixedGroupHeight = 450.0f; 
        
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
    });

    // ================== 滚动条计算 ==================
    // 既然使用了固定高度，滚动条高度也要据此调整
    float TotalContentHeight = maximum(LeftColumn.y, RightColumn.y);
    CUIRect ScrollRect;
    ScrollRect.x = MainView.x;
    ScrollRect.y = MainView.y;
    ScrollRect.w = MainView.w;
    ScrollRect.h = (MainView.y - TotalContentHeight) + MarginBetweenSections;

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
	if(DoButtonLineSize_Menu(&s_WebsiteButton, MCLocalize("Website"), 0, &Button, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
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
	if(DoButtonLineSize_Menu(&s_Config, MCLocalize("MClient Settings"), 0, &Button, LineSize, false, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
	{
		Storage()->GetCompletePath(IStorage::TYPE_SAVE, s_aConfigDomains[ConfigDomain::MCLIENT].m_aConfigPath, aBuf, sizeof(aBuf));
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
		Label.VSplitLeft(TextRender()->TextWidth(LineSize, "Developer"), &Label, &Button);
		Button.VSplitLeft(MarginSmall, nullptr, &Button);
		Button.w = LineSize, Button.h = LineSize, Button.y = Label.y + (Label.h / 2.0f - Button.h / 2.0f);
		Ui()->DoLabel(&Label, "Developer", LineSize, TEXTALIGN_ML);
		RenderDevSkin(TeeRect.Center(), 50.0f, "default", "default", false, 0, 0, 0, false, true);
	}
}
