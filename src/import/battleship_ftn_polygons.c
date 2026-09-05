/*
 * P2-6 Fighting Polygon Team (nFTKindNStart..nFTKindNEnd, kinds 14-25) runtime.
 *
 * Source: decomp/BattleShip-main/decomp/src/ft/ftchar/ftn*/ (storage only:
 * ftnmario.c, ftnfox.c, ftndonkey.c, ftnsamus.c, ftnluigi.c, ftnlink.c,
 * ftnyoshi.c, ftncaptain.c, ftnkirby.c, ftnpikachu.c, ftnpurin.c, ftnness.c,
 * already imported by battleship_ftchar_data_slots.c), FTData dFTN*Data
 * (ftdata.c:839-871 NMario, :1391-1423 NFox, :1913-1945 NDonkey, :2665-2697
 * NSamus, :3151-3183 NLuigi, :3806-3838 NLink, :4302-4334 NYoshi,
 * :4808-4840 NCaptain, :5634-5666 NKirby, :6134-6166 NPikachu,
 * :6632-6664 NPurin, :7184-7216 NNess).
 *
 * One TU for all twelve: every ftn*/ftn*.c file is a data-pointer-only
 * storage file of the same shape (e.g. ftnmario.c:4-13 gFTDataNMarioMain,
 * SubMotion, Model, ParticleBankID), and no polygon owns behavior bodies,
 * so there is nothing per-kind to compile here. Mechanics arrive through
 * the whole-TU imports below; this file only records the source contract.
 *
 * Mechanical equivalence, all data-driven through the existing seams:
 * - Each polygon owns its Main (NMario 0xcf, NFox 0xd3, NDonkey 0xd6,
 *   NSamus 0xdb, NLuigi 0xdf, NLink 0xe3, NYoshi 0xf8, NCaptain 0xed,
 *   NKirby 0xe7, NPikachu 0xf5, NPurin 0xea, NNess 0xf1;
 *   reloc_data_symbols.us.txt, port mirror
 *   src/port/reloc_backend_ftdata_symbols.c:1809-1831) and its own low-poly
 *   Model (NMario 0x12d, NFox 0x12f, NDonkey 0x134, NSamus 0x135,
 *   NLink 0x136, NYoshi 0x130, NCaptain 0x137, NKirby 0x131,
 *   NPikachu 0x133, NPurin 0x132, NNess 0x138) -- except NLuigi, which
 *   reuses NMarioModel (dFTNLuigiData points at &llNMarioModelFileID,
 *   ftdata.c:3156; no llNLuigiModel exists, and the port mirror carries
 *   only llNLuigiMainFileID 0xdf). Every polygon reuses its base
 *   MainMotion and ShieldPose and carries zero specials (all 0x00000000,
 *   ftdata.c rows above); NPikachu alone keeps &llPikachuSpecial2FileID
 *   (:6142). The models are already near DS budgets; no decimation.
 * - Status tables shared verbatim: dFTMainSpecialStatusDescs puts each
 *   base table in its N slot in order (ftmain.c:78-107, :93-105), compiled
 *   via battleship_ftmain.c whole import. Port kinds match source order
 *   exactly (include/ft/fighter.h:89-104 vs ftdef.h:1107-1125), so the
 *   30-stock wave spawn arithmetic `+ nFTKindNStart` (sc1pgame.c:1160-1201,
 *   variations :1164-1173) resolves against the port enum.
 * - Attributes zero every special, the catch and the voice:
 *   207_NMarioMain.c:201-208, 211_NFoxMain.c:215-222,
 *   214_NDonkeyMain.c:198-205, 219_NSamusMain.c:221-228,
 *   223_NLuigiMain.c:201-208, 227_NLinkMain.c:209-216,
 *   248_NYoshiMain.c:205ff, 237_NCaptainMain.c:199-206,
 *   231_NKirbyMain.c:203-210, 245_NPikachuMain.c:208ff,
 *   234_NPurinMain.c:204-211, 241_NNessMain.c:215-222. Voice-off and the
 *   missing specials gate through the existing is_have_* checks; no CSS
 *   art or stock icons exist for any polygon.
 * - Entry is source EntryNull for all twelve (ftcommonentry.c:194-207
 *   carries no N arm); the port else (battleship_ftcommon_entry.c:397-402)
 *   already yields it, so no entry arm is admitted.
 * - Kind arms the source gives the N twins already exist in the port, and
 *   none the source lacks was added: Mario/MMario/NMario tornado init
 *   (reloc_backend_compat_shims.c:7239-7249), Donkey/NDonkey/GDonkey charge
 *   ColAnim remap (:2000-2003) and charge_level check (:3786-3797),
 *   Samus/NSamus charge pair (:3798-3806), Donkey triple cargo path
 *   (battleship_ftcommon_get.c:241), Fox/NFox laser suppression and status
 *   compat (battleship_ftmain.c:57-71,
 *   reloc_backend_ftmain_status_compat.c:726-749,4079-4233); the charge
 *   joint arm (:10600-10605) covers Donkey/GDonkey per source
 *   ftparam.c:1842-1847 with no NDonkey arm, matching the source. Throw,
 *   damage, item-throw and dokan triples ride the whole ftcommon imports.
 * - Venue/driver: the 30-stock wave on nGRKindZako with trait
 *   nFTComputerTraitPolyTeam (sc1pgame.c:477-489) rides the driver already
 *   imported (battleship_sc1pgame_runtime.c); only admission (!=0 flags,
 *   reloc closures, owner packets) gates it.
 *
 * No behavior bodies live here: ftn*/ holds no specials of its own.
 */
#include <ft/fighter.h>
