
// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(McName, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(McName, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(McName, ScriptName, Len, Def, Save, Desc) ;
#endif

// MClient Fun Panel - 功能面板配置

// 钩子角度辅助相关配置：辅助找到能穿过墙角缝隙的最佳钩子角度
MACRO_CONFIG_INT(McHookAngleHelper, mc_hook_angle_helper, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable hook angle helper for DDR mode")
MACRO_CONFIG_INT(McHookAngleScanRange, mc_hook_angle_scan_range, 90, 30, 180, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hook angle scan range in degrees")
MACRO_CONFIG_INT(McHookAngleScanStep, mc_hook_angle_scan_step, 2, 1, 10, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hook angle scan step in degrees")
MACRO_CONFIG_INT(McHookAngleShowScanRange, mc_hook_angle_show_scan_range, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show scan range")
MACRO_CONFIG_INT(McHookAngleShowAllAngles, mc_hook_angle_show_all_angles, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show all test angles")
MACRO_CONFIG_INT(McHookAngleShowCollision, mc_hook_angle_show_collision, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Show collision points")

// 交互选项
MACRO_CONFIG_INT(McHookAngleAutoApply, mc_hook_angle_auto_apply, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto apply best angle")

// 性能选项
MACRO_CONFIG_INT(McHookAngleUpdateRate, mc_hook_angle_update_rate, 60, 1, 120, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Update rate in Hz")

// 随机皮肤旋转相关配置：控制皮肤和颜色的随机旋转功能
MACRO_CONFIG_INT(McRandomSkinRotate, mc_random_skin_rotate, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotation")
MACRO_CONFIG_INT(McRandomSkinRotateInterval, mc_random_skin_rotate_interval, 5, 1, 60, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotation interval in seconds")
MACRO_CONFIG_INT(McRandomSkinRotateOnlyLeftClick, mc_random_skin_rotate_only_left_click, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotation only on left click")
MACRO_CONFIG_INT(McRandomSkinRotateMain, mc_random_skin_rotate_main, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate main player")
MACRO_CONFIG_INT(McRandomSkinRotateDummy, mc_random_skin_rotate_dummy, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate dummy")
MACRO_CONFIG_INT(McRandomSkinRotateMainColor, mc_random_skin_rotate_main_color, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate main player color")
MACRO_CONFIG_INT(McRandomSkinRotateDummyColor, mc_random_skin_rotate_dummy_color, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate dummy color")

// 玩家克隆相关配置：控制玩家克隆功能的触发条件和行为
MACRO_CONFIG_INT(McClonePlayer, mc_clone_player, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone player")
MACRO_CONFIG_INT(McCloneCopyName, mc_clone_copy_name, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone copy name")
MACRO_CONFIG_INT(McCloneOnHook, mc_clone_on_hook, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone on hook")
MACRO_CONFIG_INT(McCloneOnHammer, mc_clone_on_hammer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone on hammer")
MACRO_CONFIG_INT(McCloneOnDistance, mc_clone_on_distance, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone on distance")

// 彩虹Tee相关配置：控制Tee身体和脚部的彩虹渐变效果及速度
MACRO_CONFIG_INT(McRainbowTee, mc_rainbow_tee, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee")
MACRO_CONFIG_INT(McRainbowTeeBody, mc_rainbow_tee_body, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee body")
MACRO_CONFIG_INT(McRainbowTeeBodySpeed, mc_rainbow_tee_body_speed, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee body speed")
MACRO_CONFIG_INT(McRainbowTeeFeet, mc_rainbow_tee_feet, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee feet")
MACRO_CONFIG_INT(McRainbowTeeFeetSpeed, mc_rainbow_tee_feet_speed, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee feet speed")

// MClient Utility - 实用功能配置

// 武器快捷切换相关配置：控制最近使用武器的快速切换功能
MACRO_CONFIG_INT(McWeaponSwitch, mc_weapon_switch, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Weapon quick switch between last two weapons")

// 消息复读相关配置：控制聊天消息的复读功能
MACRO_CONFIG_INT(McRepeaterEnable, mc_repeater_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable message repeater")
MACRO_CONFIG_INT(McRepeaterCooldown, mc_repeater_cooldown, 2, 1, 10, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Message repeater cooldown in seconds")

// 自动加一相关配置：自动复读连续相同的消息
MACRO_CONFIG_INT(McAutoPlusOneEnable, mc_auto_plus_one_enable, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable auto plus one")
MACRO_CONFIG_INT(McAutoPlusOneCooldown, mc_auto_plus_one_cooldown, 3, 1, 10, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto plus one cooldown in seconds")

// 好友上线提醒相关配置：控制好友上线时的提醒功能
MACRO_CONFIG_INT(McFriendNotify, mc_friend_notify, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Friend online notification")
MACRO_CONFIG_INT(McFriendNotifyAutoRefresh, mc_friend_notify_auto_refresh, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto refresh server list for friend notification")
MACRO_CONFIG_INT(McFriendNotifyRefreshInterval, mc_friend_notify_refresh_interval, 30, 10, 300, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Friend notification refresh interval in seconds")
MACRO_CONFIG_INT(McFriendNotifyOffline, mc_friend_notify_offline, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Friend offline notification")
MACRO_CONFIG_INT(McFriendAutoGreet, mc_friend_auto_greet, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto greet friend when they join")
MACRO_CONFIG_STR(McFriendAutoGreetText, mc_friend_auto_greet_text, 128, "Hi {name}!", CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto greet text for friend joining")
