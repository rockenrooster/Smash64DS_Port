# The outer make: the NitroFS inventory each configuration stages -- the
# relocation files (maps, stages, bonus boards, fighters, items, modes,
# interface), audio, the P4 contents and effects.
# Included in place from the Makefile (P8 G); its text is unchanged.

NDS_MAPS_RELOC_FILES := \
	reloc_fighters_common/FTEmblemSprites \
	reloc_menus/MNSelectCommon \
	reloc_menus/MNMaps \
	reloc_menus/MNCommonFonts \
	reloc_stages/GRWallpaperTrainingBlack

NDS_PUPUPU_STAGE_RELOC_FILES := \
	reloc_stages/GRPupupuMap \
	reloc_stages/StageDreamLand \
	reloc_extern_data/ExternDataBank103 \
	reloc_extern_data/ExternDataBank104 \
	reloc_extern_data/MiscDataBank152

NDS_STAGE_SCOUT_RELOC_FILES := \
	reloc_stages/GRInishieMap \
	reloc_stages/GRHyruleMap \
	reloc_stages/StageCastle \
	reloc_extern_data/ExternDataBank113

# P2-4 first stage: Yoshi's Island (Yoster), opt-in behind NDS_STAGE_YOSTER.
# Payload mirrors the Pupupu set above: map header (263), wallpaper sprite
# container (StageYoshi, file 0x5d), geometry/display (ExternDataBank111 =
# StageYosterFile2) and map nodes (MiscDataBank154 = StageYosterFile3).
# The flag itself is defaulted with the other admission flags near :722.
ifeq ($(NDS_STAGE_YOSTER),1)
NDS_YOSTER_STAGE_RELOC_FILES := \
	reloc_stages/GRYosterMap \
	reloc_stages/StageYoshi \
	reloc_extern_data/ExternDataBank110 \
	reloc_extern_data/ExternDataBank111 \
	reloc_extern_data/MiscDataBank154
else
NDS_YOSTER_STAGE_RELOC_FILES :=
endif

# P2-4 stage 2: Peach's Castle (Castle), opt-in behind NDS_STAGE_CASTLE.
# Payload mirrors the two sets above. A bank name carries the relocData file
# number in decimal and the asset id is that number in hex, which is how each
# of these was derived rather than guessed: map header 259 = 0x103
# (GRCastleMap), wallpaper sprite container 90 = 0x5a
# (MVOpeningRoomWallpaper -- the map header references
# dMVOpeningRoomWallpaper_sprite_0x26C88, so Castle borrows the opening
# movie's room wallpaper where Yoster borrows StageYoshi's; it is already
# staged unconditionally at :4138 and rowed as asset 90, so it is not
# repeated below), geometry and
# display 106 = 0x6a (ExternDataBank106 = StageCastleFile2), and map nodes
# 156 = 0x9c (MiscDataBank156 = StageCastleFile3). The sprite list
# reloc_stages/StageCastle (95 = 0x5f) is already staged unconditionally
# above.
ifeq ($(NDS_STAGE_CASTLE),1)
NDS_CASTLE_STAGE_RELOC_FILES := \
	reloc_stages/GRCastleMap \
	reloc_extern_data/ExternDataBank106 \
	reloc_extern_data/MiscDataBank156
else
NDS_CASTLE_STAGE_RELOC_FILES :=
endif

# P2-4 stage 3: Congo Jungle (Jungle), opt-in behind NDS_STAGE_JUNGLE.
# Derived by the same bank-number rule and then CHECKED against the map
# header's own extern list rather than assumed: GRJungleMap (261 = 0x105)
# declares exactly 0x5c, 0x6c and 0x9e, which are StageJungle (92),
# ExternDataBank108 (StageJungleFile2, geometry and display) and
# MiscDataBank158 (StageJungleFile3, map nodes and the cannon animations).
# All three declare no externs of their own, so the closure is complete.
ifeq ($(NDS_STAGE_JUNGLE),1)
NDS_JUNGLE_STAGE_RELOC_FILES := \
	reloc_stages/GRJungleMap \
	reloc_stages/StageJungle \
	reloc_extern_data/ExternDataBank108 \
	reloc_extern_data/MiscDataBank158
else
NDS_JUNGLE_STAGE_RELOC_FILES :=
endif

# P2-4 stage 4: Planet Zebes, checked against the map header the same way:
# GRZebesMap (257 = 0x101) declares exactly 0x59, 0x69 and 0x9d -- StageZebes
# (89), ExternDataBank105 (StageZebesFile2) and MiscDataBank157
# (StageZebesFile3) -- and none of the three declares an extern of its own.
ifeq ($(NDS_STAGE_ZEBES),1)
NDS_ZEBES_STAGE_RELOC_FILES := \
	reloc_stages/GRZebesMap \
	reloc_stages/StageZebes \
	reloc_extern_data/ExternDataBank105 \
	reloc_extern_data/MiscDataBank157
else
NDS_ZEBES_STAGE_RELOC_FILES :=
endif

# P2-4 stage 6: Saffron City. GRYamabukiMap (264 = 0x108) declares four:
# StagePokemon (94), ExternDataBank112, MiscDataBank159 and MiscDataBank160.
# It is the only stage so far whose map needs two Misc banks.
ifeq ($(NDS_STAGE_YAMABUKI),1)
NDS_YAMABUKI_STAGE_RELOC_FILES := \
	reloc_stages/GRYamabukiMap \
	reloc_stages/StagePokemon \
	reloc_extern_data/ExternDataBank112 \
	reloc_extern_data/MiscDataBank159 \
	reloc_extern_data/MiscDataBank160
else
NDS_YAMABUKI_STAGE_RELOC_FILES :=
endif

# P2-4 stage 7: Mushroom Kingdom. GRInishieMap (260 = 0x104) declares three:
# StageHyruleWallpaper (91), ExternDataBank107 and MiscDataBank155. The map
# itself is already rowed unconditionally.
ifeq ($(NDS_STAGE_INISHIE),1)
NDS_INISHIE_STAGE_RELOC_FILES := \
	reloc_stages/GRInishieMap \
	reloc_stages/StageHyruleWallpaper \
	reloc_extern_data/ExternDataBank107 \
	reloc_extern_data/MiscDataBank155
else
NDS_INISHIE_STAGE_RELOC_FILES :=
endif

# P2-4 stage 8: Sector Z. GRSectorMap (262 = 0x106) declares three:
# StageSector (99), ExternDataBank109 and MiscDataBank153.
ifeq ($(NDS_STAGE_SECTOR),1)
NDS_SECTOR_STAGE_RELOC_FILES := \
	reloc_stages/GRSectorMap \
	reloc_stages/StageSector \
	reloc_extern_data/ExternDataBank109 \
	reloc_extern_data/MiscDataBank153
else
NDS_SECTOR_STAGE_RELOC_FILES :=
endif

