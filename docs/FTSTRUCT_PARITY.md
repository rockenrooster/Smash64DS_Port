# FTStruct Parity Report

Generated from `include/ft/fighter.h` and read-only
`decomp/BattleShip-main/decomp/src/ft/fttypes.h`. The first comparison below
was produced before the layout-convergence edit and records the drift that
motivated the slice. The converged result section records the landed layout.

## Converged Result

- The `FTStruct` original-field region now follows BattleShip `fttypes.h`
  order, types, and nesting through `display_mode`.
- The source-layout region is frozen at `NDS_FTSTRUCT_SOURCE_SIZE == 2896`.
- Port-only DS/proof extensions begin after the source region at offset `2896`
  and the complete port struct is frozen at `sizeof(FTStruct) == 3012`.
- Permanent `_Static_assert` guards now cover the shared fighter fields touched
  by imported TUs: `status_id`, `motion_id`, `percent_damage`, `hitlag_tics`,
  `physics`, `coll_data`, `motion_vars`, `input`, `computer`,
  `motion_attack_id`, `stat_flags`, `attack_colls`, `damage_colls`,
  `damage_stat_flags`, `motion_scripts`, `joints`, `modelpart_status`,
  callback slots, `fog_color`, `key`, `passive_vars`, `hammer_tics`,
  `status_vars`, and the DS extension boundary.

| Field | Converged port offset | BattleShip source-layout offset |
|---|---:|---:|
| `status_id` | `36` | `36` |
| `coll_data` | `120` | `120` |
| `motion_attack_id` | `648` | `648` |
| `stat_flags` | `654` | `654` |
| `attack_colls` | `660` | `660` |
| `joints` | `2280` | `2280` |
| `modelpart_status` | `2428` | `2428` |
| `proc_update` | `2516` | `2516` |
| `proc_physics` | `2528` | `2528` |
| `proc_map` | `2532` | `2532` |
| `proc_damage` | `2540` | `2540` |
| `proc_lagupdate` | `2560` | `2560` |
| `proc_lagstart` | `2564` | `2564` |
| `proc_status` | `2572` | `2572` |
| DS extension `damage` | `2896` | n/a |

The default build remains on the guarded local `ftMain*` seam. Retesting the
fenced `NDS_IMPORT_BATTLESHIP_FTMAIN=1` path after convergence no longer showed
the earlier data-abort signature in init/wait/dash-run, but it is still not
green: Wait/Dash-run proof counters drift, and the continuous live-hit import
build still conflicts with remaining local ftMain seam definitions.

## Initial Drift Summary

- Source fields parsed: 194
- Port fields parsed: 201
- Same field name and declaration text: 107
- Same field name but drifted declaration/order/type: 71
- Original-only fields missing from the port layout: 16
- Port-only fields/extensions: 23

## Initial Important Offset Drift

| Field | Port offset | BattleShip source-layout offset |
|---|---:|---:|
| `status_id` | `56` | `36` |
| `coll_data` | `204` | `120` |
| `motion_attack_id` | `2504` | `648` |
| `stat_flags` | `2676` | `654` |
| `attack_colls` | `632` | `660` |
| `joints` | `412` | `2280` |
| `modelpart_status` | `560` | `2428` |
| `proc_update` | `2680` | `2516` |
| `proc_physics` | `2688` | `2528` |
| `proc_map` | `2692` | `2532` |
| `proc_damage` | `2708` | `2540` |
| `proc_lagupdate` | `2728` | `2560` |
| `proc_lagstart` | `2724` | `2564` |
| `proc_status` | `2700` | `2572` |

## Field Comparison In Source Order

