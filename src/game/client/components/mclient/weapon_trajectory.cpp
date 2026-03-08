#include "weapon_trajectory.h"

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <base/system.h>

#include <game/client/gameclient.h>
#include <game/collision.h>
#include <game/gamecore.h>
#include <game/mapitems.h>

#include <generated/protocol.h>

#include <vector>

vec4 CWeaponTrajectory::GetWeaponColor(int Weapon)
{
	switch(Weapon)
	{
	case WEAPON_GUN: return vec4(1.0f, 1.0f, 0.0f, 0.6f);        // 黄色
	case WEAPON_SHOTGUN: return vec4(1.0f, 0.6f, 0.0f, 0.6f);   // 橙色
	case WEAPON_GRENADE: return vec4(1.0f, 0.2f, 0.2f, 0.6f);   // 红色
	case WEAPON_LASER: return vec4(0.2f, 1.0f, 0.2f, 0.6f);     // 绿色
	default: return vec4(1.0f, 1.0f, 1.0f, 0.6f);                // 白色
	}
}

void CWeaponTrajectory::OnRender()
{
	if(!Config()->m_McWeaponTrajectoryEnable)
		return;

	if(!GameClient()->m_Snap.m_pLocalInfo)
		return;

	int LocalClientId = GameClient()->m_Snap.m_pLocalInfo->m_ClientId;
	if(LocalClientId < 0 || LocalClientId >= MAX_CLIENTS)
		return;

	const auto &Client = GameClient()->m_aClients[LocalClientId];
	if(Client.m_Team == TEAM_SPECTATORS)
		return;

	int CurWeapon = Client.m_Predicted.m_ActiveWeapon;

	if(CurWeapon == WEAPON_HAMMER || CurWeapon == WEAPON_NINJA)
		return;

	const CTuningParams *pTuning = GameClient()->GetTuning(0);

	vec2 PlayerPos = Client.m_Predicted.m_Pos;
	vec2 TargetVec = vec2((float)Client.m_Predicted.m_Input.m_TargetX, (float)Client.m_Predicted.m_Input.m_TargetY);
	if(length(TargetVec) < 0.001f)
		return;
	vec2 PlayerDir = normalize(TargetVec);

	// Match prediction fire start: m_Pos + Direction * m_ProximityRadius * 0.75f
	// Note: In client render code we only have the predicted CCharacterCore (no CEntity::m_ProximityRadius),
	// so use the physical tee size as an approximation.
	vec2 ProjStartPos = PlayerPos + PlayerDir * (CCharacterCore::PhysicalSize() * 0.75f);

	auto TuneZoneForPos = [this](vec2 Pos) {
		if(!GameClient()->m_GameWorld.m_WorldConfig.m_UseTuneZones)
			return 0;
		int TuneZone = Collision()->IsTune(Collision()->GetMapIndex(Pos));
		return TuneZone < 0 ? 0 : TuneZone;
	};

	const int ProjectileTuneZone = TuneZoneForPos(ProjStartPos);
	const int LaserTuneZone = TuneZoneForPos(PlayerPos);

	switch(CurWeapon)
	{
	case WEAPON_GUN:
	case WEAPON_GRENADE:
		SimulateProjectile(ProjStartPos, PlayerDir, CurWeapon, ProjectileTuneZone);
		break;
	case WEAPON_SHOTGUN:
		if(GameClient()->m_GameWorld.m_WorldConfig.m_IsVanilla)
		{
			constexpr int ShotSpread = 2;
			constexpr float aSpreading[] = {-0.185f, -0.070f, 0.0f, 0.070f, 0.185f};
			for(int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float a = angle(PlayerDir);
				a += aSpreading[i + ShotSpread];
				float v = 1.0f - (absolute(i) / (float)ShotSpread);
				const float Speed = mix((float)pTuning->m_ShotgunSpeeddiff, 1.0f, v);
				SimulateProjectile(ProjStartPos, direction(a) * Speed, CurWeapon, ProjectileTuneZone);
			}
		}
		else
			SimulateLaser(PlayerPos, PlayerDir, WEAPON_SHOTGUN, LaserTuneZone);
		break;
	case WEAPON_LASER:
		SimulateLaser(PlayerPos, PlayerDir, WEAPON_LASER, LaserTuneZone);
		break;
	}
}