# P2-6 step 7 (boss): Final Destination's runtime map (Last), behind
# NDS_1P_GAME. Derived by the same bank-number rule as the stages above:
# a bank name carries the relocData file number in decimal and the asset id
# is that number in hex -- map 266 = 0x10a (GRLastMap) and geometry/display
# 114 = 0x72 (ExternDataBank114 = StageLastFile2). The map's extern table also
# names the wallpaper sprite container 96 = 0x60 (StageLastBackground, held
# by the o2r container NAMED StageYamabukiWallpaper -- the o2r's supplemental
# wallpaper names are a consistent mislabelling, see nds_reloc_assets.c): the
# native packet draws the wallpaper, but the
# extern-tree loader refuses a parent with an unrowed dependency, so it is
# staged to satisfy the fixup. No new NDS_STAGE_LAST flag: the 1P arenas
# ride the 1P Game flag. The list also carries Meta Crystal (GRMetalMap 269 =
# 0x10d, StageMetalFile2 117 = 0x75, wallpaper 98 = 0x62 held by the container
# named StageLastWallpaper),
# Small Yoshi's Island (GRYosterSmallMap 270 = 0x10e, StageYosterSmallFile2 118 =
# 0x76; its images bank 110 and the StageYoshi wallpaper are staged with Yoshi's
# Island) and Beta Dream Land (GRPupupuSmallMap 256 = 0x100, StagePupupuBeta1
# 101 = 0x65, StagePupupuBetaImages 100 = 0x64; the StageDreamLand wallpaper is
# staged since P1) and Duel Zone (GRZakoMap 268 = 0x10c, StageBattlefieldFile2
# 116 = 0x74, wallpaper 97 = 0x61 held by the container named
# StageInishieWallpaper).
ifeq ($(NDS_1P_GAME),1)
NDS_LAST_STAGE_RELOC_FILES := \
	reloc_stages/GRLastMap \
	reloc_extern_data/ExternDataBank114 \
	reloc_stages/StageYamabukiWallpaper \
	reloc_stages/GRMetalMap \
	reloc_extern_data/ExternDataBank117 \
	reloc_stages/StageLastWallpaper \
	reloc_stages/GRYosterSmallMap \
	reloc_extern_data/ExternDataBank118 \
	reloc_stages/GRPupupuSmallMap \
	reloc_extern_data/ExternDataBank101 \
	reloc_extern_data/ExternDataBank100 \
	reloc_stages/GRZakoMap \
	reloc_extern_data/ExternDataBank116 \
	reloc_stages/StageInishieWallpaper
else
NDS_LAST_STAGE_RELOC_FILES :=
endif

# P2-6 steps 5/6: the 25 bonus boards' maps, geometry banks, the four shared
# image banks, the shared wallpaper 119 (o2r container StageMetalWallpaper),
# Race's two banks, the target model file 150, the target's attribute file
# 253 (llITBonus1ObjectHeaderFileID; its one extern is 150) and Kirby's copy
# table 230.
# Same bank-number rule as above; reloc_backend_assets.c carries the ids.
ifeq ($(NDS_1P_GAME),1)
NDS_BONUS_RELOC_FILES := \
	reloc_stages/StageMetalWallpaper \
	reloc_extern_data/ExternDataBank120 \
	reloc_extern_data/ExternDataBank121 \
	reloc_extern_data/ExternDataBank122 \
	reloc_extern_data/ExternDataBank123 \
	reloc_stages/GRBonus1MarioMap \
	reloc_extern_data/ExternDataBank124 \
	reloc_stages/GRBonus1FoxMap \
	reloc_extern_data/ExternDataBank125 \
	reloc_stages/GRBonus1DonkeyMap \
	reloc_extern_data/ExternDataBank126 \
	reloc_stages/GRBonus1SamusMap \
	reloc_extern_data/ExternDataBank127 \
	reloc_stages/GRBonus1LuigiMap \
	reloc_extern_data/ExternDataBank128 \
	reloc_stages/GRBonus1LinkMap \
	reloc_extern_data/ExternDataBank129 \
	reloc_stages/GRBonus1YoshiMap \
	reloc_extern_data/ExternDataBank130 \
	reloc_stages/GRBonus1CaptainMap \
	reloc_extern_data/ExternDataBank131 \
	reloc_stages/GRBonus1KirbyMap \
	reloc_extern_data/ExternDataBank132 \
	reloc_stages/GRBonus1PikachuMap \
	reloc_extern_data/ExternDataBank133 \
	reloc_stages/GRBonus1PurinMap \
	reloc_extern_data/ExternDataBank134 \
	reloc_stages/GRBonus1NessMap \
	reloc_extern_data/ExternDataBank135 \
	reloc_stages/GRBonus2MarioMap \
	reloc_bonus/BonusDataBank137 \
	reloc_stages/GRBonus2FoxMap \
	reloc_bonus/BonusDataBank138 \
	reloc_stages/GRBonus2DonkeyMap \
	reloc_bonus/BonusDataBank139 \
	reloc_stages/GRBonus2SamusMap \
	reloc_bonus/BonusDataBank140 \
	reloc_stages/GRBonus2LuigiMap \
	reloc_bonus/BonusDataBank141 \
	reloc_stages/GRBonus2LinkMap \
	reloc_bonus/BonusDataBank142 \
	reloc_stages/GRBonus2YoshiMap \
	reloc_bonus/BonusDataBank143 \
	reloc_stages/GRBonus2CaptainMap \
	reloc_bonus/BonusDataBank144 \
	reloc_stages/GRBonus2KirbyMap \
	reloc_bonus/BonusDataBank145 \
	reloc_stages/GRBonus2PikachuMap \
	reloc_bonus/BonusDataBank146 \
	reloc_stages/GRBonus2PurinMap \
	reloc_bonus/BonusDataBank147 \
	reloc_stages/GRBonus2NessMap \
	reloc_bonus/BonusDataBank148 \
	reloc_stages/GRBonus3Map \
	reloc_bonus/BonusDataBank149 \
	reloc_extern_data/MiscData162 \
	reloc_bonus/BonusDataBank150 \
	reloc_extern_data/MiscData253 \
	reloc_extern_data/MiscData230
else
NDS_BONUS_RELOC_FILES :=
endif