| Source # | Source line | Source field | Source declaration | Port # | Port line | Port declaration | Classification |
|---:|---:|---|---|---:|---:|---|---|
| 1 | 978 | `next` | `FTStruct *next` |  |  |  | original-only missing |
| 2 | 979 | `fighter_gobj` | `GObj *fighter_gobj` | 2 | 1732 | `GObj *fighter_gobj` | match |
| 3 | 980 | `fkind` | `s32 fkind` | 3 | 1733 | `s32 fkind` | match |
| 4 | 981 | `team` | `u8 team` | 8 | 1739 | `u8 team` | same declaration, different order |
| 5 | 982 | `player` | `u8 player` | 9 | 1740 | `u8 player` | same declaration, different order |
| 6 | 983 | `detail_curr` | `u8 detail_curr` | 11 | 1742 | `u8 detail_curr` | same declaration, different order |
| 7 | 984 | `detail_base` | `u8 detail_base` | 12 | 1743 | `u8 detail_base` | same declaration, different order |
| 8 | 985 | `costume` | `u8 costume` | 13 | 1744 | `u8 costume` | same declaration, different order |
| 9 | 986 | `shade` | `u8 shade` | 14 | 1745 | `u8 shade` | same declaration, different order |
| 10 | 987 | `handicap` | `u8 handicap` | 16 | 1748 | `u8 handicap` | same declaration, different order |
| 11 | 988 | `level` | `u8 level` | 17 | 1749 | `u8 level` | same declaration, different order |
| 12 | 989 | `stock_count` | `s8 stock_count` | 10 | 1741 | `u8 stock_count` | drifted declaration/order/type |
| 13 | 990 | `team_order` | `u8 team_order` |  |  |  | original-only missing |
| 14 | 991 | `dl_link` | `u8 dl_link` | 45 | 1780 | `s32 dl_link` | drifted declaration/order/type |
| 15 | 992 | `player_num` | `s32 player_num` | 157 | 1909 | `s32 player_num` | same declaration, different order |
| 16 | 994 | `status_total_tics` | `u32 status_total_tics` | 23 | 1756 | `s32 status_total_tics` | drifted declaration/order/type |
| 17 | 996 | `pkind` | `s32 pkind` | 1 | 1731 | `s32 pkind` | same declaration, different order |
| 18 | 998 | `status_id` | `s32 status_id` | 21 | 1754 | `s32 status_id` | same declaration, different order |
| 19 | 999 | `motion_id` | `s32 motion_id` | 167 | 1920 | `s32 motion_id` | same declaration, different order |
| 20 | 1001 | `percent_damage` | `s32 percent_damage` | 59 | 1799 | `s32 percent_damage` | same declaration, different order |
| 21 | 1002 | `damage_resist` | `s32 damage_resist` | 60 | 1800 | `s32 damage_resist` | same declaration, different order |
| 22 | 1003 | `shield_health` | `s32 shield_health` | 61 | 1801 | `s32 shield_health` | same declaration, different order |
| 23 | 1004 | `unk_ft_0x38` | `f32 unk_ft_0x38` | 62 | 1802 | `f32 unk_ft_0x38` | same declaration, different order |
| 24 | 1005 | `unk_ft_0x3C` | `s32 unk_ft_0x3C` | 63 | 1803 | `s32 unk_ft_0x3C` | same declaration, different order |
| 25 | 1006 | `hitlag_tics` | `u32 hitlag_tics` | 64 | 1804 | `s32 hitlag_tics` | drifted declaration/order/type |
| 26 | 1007 | `lr` | `s32 lr` | 18 | 1750 | `s32 lr` | same declaration, different order |
| 27 | 1009 | `physics` | `struct FTPhysics physics` | 69 | 1810 | `FTPhysics physics` | drifted declaration/order/type |
| 28 | 1020 | `coll_data` | `MPCollData coll_data` | 53 | 1790 | `FTCollisionData coll_data` | drifted declaration/order/type |
| 29 | 1022 | `jumps_used` | `u8 jumps_used` | 71 | 1813 | `s32 jumps_used` | drifted declaration/order/type |
| 30 | 1023 | `unk_ft_0x149` | `u8 unk_ft_0x149` | 72 | 1814 | `u8 unk_ft_0x149` | same declaration, different order |
| 31 | 1024 | `ga` | `sb32 ga` | 70 | 1812 | `s32 ga` | drifted declaration/order/type |
| 32 | 1026 | `attack1_followup_frames` | `f32 attack1_followup_frames` | 143 | 1894 | `f32 attack1_followup_frames` | same declaration, different order |
| 33 | 1027 | `attack1_status_id` | `s32 attack1_status_id` | 144 | 1895 | `s32 attack1_status_id` | same declaration, different order |
| 34 | 1028 | `attack1_input_count` | `s32 attack1_input_count` | 145 | 1896 | `s32 attack1_input_count` | same declaration, different order |
| 35 | 1029 | `cliffcatch_wait` | `s32 cliffcatch_wait` | 88 | 1831 | `s32 cliffcatch_wait` | same declaration, different order |
| 36 | 1030 | `tics_since_last_z` | `s32 tics_since_last_z` | 92 | 1835 | `s32 tics_since_last_z` | same declaration, different order |
| 37 | 1031 | `acid_wait` | `s32 acid_wait` | 93 | 1836 | `s32 acid_wait` | same declaration, different order |
| 38 | 1032 | `twister_wait` | `s32 twister_wait` | 94 | 1837 | `s32 twister_wait` | same declaration, different order |
| 39 | 1033 | `tarucann_wait` | `s32 tarucann_wait` | 95 | 1838 | `s32 tarucann_wait` | same declaration, different order |
| 40 | 1034 | `damagefloor_wait` | `s32 damagefloor_wait` | 96 | 1839 | `s32 damagefloor_wait` | same declaration, different order |
| 41 | 1035 | `playertag_wait` | `s32 playertag_wait` | 190 | 1968 | `s32 playertag_wait` | same declaration, different order |
| 42 | 1037 | `card_anim_frame_id` | `s32 card_anim_frame_id` |  |  |  | original-only missing |
| 43 | 1039 | `motion_vars` | `union FTCommandVars motion_vars` | 187 | 1964 | `FTMotionVars motion_vars` | drifted declaration/order/type |
| 44 | 1064 | `is_attack_active` | `ub32 is_attack_active : 1` | 76 | 1818 | `sb32 is_attack_active` | drifted declaration/order/type |
| 45 | 1065 | `is_hitstatus_nodamage` | `ub32 is_hitstatus_nodamage : 1` | 77 | 1819 | `sb32 is_hitstatus_nodamage` | drifted declaration/order/type |
| 46 | 1066 | `is_damage_coll_modify` | `ub32 is_damage_coll_modify : 1` | 78 | 1820 | `sb32 is_damage_coll_modify` | drifted declaration/order/type |
| 47 | 1067 | `is_modelpart_modify` | `ub32 is_modelpart_modify : 1` | 79 | 1821 | `sb32 is_modelpart_modify` | drifted declaration/order/type |
| 48 | 1068 | `is_texturepart_modify` | `ub32 is_texturepart_modify : 1` | 80 | 1822 | `sb32 is_texturepart_modify` | drifted declaration/order/type |
| 49 | 1069 | `is_reflect` | `ub32 is_reflect : 1` | 73 | 1815 | `sb32 is_reflect` | drifted declaration/order/type |
| 50 | 1070 | `reflect_lr` | `s32 reflect_lr : 2` | 140 | 1890 | `s32 reflect_lr` | drifted declaration/order/type |
| 51 | 1071 | `is_absorb` | `ub32 is_absorb : 1` | 74 | 1816 | `sb32 is_absorb` | drifted declaration/order/type |
| 52 | 1072 | `absorb_lr` | `s32 absorb_lr : 2` | 141 | 1891 | `s32 absorb_lr` | drifted declaration/order/type |
| 53 | 1073 | `is_goto_attack100` | `ub32 is_goto_attack100 : 1` | 193 | 1971 | `sb32 is_goto_attack100` | drifted declaration/order/type |
| 54 | 1074 | `is_fastfall` | `ub32 is_fastfall : 1` | 156 | 1908 | `sb32 is_fastfall` | drifted declaration/order/type |
| 55 | 1075 | `is_magnify_show` | `ub32 is_magnify_show : 1` |  |  |  | original-only missing |
| 56 | 1076 | `is_limit_map_bounds` | `ub32 is_limit_map_bounds : 1` |  |  |  | original-only missing |
| 57 | 1077 | `is_invisible` | `ub32 is_invisible : 1` | 29 | 1763 | `sb32 is_invisible` | drifted declaration/order/type |
| 58 | 1078 | `is_shadow_hide` | `ub32 is_shadow_hide : 1` | 30 | 1764 | `sb32 is_shadow_hide` | drifted declaration/order/type |
| 59 | 1079 | `is_rebirth` | `ub32 is_rebirth : 1` | 36 | 1770 | `sb32 is_rebirth` | drifted declaration/order/type |
| 60 | 1080 | `is_magnify_ignore` | `ub32 is_magnify_ignore : 1` | 31 | 1765 | `sb32 is_magnify_ignore` | drifted declaration/order/type |
| 61 | 1081 | `is_playertag_hide` | `ub32 is_playertag_hide : 1` | 33 | 1767 | `sb32 is_playertag_hide` | drifted declaration/order/type |
| 62 | 1082 | `is_playertag_bossend` | `ub32 is_playertag_bossend : 1` |  |  |  | original-only missing |
| 63 | 1083 | `is_effect_skip` | `ub32 is_effect_skip : 1` | 37 | 1771 | `sb32 is_effect_skip` | drifted declaration/order/type |
| 64 | 1084 | `effect_joint_array_id` | `u32 effect_joint_array_id : 4` |  |  |  | original-only missing |
| 65 | 1085 | `is_shield` | `ub32 is_shield : 1` | 75 | 1817 | `sb32 is_shield` | drifted declaration/order/type |
| 66 | 1086 | `is_effect_attach` | `ub32 is_effect_attach : 1` | 81 | 1823 | `sb32 is_effect_attach` | drifted declaration/order/type |
| 67 | 1087 | `is_jostle_ignore` | `ub32 is_jostle_ignore : 1` | 82 | 1824 | `sb32 is_jostle_ignore` | drifted declaration/order/type |
| 68 | 1088 | `is_have_translate_scale` | `ub32 is_have_translate_scale : 1` | 32 | 1766 | `sb32 is_have_translate_scale` | drifted declaration/order/type |
| 69 | 1089 | `is_control_disable` | `ub32 is_control_disable : 1` | 35 | 1769 | `sb32 is_control_disable` | drifted declaration/order/type |
| 70 | 1090 | `is_hitstun` | `ub32 is_hitstun : 1` | 159 | 1911 | `sb32 is_hitstun` | drifted declaration/order/type |
| 71 | 1091 | `slope_contour` | `u32 slope_contour : 3` | 42 | 1776 | `s32 slope_contour` | drifted declaration/order/type |
| 72 | 1092 | `is_use_animlocks` | `ub32 is_use_animlocks : 1` | 160 | 1912 | `sb32 is_use_animlocks` | drifted declaration/order/type |
| 73 | 1093 | `is_muted` | `ub32 is_muted : 1` | 38 | 1772 | `sb32 is_muted` | drifted declaration/order/type |
| 74 | 1094 | `unk_ft_0x190_b5` | `ub32 unk_ft_0x190_b5 : 1` |  |  |  | original-only missing |
| 75 | 1095 | `is_item_show` | `ub32 is_item_show : 1` | 39 | 1773 | `sb32 is_item_show` | drifted declaration/order/type |
| 76 | 1096 | `is_cliff_hold` | `ub32 is_cliff_hold : 1` | 83 | 1825 | `sb32 is_cliff_hold` | drifted declaration/order/type |
| 77 | 1097 | `is_events_forward` | `ub32 is_events_forward : 1` | 40 | 1774 | `sb32 is_events_forward` | drifted declaration/order/type |
| 78 | 1098 | `is_ghost` | `ub32 is_ghost : 1` | 34 | 1768 | `sb32 is_ghost` | drifted declaration/order/type |
| 79 | 1099 | `is_damage_resist` | `ub32 is_damage_resist : 1` | 85 | 1827 | `sb32 is_damage_resist` | drifted declaration/order/type |
| 80 | 1100 | `is_menu_ignore` | `ub32 is_menu_ignore : 1` | 41 | 1775 | `sb32 is_menu_ignore` | drifted declaration/order/type |
| 81 | 1101 | `camera_mode` | `u32 camera_mode : 4` | 43 | 1777 | `s32 camera_mode` | drifted declaration/order/type |
| 82 | 1102 | `is_special_interrupt` | `ub32 is_special_interrupt : 1` | 189 | 1967 | `sb32 is_special_interrupt` | drifted declaration/order/type |
| 83 | 1103 | `is_ignore_dead` | `ub32 is_ignore_dead : 1` | 84 | 1826 | `sb32 is_ignore_dead` | drifted declaration/order/type |
| 84 | 1104 | `is_catchstatus` | `ub32 is_catchstatus : 1` | 135 | 1884 | `sb32 is_catchstatus` | drifted declaration/order/type |
| 85 | 1105 | `is_catch_or_capture` | `ub32 is_catch_or_capture : 1` | 136 | 1885 | `sb32 is_catch_or_capture` | drifted declaration/order/type |
| 86 | 1106 | `is_use_fogcolor` | `ub32 is_use_fogcolor : 1` | 163 | 1915 | `sb32 is_use_fogcolor` | drifted declaration/order/type |
| 87 | 1107 | `is_shield_catch` | `ub32 is_shield_catch : 1` | 137 | 1886 | `sb32 is_shield_catch` | drifted declaration/order/type |
| 88 | 1108 | `is_knockback_paused` | `ub32 is_knockback_paused : 1` | 65 | 1805 | `sb32 is_knockback_paused` | drifted declaration/order/type |
| 89 | 1110 | `capture_immune_mask` | `u8 capture_immune_mask` | 86 | 1828 | `u8 capture_immune_mask` | same declaration, different order |
| 90 | 1111 | `catch_mask` | `u8 catch_mask` | 87 | 1829 | `u8 catch_mask` | same declaration, different order |
| 91 | 1113 | `anim_desc` | `FTAnimDesc anim_desc` | 27 | 1760 | `FTAnimDesc anim_desc` | same declaration, different order |
| 92 | 1114 | `anim_vel` | `Vec3f anim_vel` | 28 | 1761 | `Vec3f anim_vel` | same declaration, different order |
| 93 | 1116 | `magnify_pos` | `Vec2f magnify_pos` |  |  |  | original-only missing |
| 94 | 1118 | `input` | `struct FTInputStruct input` | 47 | 1784 | `FTInput input` | drifted declaration/order/type |
| 95 | 1130 | `computer` | `FTComputer computer` |  |  |  | original-only missing |
| 96 | 1132 | `damage_coll_size` | `Vec2f damage_coll_size` |  |  |  | original-only missing |
| 97 | 1134 | `tap_stick_x` | `u8 tap_stick_x` | 48 | 1785 | `u8 tap_stick_x` | same declaration, different order |
| 98 | 1135 | `tap_stick_y` | `u8 tap_stick_y` | 49 | 1786 | `u8 tap_stick_y` | same declaration, different order |
| 99 | 1136 | `hold_stick_x` | `u8 hold_stick_x` | 50 | 1787 | `u8 hold_stick_x` | same declaration, different order |
| 100 | 1137 | `hold_stick_y` | `u8 hold_stick_y` | 51 | 1788 | `u8 hold_stick_y` | same declaration, different order |
| 101 | 1139 | `breakout_wait` | `s32 breakout_wait` | 89 | 1832 | `s32 breakout_wait` | same declaration, different order |
| 102 | 1140 | `breakout_lr` | `s8 breakout_lr` | 90 | 1833 | `s32 breakout_lr` | drifted declaration/order/type |
| 103 | 1141 | `breakout_ud` | `s8 breakout_ud` | 91 | 1834 | `s32 breakout_ud` | drifted declaration/order/type |
| 104 | 1143 | `shuffle_frame_index` | `u8 shuffle_frame_index` | 161 | 1913 | `s32 shuffle_frame_index` | drifted declaration/order/type |
| 105 | 1144 | `shuffle_index_max` | `u8 shuffle_index_max` | 162 | 1914 | `s32 shuffle_index_max` | drifted declaration/order/type |
| 106 | 1145 | `is_shuffle_electric` | `ub8 is_shuffle_electric` | 164 | 1916 | `sb32 is_shuffle_electric` | drifted declaration/order/type |
| 107 | 1146 | `shuffle_tics` | `u16 shuffle_tics` | 165 | 1917 | `s32 shuffle_tics` | drifted declaration/order/type |
| 108 | 1148 | `throw_gobj` | `GObj *throw_gobj` | 125 | 1872 | `GObj *throw_gobj` | same declaration, different order |
| 109 | 1149 | `throw_fkind` | `s32 throw_fkind` | 126 | 1873 | `s32 throw_fkind` | same declaration, different order |
| 110 | 1150 | `throw_team` | `u8 throw_team` | 127 | 1874 | `u8 throw_team` | same declaration, different order |
| 111 | 1151 | `throw_player` | `u8 throw_player` | 128 | 1875 | `u8 throw_player` | same declaration, different order |
| 112 | 1152 | `throw_player_num` | `s32 throw_player_num` | 129 | 1876 | `s32 throw_player_num` | same declaration, different order |
| 113 | 1154 | `motion_attack_id` | `u32 motion_attack_id` | 166 | 1919 | `s32 motion_attack_id` | drifted declaration/order/type |
| 114 | 1155 | `motion_count` | `u16 motion_count` | 174 | 1927 | `s32 motion_count` | drifted declaration/order/type |
| 115 | 1156 | `stat_flags` | `GMStatFlags stat_flags` | 178 | 1931 | `GMStatFlags stat_flags` | same declaration, different order |
| 116 | 1157 | `stat_count` | `u16 stat_count` | 176 | 1929 | `s32 stat_count` | drifted declaration/order/type |
| 117 | 1159 | `attack_colls` | `FTAttackColl attack_colls[4]` | 57 | 1796 | `FTAttackColl attack_colls[FTATTACKCOLL_NUM_MAX]` | drifted declaration/order/type |
| 118 | 1161 | `invincible_tics` | `s32 invincible_tics` | 119 | 1864 | `s32 invincible_tics` | same declaration, different order |
| 119 | 1162 | `intangible_tics` | `s32 intangible_tics` | 120 | 1865 | `s32 intangible_tics` | same declaration, different order |
| 120 | 1163 | `special_hitstatus` | `s32 special_hitstatus` | 124 | 1870 | `s32 special_hitstatus` | same declaration, different order |
| 121 | 1164 | `star_invincible_tics` | `s32 star_invincible_tics` | 121 | 1866 | `s32 star_invincible_tics` | match |
| 122 | 1165 | `star_hitstatus` | `s32 star_hitstatus` | 123 | 1869 | `s32 star_hitstatus` | same declaration, different order |
| 123 | 1166 | `hitstatus` | `s32 hitstatus` | 122 | 1868 | `s32 hitstatus` | same declaration, different order |
| 124 | 1168 | `damage_colls` | `FTDamageColl damage_colls[11]` | 58 | 1797 | `FTDamageColl damage_colls[FTDAMAGECOLL_NUM_MAX]` | drifted declaration/order/type |
| 125 | 1170 | `unk_ft_0x7A0` | `f32 unk_ft_0x7A0` | 154 | 1905 | `f32 unk_ft_0x7A0` | same declaration, different order |
| 126 | 1171 | `hitlag_mul` | `f32 hitlag_mul` | 152 | 1903 | `f32 hitlag_mul` | same declaration, different order |
| 127 | 1172 | `shield_heal_wait` | `f32 shield_heal_wait` | 153 | 1904 | `f32 shield_heal_wait` | same declaration, different order |
| 128 | 1173 | `unk_ft_0x7AC` | `s32 unk_ft_0x7AC` | 155 | 1906 | `s32 unk_ft_0x7AC` | same declaration, different order |
| 129 | 1175 | `attack_damage` | `s32 attack_damage` | 97 | 1841 | `s32 attack_damage` | same declaration, different order |
| 130 | 1176 | `attack_knockback` | `f32 attack_knockback` | 146 | 1897 | `f32 attack_knockback` | same declaration, different order |
| 131 | 1177 | `attack_count` | `u16 attack_count` | 98 | 1842 | `s32 attack_count` | drifted declaration/order/type |
| 132 | 1178 | `attack_shield_push` | `s32 attack_shield_push` | 99 | 1843 | `s32 attack_shield_push` | same declaration, different order |
| 133 | 1179 | `attack_rebound` | `f32 attack_rebound` | 147 | 1898 | `f32 attack_rebound` | same declaration, different order |
| 134 | 1180 | `hit_lr` | `s32 hit_lr` | 100 | 1844 | `s32 hit_lr` | same declaration, different order |
| 135 | 1181 | `shield_damage` | `s32 shield_damage` | 101 | 1845 | `s32 shield_damage` | same declaration, different order |
| 136 | 1182 | `shield_damage_total` | `s32 shield_damage_total` | 102 | 1846 | `s32 shield_damage_total` | same declaration, different order |
| 137 | 1183 | `shield_lr` | `s32 shield_lr` | 103 | 1847 | `s32 shield_lr` | same declaration, different order |
| 138 | 1184 | `shield_player` | `s32 shield_player` | 104 | 1848 | `s32 shield_player` | same declaration, different order |
| 139 | 1185 | `reflect_damage` | `s32 reflect_damage` | 142 | 1892 | `s32 reflect_damage` | same declaration, different order |
| 140 | 1186 | `damage_lag` | `s32 damage_lag` | 105 | 1849 | `s32 damage_lag` | same declaration, different order |
| 141 | 1187 | `damage_knockback` | `f32 damage_knockback` | 151 | 1902 | `f32 damage_knockback` | same declaration, different order |
| 142 | 1188 | `knockback_resist_passive` | `f32 knockback_resist_passive` | 150 | 1901 | `f32 knockback_resist_passive` | same declaration, different order |
| 143 | 1189 | `knockback_resist_status` | `f32 knockback_resist_status` | 149 | 1900 | `f32 knockback_resist_status` | same declaration, different order |
| 144 | 1190 | `damage_knockback_stack` | `f32 damage_knockback_stack` | 148 | 1899 | `f32 damage_knockback_stack` | same declaration, different order |
| 145 | 1191 | `damage_queue` | `s32 damage_queue` | 106 | 1850 | `s32 damage_queue` | same declaration, different order |
| 146 | 1192 | `damage_angle` | `s32 damage_angle` | 107 | 1851 | `s32 damage_angle` | same declaration, different order |
| 147 | 1193 | `damage_element` | `s32 damage_element` | 108 | 1852 | `s32 damage_element` | same declaration, different order |
| 148 | 1194 | `damage_lr` | `s32 damage_lr` | 109 | 1853 | `s32 damage_lr` | same declaration, different order |
| 149 | 1195 | `damage_index` | `s32 damage_index` | 110 | 1854 | `s32 damage_index` | same declaration, different order |
| 150 | 1196 | `damage_joint_id` | `s32 damage_joint_id` | 118 | 1862 | `s32 damage_joint_id` | same declaration, different order |
| 151 | 1197 | `damage_player_num` | `s32 damage_player_num` | 111 | 1855 | `s32 damage_player_num` | same declaration, different order |
| 152 | 1198 | `damage_player` | `s32 damage_player` | 112 | 1856 | `s32 damage_player` | same declaration, different order |
| 153 | 1199 | `damage_count` | `u16 damage_count` | 115 | 1859 | `s32 damage_count` | drifted declaration/order/type |
| 154 | 1200 | `damage_kind` | `s32 damage_kind` | 116 | 1860 | `s32 damage_kind` | same declaration, different order |
| 155 | 1201 | `damage_heal` | `s32 damage_heal` | 117 | 1861 | `s32 damage_heal` | same declaration, different order |
| 156 | 1202 | `damage_mul` | `f32 damage_mul` | 20 | 1752 | `f32 damage_mul` | same declaration, different order |
| 157 | 1203 | `damage_object_class` | `s32 damage_object_class` | 113 | 1857 | `s32 damage_object_class` | same declaration, different order |
| 158 | 1204 | `damage_object_kind` | `s32 damage_object_kind` | 114 | 1858 | `s32 damage_object_kind` | same declaration, different order |
| 159 | 1205 | `damage_stat_flags` | `GMStatFlags damage_stat_flags` |  |  |  | original-only missing |
| 160 | 1206 | `damage_stat_count` | `u16 damage_stat_count` | 177 | 1930 | `s32 damage_stat_count` | drifted declaration/order/type |
| 161 | 1208 | `public_knockback` | `f32 public_knockback` | 158 | 1910 | `f32 public_knockback` | same declaration, different order |
| 162 | 1210 | `search_gobj` | `GObj *search_gobj` | 131 | 1878 | `GObj *search_gobj` | same declaration, different order |
| 163 | 1211 | `search_gobj_dist` | `f32 search_gobj_dist` | 132 | 1879 | `f32 search_gobj_dist` | same declaration, different order |
| 164 | 1214 | `catch_gobj` | `GObj *catch_gobj` | 133 | 1882 | `GObj *catch_gobj` | same declaration, different order |
| 165 | 1215 | `capture_gobj` | `GObj *capture_gobj` | 134 | 1883 | `GObj *capture_gobj` | same declaration, different order |
| 166 | 1217 | `throw_desc` | `FTThrowHitDesc *throw_desc` | 130 | 1877 | `FTThrowHitDesc *throw_desc` | same declaration, different order |
| 167 | 1219 | `item_gobj` | `GObj *item_gobj` | 138 | 1887 | `GObj *item_gobj` | same declaration, different order |
| 168 | 1221 | `special_coll` | `FTSpecialColl *special_coll` | 139 | 1888 | `FTSpecialColl *special_coll` | same declaration, different order |
| 169 | 1223 | `entry_pos` | `Vec3f entry_pos` |  |  |  | original-only missing |
| 170 | 1225 | `camera_zoom_frame` | `f32 camera_zoom_frame` | 46 | 1782 | `f32 camera_zoom_frame` | same declaration, different order |
| 171 | 1226 | `camera_zoom_range` | `f32 camera_zoom_range` | 52 | 1789 | `f32 camera_zoom_range` | same declaration, different order |
| 172 | 1228 | `motion_scripts` | `FTMotionScript motion_scripts[2][2]` | 169 | 1922 | `FTMotionScript motion_scripts[2][2]` | same declaration, different order |
| 173 | 1230 | `joints` | `DObj *joints[FTPARTS_JOINT_NUM_MAX]` | 54 | 1792 | `DObj *joints[nFTPartsJointNumMax]` | drifted declaration/order/type |
| 174 | 1233 | `modelpart_status` | `FTModelPartStatus modelpart_status[FTPARTS_JOINT_NUM_MAX - nFTPartsJointCommonStart]` | 55 | 1793 | `FTModelPartStatus modelpart_status[nFTPartsJointNumMax - nFTPartsJointCommonStart]` | drifted declaration/order/type |
| 175 | 1235 | `texturepart_status` | `FTTexturePartStatus texturepart_status[2]` | 56 | 1795 | `FTTexturePartStatus texturepart_status[2]` | same declaration, different order |
| 176 | 1237 | `data` | `FTData *data` | 4 | 1734 | `FTData *data` | same declaration, different order |
| 177 | 1238 | `attr` | `FTAttributes *attr` | 5 | 1735 | `FTAttributes *attr` | same declaration, different order |
| 178 | 1240 | `figatree` | `void **figatree` | 7 | 1737 | `void *figatree` | drifted declaration/order/type |
| 179 | 1241 | `figatree_heap` | `void **figatree_heap` | 6 | 1736 | `void *figatree_heap` | drifted declaration/order/type |
| 180 | 1259 | `p_sfx` | `alSoundEffect *p_sfx` | 179 | 1949 | `alSoundEffect *p_sfx` | same declaration, different order |
| 181 | 1260 | `sfx_id` | `u16 sfx_id` | 180 | 1950 | `u16 sfx_id` | same declaration, different order |
| 182 | 1261 | `p_voice` | `alSoundEffect *p_voice` | 181 | 1951 | `alSoundEffect *p_voice` | same declaration, different order |
| 183 | 1262 | `voice_id` | `u16 voice_id` | 182 | 1952 | `u16 voice_id` | same declaration, different order |
| 184 | 1263 | `p_loop_sfx` | `alSoundEffect *p_loop_sfx` | 183 | 1953 | `alSoundEffect *p_loop_sfx` | same declaration, different order |
| 185 | 1264 | `loop_sfx_id` | `u16 loop_sfx_id` | 184 | 1954 | `u16 loop_sfx_id` | same declaration, different order |
| 186 | 1266 | `colanim` | `GMColAnim colanim` | 185 | 1955 | `GMColAnim colanim` | same declaration, different order |
| 187 | 1268 | `fog_color` | `SYColorRGBA fog_color` |  |  |  | original-only missing |
| 188 | 1269 | `shade_color` | `SYColorRGBA shade_color` | 15 | 1746 | `SYColorRGBA shade_color` | same declaration, different order |
| 189 | 1271 | `key` | `FTKey key` |  |  |  | original-only missing |
| 190 | 1273 | `afterimage` | `struct FTAfterImageInfo afterimage` | 186 | 1957 | `struct afterimage` | drifted declaration/order/type |
| 191 | 1283 | `passive_vars` | `union FTPassiveVars passive_vars` | 194 | 1973 | `union passive_vars` | drifted declaration/order/type |
| 192 | 1298 | `hammer_tics` | `s32 hammer_tics` |  |  |  | original-only missing |
| 193 | 1301 | `status_vars` | `union FTStatusVars status_vars` | 188 | 1965 | `FTStatusVars status_vars` | drifted declaration/order/type |
| 194 | 1318 | `display_mode` | `s32 display_mode` | 44 | 1779 | `s32 display_mode` | same declaration, different order |

