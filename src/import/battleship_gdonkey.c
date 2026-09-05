/*
 * P2-6 Giant DK (nFTKindGDonkey, kind 26) runtime.
 *
 * Source: decomp/BattleShip-main/decomp/src/ft/ftchar/ftgdonkey/ (storage only:
 * ftgdonkey.c + ftgdonkey.h, already imported by battleship_ftchar_data_slots.c),
 * FTData dFTGDonkeyData (ftdata.c:2174-2206), Main 215_GDonkeyMain.c (file 0xd7).
 *
 * Mechanical equivalence, all data-driven through the existing seams:
 * - Own Main 0xd7 only; reuses DonkeyModel 0x13d, DonkeyMainMotion, ShieldPose
 *   and Special2 (ftdata.c:2176-2182; no llGDonkeyModel exists). The native
 *   owner packet is Donkey's verbatim (admit_fighter.py reuse_owner).
 * - knockback_resist_passive 48.0 (ftmanager.c:587-588), fallthrough Donkey /
 *   NDonkey charge_level=0 init (:592-594). Compiled via battleship_ftmanager.c
 *   whole import; no shim duplicate.
 * - Size 2.0 and weight 0.93 are attribute data (215_GDonkeyMain.c:228 size,
 *   :254 weight) applied by the existing attr->size path (ftmanager.c:537
 *   scale.vec = attr->size; attr fetch :694). No per-kind scale table exists.
 * - Status tables shared verbatim: dFTMainSpecialStatusDescs puts the Donkey
 *   table in the GDonkey slot (ftmain.c:82,106), compiled via
 *   battleship_ftmain.c whole import.
 * - Entry reuses the Donkey AppearR/AppearL pair and the Taru barrel effect
 *   (ftcommonentry.c table :39 + switch :204-207; port arm in
 *   battleship_ftcommon_entry.c under NDS_P2_GDONKEY).
 * - Donkey triples already carry GDonkey: charge ColAnim remap
 *   (reloc_backend_compat_shims.c:2000-2003), charge_level check (:3788-3796),
 *   charge joint 16 x=100 (:10600-10604, source ftparam.c:1842-1847), get.c:241
 *   cargo path; throw/damage/itemthrow triples ride whole ftcommon imports.
 * - Only attr delta besides size/weight/speeds: is_have_specialairlw 0
 *   (215:299), is_have_voice 1 (:301).
 *
 * No behavior bodies live here: ftgdonkey/ holds no specials of its own.
 */
#include <ft/fighter.h>