NDS_MARIOFOX_FIGHTER_RELOC_FILES := \
	reloc_fighters_common/FTManagerCommon \
	reloc_fighters_main/MarioMain \
	reloc_fighters_main/MarioMainMotion \
	reloc_fighters_main/MarioModel \
	reloc_fighters_main/MarioShieldPose \
	reloc_fighters_main/MarioSpecial1 \
	reloc_fighters_main/MarioSpecial2 \
	reloc_fighters_main/MarioSpecial3 \
	reloc_fighters_main/FoxMain \
	reloc_fighters_main/FoxMainMotion \
	reloc_fighters_main/FoxModel \
	reloc_fighters_main/FoxShieldPose \
	reloc_fighters_main/FoxSpecial1 \
	reloc_fighters_main/FoxSpecial2 \
	reloc_fighters_main/FoxSpecial3 \
	reloc_fighters_main/FoxSpecial4 \
	reloc_extern_data/MiscData201 \
	reloc_extern_data/MiscData299 \
	reloc_extern_data/MiscData315 \
	reloc_extern_data/ExternDataBank109 \
	reloc_animations/FTMarioAnimWait \
	reloc_animations/FTMarioAnim001 \
	reloc_animations/FTMarioAnim002 \
	reloc_animations/FTMarioAnim003 \
	reloc_animations/FTMarioAnim004 \
	reloc_animations/FTMarioAnim005 \
	reloc_animations/FTMarioAnim006 \
	reloc_animations/FTMarioAnim007 \
	reloc_animations/FTMarioAnim008 \
	reloc_animations/FTMarioAnim009 \
	reloc_animations/FTMarioAnim010 \
	reloc_animations/FTMarioAnim011 \
	reloc_animations/FTMarioAnim012 \
	reloc_animations/FTMarioAnim013 \
	reloc_animations/FTMarioAnim014 \
	reloc_animations/FTMarioAnim015 \
	reloc_animations/FTMarioAnim016 \
	reloc_animations/FTMarioAnim017 \
	reloc_animations/FTMarioAnim018 \
	reloc_animations/FTMarioAnim019 \
	reloc_animations/FTMarioAnim020 \
	reloc_animations/FTMarioAnim021 \
	reloc_animations/FTMarioAnim022 \
	reloc_animations/FTMarioAnim023 \
	reloc_animations/FTMarioAnim024 \
	reloc_animations/FTMarioAnim025 \
	reloc_animations/FTMarioAnim026 \
	reloc_animations/FTMarioAnim027 \
	reloc_animations/FTMarioAnim028 \
	reloc_animations/FTMarioAnim029 \
	reloc_animations/FTMarioAnim030 \
	reloc_animations/FTMarioAnim031 \
	reloc_animations/FTMarioAnim032 \
	reloc_animations/FTMarioAnim033 \
	reloc_animations/FTMarioAnim034 \
	reloc_animations/FTMarioAnim035 \
	reloc_animations/FTMarioAnim036 \
	reloc_animations/FTMarioAnim037 \
	reloc_animations/FTMarioAnim038 \
	reloc_animations/FTMarioAnim039 \
	reloc_animations/FTMarioAnim040 \
	reloc_animations/FTMarioAnim041 \
	reloc_animations/FTMarioAnim042 \
	reloc_animations/FTMarioAnim043 \
	reloc_animations/FTMarioAnimDownBounceD \
	reloc_animations/FTMarioAnim045 \
	reloc_animations/FTMarioAnimDownStandD \
	reloc_animations/FTMarioAnim047 \
	reloc_animations/FTMarioAnim048 \
	reloc_animations/FTMarioAnim049 \
	reloc_animations/FTMarioAnim050 \
	reloc_animations/FTMarioAnim051 \
	reloc_animations/FTMarioAnim052 \
	reloc_animations/FTMarioAnim053 \
	reloc_animations/FTMarioAnim054 \
	reloc_animations/FTMarioAnim055 \
	reloc_animations/FTMarioAnim056 \
	reloc_animations/FTMarioAnim057 \
	reloc_animations/FTMarioAnim058 \
	reloc_animations/FTMarioAnim059 \
	reloc_animations/FTMarioAnim060 \
	reloc_animations/FTMarioAnim061 \
	reloc_animations/FTMarioAnim062 \
	reloc_animations/FTMarioAnim063 \
	reloc_animations/FTMarioAnim064 \
	reloc_animations/FTMarioAnim065 \
	reloc_animations/FTMarioAnim066 \
	reloc_animations/FTMarioAnim067 \
	reloc_animations/FTMarioAnim068 \
	reloc_animations/FTMarioAnim069 \
	reloc_animations/FTMarioAnim070 \
	reloc_animations/FTMarioAnim071 \
	reloc_animations/FTMarioAnim072 \
	reloc_animations/FTMarioAnim073 \
	reloc_animations/FTMarioAnim074 \
	reloc_animations/FTMarioAnim075 \
	reloc_animations/FTMarioAnim076 \
	reloc_animations/FTMarioAnim077 \
	reloc_animations/FTMarioAnim078 \
	reloc_animations/FTMarioAnim079 \
	reloc_animations/FTMarioAnim080 \
	reloc_animations/FTMarioAnim081 \
	reloc_animations/FTMarioAnim082 \
	reloc_animations/FTMarioAnim083 \
	reloc_animations/FTMarioAnim084 \
	reloc_animations/FTMarioAnim085 \
	reloc_animations/FTMarioAnim086 \
	reloc_animations/FTMarioAnim087 \
	reloc_animations/FTMarioAnim088 \
	reloc_animations/FTMarioAnim089 \
	reloc_animations/FTMarioAnim090 \
	reloc_animations/FTMarioAnim091 \
	reloc_animations/FTMarioAnim092 \
	reloc_animations/FTMarioAnim093 \
	reloc_animations/FTMarioAnim094 \
	reloc_animations/FTMarioAnim095 \
	reloc_animations/FTMarioAnim096 \
	reloc_animations/FTMarioAnim097 \
	reloc_animations/FTMarioAnim098 \
	reloc_animations/FTMarioAnim099 \
	reloc_animations/FTMarioAnim100 \
	reloc_animations/FTMarioAnim101 \
	reloc_animations/FTMarioAnim102 \
	reloc_animations/FTMarioAnim103 \
	reloc_animations/FTMarioAnim104 \
	reloc_animations/FTMarioAnim105 \
	reloc_animations/FTMarioAnim106 \
	reloc_animations/FTMarioAnim107 \
	reloc_animations/FTMarioAnim108 \
	reloc_animations/FTMarioAnim109 \
	reloc_animations/FTMarioAnim110 \
	reloc_animations/FTMarioAnim111 \
	reloc_animations/FTMarioAnim112 \
	reloc_animations/FTMarioAnim113 \
	reloc_animations/FTMarioAnim114 \
	reloc_animations/FTMarioAnim115 \
	reloc_animations/FTMarioAnim116 \
	reloc_animations/FTMarioAnim117 \
	reloc_animations/FTMarioAnim118 \
	reloc_animations/FTMarioAnim119 \
	reloc_animations/FTMarioAnim120 \
	reloc_animations/FTMarioAnim121 \
	reloc_animations/FTMarioAnim122 \
	reloc_animations/FTMarioAnim123 \
	reloc_animations/FTMarioAnim124 \
	reloc_animations/FTMarioAnim125 \
	reloc_animations/FTMarioAnim126 \
	reloc_animations/FTMarioAnim127 \
	reloc_animations/FTMarioAnim128 \
	reloc_animations/FTMarioAnim129 \
	reloc_animations/FTMarioAnim130 \
	reloc_animations/FTMarioAnim131 \
	reloc_animations/FTMarioAnim132 \
	reloc_animations/FTMarioAnim133 \
	reloc_animations/FTMarioAnim134 \
	reloc_animations/FTMarioAnim135 \
	reloc_animations/FTMarioAnim136 \
	reloc_animations/FTMarioAnim137 \
	reloc_animations/FTMarioAnim138 \
	reloc_animations/FTMarioAnim139 \
	reloc_animations/FTMarioAnim140 \
	reloc_animations/FTMarioAnim141 \
	reloc_animations/FTMarioAnim142 \
	reloc_animations/FTFoxAnim000 \
	reloc_animations/FTFoxAnim001 \
	reloc_animations/FTFoxAnim002 \
	reloc_animations/FTFoxAnim003 \
	reloc_animations/FTFoxAnim004 \
	reloc_animations/FTFoxAnim005 \
	reloc_animations/FTFoxAnim006 \
	reloc_animations/FTFoxAnim007 \
	reloc_animations/FTFoxAnim008 \
	reloc_animations/FTFoxAnim009 \
	reloc_animations/FTFoxAnim010 \
	reloc_animations/FTFoxAnim011 \
	reloc_animations/FTFoxAnim012 \
	reloc_animations/FTFoxAnim013 \
	reloc_animations/FTFoxAnim014 \
	reloc_animations/FTFoxAnim015 \
	reloc_animations/FTFoxAnim016 \
	reloc_animations/FTFoxAnim017 \
	reloc_animations/FTFoxAnim018 \
	reloc_animations/FTFoxAnim019 \
	reloc_animations/FTFoxAnim020 \
	reloc_animations/FTFoxAnim021 \
	reloc_animations/FTFoxAnim022 \
	reloc_animations/FTFoxAnim023 \
	reloc_animations/FTFoxAnim024 \
	reloc_animations/FTFoxAnim025 \
	reloc_animations/FTFoxAnim026 \
	reloc_animations/FTFoxAnim027 \
	reloc_animations/FTFoxAnim028 \
	reloc_animations/FTFoxAnim029 \
	reloc_animations/FTFoxAnim030 \
	reloc_animations/FTFoxAnim031 \
	reloc_animations/FTFoxAnim032 \
	reloc_animations/FTFoxAnim033 \
	reloc_animations/FTFoxAnim034 \
	reloc_animations/FTFoxAnim035 \
	reloc_animations/FTFoxAnim036 \
	reloc_animations/FTFoxAnim037 \
	reloc_animations/FTFoxAnim038 \
	reloc_animations/FTFoxAnim039 \
	reloc_animations/FTFoxAnim040 \
	reloc_animations/FTFoxAnim041 \
	reloc_animations/FTFoxAnim042 \
	reloc_animations/FTFoxAnim043 \
	reloc_animations/FTFoxAnim044 \
	reloc_animations/FTFoxAnim045 \
	reloc_animations/FTFoxAnim046 \
	reloc_animations/FTFoxAnim047 \
	reloc_animations/FTFoxAnim048 \
	reloc_animations/FTFoxAnim049 \
	reloc_animations/FTFoxAnim050 \
	reloc_animations/FTFoxAnim051 \
	reloc_animations/FTFoxAnim052 \
	reloc_animations/FTFoxAnim053 \
	reloc_animations/FTFoxAnim054 \
	reloc_animations/FTFoxAnim055 \
	reloc_animations/FTFoxAnim056 \
	reloc_animations/FTFoxAnim057 \
	reloc_animations/FTFoxAnim058 \
	reloc_animations/FTFoxAnim059 \
	reloc_animations/FTFoxAnim060 \
	reloc_animations/FTFoxAnim061 \
	reloc_animations/FTFoxAnim062 \
	reloc_animations/FTFoxAnim063 \
	reloc_animations/FTFoxAnim064 \
	reloc_animations/FTFoxAnim065 \
	reloc_animations/FTFoxAnim066 \
	reloc_animations/FTFoxAnim067 \
	reloc_animations/FTFoxAnim068 \
	reloc_animations/FTFoxAnim069 \
	reloc_animations/FTFoxAnim070 \
	reloc_animations/FTFoxAnim071 \
	reloc_animations/FTFoxAnim072 \
	reloc_animations/FTFoxAnim073 \
	reloc_animations/FTFoxAnim074 \
	reloc_animations/FTFoxAnim075 \
	reloc_animations/FTFoxAnim076 \
	reloc_animations/FTFoxAnim077 \
	reloc_animations/FTFoxAnim078 \
	reloc_animations/FTFoxAnim079 \
	reloc_animations/FTFoxAnim080 \
	reloc_animations/FTFoxAnim081 \
	reloc_animations/FTFoxAnim082 \
	reloc_animations/FTFoxAnim083 \
	reloc_animations/FTFoxAnim084 \
	reloc_animations/FTFoxAnim085 \
	reloc_animations/FTFoxAnim086 \
	reloc_animations/FTFoxAnim087 \
	reloc_animations/FTFoxAnim088 \
	reloc_animations/FTFoxAnim089 \
	reloc_animations/FTFoxAnim090 \
	reloc_animations/FTFoxAnim091 \
	reloc_animations/FTFoxAnim092 \
	reloc_animations/FTFoxAnim093 \
	reloc_animations/FTFoxAnim094 \
	reloc_animations/FTFoxAnim095 \
	reloc_animations/FTFoxAnim096 \
	reloc_animations/FTFoxAnim097 \
	reloc_animations/FTFoxAnim098 \
	reloc_animations/FTFoxAnim099 \
	reloc_animations/FTFoxAnim100 \
	reloc_animations/FTFoxAnim101 \
	reloc_animations/FTFoxAnim102 \
	reloc_animations/FTFoxAnim103 \
	reloc_animations/FTFoxAnim104 \
	reloc_animations/FTFoxAnim105 \
	reloc_animations/FTFoxAnim106 \
	reloc_animations/FTFoxAnim107 \
	reloc_animations/FTFoxAnim108 \
	reloc_animations/FTFoxAnim109 \
	reloc_animations/FTFoxAnim110 \
	reloc_animations/FTFoxAnim111 \
	reloc_animations/FTFoxAnim112 \
	reloc_animations/FTFoxAnim113 \
	reloc_animations/FTFoxAnim114 \
	reloc_animations/FTFoxAnim115 \
	reloc_animations/FTFoxAnim116 \
	reloc_animations/FTFoxAnim117 \
	reloc_animations/FTFoxAnim118 \
	reloc_animations/FTFoxAnim119 \
	reloc_animations/FTFoxAnim120 \
	reloc_animations/FTFoxAnim121 \
	reloc_animations/FTFoxAnim122 \
	reloc_animations/FTFoxAnim123 \
	reloc_animations/FTFoxAnim124 \
	reloc_animations/FTFoxAnim125 \
	reloc_animations/FTFoxAnim126 \
	reloc_animations/FTFoxAnim127 \
	reloc_animations/FTFoxAnim128 \
	reloc_animations/FTFoxAnim129 \
	reloc_animations/FTFoxAnim130 \
	reloc_animations/FTFoxAnim131 \
	reloc_animations/FTFoxAnim132 \
	reloc_animations/FTFoxAnim133 \
	reloc_animations/FTFoxAnim134 \
	reloc_animations/FTFoxAnim135 \
	reloc_animations/FTFoxAnim136 \
	reloc_animations/FTFoxAnim137 \
	reloc_animations/FTFoxAnim138 \
	reloc_animations/FTFoxAnim139 \
	reloc_animations/FTFoxAnim140 \
	reloc_animations/FTFoxAnim141 \
	reloc_animations/FTFoxAnim142 \
	reloc_animations/FTFoxAnim143 \
	reloc_animations/FTFoxAnim144 \
	reloc_animations/FTFoxAnim145 \
	reloc_animations/FTFoxAnim146 \
	reloc_animations/FTFoxAnim147 \
	reloc_animations/FTFoxAnim148 \
	reloc_animations/FTFoxAnim149 \
	reloc_animations/FTFoxAnim150 \
	reloc_animations/FTFoxAnim151 \
	reloc_animations/FTFoxAnim152 \
	reloc_animations/FTFoxAnim153 \
	reloc_animations/FTFoxAnim154 \
	reloc_animations/FTFoxAnim155 \
	reloc_animations/FTFoxAnim156 \
	reloc_animations/FTFoxAnim157

