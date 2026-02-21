
#include "mclient.h"

#include <base/color.h>
#include <base/math.h>
#include <base/system.h>

#include <engine/client.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>

#include <game/client/gameclient.h>
#include <game/client/render.h>

CMClient::CMClient()
{
}

void CMClient::OnInit()
{
}

void CMClient::OnConsoleInit()
{
	// Random Skin Rotation
	Console()->Register("mc_random_skin_rotation", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConRandomSkinRotation, this, "Enable random skin rotation");
	Console()->Register("mc_random_skin_left_click_only", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConRandomSkinRotationLeftClickOnly, this, "Enable random skin rotation only on left click");

	// Clone
	Console()->Register("mc_clone_enabled", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConCloneEnabled, this, "Enable clone");
	Console()->Register("mc_clone_copy_name", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConCloneCopyName, this, "Copy name when cloning");
	Console()->Register("mc_clone_hold", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConCloneHold, this, "Enable clone on hold");
	Console()->Register("mc_clone_hammer", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConCloneHammer, this, "Enable clone on hammer");
	Console()->Register("mc_clone_distance", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConCloneDistance, this, "Enable clone on distance");

	// Rainbow Tee
	Console()->Register("mc_rainbow_tee_enabled", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConRainbowTeeEnabled, this, "Enable rainbow tee");

	// Rainbow Body
	Console()->Register("mc_rainbow_body_enabled", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConRainbowBodyEnabled, this, "Enable rainbow body");

	// Rainbow Feet
	Console()->Register("mc_rainbow_feet_enabled", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConRainbowFeetEnabled, this, "Enable rainbow feet");

	// Rainbow Speed
	Console()->Register("mc_rainbow_speed", "", CFGFLAG_CLIENT | CFGFLAG_SAVE, ConRainbowSpeed, this, "Rainbow color change speed (0-100)");
}

void CMClient::OnRender()
{
	// Rainbow effect implementation will be added here
}

void CMClient::OnMessage(int MsgType, void *pRawMsg)
{
	// Message handling implementation will be added here
}

// Console command implementations
void CMClient::ConRandomSkinRotation(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McRandomSkinRotation = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_random_skin_rotation value: %d", g_Config.m_McRandomSkinRotation);
}

void CMClient::ConRandomSkinRotationLeftClickOnly(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McRandomSkinLeftClickOnly = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_random_skin_left_click_only value: %d", g_Config.m_McRandomSkinLeftClickOnly);
}

void CMClient::ConCloneEnabled(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McCloneEnabled = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_clone_enabled value: %d", g_Config.m_McCloneEnabled);
}

void CMClient::ConCloneCopyName(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McCloneCopyName = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_clone_copy_name value: %d", g_Config.m_McCloneCopyName);
}

void CMClient::ConCloneHold(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McCloneHold = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_clone_hold value: %d", g_Config.m_McCloneHold);
}

void CMClient::ConCloneHammer(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McCloneHammer = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_clone_hammer value: %d", g_Config.m_McCloneHammer);
}

void CMClient::ConCloneDistance(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McCloneDistance = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_clone_distance value: %d", g_Config.m_McCloneDistance);
}

void CMClient::ConRainbowTeeEnabled(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McRainbowTeeEnabled = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_rainbow_tee_enabled value: %d", g_Config.m_McRainbowTeeEnabled);
}

void CMClient::ConRainbowBodyEnabled(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McRainbowBodyEnabled = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_rainbow_body_enabled value: %d", g_Config.m_McRainbowBodyEnabled);
}

void CMClient::ConRainbowFeetEnabled(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McRainbowFeetEnabled = pResult->GetInteger(0);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_rainbow_feet_enabled value: %d", g_Config.m_McRainbowFeetEnabled);
}

void CMClient::ConRainbowSpeed(IConsole::IResult *pResult, void *pUserData)
{
	CMClient *pThis = static_cast<CMClient *>(pUserData);
	if(pResult->NumArguments() == 1)
		g_Config.m_McRainbowSpeed = clamp(pResult->GetInteger(0), 0, 100);
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "mclient", "mc_rainbow_speed value: %d", g_Config.m_McRainbowSpeed);
}
