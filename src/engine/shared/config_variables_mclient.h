
// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc) ;
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc) ;
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc) ;
#endif

// Random Skin Rotation
MACRO_CONFIG_INT(McRandomSkinRotation, mc_random_skin_rotation, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable random skin rotation")
MACRO_CONFIG_INT(McRandomSkinLeftClickOnly, mc_random_skin_left_click_only, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable random skin rotation only on left click")

// Clone
MACRO_CONFIG_INT(McCloneEnabled, mc_clone_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable clone")
MACRO_CONFIG_INT(McCloneCopyName, mc_clone_copy_name, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Copy name when cloning")
MACRO_CONFIG_INT(McCloneHold, mc_clone_hold, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable clone on hold")
MACRO_CONFIG_INT(McCloneHammer, mc_clone_hammer, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable clone on hammer")
MACRO_CONFIG_INT(McCloneDistance, mc_clone_distance, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable clone on distance")

// Rainbow Tee
MACRO_CONFIG_INT(McRainbowTeeEnabled, mc_rainbow_tee_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable rainbow tee")

// Rainbow Body
MACRO_CONFIG_INT(McRainbowBodyEnabled, mc_rainbow_body_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable rainbow body")

// Rainbow Feet
MACRO_CONFIG_INT(McRainbowFeetEnabled, mc_rainbow_feet_enabled, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable rainbow feet")

// Rainbow Speed
MACRO_CONFIG_INT(McRainbowSpeed, mc_rainbow_speed, 50, 0, 100, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Rainbow color change speed (0-100)")