# P2-3 appends only the files a new fighter adds beyond Mario/Fox.  The fragment
# is generated from BattleShip FTData + generated relocData names + O2R headers;
# no fighter file inventory belongs in this Makefile from this point forward.
include $(PROJECT_ROOT)/scripts/fighters/fighter_production_files.mk
NDS_FIGHTER_RELOC_FILES :=
ifeq ($(NDS_LUIGI),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_LUIGI_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_DONKEY),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_DONKEY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_CAPTAIN),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_CAPTAIN_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_SAMUS),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_SAMUS_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_LINK),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_LINK_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_PIKACHU),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_PIKACHU_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_YOSHI),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_YOSHI_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NESS),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NESS_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_PURIN),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_PURIN_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_KIRBY),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_KIRBY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_GDONKEY),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_GDONKEY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_MMARIO),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_MMARIO_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NMARIO),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NMARIO_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NFOX),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NFOX_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NDONKEY),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NDONKEY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NSAMUS),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NSAMUS_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NLUIGI),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NLUIGI_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NLINK),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NLINK_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NYOSHI),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NYOSHI_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NCAPTAIN),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NCAPTAIN_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NKIRBY),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NKIRBY_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NPIKACHU),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NPIKACHU_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NPURIN),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NPURIN_FIGHTER_RELOC_FILES)
endif
ifeq ($(NDS_NNESS),1)
NDS_FIGHTER_RELOC_FILES += $(NDS_NNESS_FIGHTER_RELOC_FILES)
endif

