
#include <base/math.h>
#include <base/system.h>

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <engine/shared/localization.h>
#include <engine/textrender.h>

#include <game/client/components/menus.h>
#include <game/client/components/mclient/mclient.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>

using namespace FontIcons;

const float FontSize = 14.0f;
const float LineSize = 20.0f;
const float HeadlineFontSize = 20.0f;
const float Margin = 10.0f;
const float MarginSmall = 5.0f;

enum
{
	MCLIENT_TAB_FUN = 0,
	NUMBER_OF_MCLIENT_TABS
};

void CMenus::RenderMClientSettings(CUIRect MainView)
{
	CUIRect Label, Button, Left, Right;

	// Split view for tabs and content
	CUIRect TabBar, Content;
	MainView.VSplitLeft(150.0f, &TabBar, &Content);

	// Render tabs
	TabBar.Margin(5.0f, &TabBar);

	CUIRect Tab;
	static CButtonContainer s_aTabButtons[NUMBER_OF_MCLIENT_TABS];

	for(int i = 0; i < NUMBER_OF_MCLIENT_TABS; i++)
	{
		TabBar.HSplitTop(25.0f, &Tab, &TabBar);
		TabBar.HSplitTop(MarginSmall, nullptr, &TabBar);

		const char *pTabName = "";
		switch(i)
		{
		case MCLIENT_TAB_FUN:
			pTabName = Localize("趣味面板");
			break;
		}

		if(DoButton_MenuTab(&s_aTabButtons[i], pTabName, m_ActiveMClientTab == i, &Tab, IGraphics::CORNER_ALL))
		{
			m_ActiveMClientTab = i;
		}
	}

	// Render content
	Content.Margin(10.0f, &Content);

	if(m_ActiveMClientTab == MCLIENT_TAB_FUN)
	{
		RenderMClientFunSettings(Content);
	}
}

