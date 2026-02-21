
#ifndef GAME_CLIENT_COMPONENTS_MCLIENT_MCLIENT_H
#define GAME_CLIENT_COMPONENTS_MCLIENT_MCLIENT_H

#include <engine/client/enums.h>
#include <engine/shared/console.h>
#include <engine/shared/config.h>

#include <game/client/component.h>

class CMClient : public CComponent
{
	// Random Skin Rotation
	static void ConRandomSkinRotation(IConsole::IResult *pResult, void *pUserData);
	static void ConRandomSkinRotationLeftClickOnly(IConsole::IResult *pResult, void *pUserData);

	// Clone
	static void ConCloneEnabled(IConsole::IResult *pResult, void *pUserData);
	static void ConCloneCopyName(IConsole::IResult *pResult, void *pUserData);
	static void ConCloneHold(IConsole::IResult *pResult, void *pUserData);
	static void ConCloneHammer(IConsole::IResult *pResult, void *pUserData);
	static void ConCloneDistance(IConsole::IResult *pResult, void *pUserData);

	// Rainbow Tee
	static void ConRainbowTeeEnabled(IConsole::IResult *pResult, void *pUserData);

	// Rainbow Body
	static void ConRainbowBodyEnabled(IConsole::IResult *pResult, void *pUserData);

	// Rainbow Feet
	static void ConRainbowFeetEnabled(IConsole::IResult *pResult, void *pUserData);

	// Rainbow Speed
	static void ConRainbowSpeed(IConsole::IResult *pResult, void *pUserData);

public:
	CMClient();
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;
	void OnConsoleInit() override;
	void OnRender() override;
	void OnMessage(int MsgType, void *pRawMsg) override;
};

#endif