# BPS1 replaces these AObj16 O2R payloads rather than duplicating them. Keeping
# both crosses the four-CPU ROM's measured 16 MiB runner boundary; more
# importantly, every retained source file would be dead ROM once the verified
# compact clip is the authoritative acquisition path. AObj32 entry/effect files
# remain on the ordinary reloc loader and are restored after the prefix filter.
NDS_FTANIM_STREAM_REPLACED_RELOC_FILES :=
NDS_FTANIM_STREAM_PATTERNS := \
	reloc_animations/FTMarioAnim% \
	reloc_animations/FTFoxAnim% \
	reloc_animations/FTLuigiAnim% \
	reloc_animations/FTDonkeyAnim% \
	reloc_animations/FTSamusAnim% \
	reloc_animations/FTLinkAnim% \
	reloc_animations/FTKirbyAnim% \
	reloc_animations/FTKirbyCopyAnim% \
	reloc_animations/FTCaptainAnim% \
	reloc_animations/FTPurinAnim% \
	reloc_animations/FTNessAnim% \
	reloc_animations/FTYoshiAnim% \
	reloc_animations/FTPikachuAnim%
NDS_FTANIM_STREAM_AOBJ32_FILES := \
	reloc_animations/FTMarioAnim134 \
	reloc_animations/FTMarioAnim135 \
	reloc_animations/FTFoxAnim135 \
	reloc_animations/FTFoxAnim136 \
	reloc_animations/FTDonkeyAnim132 \
	reloc_animations/FTDonkeyAnim133 \
	reloc_animations/FTSamusAnim137 \
	reloc_animations/FTSamusAnim138 \
	reloc_animations/FTLinkAnim131 \
	reloc_animations/FTLinkAnim132 \
	reloc_animations/FTKirbyAnim153 \
	reloc_animations/FTKirbyAnim154 \
	reloc_animations/FTCaptainAnim136 \
	reloc_animations/FTCaptainAnim137 \
	reloc_animations/FTCaptainAnim138 \
	reloc_animations/FTCaptainAnim139 \
	reloc_animations/FTPurinAnim001 \
	reloc_animations/FTPurinAnim002 \
	reloc_animations/FTNessAnim135 \
	reloc_animations/FTNessAnim136 \
	reloc_animations/FTNessAnim137 \
	reloc_animations/FTNessAnim138 \
	reloc_animations/FTNessAnim139 \
	reloc_animations/FTYoshiAnim138 \
	reloc_animations/FTYoshiAnim139 \
	reloc_animations/FTPikachuAnim124 \
	reloc_animations/FTPikachuAnim125
# These AObj16 clips carry embedded SYInterpDesc data. BPS1 cannot encode
# that pointer-bearing layout; the source-command census routes them through
# the original O2R loader and checks this retention list.
NDS_FTANIM_STREAM_SPLINE_FILES := \
	reloc_animations/FTSamusAnim060 \
	reloc_animations/FTSamusAnim061
NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL := $(NDS_MARIOFOX_FIGHTER_RELOC_FILES)
NDS_FIGHTER_RELOC_FILES_FULL := $(NDS_FIGHTER_RELOC_FILES)
NDS_FTANIM_STREAM_REPLACED_RELOC_FILES := $(filter \
	$(NDS_FTANIM_STREAM_PATTERNS),$(NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL) \
	$(NDS_FIGHTER_RELOC_FILES_FULL))
NDS_MARIOFOX_FIGHTER_RELOC_FILES := $(filter-out \
	$(NDS_FTANIM_STREAM_PATTERNS),$(NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL)) \
	$(filter $(NDS_FTANIM_STREAM_AOBJ32_FILES) $(NDS_FTANIM_STREAM_SPLINE_FILES),$(NDS_MARIOFOX_FIGHTER_RELOC_FILES_FULL))
NDS_FIGHTER_RELOC_FILES := $(filter-out \
	$(NDS_FTANIM_STREAM_PATTERNS),$(NDS_FIGHTER_RELOC_FILES_FULL)) \
	$(filter $(NDS_FTANIM_STREAM_AOBJ32_FILES) $(NDS_FTANIM_STREAM_SPLINE_FILES),$(NDS_FIGHTER_RELOC_FILES_FULL))

NDS_EFFECT_RELOC_FILES := \
	reloc_effects/EFCommonEffects1 \
	reloc_effects/EFCommonEffects2 \
	reloc_effects/EFCommonEffects3

# P2-3f48. The item subsystem's shared data file, and the one file its 68
# external pointers all resolve into. ITCommonData carries the descriptors,
# models, textures and animation for every common, monster and stage item, so
# the whole of P2-5 waits on it being resident; it is 3,392 bytes of payload.
# MiscData086 is 79,584 and Yoshi's own reloc closure already stages it, which
# is why this pair costs 3,392 bytes on a ROM carrying him and 82,976 on one
# that is not. Duplicated staging is harmless -- the NitroFS copy rule is
# idempotent and the asset table returns the first matching row.
NDS_ITEM_RELOC_FILES := \
	reloc_items/ITCommonData \
	reloc_extern_data/MiscData086 \
	reloc_interface/IFCommonItem