void CMenus::RenderMClientFunSettings(CUIRect View)
{
	CUIRect Label, Button, Left, Right, SubView;

	// Random Skin Rotation
	View.HSplitTop(HeadlineFontSize + Margin, &Label, &View);
	Ui()->DoLabel(&Label, Localize("随机皮肤轮换"), HeadlineFontSize, TEXTALIGN_ML);

	View.HSplitTop(Margin, nullptr, &View);
	View.HSplitTop(LineSize, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_McRandomSkinRotation, Localize("随机皮肤轮换"), g_Config.m_McRandomSkinRotation, &Button))
		g_Config.m_McRandomSkinRotation ^= 1;

	// Random Skin Rotation - Left Click Only
	if(g_Config.m_McRandomSkinRotation)
	{
		View.HSplitTop(MarginSmall, nullptr, &View);
		View.HSplitTop(LineSize, &Button, &View);
		Button.VSplitLeft(20.0f, nullptr, &Button); // Indent
		if(DoButton_CheckBox(&g_Config.m_McRandomSkinLeftClickOnly, Localize("仅在左键时生效"), g_Config.m_McRandomSkinLeftClickOnly, &Button))
			g_Config.m_McRandomSkinLeftClickOnly ^= 1;
	}

	View.HSplitTop(Margin, nullptr, &View);

	// Clone
	View.HSplitTop(HeadlineFontSize + Margin, &Label, &View);
	Ui()->DoLabel(&Label, Localize("克隆人"), HeadlineFontSize, TEXTALIGN_ML);

	View.HSplitTop(Margin, nullptr, &View);
	View.HSplitTop(LineSize, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_McCloneEnabled, Localize("克隆人"), g_Config.m_McCloneEnabled, &Button))
		g_Config.m_McCloneEnabled ^= 1;

	// Clone options
	if(g_Config.m_McCloneEnabled)
	{
		View.HSplitTop(MarginSmall, nullptr, &View);
		View.HSplitTop(LineSize, &Button, &View);
		Button.VSplitLeft(20.0f, nullptr, &Button);
		if(DoButton_CheckBox(&g_Config.m_McCloneCopyName, Localize("复制名字"), g_Config.m_McCloneCopyName, &Button))
			g_Config.m_McCloneCopyName ^= 1;

		View.HSplitTop(MarginSmall, nullptr, &View);
		View.HSplitTop(LineSize, &Button, &View);
		Button.VSplitLeft(20.0f, nullptr, &Button);
		if(DoButton_CheckBox(&g_Config.m_McCloneHold, Localize("勾住生效"), g_Config.m_McCloneHold, &Button))
			g_Config.m_McCloneHold ^= 1;

		View.HSplitTop(MarginSmall, nullptr, &View);
		View.HSplitTop(LineSize, &Button, &View);
		Button.VSplitLeft(20.0f, nullptr, &Button);
		if(DoButton_CheckBox(&g_Config.m_McCloneHammer, Localize("锤击生效"), g_Config.m_McCloneHammer, &Button))
			g_Config.m_McCloneHammer ^= 1;

		View.HSplitTop(MarginSmall, nullptr, &View);
		View.HSplitTop(LineSize, &Button, &View);
		Button.VSplitLeft(20.0f, nullptr, &Button);
		if(DoButton_CheckBox(&g_Config.m_McCloneDistance, Localize("距离生效"), g_Config.m_McCloneDistance, &Button))
			g_Config.m_McCloneDistance ^= 1;
	}

	View.HSplitTop(Margin, nullptr, &View);

	// Rainbow Tee
	View.HSplitTop(HeadlineFontSize + Margin, &Label, &View);
	Ui()->DoLabel(&Label, Localize("同步彩虹Tee"), HeadlineFontSize, TEXTALIGN_ML);

	View.HSplitTop(Margin, nullptr, &View);
	View.HSplitTop(LineSize, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_McRainbowTeeEnabled, Localize("同步彩虹Tee"), g_Config.m_McRainbowTeeEnabled, &Button))
		g_Config.m_McRainbowTeeEnabled ^= 1;

	View.HSplitTop(Margin, nullptr, &View);

	// Rainbow Body
	View.HSplitTop(HeadlineFontSize + Margin, &Label, &View);
	Ui()->DoLabel(&Label, Localize("身体"), HeadlineFontSize, TEXTALIGN_ML);

	View.HSplitTop(Margin, nullptr, &View);
	View.HSplitTop(LineSize, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_McRainbowBodyEnabled, Localize("身体"), g_Config.m_McRainbowBodyEnabled, &Button))
		g_Config.m_McRainbowBodyEnabled ^= 1;

	// Rainbow Body Speed
	if(g_Config.m_McRainbowBodyEnabled)
	{
		View.HSplitTop(MarginSmall, nullptr, &View);
		View.HSplitTop(LineSize, &Button, &View);
		Button.VSplitLeft(20.0f, nullptr, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Ui()->DoLabel(&Label, Localize("速度:"), FontSize, TEXTALIGN_ML);
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_McRainbowSpeed);
		if(DoEditBox(&m_McRainbowSpeedInput, &Button, aBuf, g_Config.m_McRainbowSpeed, sizeof(g_Config.m_McRainbowSpeed)))
		{
			int Value = clamp(str_toint(aBuf), 0, 100);
			g_Config.m_McRainbowSpeed = Value;
		}
	}

	View.HSplitTop(Margin, nullptr, &View);

	// Rainbow Feet
	View.HSplitTop(HeadlineFontSize + Margin, &Label, &View);
	Ui()->DoLabel(&Label, Localize("脚"), HeadlineFontSize, TEXTALIGN_ML);

	View.HSplitTop(Margin, nullptr, &View);
	View.HSplitTop(LineSize, &Button, &View);
	if(DoButton_CheckBox(&g_Config.m_McRainbowFeetEnabled, Localize("脚"), g_Config.m_McRainbowFeetEnabled, &Button))
		g_Config.m_McRainbowFeetEnabled ^= 1;

	// Rainbow Feet Speed
	if(g_Config.m_McRainbowFeetEnabled)
	{
		View.HSplitTop(MarginSmall, nullptr, &View);
		View.HSplitTop(LineSize, &Button, &View);
		Button.VSplitLeft(20.0f, nullptr, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Ui()->DoLabel(&Label, Localize("速度:"), FontSize, TEXTALIGN_ML);
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_McRainbowSpeed);
		if(DoEditBox(&m_McRainbowSpeedInput, &Button, aBuf, g_Config.m_McRainbowSpeed, sizeof(g_Config.m_McRainbowSpeed)))
		{
			int Value = clamp(str_toint(aBuf), 0, 100);
			g_Config.m_McRainbowSpeed = Value;
		}
	}
}
