#ifndef GAME_CLIENT_COMPONENTS_MCLIENT_WEAPON_TRAJECTORY_H
#define GAME_CLIENT_COMPONENTS_MCLIENT_WEAPON_TRAJECTORY_H

#include <game/client/component.h>
#include <engine/config.h>
#include <engine/shared/config.h>
#include <base/vmath.h>

// 配置变量全局对象
extern CConfig g_Config;

#include <vector>

// 武器弹道可视化组件
class CWeaponTrajectory : public CComponent
{
public:
	// 组件接口
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override {}
	void OnRender() override;

	// 获取当前武器颜色
	static vec4 GetWeaponColor(int Weapon);

private:
	// 模拟投射物弹道（gun, shotgun, grenade）
	void SimulateProjectile(vec2 StartPos, vec2 Direction, int Weapon, int TuneZone);

	// 模拟激光弹道（laser）
	void SimulateLaser(vec2 StartPos, vec2 Direction, int Weapon, int TuneZone);

	// 绘制线条
	void DrawTrajectory(const std::vector<vec2> &Path, const vec4 &Color);

	// 手雷专用绘制（渐变线 + 轨迹点 + 爆炸范围）
	void DrawGrenadeTrajectory(const std::vector<vec2> &Path, const vec4 &Color);
};

#endif // GAME_CLIENT_COMPONENTS_MCLIENT_WEAPON_TRAJECTORY_H