## Port-Only Fields

| Port # | Port line | Field | Declaration | Note |
|---:|---:|---|---|---|
| 19 | 1751 | `damage` | `s32 damage` | port compatibility/proof field or widened original flag |
| 22 | 1755 | `status_prev` | `s32 status_prev` | port compatibility/proof field or widened original flag |
| 24 | 1757 | `motion_frame` | `f32 motion_frame` | port compatibility/proof field or widened original flag |
| 25 | 1758 | `anim_frame` | `f32 anim_frame` | port compatibility/proof field or widened original flag |
| 26 | 1759 | `anim_speed` | `f32 anim_speed` | port compatibility/proof field or widened original flag |
| 66 | 1807 | `vel_air` | `Vec3f vel_air` | port compatibility/proof field or widened original flag |
| 67 | 1808 | `vel_ground` | `Vec3f vel_ground` | port compatibility/proof field or widened original flag |
| 68 | 1809 | `vel_push` | `Vec3f vel_push` | port compatibility/proof field or widened original flag |
| 168 | 1921 | `motion_script_id` | `s32 motion_script_id` | port compatibility/proof field or widened original flag |
| 170 | 1923 | `status_attack_id` | `s32 status_attack_id` | port compatibility/proof field or widened original flag |
| 171 | 1924 | `status_is_smash` | `s32 status_is_smash` | port compatibility/proof field or widened original flag |
| 172 | 1925 | `status_is_projectile` | `s32 status_is_projectile` | port compatibility/proof field or widened original flag |
| 173 | 1926 | `status_flags` | `u32 status_flags` | port compatibility/proof field or widened original flag |
| 175 | 1928 | `stat_attack_id` | `s32 stat_attack_id` | port compatibility/proof field or widened original flag |
| 191 | 1969 | `is_wait_status_setup` | `sb32 is_wait_status_setup` | port compatibility/proof field or widened original flag |
| 192 | 1970 | `is_wait_motion_setup` | `sb32 is_wait_motion_setup` | port compatibility/proof field or widened original flag |
| 195 | 1986 | `nds_magic` | `u32 nds_magic` | DS extension |
| 196 | 1987 | `nds_slot` | `u32 nds_slot` | DS extension |
| 197 | 1988 | `nds_joint_count` | `u32 nds_joint_count` | DS extension |
| 198 | 1989 | `nds_common_joint_count` | `u32 nds_common_joint_count` | DS extension |
| 199 | 1990 | `nds_init_mask` | `u32 nds_init_mask` | DS extension |
| 200 | 1991 | `nds_init_floor_project_attempted` | `u32 nds_init_floor_project_attempted` | DS extension |
| 201 | 1992 | `nds_init_floor_project_result` | `u32 nds_init_floor_project_result` | DS extension |

## Required Convergence

- Make the original-field region match `fttypes.h` order, types, and nesting.
- Keep port-only `nds_*` and proof-only state at the end of `FTStruct` or in a side table so original offsets cannot move.
- Replace the current active-layout static asserts with source-layout guards for the original region plus explicit guards for the extension boundary.

## FTData Parity

`include/ft/fighter.h` now matches BattleShip `src/ft/fttypes.h:86-118` for
`FTData`: source fields run from `file_main_id` at offset `0` through
`file_anim_size` at offset `116`, with total size `120` bytes on ARM9. The
previous DS seed layout only exposed `mainmotion`, `submotion`,
`p_file_shieldpose`, `p_file_mainmotion`, and `p_file_submotion`; those are now
source-layout fields, and no DS-only `FTData` extension fields are present.

Static guards in `include/ft/fighter.h` assert every `FTData` source offset so
`ft/ftdata.c` and `ft/ftmanager.c` imports cannot drift silently.
