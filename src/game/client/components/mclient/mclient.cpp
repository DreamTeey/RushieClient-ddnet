#include "mclient.h"

#include <base/log.h>
#include <engine/client.h>
#include <engine/shared/config.h>

CMClient::CMClient()
{
	OnReset();
}

void CMClient::OnInit()
{
	// 初始化代码
}

void CMClient::OnConsoleInit()
{
	// 注册控制台命令
}

void CMClient::OnRender()
{
	// 渲染代码
}