NDS_1P_RELOC_FILES := \
	reloc_scene/SC1PStageClear1 \
	reloc_scene/SC1PStageClear2 \
	reloc_scene/SC1PStageClear3 \
	reloc_scene/SC1PChallenger \
	reloc_scene/SC1PIntro \
	reloc_menus/MN1P \
	reloc_menus/MN1PContinue \
	reloc_menus/MNPlayers1PMode \
	reloc_menus/MNMessage \
	reloc_scene/SCStaffroll \
	reloc_movies/MVEnding \
	reloc_menus/MNCongraCaptainBottom \
	reloc_menus/MNCongraCaptainTop \
	reloc_menus/MNCongraDonkeyBottom \
	reloc_menus/MNCongraDonkeyTop \
	reloc_menus/MNCongraFoxBottom \
	reloc_menus/MNCongraFoxTop \
	reloc_menus/MNCongraKirbyBottom \
	reloc_menus/MNCongraKirbyTop \
	reloc_menus/MNCongraLinkBottom \
	reloc_menus/MNCongraLinkTop \
	reloc_menus/MNCongraLuigiBottom \
	reloc_menus/MNCongraLuigiTop \
	reloc_menus/MNCongraMarioBottom \
	reloc_menus/MNCongraMarioTop \
	reloc_menus/MNCongraNessBottom \
	reloc_menus/MNCongraNessTop \
	reloc_menus/MNCongraPikachuBottom \
	reloc_menus/MNCongraPikachuTop \
	reloc_menus/MNCongraPurinBottom \
	reloc_menus/MNCongraPurinTop \
	reloc_menus/MNCongraSamusBottom \
	reloc_menus/MNCongraSamusTop \
	reloc_menus/MNCongraYoshiBottom \
	reloc_menus/MNCongraYoshiTop \
	reloc_menus/MNPlayersDifficulty \
	reloc_misc_named/CharacterNames \
	reloc_bonus/BonusPicture \
	reloc_bonus/BonusPicturePlatform \
	reloc_bonus/Bonus2Common \
	reloc_fighters_main/BossMain \
	reloc_fighters_main/BossMainMotion \
	reloc_fighters_main/BossModel \
	reloc_fighters_main/NMarioMain \
	reloc_fighters_main/NMarioModel \
	reloc_fighters_main/NFoxMain \
	reloc_fighters_main/NFoxModel \
	reloc_fighters_main/NDonkeyMain \
	reloc_fighters_main/NDonkeyModel \
	reloc_fighters_main/NSamusMain \
	reloc_fighters_main/NSamusModel \
	reloc_fighters_main/NLuigiMain \
	reloc_fighters_main/NLinkMain \
	reloc_fighters_main/NLinkModel \
	reloc_fighters_main/NYoshiMain \
	reloc_fighters_main/NYoshiModel \
	reloc_fighters_main/NCaptainMain \
	reloc_fighters_main/NCaptainModel \
	reloc_fighters_main/NKirbyMain \
	reloc_fighters_main/NKirbyModel \
	reloc_fighters_main/NPikachuMain \
	reloc_fighters_main/NPikachuModel \
	reloc_fighters_main/NPurinMain \
	reloc_fighters_main/NPurinModel \
	reloc_fighters_main/NNessMain \
	reloc_fighters_main/NNessModel \
	reloc_extern_data/MiscData302 \
	reloc_fighters_main/MasterHandIcon

NDS_MODES_RELOC_FILES := \
	reloc_menus/MNData \
	reloc_menus/MNDataCommon \
	reloc_menus/MNVSRecordMain \
	reloc_menus/MNCharacters \
	reloc_menus/MNOption \
	reloc_menus/MNBackupClear \
	reloc_menus/MNBackupClearHeaderOption \
	reloc_menus/MNSoundTest \
	reloc_scene/SC1PTrainingMode \
	reloc_stages/GRWallpaperTrainingBlack \
	reloc_stages/GRWallpaperTrainingBlue \
	reloc_stages/GRWallpaperTrainingYellow \
	reloc_scene/SCExplainGraphics \
	reloc_scene/SCExplainMain \
	reloc_menus/MNScreenAdjust \
	reloc_extern_data/MiscData029

NDS_VSBATTLE_RELOC_FILES := \
	reloc_interface/IFCommonPlayer \
	reloc_interface/IFCommonGameStatus \
	reloc_interface/IFCommonPlayerDamage \
	reloc_interface/IFCommonTimer \
	reloc_interface/IFCommonDigits \
	reloc_interface/IFCommonBattlePause \
	reloc_interface/IFCommonPlayerTags \
	reloc_interface/IFCommonAnnounceCommon \
	reloc_misc_named/SYKseg1Validate

NDS_VS_RESULTS_RELOC_FILES :=
NDS_VS_RESULTS_RELOC_FILES := \
	reloc_menus/MNVSResults \
	reloc_fighters_common/FTEmblemModels \
	reloc_fighters_common/FTStocksZako \
	reloc_transitions/LBTransitionAeroplane \
	reloc_transitions/LBTransitionCheck \
	reloc_transitions/LBTransitionGakubuthi \
	reloc_transitions/LBTransitionKannon \
	reloc_transitions/LBTransitionStar \
	reloc_transitions/LBTransitionSudare1 \
	reloc_transitions/LBTransitionSudare2 \
	reloc_transitions/LBTransitionCamera \
	reloc_transitions/LBTransitionBlock \
	reloc_transitions/LBTransitionRotScale \
	reloc_transitions/LBTransitionCurtain
# VS Results demo poses (BUGS.md R02/R03/K06).  Every Win/Claps animation a
# `dFT<Kind>SubMotionDescs` Results row names lives under `reloc_submotions/`,
# and nothing in any fighter's motion or extern closure reaches that directory,
# so no list staged it and `ftMainSetStatus` silently bound the stale figatree
# heap instead -- every fighter held his row-0 pose.  The file lists come from
# `fighter_production_files.mk` (included above), generated from the SAME table
# that emits the runtime token rows, so a staged file and the route that
# resolves it cannot drift apart.  They ride the existing generic
# `$(NITROFS_DIR)/reloc/%: $(BATTLESHIP_O2R)/%` rule like every other reloc
# path.  Mario and Fox are the always-compiled base pair (Mario's Claps is
# Luigi's DemoLose too); every other list is roster-gated so a build without a
# fighter does not pack his poses.
NDS_VS_RESULTS_RELOC_FILES += $(NDS_MARIOFOX_DEMO_RELOC_FILES)
ifeq ($(NDS_LUIGI),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_LUIGI_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_DONKEY),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_DONKEY_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_CAPTAIN),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_CAPTAIN_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_SAMUS),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_SAMUS_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_LINK),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_LINK_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_PIKACHU),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_PIKACHU_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_YOSHI),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_YOSHI_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_NESS),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_NESS_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_PURIN),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_PURIN_DEMO_RELOC_FILES)
endif
ifeq ($(NDS_KIRBY),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_KIRBY_DEMO_RELOC_FILES)
endif
# P2-6: the 1P scenes' demo poses -- the intro's IntroL/IntroR (team
# formations included) and the figure's dolls -- from the same generated table
# as their runtime token rows (NDS_1P_DEMO_ANIM_ASSET_ROWS).
ifeq ($(NDS_1P_GAME),1)
NDS_VS_RESULTS_RELOC_FILES += $(NDS_1P_DEMO_RELOC_FILES)
endif

NDS_STARTUP_RELOC_FILES := \
	reloc_misc_named/N64Logo

NDS_AUDIO_FILES := \
	audio/S1_music_sbk \
	audio/B1_sounds1_ctl \
	audio/B1_sounds1_tbl \
	audio/B1_sounds2_ctl \
	audio/B1_sounds2_tbl \
	audio/fgm_unk \
	audio/fgm_tbl \
	audio/fgm_ucd

