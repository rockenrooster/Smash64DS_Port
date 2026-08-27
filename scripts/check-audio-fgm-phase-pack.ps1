param([string]$Python = 'python')

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $PSScriptRoot 'sfx\render-audio-fgm-phase-pack.py'
$verifierPath = Join-Path $PSScriptRoot 'verify-audio-fgm-phase-pack.ps1'
$metadataPath = Join-Path $root 'assets/audio/fgm_phase_pack_ima.json'
$packPath = Join-Path $root 'assets/audio/fgm_phase_pack_ima.bin'
$headerPath = Join-Path $root 'include/nds/nds_audio_fgm.h'
$runtimePath = Join-Path $root 'src/nds/nds_audio_fgm.c'

if ($null -eq (Get-Command $Python -ErrorAction SilentlyContinue)) {
    throw "Python command not found: $Python"
}
$verifier = Get-Content -LiteralPath $verifierPath -Raw
if (($verifier -notmatch '-MuteAudio') -or
    ($verifier -notmatch 'Audio FGM verification must use isolated runner slot')) {
    throw 'Audio FGM verifier lost its mute or isolated-runner guard.'
}
& $Python $generator --repo-root $root --check
if ($LASTEXITCODE -ne 0) {
    throw 'Generated FGM pack differs from its BattleShip sources.'
}

$metadata = Get-Content -LiteralPath $metadataPath -Raw | ConvertFrom-Json
$expectedIDs = @(626,470,469,467,490,74,363,364,372,373,374,430,439,292,
    370,289,300,303,154,77,215,40,38,37,34,32,31,375,429,431,435,440,
    19,41,42,43,185,186,187,189,190,217,218,219,216,28,2,0,188,
    436,432,362,433,360,12,285,
    # The announcer: TIME UP, GAME SET, "this game's winner is", and the two
    # fighter names the Results scene reads out.
    527,488,534,499,486,472,471,
    # And the crowd's win roar at Results scene start.
    621,
    # The reactive crowd: Fox/Mario chants, GaspL/M/S, Cheer, Amazed,
    # GaspClap, DamageL/M/S -- the eleven ft/ftpublic.c reaches in a P1 match.
    605, 609, 615, 616, 617, 618, 619, 620, 622, 623, 625,
    # The miss ring's three survivors: the ground grind, the altitude warning
    # (the pack's second DS hardware loop), and UnkGrind4 -- whose first note
    # asks for 90,510 Hz and therefore renders full-program AOT at 32,000.
    96, 153, 85,
    # And the five only a BOTH-CPU stress match reaches: dodge, shield on/off,
    # pause, and Fox's ledge teeter -- all core P1 gameplay.
    11, 13, 14, 278, 369,
    # And the last two the ring named on the soak after those five landed:
    # the zoom pulse and Fox's win voice at Results.
    271, 368,
    # And three from the run in which every fireball spawned and the match went
    # to SUDDEN DEATH: a light swing, the Sudden Death call, Fox's select voice.
    18, 365, 514,
    # P2-1c-1: the four menu SFX the UI kit's seam (ndsUiKitSfx) already asks
    # for and the pack did not carry -- MenuSelect, MenuScroll1, MenuScroll2,
    # MenuDenied. Proven by the miss ring, not inferred: 2026-08-17 P2-1c
    # evidence read `UKMISS ring=3 id0=164 c0=17 id1=165 c1=6`.
    158, 163, 164, 165,
    # P2-1d-1: the title screen's own confirm cue, which the menu shell's seam
    # already asks for with the source's own id (mntitle.c:501) and the pack
    # did not carry. Proven by the miss ring, not inferred: 2026-08-18 P2-1d
    # evidence read `MSMISS ring=1 id0=157 c0=1` -- the only cue any menu
    # screen misses.
    157,
    # P2-1e-1: the character select's own four cues (nds_menu_shell.c,
    # NDS_CSS_FGM_ANNOUNCE_WHOOSH/_GRAB/_SLOT_WHOOSH/NDS_CSS_VOICE_FREE_FOR_ALL),
    # already asked for with the source's own ids at P2-1e landing so the gap
    # could be measured. Proven by the miss ring, not inferred: 2026-08-18
    # P2-1e evidence read `MSMISS ring=4 id0=512 c0=1 id1=127 c1=1 id2=121
    # c2=2 id3=167 c3=1`.
    121, 127, 167, 512,
    # P2-1f-1: the stage select's own confirm cue (NDS_SSS_FGM_CONFIRM in
    # nds_menu_shell.c, mnmaps.c:1470's A/START confirm), already asked for
    # with the source's own id at the SSS screen's own landing so the gap
    # could be measured. Proven by the miss ring, not inferred: 2026-08-18
    # P2-1f evidence read `MSMISS ring=1 id0=159 c0=1` one-pass, `c0=3`
    # three-lap.
    159,
    # P2-1N (3)+(4), 2026-08-19: the shutter arrival cue and the mode
    # toggle's announcer line -- the shell asked, the miss ring proved the
    # gap on the owner's own build, and both derive as simple single-voice
    # articulations (166 six notes no forks, 526 one note).
    166, 526,
    # P2-3 Luigi CSS announcer and the selected-animation FuraFura voice
    # reached from dFTLuigiSubMotionDescs[1].
    498, 421,
    # P2-3 Donkey Kong production bank, announcer and crowd chant.
    324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336,
    483, 603,
    # No Contest Results: source announcer at tic 2 and crowd response at tic 71.
    502, 624,
    # P2-3f13 Captain Falcon's production bank: his ten FGM cues, twenty-two of
    # his twenty-three voices, his announcer line and his crowd chant. He landed
    # selectable at P2-3f8 with NOTHING packed. 356 FuraSleep is the one
    # omission and it is measured: its AOT body is 65,324 bytes against the
    # 53,248-byte largest runtime cache slot, so it can never be played.
    73, 106, 117, 180, 181, 182, 183, 184, 288, 299,
    337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350,
    351, 352, 353, 354, 355, 357, 358, 359, 485, 604,
    # P2-3f14 Donkey Kong's FGM bank. He received his VOICE bank at P2-3 and
    # never his sound effects, so every cue 212_DonkeyMainMotion.c, his
    # dead_fgm_ids[1] (0x11f) and the shared DownBounce table reach was failing
    # closed -- 90 requests in one measured minute, 56 of them DonkeyCharge.
    # 10 DonkeySlap2 rides with them: Captain/Kirby/Purin/Yoshi play it and
    # 175/176 fork it, so it closes one of the seven shared cues P2-3f13 left
    # open. 105/116/287/298 were already in the pack as somebody else's fork
    # target, and their extents land on the numbers those entries carry.
    9, 10, 72, 105, 116, 175, 176, 177, 178, 179, 287, 298,
    # P2-3f15 Luigi's voice bank: the contiguous source run 416..428 minus the
    # already-packed 421, plus his crowd chant 608. He shipped selectable with
    # exactly two cues packed, so every voice his FTAttributes lanes and motion
    # scripts name was failing closed -- which is why P2-3f12's normalizer
    # repair was latent. 425 Lets and 428 HereWe are source-marked unused and
    # packed anyway so the bank never has to be reopened.
    416, 417, 418, 419, 420, 422, 423, 424, 425, 426, 427, 428, 608,
    # P2-3f16 fighter entry audio: Mario/Luigi's shared pipe, Fox's Arwing and
    # DK's shared container-smash cue. These are the exact source motion-script
    # calls, and all three render their full multi-note programs AOT.
    214, 191, 59,
    # P2-3 Samus CSS: source announcer line plus the selected-motion BladeDraw
    # cue from scsubsysdatasamus.c. BladeDraw's two-note one-voice program is
    # rendered AOT so the mid-program control change is preserved.
    513, 264,
    # P2-3 Samus bounded gameplay bank. These thirty are source-program AOT;
    # Charge0..7, ShootF and SpecialHi stay out until their harder source
    # sequencer/cache semantics are represented rather than approximated.
    17, 22, 23, 24, 81, 92, 103, 114, 128, 236, 237, 238, 247, 248, 250,
    251, 296, 307, 573, 574, 575, 576, 577, 578, 579, 580, 581, 582, 613,
    639)
