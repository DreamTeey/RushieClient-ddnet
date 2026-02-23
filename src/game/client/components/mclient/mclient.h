#ifndef GAME_CLIENT_COMPONENTS_MCLIENT_MCLIENT_H
#define GAME_CLIENT_COMPONENTS_MCLIENT_MCLIENT_H

#include <game/client/component.h>

class CMClient : public CComponent
{
public:
	CMClient();

	// DDNet 组件标准方法
	virtual int Sizeof() const override { return sizeof(*this); }
	virtual void OnInit() override;
	virtual void OnConsoleInit() override;
	virtual void OnRender() override;

	// 克隆人皮肤复制方法
	void CheckCloneSkin();
	void UpdateRainbow();	
	// 颜色转换函数
	int getIntFromColor(float Hue, float Sat, float LhT)
	{
		int R = round(255 * Hue);
		int G = round(255 * Sat);
		int B = round(255 * LhT);
		R = (R << 16) & 0x00FF0000;
		G = (G << 8) & 0x0000FF00;
		B = B & 0x000000FF;
		return 0xFF000000 | R | G | B;
	}
private:
	// 逻辑计时器
	float m_LastRotateTime;

	// 克隆人皮肤复制计时器
	int m_LastCloneTick;

	// 最后复制的玩家ID
	int m_LastClonedClientId;
	int64_t m_RainbowDelay;


};

#endif