NDS_AUDIO_DERIVED_FILES :=
NDS_AUDIO_DERIVED_FILES := \
	audio/bgm_pupupu_ima.bin \
	audio/bgm_win_mario_ima.bin \
	audio/bgm_win_fox_ima.bin \
	audio/bgm_results_ima.bin \
	audio/bgm_mode_select_ima.bin \
	audio/bgm_battle_select_ima.bin \
	audio/bgm_win_default_ima.bin \
	audio/bgm_win_metroid_ima.bin \
	audio/bgm_win_donkey_ima.bin \
	audio/bgm_win_kirby_ima.bin \
	audio/bgm_win_mother_ima.bin \
	audio/bgm_win_yoshi_ima.bin \
	audio/bgm_win_fzero_ima.bin \
	audio/bgm_win_pmonsters_ima.bin \
	audio/bgm_win_zelda_ima.bin
# P2-4 Yoster BGM: staged asset lands only when the stage flag is on. Like
# every BGM here the asset lives under the gitignored assets/ tree and is
# rendered offline rather than by a rule, so reproduce it with:
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 8 \
#          --output assets/audio/bgm_yoster_ima.bin
# Sequence 8 is nSYAudioBGMYoster (gm/gmsound.h:31-40, sequential from 0).
# Nested inside the BGM-import gate like every other derived BGM so flag-off
# output is identical.
ifeq ($(NDS_STAGE_YOSTER),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_yoster_ima.bin
endif
# P2-4 Castle BGM, same shape. Reproduce with:
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 6 \
#          --output assets/audio/bgm_castle_ima.bin
# Sequence 6 is nSYAudioBGMCastle (gm/gmsound.h:31-37).
ifeq ($(NDS_STAGE_CASTLE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_castle_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 1 \
#          --output assets/audio/bgm_zebes_ima.bin
ifeq ($(NDS_STAGE_ZEBES),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_zebes_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 9 \
#          --output assets/audio/bgm_hyrule_ima.bin
ifeq ($(NDS_STAGE_HYRULE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_hyrule_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 7 \
#          --output assets/audio/bgm_yamabuki_ima.bin
ifeq ($(NDS_STAGE_YAMABUKI),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_yamabuki_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 2 \
#          --output assets/audio/bgm_inishie_ima.bin
# The renderer picks its step-index Viterbi encoder for sequences 2 and 3 by
# itself (TRELLIS_IMA_SEQUENCES); no flag. 2026-09-22: IMA again after the
# PCM16 stream of 2026-09-07, on the owner's "ALL BGM should be IMA-ADPCM".
ifeq ($(NDS_STAGE_INISHIE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_inishie_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 5 \
#          --output assets/audio/bgm_jungle_ima.bin
# Held until 2026-09-03 on a doubling suspicion: its loop start sits at
# 50.06% of the track. A read of render-audio-bgm.py:271-273 settles it --
# a song whose last note runs past one period renders periods_needed=2, so
# loop_start = base + P against loop_end = base + 2P, which IS the midpoint
# when the intro is short. The six accepted tracks all render one period.
# Not a unit bug: bytes and samples convert consistently at :286-287, :334
# and :625, and the two channels agree on the period to within 80 ticks.
ifeq ($(NDS_STAGE_JUNGLE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_jungle_ima.bin
endif
# Reproduce: python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 4 \
#          --output assets/audio/bgm_sector_ima.bin
# Same two-period shape as Jungle above, loop start at 53.62%.
ifeq ($(NDS_STAGE_SECTOR),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_sector_ima.bin
endif
# Mushroom Kingdom's <=30-second source swap uses sequence 3
# (nSYAudioBGMInishieHurry), Viterbi-encoded IMA like sequence 2 through the
# same 16,392-byte ring as every track. Reproduce with:
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index 3 \
#          --output assets/audio/bgm_inishie_hurry_ima.bin
ifeq ($(NDS_STAGE_INISHIE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_inishie_hurry_ima.bin
endif
# 2026-09-05: reproduce each with
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index <gmMusicID> \
#          --output assets/audio/bgm_<name>_ima.bin (indices in nds_audio_bgm.h).
ifeq ($(NDS_ITEM_CORE),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_hammer_ima.bin \
	audio/bgm_star_ima.bin
endif
# 2026-09-05: reproduce each with
#   python scripts/sfx/bgm/render-audio-bgm.py --sequence-index <gmMusicID> \
#          --output assets/audio/bgm_<name>_ima.bin (indices in nds_audio_bgm.h).
ifeq ($(NDS_1P_GAME),1)
NDS_AUDIO_DERIVED_FILES += \
	audio/bgm_boss_stage_ima.bin \
	audio/bgm_boss_entry_ima.bin \
	audio/bgm_last_ima.bin \
	audio/bgm_bonus_stage_ima.bin \
	audio/bgm_stage_clear_ima.bin \
	audio/bgm_bonus_stage_clear_ima.bin \
	audio/bgm_game_clear_ima.bin \
	audio/bgm_bonus_stage_failure_ima.bin \
	audio/bgm_game_end_choice_ima.bin \
	audio/bgm_game_over_ima.bin \
	audio/bgm_opening_ima.bin \
	audio/bgm_explain_ima.bin \
	audio/bgm_intro_ima.bin \
	audio/bgm_zako_ima.bin \
	audio/bgm_metal_ima.bin \
	audio/bgm_ending_ima.bin \
	audio/bgm_staffroll_ima.bin \
	audio/bgm_message_ima.bin \
	audio/bgm_challenger_ima.bin \
	audio/bgm_training_mode_ima.bin \
	audio/bgm_data_ima.bin
endif

# Superseded BGM assets can survive an incremental build-directory reuse and
# are otherwise silently repacked by ndstool. The two Mushroom Kingdom PCM16
# streams (5,598,330 B) were retired 2026-09-22 for the IMA pair above.
export NDS_AUDIO_OBSOLETE_DERIVED_FILES := \
	audio/bgm_pupupu_pcm16.raw \
	audio/bgm_win_mario_pcm16.raw \
	audio/bgm_win_fox_pcm16.raw \
	audio/bgm_inishie_pcm16.raw \
	audio/bgm_inishie_hurry_pcm16.raw \
	audio/bgm_results_pcm16.raw
NDS_AUDIO_DERIVED_FILES += \
	audio/fgm_phase_pack_ima.bin

NDS_INISHIE_SCALE_RELOCDATA_FILES :=
ifeq ($(NDS_ENABLE_INISHIE_SOURCE_SCALE_SETUP),1)
NDS_INISHIE_SCALE_RELOCDATA_FILES := \
	155.vpk0.bin
endif

# P4 generated data (rules beside the import overlay's below).
export NDS_P4_GEN := $(PROJECT_ROOT)/$(BUILD)/p4
# P5 generated data (scripts/p5/p5_rules.mk).
export NDS_P5_GEN := $(PROJECT_ROOT)/$(BUILD)/p5
export NDS_NITROFS_P4_FILES :=
# The character select (scripts/p4/p4_css.py) names the compiled contents by
# their tokens (Remix names, or an authored content's NAME).
export NDS_P4_CSS_DIR := $(NDS_P4_GEN)/css
export NDS_P4_CONTENTS := $(foreach row,$(NDS_P4_ENABLED_ROWS),$(call nds_p4_field,$(row),3))
# Per content: its generated data, its O2R files and native images in NitroFS,
# and its character-select preview pack (kind NDS_P4_SEL_BASE + id = 64 + id).
export NDS_P4_NATIVE := $(NDS_P4_GEN)/native
export NDS_P4_CONTENTS_HEADER := $(NDS_P4_GEN)/nds_p4_contents.generated.h
NDS_NITROFS_P4_FILES += $(foreach row,$(NDS_P4_ENABLED_ROWS), \
	$(NITROFS_DIR)/fighters/$(call nds_p4_field,$(row),1)_high.bin \
	$(NITROFS_DIR)/fighters/$(call nds_p4_field,$(row),1)_low.bin \
	$(NITROFS_DIR)/fighters/preview/$(shell echo $$((64 + $(call nds_p4_field,$(row),4)))).fpc \
	$(NITROFS_DIR)/fighters/preview/$(shell echo $$((64 + $(call nds_p4_field,$(row),4)))).ext \
	$(NITROFS_DIR)/fighters/battle/$(shell echo $$((64 + $(call nds_p4_field,$(row),4)))).fpc \
	$(NITROFS_DIR)/fighters/shield_pose/$(shell echo $$((64 + $(call nds_p4_field,$(row),4)))).bin \
	$(NITROFS_DIR)/p4/$(call nds_p4_field,$(row),1).tab)
# The contents' Remix sounds as a second FGM pack (scripts/p4/p4_audio.py).
ifeq ($(NDS_P4),1)
export NDS_P4_AUDIO_DIR := $(NDS_P4_GEN)/audio
export NDS_P4_EXPORT_FILES := $(foreach name,$(NDS_P4_ENABLED),$(NDS_P4_EXPORT)/$(name)/resolved.json)
NDS_NITROFS_P4_FILES += $(NDS_P4_AUDIO_DIR)/.nitrofs-staged
NDS_NITROFS_P4_FILES += $(NDS_P4_GEN)/.nitrofs-staged
# Selectable Metal Mario, the source MMario kind (scripts/p4/p4_css.py KINDS):
# his VS select preview's pack (rule in scripts/p4/p4_rules.mk).
NDS_NITROFS_P4_FILES += $(NITROFS_DIR)/fighters/preview/13.fpc
endif

export NDS_NITROFS_RELOC_FILES := \
	$(foreach file,$(NDS_STARTUP_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_ROOM_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_PORTRAITS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_MARIO_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_OPENING_ACTION_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_TITLE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_VS_MODE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_PLAYERS_VS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_MAPS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_PUPUPU_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_YOSTER_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_CASTLE_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_JUNGLE_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_ZEBES_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_YAMABUKI_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_INISHIE_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_SECTOR_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_LAST_STAGE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_BONUS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_STAGE_SCOUT_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_MARIOFOX_FIGHTER_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_FIGHTER_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_EFFECT_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_ITEM_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_VSBATTLE_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_MODES_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_1P_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file)) \
	$(foreach file,$(NDS_VS_RESULTS_RELOC_FILES),$(NITROFS_DIR)/reloc/$(file))

export NDS_NITROFS_AUDIO_FILES := \
	$(foreach file,$(NDS_AUDIO_FILES),$(NITROFS_DIR)/$(file)) \
	$(foreach file,$(NDS_AUDIO_DERIVED_FILES),$(NITROFS_DIR)/$(file))

export NDS_NITROFS_RELOCDATA_FILES := \
	$(foreach file,$(NDS_INISHIE_SCALE_RELOCDATA_FILES),$(NITROFS_DIR)/relocdata/us/$(file))

export NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES :=
ifeq ($(NDS_DEV_SCENE_HARNESS_ID),163)
ifeq ($(NDS_RENDERER_HW_TRIANGLES),1)
NDS_NITROFS_BATTLE_STATIC_TEXTURE_FILES := \
	$(NITROFS_DIR)/renderer/battle_playable_static_textures.rgb5a1.bin
endif
endif

# P2-1c's UI pack ships only with the runtime that reads it. Empty by default,
# so a ROM built without NDS_UI_KIT does not carry 24,384 bytes nothing
# opens -- which is what makes the published ROMs byte-identical across this
# row.
export NDS_NITROFS_MN_UI_KIT_FILES :=
ifeq ($(NDS_UI_KIT),1)
NDS_NITROFS_MN_UI_KIT_FILES := $(NITROFS_DIR)/menus/mn_ui_kit.bin
# P2-1h's backdrop art rides the same flag and the same reason: without the kit
# there is no reader, and 177,900 bytes of ROM nothing opens is what keeps the
# published ROMs byte-identical across this row.
NDS_NITROFS_MN_UI_KIT_FILES += $(NITROFS_DIR)/menus/mn_surfaces.bin
# P3: the networked character select's 2P-4P player art (9,984 bytes).
NDS_NITROFS_MN_UI_KIT_FILES += $(NITROFS_DIR)/menus/mn_ui_kit_net.bin
endif

# The efcommon payloads only ship with the interpreter that reads them; without
# the runtime they are 200,896 bytes of ROM nothing opens. Two files, two
# encodings of the same texels: the .ds.bin is per-texture DS formats with
# palettes (the pack), the .rgb5a1.bin is the 22 admitted textures as RGB555+A1
# (the draw path, which uploads through a texture cache with no palette slot).
# Slice 32's baked animation bank ships only when a reader is compiled in. Empty
# by default, so the published ROM is byte-identical until the runtime bind
# lands -- see NDS_R2_FTANIM_DENSE above.
export NDS_NITROFS_FTANIM_FILES :=
ifeq ($(NDS_R2_FTANIM_DENSE),1)
NDS_NITROFS_FTANIM_FILES := $(NITROFS_DIR)/animation/ftanim_dense_bank.bin
endif
NDS_NITROFS_FTANIM_FILES += $(NITROFS_DIR)/animation/ftanim_stream_pack.bin

# Slice 1 phase 5's resident figatree pack. Empty unless a reader is compiled
# in, for the same reason as the bank above: without NDS_R2_BATTLEPACK it is
# 287,904 bytes of ROM nothing opens.
export NDS_NITROFS_BATTLEPACK_FILES :=
NDS_NITROFS_BATTLEPACK_FILES := $(NITROFS_DIR)/animation/battlepack_fox.bin

export NDS_NITROFS_PARTICLE_FILES :=
NDS_NITROFS_PARTICLE_FILES := \
	$(NITROFS_DIR)/particles/efcommon_particle_textures.ds.bin \
	$(NITROFS_DIR)/particles/efcommon_particle_quads.a5i3.bin
NDS_NITROFS_PARTICLE_FILES += \
	$(NITROFS_DIR)/particles/grpupupu_whispy_native.ds.bin
ifeq ($(NDS_STAGE_HYRULE),1)
NDS_NITROFS_PARTICLE_FILES += \
	$(NITROFS_DIR)/particles/grhyrule_native.ds.bin
endif

# The Task 39 hit-spark sheet. Unlike the payload above this one has a live
# reader (ndsFxPrepareHitSparks), so the ROM does not boot correctly
# without it whenever the sprite lane is on.
export NDS_NITROFS_EFFECT_FILES :=
NDS_NITROFS_EFFECT_FILES := \
	$(NITROFS_DIR)/effects/task39_hit_sparks.rgb5a1.bin

