#include "MCLMenus.h"

MCLEncoder options_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder options_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param1(0, 11, ENCODER_RES_SYS);
MCLEncoder config_param2(0, 17, ENCODER_RES_SYS);

MCLEncoder config_param3(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param4(0, 17, ENCODER_RES_SYS);
MCLEncoder config_param5(0, 17, ENCODER_RES_SYS);

MenuPage system_page(&system_menu_layout, &options_param1, &options_param2);
MenuPage midi_config_page(&midiconfig_menu_layout, &config_param1, &config_param3);
MenuPage md_config_page(&mdconfig_menu_layout, &config_param1, &config_param4);
MenuPage mcl_config_page(&mclconfig_menu_layout, &config_param1, &config_param5);

