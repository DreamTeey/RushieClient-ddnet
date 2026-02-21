
// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(McName, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(McName, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(McName, ScriptName, Len, Def, Save, Desc) ;
#endif

//MClient Fun Panel
MACRO_CONFIG_INT(McRandomSkinRotate, mc_random_skin_rotate, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotation")
MACRO_CONFIG_INT(McRandomSkinRotateInterval, mc_random_skin_rotate_interval, 5, 1, 60, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotation interval in seconds")
MACRO_CONFIG_INT(McRandomSkinRotateOnlyLeftClick, mc_random_skin_rotate_only_left_click, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotation only on left click")
MACRO_CONFIG_INT(McRandomSkinRotateMain, mc_random_skin_rotate_main, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate main player")
MACRO_CONFIG_INT(McRandomSkinRotateDummy, mc_random_skin_rotate_dummy, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate dummy")
MACRO_CONFIG_INT(McRandomSkinRotateMainColor, mc_random_skin_rotate_main_color, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate main player color")
MACRO_CONFIG_INT(McRandomSkinRotateDummyColor, mc_random_skin_rotate_dummy_color, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Random skin rotate dummy color")
MACRO_CONFIG_INT(McClonePlayer, mc_clone_player, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone player")
MACRO_CONFIG_INT(McCloneCopyName, mc_clone_copy_name, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone copy name")
MACRO_CONFIG_INT(McCloneOnHook, mc_clone_on_hook, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone on hook")
MACRO_CONFIG_INT(McCloneOnHammer, mc_clone_on_hammer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone on hammer")
MACRO_CONFIG_INT(McCloneOnDistance, mc_clone_on_distance, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Clone on distance")
MACRO_CONFIG_INT(McRainbowTee, mc_rainbow_tee, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee")
MACRO_CONFIG_INT(McRainbowTeeBody, mc_rainbow_tee_body, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee body")
MACRO_CONFIG_INT(McRainbowTeeBodySpeed, mc_rainbow_tee_body_speed, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee body speed")
MACRO_CONFIG_INT(McRainbowTeeFeet, mc_rainbow_tee_feet, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee feet")
MACRO_CONFIG_INT(McRainbowTeeFeetSpeed, mc_rainbow_tee_feet_speed, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow tee feet speed")