$actualIDs = @($metadata.entries | ForEach-Object { [int]$_.id })
if (($actualIDs -join ',') -ne ($expectedIDs -join ',')) {
    throw "Unexpected FGM mapping: $($actualIDs -join ',')"
}
if (([int]$metadata.format_version -ne 4) -or
    ([int]$metadata.entry_bytes -ne 32) -or
    ([int]$metadata.envelope_point_bytes -ne 4) -or
    # 725900 -> 725896 on 2026-08-02: FGM 430 and 439 moved onto the source
    # note schedule, which re-renders them and lands four bytes shorter.
    # 725896 -> 887160 later the same day: the seven defective crowd cues
    # (615/616/618/619/620/623/625) joined FULL_PROGRAM_AOT_IDS, which leaves
    # the shared-sample-37 dedup behind and is the whole point of the change.
    # 887160 -> 913168 on 2026-08-02: FGM 153 AltitudeWarn left the DS
    # hardware-repeat path for the schedule-walking AOT render, because a
    # hardware repeat cannot reproduce the pitch sweep its articulation puts
    # inside the loop. 0.108 s monotone -> 1.725 s swept.
    # 913168 -> 920152: FGM 12 DeadUpStar joined it too, recovering the source loop
    # it was dropping (0.425 s of a 0.863 s note) and clearing its clipping.
    # 920152 -> 938996 on 2026-08-06: FGM 617 GaspS and 622 DamageL joined as
    # well. The 2026-08-02 note kept them flat because "their only DECLARED debt
    # is untrimmed_shared_source_reuse, a dedup note, not a defect" -- true about
    # forks and pitch automation, but it never asked whether the cue has a
    # MULTI-NOTE SCHEDULE, which the flat path also cannot express. Both do:
    # 617 is (6,7,70)(6,7,180) = 250 ticks and 622 is (7,7,80)(7,7,100)(7,7,200)
    # = 380 ticks, so each rendered one one-shot and then silence -- 617 short by
    # 299 ms, 622 by 744 ms. Now 1,437 ms and 2,185 ms, matching their note
    # totals exactly. 605 and 609 carry `pitch_code`, not `notes`; they are
    # genuinely single-note and stay flat.
    # 938996 -> 948068 on 2026-08-18 (P2-1c-1): the UI kit's four menu SFX
    # (158 MenuSelect, 163 MenuScroll1, 164 MenuScroll2, 165 MenuDenied)
    # joined SELECTED, 88 -> 92 entries. 158/163/164 share one short UI-click
    # wave (untrimmed, no loop); 165's 70-tick hold outlives its 1,664-sample
    # source, rendered via the same `render_source_loop` software path as 216
    # and 28 above (source loop replayed to the note's proven reach, then a
    # normal one-shot IMA encode -- no DS hardware repeat, no hand-derived
    # IMA seed).
    # 948068 -> 950168 on 2026-08-18 (P2-1d-1): FGM 157 TitlePressStart joined
    # SELECTED, 92 -> 93 entries (+2100 bytes: 2068-byte IMA body + 32-byte
    # entry header). A flat two-note schedule (both notes pitch code 9) whose
    # ceiling reach exceeds its 4,128-sample decoded source, so it retains the
    # full untrimmed source exactly like 158/163/164 above.
    # 950168 -> 973524 on 2026-08-18 (P2-1e-1): the CSS's four cues (121
    # MarioDash, 127 SamusDash, 167 PlayerSlotWhoosh, 512 AnnounceFreeForAll)
    # joined SELECTED, 93 -> 97 entries. 127/167/512 are plain flat renders
    # (6880/3681/33829 retained samples); 121 forks to 118 FoxDash with no
    # local notes and 118's first note overflows the u16 frequency field
    # (71,838 Hz), so it renders full-program AOT like 85/153/189/190/219 and
    # stores 2,024 samples at FGM_OUTPUT_RATE.
    # 973524 -> 990120 on 2026-08-18 (P2-1f-1): FGM 159 StageSelect joined
    # SELECTED, 97 -> 98 entries. Unlike 121, 159 has a real local note of its
    # own (pitch 6, 180 ticks) AND forks two voices at tick 0 (163
    # MenuScroll1, 6 UnkSmallPing1), so it joins FULL_PROGRAM_AOT_IDS to
    # render all three voices fused -- 33,120 samples at FGM_OUTPUT_RATE,
    # +16,596 bytes (16,564-byte IMA body + 32-byte entry header).
    # 1003932 -> 1013824 -> 1022692 on 2026-08-21 (P2-3): source Luigi
    # announcer 498 and selected-animation FuraFura 421 joined SELECTED,
    # 100 -> 101 -> 102 entries. 1022692 -> 1201060 on 2026-08-22 for DK's
    # 324..336 voice bank, announcer 483 and crowd chant 603. 324 is compact
    # source-note replay (18,564-byte wave, two timed retriggers), not its
    # impossible 112 KiB baked timeline. Runtime cache remains 204800.
    # 1201060 -> 1261628 on 2026-08-24 for the two No Contest Results cues.
    # 1261628 -> 1511844 on 2026-08-25 (P2-3f13) for Captain Falcon's bank:
    # ten FGM cues, twenty-two voices, announcer 485 and crowd chant 604,
    # 119 -> 153 entries, +250,216 bytes of ROM. Runtime cache stays 204800.
    # 1511844 -> 1551484 on 2026-08-25 (P2-3f14) for Donkey Kong's FGM bank:
    # his eleven cues plus the shared 10 DonkeySlap2, 153 -> 165 entries,
    # +39,640 bytes of ROM. Four of the twelve (105/116/287/298) were already
    # in the pack as somebody else's fork target, so their bodies dedup and the
    # growth is mostly the six genuinely new ones. Runtime cache stays 204800.
    # 1551484 -> 1733284 on 2026-08-25 (P2-3f15) for Luigi's voice bank: his
    # twelve unpacked voices plus crowd chant 608, 165 -> 178 entries,
    # +181,800 bytes of ROM. Largest is 420 DeadUp at 32,664 B, inside the
    # 53,248-byte slot; nothing here needs runtime_note_replay and nothing is
    # refused. Runtime cache stays 204800.
    # 1733284 -> 1770008 on 2026-08-26 (P2-3f16) for the three entry cues:
    # 214 MarioDokan, 191 FoxAppearArwing and 59 ContainerSmash, 178 -> 181
    # entries, +36,724 bytes of ROM. Runtime cache stays 204800.
    # 1770008 -> 1785424 on 2026-08-26 for Samus CSS audio: announcer 513 and
    # selected-pose BladeDraw 264, 181 -> 183 entries, +15,416 bytes of ROM.
    # BladeDraw is full-program AOT; runtime cache stays 204800.
    # 1785424 -> 2014020 on 2026-08-27 for Samus's thirty bounded gameplay
    # cues, 183 -> 213 entries, +228,596 bytes of ROM. Every cue is rendered by
    # the source-program AOT path; the 200 KiB streaming cache is unchanged.
    ([int64]$metadata.resident_bytes -ne 2014020) -or
    ([int64]$metadata.resident_limit_bytes -ne 204800) -or
    # ROM, not RAM: the runtime streams cues into resident_limit_bytes and never
    # holds the pack. 512 KiB blocked the five announcer lines and 768 KiB then
    # blocked the seven crowd cues, both for no runtime reason; the bound that
    # is real is the 53,248-byte cache-slot gate below.
    ([int64]$metadata.pack_limit_bytes -ne 2097152) -or
    # 0x984c7da6 -> 0x4fb97922 -> 0xb6be788e on 2026-08-02: this hash covers the
    # cue SELECTOR table. 430/439 gained "aot_source_schedule", then the seven
    # crowd cues gained the full-program AOT render. A mapping change is
    # expected whenever a cue's render strategy changes and must never be
    # repinned without one.
    # 0xb6be788e -> 0x5d1c7cf5 -> 0x885657f4 on 2026-08-06 for 617 and 622
    # changing render strategy, which is exactly the case this hash exists to
    # catch and must never be repinned without.
    # 0x885657f4 -> 0xe4b8921c on 2026-08-18 (P2-1c-1) for the four menu SFX
    # joining SELECTED -- same reason, the selector table changed.
    # 0xe4b8921c -> 0x9bc3e069 on 2026-08-18 (P2-1d-1) for FGM 157 joining
    # SELECTED -- same reason, the selector table changed.
    # 0x9bc3e069 -> 0xcb181af6 on 2026-08-18 (P2-1e-1) for the CSS's four cues
    # joining SELECTED -- same reason, the selector table changed.
    # 0xcb181af6 -> 0x3d9a9ac2 on 2026-08-18 (P2-1f-1) for FGM 159 joining
    # SELECTED -- same reason, the selector table changed.
    # 0x3d9a9ac2 -> 0xf6b94a48 on 2026-08-19 (P2-1N) for FGM 166 and voice 526
    # joining SELECTED -- same reason, the selector table changed.
    # 0xf6b94a48 -> 0xdf21d357 -> 0x393e86e8 on 2026-08-21 (P2-3) for
    # Luigi's source announcer 498 and selected-animation voice 421;
    # -> 0x476d5727 on 2026-08-22 for DK's source bank/announcer/crowd set;
    # -> 0x63fcb476 on 2026-08-24 for the two No Contest Results cues;
    # -> 0x17d8f4ff on 2026-08-25 (P2-3f13) for Captain Falcon's thirty-four
    # selectors;
    # -> 0xfb3507c6 on 2026-08-25 (P2-3f14) for Donkey Kong's twelve FGM
    # selectors;
    # -> 0x7b3a0107 on 2026-08-25 (P2-3f15) for Luigi's thirteen -- same
    # reason, the selector table changed;
    # -> 0xeb6ba1dc on 2026-08-26 (P2-3f16) for the three source entry cues;
    # -> 0x64710073 for Samus's source announcer + selected BladeDraw pair;
    # -> 0xa1f3ba41 for her thirty bounded gameplay selectors.
    ($metadata.mapping_sha256_lo -ne '0xa1f3ba41') -or
    # Repinned 2026-08-02: FGM 11 (the rolling dodge) dropped 127 -> 96 -> 68 ->
    # 48 on the owner's ear via FGM_OWNER_VOLUME_TRIM, -8.4 dB total against the
    # source; the 68 pin was
    # 967f9212524950c3a2277a8e59ca2c545117f20c56c58e3e0237e94cd2dd45c2 and the 48
    # pin was b478f9eb8366b175596238a55c7200cdb58a8c925c9f1258b4dc407fbfe97fb2.
    # Repinned again the same day for FGM 12 DeadUpStar 127 -> 90, which is not a
    # taste trim: 90 is 127 * 180/255, i.e. the cue's own ucd_volume restored
    # after FULL_PROGRAM_AOT_IDS normalised the render to full scale.
    # Note that a level trim moves THIS hash and not mapping_sha256_lo: ds_volume
    # is a record field, not a selector, so the mapping stays 0x5d1c7cf5 and
    # resident_bytes stays 920152. The earlier pin was
    # 81b94d1f3178b6b57d998fb7d01fe1316e20ac46ce22ccb82800c6b02d26cb75, and it
    # is worth knowing that the first attempt at that trim did NOT move this
    # hash -- it edited the metadata dict instead of the `records` entry that
    # PACK_ENTRY.pack writes, so the manifest claimed 96 while the ROM still
    # played 127. An unchanged hash after an intended payload change is the
    # signal that the change did not land.
    # Repinned 2026-08-06 with the 617/622 render change; the prior pin was
    # 5f12e380c4036401414cc490f4a29cc708281ba574813ccaf13acb56327fa6db.
    # Repinned 2026-08-18 (P2-1c-1) for the four menu SFX joining SELECTED;
    # the prior pin was
    # 51ac736c2421fe63b0f5cba4e791572ed5c453e1c85614303b464cd3374d749e.
    # Repinned 2026-08-18 (P2-1d-1) for FGM 157 joining SELECTED; the prior
    # pin was
    # 101fd1d5b369dc6932090e6de3a43508fb0b79d2f2507c56151a99f7d7e3d2b7.
    # Repinned 2026-08-18 (P2-1e-1) for the CSS's four cues joining SELECTED;
    # the prior pin was
    # 011a59be88752138b985faee776814d4c0fed048173f1d337ef51351adec741a.
    # Repinned 2026-08-18 (P2-1f-1) for FGM 159 joining SELECTED; the prior
    # pin was
    # 6cc5f91c35a82833d23bf3001c0c108225615e03a04ccc49eda800812f86c0b7.
    # P2-3 Luigi selected-animation voice 421 repin; prior 101-entry pin was
    # ffdeefd578da5dfe99f715b7f10aeaea9fcc9c2b75f1c3d269c9480ab0b837e3.
    # DK's complete source voice admission, including compact 324 replay, moves
    # the binary identity again while leaving the resident cache unchanged.
    # No Contest 502/624 move it once more on 2026-08-24.
    # Captain Falcon's thirty-four cues move it again on 2026-08-25 (P2-3f13);
    # the 119-entry pin was
    # 9012a748c88ee15daf3ddc70c6a2dfff60c60fde1f2027d3a2e57fa6f73165b6.
    # Donkey Kong's twelve FGM cues move it again the same day (P2-3f14); the
    # 153-entry pin was
    # bd26c263c895617ccc0c7995d92f2748f2a0d877465369863d9478fc691bb393.
    # Luigi's thirteen move it once more (P2-3f15); the 165-entry pin was
    # 113da5fb91c83ff3b6f4bd4b63f840485ad08185f3967f120c320d9b175377f0.
    # The three P2-3f16 entry cues move it again; the 178-entry pin was
    # 5299db705baa652b4fa7d0ee35bd0d18a0bdecb36a35507bd30b35d0cb23353c.
    # Samus's two CSS cues move it once more; the 181-entry pin was
    # 87c0c17bb1a89f2353153e88c188336ba182d5c05a39e2bff5b16e9533e19994.
    # 2026-08-27: no selector, extent or mapping changed here. The payload hash
    # moved because the AOT source model now implements n_env.c's active-
    # modulator targets 24+ instead of dropping them. Five already-packed cues
    # (11/85/92/251/639) therefore changed acoustically while entry count,
    # resident bytes and mapping stay exactly 213 / 2014020 / 0xa1f3ba41.
    ($metadata.pack_sha256 -ne
        '0f62d3b2f280fc260a46161c1ba86aebf321a1994df4ba3e22f05ea4ef89aa7a')) {
    throw 'FGM pack format, budget, mapping, or binary identity changed.'
}
if ((@($metadata.excluded_entries).Count -ne 0) -or
    (@($metadata.runtime_excluded_hit_ids).Count -ne 0)) {
    throw 'A battle-reachable FGM remains excluded.'
}
foreach ($entry in $metadata.entries) {
    if (([double]$entry.decoded_rms -le 0.0) -or
        ([int64]$entry.decoded_peak -le 0) -or
        ([double]$entry.ima_snr_db -lt 14.0) -or
        ([int64]$entry.ima_adpcm_bytes -gt 53248) -or
        ([int]$entry.packed_envelope_count -gt 32)) {
        throw "FGM $($entry.id) failed its acoustic/cache gate."
    }
    # A REST MUST NEVER SET THE PLAYBACK RATE. The DS pack plays one sample at
    # one rate, so the rate comes from a cue's first note -- and pitch code 0 is
    # a rest, not a note. FGM 488 GAME SET is the one P1 cue whose program opens
    # with one (a 60-tick rest, then the line at code 13), and it shipped at
    # 7,565 Hz against every other announcer line's 16,000: thirteen semitones
    # low, which is the owner's "sounds really low pitched" row in BUGS.md.
    #
    # Nothing caught it because the pack self-checks against its own derivation
    # and the derivation had the same bug, so the guard has to be an external
    # bound.
    #
    # P2-3f13: it used to be a 12,000 Hz FLOOR, chosen because a rest at the
    # announcer articulation lands on 7,565 while the lowest legitimately
    # packed rate was then 15,102. That bound was a proxy, and Captain Falcon's
    # Flyer engine (180) falsified it on the first cue that is legitimately
    # deep: its first note is a real pitch code 6, its articulation drops
    # another 1,200 cents, and it voices at 10,679 Hz on purpose. Assert the
    # thing the guard actually means instead -- the rate must come from a NOTE,
    # never from a REST -- by recomputing the sounding pitch code from the
    # entry's own program. Strictly stronger than the floor: it catches 488's
    # defect at any frequency, and it does not reject a deep cue for being deep.
    $notes = @($entry.ucd_program | Where-Object { $_[0] -eq 'note' })
    if ($notes.Count -gt 0) {
        $sounding = @($notes | Where-Object { [int]$_[1] -ne 0 })
        if ($sounding.Count -eq 0) {
            throw "FGM $($entry.id) has no sounding note; its rate is a rest."
        }
        $artPitch = @($entry.articulation_program |
            Where-Object { $_[0] -eq 'pitch' })
        $artCents = if ($artPitch.Count -gt 0) { [int]$artPitch[0][1] } else { 0 }
        $ratePitchCode = [int](($entry.net_pitch_cents - $artCents + 1300) / 100)
        if ($ratePitchCode -eq 0) {
            throw ("FGM $($entry.id) plays at $($entry.ds_frequency_hz) Hz " +
                'from pitch code 0 -- a REST set its playback rate.')
        }
        if ($ratePitchCode -ne [int]$sounding[0][1]) {
            throw ("FGM $($entry.id) rate pitch code $ratePitchCode is not " +
                "its first sounding note $([int]$sounding[0][1]).")
        }
    }
}
foreach ($id in @(154,40,38,37,34,32,31)) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq $id }
    if ((@($entry.omitted_fork_programs).Count -ne 0) -or
        ($entry.acoustic_oracle.aot_strategy -ne
            'source_program_schedule_and_simultaneous_forks')) {
        throw "FGM $id did not ship its fused source fork."
    }
}
# BUGS.md #3.  Whispy's gust was the FIRST DS hardware loop in the pack (153
# AltitudeWarn is the second, pinned below), and it is the whole reason the
# hazard sounds for its full 470 ticks instead of puffing once.  Losing the
# loop flag was the original defect, so pin the flag, the PNT/LEN geometry,
# and the three DS repeat proofs together.
$fgm285 = $metadata.entries | Where-Object { [int]$_.id -eq 285 }
$oracle285 = $fgm285.acoustic_oracle
if (($fgm285.ds_loop_strategy -ne 'source_loop_ds_hardware') -or
    ([int]$fgm285.ds_loop_flag -ne 1) -or
    ([int]$fgm285.ds_loop_point_words -ne 1) -or
    ([int]$fgm285.ds_loop_length_words -ne 1663) -or
    ([int]$fgm285.ds_ima_loop_body_nibbles -ne 13304) -or
    (@($fgm285.ds_ima_guard_nibbles).Count -ne 0) -or
    ([int]$fgm285.ds_ima_header_predictor -ne 335) -or
    ([int]$fgm285.ds_ima_header_index -ne 56) -or
    ($oracle285.ds_repeat_oracle_model -ne
        'header_once_pnt_latch_len_restore') -or
    ($oracle285.ds_repeat_oracle_missing_restore_detected -ne $true) -or
    ($oracle285.ds_repeat_oracle_wrong_pnt_detected -ne $true) -or
    ($oracle285.ds_repeat_oracle_wrong_len_detected -ne $true)) {
    throw 'FGM 285 lost its proven DS hardware wind loop.'
}
# 153 AltitudeWarn, the second hardware loop and the cue that turned the
# WHISPY_* module constants into a per-cue spec. Its schedule (300 ticks,
# 1.725 s) outlives its sample (22,208 at 29,344 Hz, 0.757 s), so losing the
# loop flag here cuts the warning off two thirds of the way through -- exactly
# 285's failure, on a cue the owner would hear every time a fighter drops
# below the stage.
# ...and the paragraph above is what this pin USED to enforce. It is kept
# because it is the reasoning that has to be answered, not because it was right:
# it says the sample is 22,208 at 29,344 Hz (0.757 s), and the artifact never
# was. A hardware repeat stores PNT + LEN, and 153 shipped PNT 1 word + LEN 396
# words -- about 3,168 samples, 0.108 s. The attack was gone and the comment
# describing 0.757 s was aspirational.
#
# 2026-08-02, and the owner found this one by ear out of all 88: articulation
# 150 opens `pitch 550` and then steps `pitch 2390` INSIDE mark_loop/jump_loop,
# roughly an octave and a half of sweep on repeat. That sweep is the altitude
# warning. A DS hardware repeat replays its body bit-identically, which this
# file already says for 621/626 -- "cannot ramp" -- so the cue came out as a
# 0.108 s monotone blip looping, and read as an unfamiliar sound rather than a
# missing one, because it fires on exactly the right trigger.
#
# It renders through FULL_PROGRAM_AOT_IDS now: the schedule is walked and the
# sweep baked into 55,200 samples at 32,000 Hz = 1.725 s, matching the source's
# declared duration exactly. Measured after the change, zero-crossing rate falls
# 1,820 Hz -> 1,063 Hz across the first second, so the slide is really in there.
# STILL OPEN and deliberately not pinned as fixed: the render decays to silence
# at ~1.05 s and pads the rest, and it no longer repeats. The source loops
# infinitely, so if the warning has to persist while a fighter is out of bounds,
# that is the next piece of work.
$fgm153 = $metadata.entries | Where-Object { [int]$_.id -eq 153 }
if (($fgm153.ds_loop_strategy -ne 'source_program_aot') -or
    ([int]$fgm153.ds_frequency_hz -ne 32000) -or
    ([int]$fgm153.ds_sample_count -ne 55200) -or
    (@($fgm153.runtime_fidelity_debt).Count -ne 0) -or
    ($fgm153.acoustic_oracle.aot_strategy -ne
        'source_program_schedule_and_simultaneous_forks')) {
    throw ('FGM 153 AltitudeWarn is not on the schedule-walking AOT render; a ' +
        'hardware repeat cannot repeat its pitch sweep.')
}
# 85 UnkGrind4 is the pack's answer to `source_rate_above_u16`, and the answer
# is "do not store that rate at all". Its first note asks for 90,510 Hz --
# 32,000 * 2^(1800/1200) -- against a u16 `frequency` field, so it renders its
# whole three-note schedule AOT and the entry stores FGM_OUTPUT_RATE. Pin both
# halves: a regression that reverted the AOT strategy would store a truncated
# rate and the cue would play at some unrelated pitch rather than fail.
$fgm85 = $metadata.entries | Where-Object { [int]$_.id -eq 85 }
if (([int]$fgm85.ds_frequency_hz -ne 32000) -or
    ([int]$fgm85.net_pitch_cents -ne 1800) -or
    ($fgm85.ds_loop_strategy -ne 'source_program_aot') -or
    ([int]$fgm85.ds_loop_flag -ne 0) -or
    ([int]$fgm85.acoustic_oracle.aot_output_frequency_hz -ne 32000) -or
    ([int]$fgm85.acoustic_oracle.aot_output_samples -ne 2576)) {
    throw 'FGM 85 no longer renders its above-u16 note schedule AOT.'
}
# BattleShip n_env.c's modulator targets 24+ do NOT mean "ignore this because
# it belongs to another audio voice". They select another ACTIVE MODULATOR on
# the same FGM voice: `(target - 24) / 8` chooses its id and the low three bits
# choose period/amplitude/offset/phase. That distinction is audible and was
# previously dropped by the AOT model. Pin representative outputs from every
# packed source shape that uses it: target28 rewrites modulator-0 offset in
# Escape/UnkGrind4/SamusJumpAerial/CharacterUnkZip10; target36 rewrites
# modulator-1 offset in SamusUnkCharge. These are PCM hashes before IMA encoding,
# so this gate is about source sequencer semantics rather than codec bytes.
$crossModPcm = @{
    11  = 'c1405a6a9b1538f53abccd4b7c6d13d16cfae78a7c9139f72305810b3de4147c'
    85  = '9634dc51a4e585abd4f9860aa43fbecdc833ef69d1d071aba44c4e3298c45b51'
    92  = 'a22d1068dd0328e1b7ccfd38bd9f467928ea6899ba9bdc52f0f9273a71263616'
    251 = '02e4e4107151a69619eb17bd9f47af53f41d0ab16dcd4e7e1b4f5da6baef30d4'
    639 = 'b4081a1480bb03e9a0c1739777bebe112176d67c01bd7188962dc1016fa8532a'
}
foreach ($id in $crossModPcm.Keys) {
    $entry = $metadata.entries | Where-Object { [int]$_.id -eq [int]$id }
    if (($null -eq $entry) -or
        ($entry.acoustic_oracle.aot_rendered_pcm_sha256 -ne $crossModPcm[$id])) {
        throw "FGM $id lost BattleShip active-modulator cross-target semantics."
    }
}
$fgm218 = $metadata.entries | Where-Object { [int]$_.id -eq 218 }
if (($fgm218.acoustic_oracle.source_custom_fx_dry_only -ne $true) -or
    ([int]$metadata.attack_activation_qualification.fgm_218_feasibility.source_effective_fx_mix -ne 25)) {
    throw 'FGM 218 named dry custom-FX lever changed.'
}
$header = Get-Content -LiteralPath $headerPath -Raw
$runtime = Get-Content -LiteralPath $runtimePath -Raw
foreach ($token in @(
    '#define NDS_AUDIO_FGM_CACHE_BYTES 204800u')) {
    if (-not $header.Contains($token)) { throw "Runtime header lost: $token" }
}
# NDS_AUDIO_FGM_ENTRY_COUNT, NDS_AUDIO_FGM_PACK_BYTES and
# NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO are DERIVED from the pack, never pinned as
# text here. Duplication has caused the same total-silence class twice: on
# 2026-08-02 size/hash drift rejected all 88 cues; on 2026-08-24 adding No Contest
# 502/624 produced a 119-entry pack while the runtime still compiled for 117.
# The loader intentionally rejects the entire pack on ANY mismatch, so compare
# all three against the binary itself.
$packBytes = (Get-Item -LiteralPath $packPath).Length
$packHeaderBlob = [System.IO.File]::ReadAllBytes($packPath)[0..15]
$packEntryCount = [System.BitConverter]::ToUInt16($packHeaderBlob, 6)
$packSizeField = [System.BitConverter]::ToUInt32($packHeaderBlob, 8)
$packMappingLo = '0x{0:x8}' -f [System.BitConverter]::ToUInt32($packHeaderBlob, 12)
if ($packSizeField -ne $packBytes) {
    throw "Pack header size field $packSizeField disagrees with its own length $packBytes."
}
foreach ($pair in @(
    @{ Name = 'NDS_AUDIO_FGM_ENTRY_COUNT'; Want = "${packEntryCount}u" },
    @{ Name = 'NDS_AUDIO_FGM_PACK_BYTES'; Want = "${packBytes}u" },
    @{ Name = 'NDS_AUDIO_FGM_PACK_MAPPING_SHA256_LO'; Want = "${packMappingLo}u" })) {
    $found = [regex]::Match($header, "#define $($pair.Name)\s+(\S+)")
    if (-not $found.Success) { throw "Runtime header lost: #define $($pair.Name)" }
    if ($found.Groups[1].Value -ne $pair.Want) {
        throw ("$($pair.Name) is $($found.Groups[1].Value) but the pack says $($pair.Want). " +
               'The runtime rejects the whole pack on this mismatch: the ROM boots SILENT. ' +
               "Set it in $headerPath and rebuild.")
    }
}
# A packed cue the allowlist never admits is dead ROM, and an admitted cue with
# no pack entry fails closed and is silent -- both were live defects.
#
# P2-3f13: this used to be a HAND-WRITTEN list of 28 names, "the announcer set
# at least", and a hand list only ever covers what somebody remembered to add.
# Derive it instead: EVERY entry in the pack must have a case label in
# ndsAudioFgmIDIsIncluded, by its gmsound name or by its bare numeric id (the
# switch uses `case 435u:` for two cues the port header does not name). Thirty
# four cues landed in one row here; nothing would have caught thirty three of
# them.
$missingAdmission = @()
foreach ($entry in $metadata.entries) {
    $name = [string]$entry.name
    $id = [int]$entry.id
    if ($runtime.Contains("case ${name}:")) { continue }
    if ($runtime -match "case\s+${id}u?:") { continue }
    $missingAdmission += ('{0} ({1})' -f $name, $id)
}
if ($missingAdmission.Count -gt 0) {
    throw ("Runtime allowlist does not admit {0} packed cue(s): {1}. " -f
        $missingAdmission.Count, ($missingAdmission -join ', ')) +
        'ndsAudioFgmIDIsIncluded is fail-closed bookkeeping; a packed cue it ' +
        'never names is ROM nobody can account for.'
}
foreach ($token in @('fread(sNdsAudioFgmCacheSlots[best].data',
    'sNdsAudioFgmCacheSlots[cache_slot].references++',
    'sNdsAudioFgmCacheSlots[(u32)handle->cache_slot].references--')) {
    if (-not $runtime.Contains($token)) { throw "Runtime cache lost: $token" }
}

# Counted, not spelled out. This line said "725896-byte pack" and "seven fused
# fork repairs" while the artifact was 887160 bytes with twelve, which is the
# same drift that shipped a silent ROM this morning: a hand-written summary is
# a second copy of a fact and it rots.
$fusedForks = @($metadata.entries | Where-Object {
    @($_.root_fork_programs).Count -gt 0 -and
    @($_.omitted_fork_programs).Count -eq 0 }).Count
$stillOmitting = @($metadata.entries | Where-Object {
    @($_.omitted_fork_programs).Count -gt 0 }).Count
Write-Output (("Audio FGM full coverage passed: $($metadata.entries.Count) IDs, " +
    "0 exclusions, $($metadata.resident_bytes)-byte pack, " +
    "$($metadata.resident_limit_bytes)-byte cache, $fusedForks fused fork " +
    "repairs ($stillOmitting cue(s) still omit a fork voice), FGM 285 wind on " +
    'a proven DS hardware loop, seven announcer lines, PublicWin 621 on ' +
    'PublicExcited''s AOT loop-and-ramp render.'))