void CWeaponTrajectory::SimulateProjectile(vec2 StartPos, vec2 Direction, int Weapon, int TuneZone)
{
	const CTuningParams *pTuning = GameClient()->GetTuning(TuneZone);

	float Curvature = 0.0f;
	float Speed = 0.0f;

	switch(Weapon)
	{
	case WEAPON_GRENADE:
		Curvature = pTuning->m_GrenadeCurvature;
		Speed = pTuning->m_GrenadeSpeed;
		break;
	case WEAPON_SHOTGUN:
		Curvature = pTuning->m_ShotgunCurvature;
		Speed = pTuning->m_ShotgunSpeed;
		break;
	case WEAPON_GUN:
		Curvature = pTuning->m_GunCurvature;
		Speed = pTuning->m_GunSpeed;
		break;
	default:
		return;
	}

	if(length(Direction) < 0.001f)
		return;

	std::vector<vec2> Path;
	Path.push_back(StartPos);

	int LifeTime = 0;
	switch(Weapon)
	{
	case WEAPON_GRENADE:
		LifeTime = (int)(Client()->GameTickSpeed() * pTuning->m_GrenadeLifetime);
		break;
	case WEAPON_SHOTGUN:
		LifeTime = (int)(Client()->GameTickSpeed() * pTuning->m_ShotgunLifetime);
		break;
	case WEAPON_GUN:
		LifeTime = (int)(Client()->GameTickSpeed() * pTuning->m_GunLifetime);
		break;
	default:
		return;
	}

	for(int Tick = 1; Tick <= LifeTime; Tick++)
	{
		const float PrevTime = (Tick - 1) / (float)Client()->GameTickSpeed();
		const float CurTime = Tick / (float)Client()->GameTickSpeed();
		vec2 PrevPos = CalcPos(StartPos, Direction, Curvature, Speed, PrevTime);
		vec2 CurPos = CalcPos(StartPos, Direction, Curvature, Speed, CurTime);

		vec2 ColPos;
		vec2 NewPos;
		int Collide = Collision()->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos);
		if(Collide)
		{
			Path.push_back(ColPos);
			break;
		}
		Path.push_back(CurPos);
	}

	vec4 Color = GetWeaponColor(Weapon);
	DrawTrajectory(Path, Color);
}

void CWeaponTrajectory::SimulateLaser(vec2 StartPos, vec2 Direction, int Weapon, int TuneZone)
{
	const CTuningParams *pTuning = GameClient()->GetTuning(TuneZone);
	if(length(Direction) < 0.001f)
		return;

	std::vector<vec2> Path;
	Path.push_back(StartPos);

	vec2 Pos = StartPos;
	vec2 Dir = normalize(Direction);
	float Energy = pTuning->m_LaserReach;
	int Bounces = 0;
	bool ZeroEnergyBounceInLastTick = false;

	while(Energy >= 0)
	{
		vec2 From = Pos;
		vec2 Coltile;
		vec2 To = Pos + Dir * Energy;
		int Res = Collision()->IntersectLineTeleWeapon(Pos, To, &Coltile, &To);

		if(Res)
		{
			Path.push_back(To);

			vec2 TempPos = To;
			vec2 TempDir = Dir * 4.0f;

			int OldTile = 0;
			if(Res == -1)
			{
				OldTile = Collision()->GetTile(round_to_int(Coltile.x), round_to_int(Coltile.y));
				Collision()->SetCollisionAt(round_to_int(Coltile.x), round_to_int(Coltile.y), TILE_SOLID);
			}
			Collision()->MovePoint(&TempPos, &TempDir, 1.0f, nullptr);
			if(Res == -1)
				Collision()->SetCollisionAt(round_to_int(Coltile.x), round_to_int(Coltile.y), OldTile);

			Pos = TempPos;
			if(length(TempDir) < 0.001f)
				break;
			Dir = normalize(TempDir);

			const float Distance = distance(From, Pos);
			if(Distance == 0.0f && ZeroEnergyBounceInLastTick)
			{
				Energy = -1;
			}
			else
			{
				Energy -= Distance + pTuning->m_LaserBounceCost;
			}
			ZeroEnergyBounceInLastTick = Distance == 0.0f;

			Bounces++;
			if(Bounces > pTuning->m_LaserBounceNum)
				Energy = -1;
		}
		else
		{
			Path.push_back(To);
			break;
		}
	}

	vec4 Color = GetWeaponColor(Weapon);
	DrawTrajectory(Path, Color);
}

void CWeaponTrajectory::DrawTrajectory(const std::vector<vec2> &Path, const vec4 &Color)
{
	if(Path.size() < 2)
		return;

	// 使用当前相机设置世界坐标映射
	float OldTLX, OldTLY, OldBRX, OldBRY;
	Graphics()->GetScreen(&OldTLX, &OldTLY, &OldBRX, &OldBRY);

	float Width, Height;
	Graphics()->CalcScreenParams(Graphics()->ScreenAspect(), GameClient()->m_Camera.m_Zoom, &Width, &Height);
	Graphics()->MapScreen(
		GameClient()->m_Camera.m_Center.x - Width / 2.0f,
		GameClient()->m_Camera.m_Center.y - Height / 2.0f,
		GameClient()->m_Camera.m_Center.x + Width / 2.0f,
		GameClient()->m_Camera.m_Center.y + Height / 2.0f);

	// 绘制线条
	Graphics()->TextureClear();
	Graphics()->LinesBegin();
	Graphics()->SetColor(Color.r, Color.g, Color.b, Color.a);

	std::vector<IGraphics::CLineItem> vLines;
	vLines.reserve(Path.size() - 1);

	for(size_t i = 0; i < Path.size() - 1; i++)
	{
		vLines.emplace_back(Path[i], Path[i + 1]);
	}

	Graphics()->LinesDraw(vLines.data(), vLines.size());

	Graphics()->LinesEnd();

	// 恢复之前的屏幕映射
	Graphics()->MapScreen(OldTLX, OldTLY, OldBRX, OldBRY);
}
