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

private:
    // 逻辑计时器
    float m_LastRotateTime;
};

#endif