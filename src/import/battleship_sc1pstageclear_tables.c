/* 1P Game stage-clear bonus table (P2-6 step 2).
 *
 * Textual include rejected: including
 * decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pstageclear.c whole for
 * its data would drag in function bodies that cannot link
 * (sc1PStageClearFuncLights at :571 through sc1PStageClearStartScene, plus
 * dSC1PStageClearFileIDs at :24-33), so the table below is transcribed
 * verbatim instead. Field values, source ordering, and REGION_US arms are
 * preserved exactly; repeated entries are not folded into a loop.
 */

#if NDS_P2_1P_GAME

#include <stdint.h>
#include <ssb_types.h>

/* Source: decomp/BattleShip-main/decomp/src/sc/sc1pmode/sc1pstageclear.c:36-415.
 * 58 entries in source order, index = nSC1PGameBonusCheapShot..TrueFriend
 * (decomp src/sc/scdef.h:319-379). Type SC1PStageClearScore is
 * { intptr_t offset; s32 points; } per decomp src/sc/sctypes.h:111-115;
 * the type and the llSC1PStageClear1*TextSprite symbols are NOT defined
 * here -- see unresolved references reported with this file. */
SC1PStageClearScore dSC1PStageClearBonusData[/* */] =
{
	// Cheap Shot
	{ &llSC1PStageClear1CheapShotTextSprite, -99 },

	// Star Finish
#if defined(REGION_US)
	{ &llSC1PStageClear1StarFinishTextSprite, 10000 },
#else
	{ &llSC1PStageClear1StarFinishTextSprite, 2000 },
#endif

	// No Item
#if defined(REGION_US)
	{ &llSC1PStageClear1NoItemTextSprite, 1000 },
#else
	{ &llSC1PStageClear1NoItemTextSprite, 5000 },
#endif

	// Shield Breaker
#if defined(REGION_US)
	{ &llSC1PStageClear1ShieldBreakerTextSprite, 8000 },
#else
	{ &llSC1PStageClear1ShieldBreakerTextSprite, 5000 },
#endif

	// Judo Warrior
#if defined(REGION_US)
	{ &llSC1PStageClear1JudoWarriorTextSprite, 5000 },
#else
	{ &llSC1PStageClear1JudoWarriorTextSprite, 4000 },
#endif

	// Hawk
#if defined(REGION_US)
	{ &llSC1PStageClear1HawkTextSprite, 18000 },
#else
	{ &llSC1PStageClear1HawkTextSprite, 10000 },
#endif

	// Shooter
#if defined(REGION_US)
	{ &llSC1PStageClear1ShooterTextSprite, 12000 },
#else
	{ &llSC1PStageClear1ShooterTextSprite, 5000 },
#endif

	// Heavy Damage
#if defined(REGION_US)
	{ &llSC1PStageClear1HeavyDamageTextSprite, 28000 },
#else
	{ &llSC1PStageClear1HeavyDamageTextSprite, 10000 },
#endif

	// All Variations
#if defined(REGION_US)
	{ &llSC1PStageClear1AllVariationsTextSprite, 30000 },
#else
	{ &llSC1PStageClear1AllVariationsTextSprite, 15000 },
#endif

	// Item Strike
#if defined(REGION_US)
	{ &llSC1PStageClear1ItemStrikeTextSprite, 20000 },
#else
	{ &llSC1PStageClear1ItemStrikeTextSprite, 10000 },
#endif

	// Double KO
#if defined(REGION_US)
	{ &llSC1PStageClear1DoubleKOTextSprite, 0 },
#else
	{ &llSC1PStageClear1DoubleKOTextSprite, 6000 },
#endif

	// Trickster
#if defined(REGION_US)
	{ &llSC1PStageClear1TricksterTextSprite, 11000 },
#else
	{ &llSC1PStageClear1TricksterTextSprite, 8000 },
#endif

	// Giant Impact
#if defined(REGION_US)
	{ &llSC1PStageClear1GiantImpactTextSprite, 0 },
#else
	{ &llSC1PStageClear1GiantImpactTextSprite, 7000 },
#endif

	// Speedster
#if defined(REGION_US)
	{ &llSC1PStageClear1SpeedsterTextSprite, 10000 },
#else
	{ &llSC1PStageClear1SpeedsterTextSprite, 8000 },
#endif

	// Item Throw
#if defined(REGION_US)
	{ &llSC1PStageClear1ItemThrowTextSprite, 16000 },
#else
	{ &llSC1PStageClear1ItemThrowTextSprite, 10000 },
#endif

	// Triple KO
#if defined(REGION_US)
	{ &llSC1PStageClear1TripleKOTextSprite, 0 },
#else
	{ &llSC1PStageClear1TripleKOTextSprite, 15000 },
#endif

	// Last Chance
#if defined(REGION_US)
	{ &llSC1PStageClear1LastChanceTextSprite, 0 },
#else
	{ &llSC1PStageClear1LastChanceTextSprite, 15000 },
#endif

	// Pacifist
#if defined(REGION_US)
	{ &llSC1PStageClear1PacifistTextSprite, 60000 },
#else
	{ &llSC1PStageClear1PacifistTextSprite, 30000 },
#endif

	// Perfect
#if defined(REGION_US)
	{ &llSC1PStageClear1PerfectTextSprite, 30000 },
#else
	{ &llSC1PStageClear1PerfectTextSprite, 10000 },
#endif

	// No Miss
#if defined(REGION_US)
	{ &llSC1PStageClear1NoMissTextSprite, 5000 },
#else
	{ &llSC1PStageClear1NoMissTextSprite, 1500 },
#endif

	// No Damage
#if defined(REGION_US)
	{ &llSC1PStageClear1NoDamageTextSprite, 15000 },
#else
	{ &llSC1PStageClear1NoDamageTextSprite, 10000 },
#endif

	// Full Power
	{ &llSC1PStageClear1FullPowerTextSprite, 5000 },

	// Final Stage Clear
#if defined(REGION_US)
	{ &llSC1PStageClear1VeryEasyClearTextSprite, 70000 },
#else
	{ &llSC1PStageClear1VeryEasyClearTextSprite, 40000 },
#endif

	// No Miss Clear
#if defined(REGION_US)
	{ &llSC1PStageClear1NoMissClearTextSprite, 70000 },
#else
	{ &llSC1PStageClear1NoMissClearTextSprite, 40000 },
#endif

	// No Damage Clear
#if defined(REGION_US)
	{ &llSC1PStageClear1NoDamageClearTextSprite, 400000 },
#else
	{ &llSC1PStageClear1NoDamageClearTextSprite, 300000 },
#endif

	// Speed King
#if defined(REGION_US)
	{ &llSC1PStageClear1SpeedKingTextSprite, 40000 },
#else
	{ &llSC1PStageClear1SpeedKingTextSprite, 20000 },
#endif

	// Speed Demon
#if defined(REGION_US)
	{ &llSC1PStageClear1SpeedDemonTextSprite, 80000 },
#else
	{ &llSC1PStageClear1SpeedDemonTextSprite, 60000 },
#endif

	// Mew Catcher
#if defined(REGION_US)
	{ &llSC1PStageClear1MewCatcherTextSprite, 15000 },
#else
	{ &llSC1PStageClear1MewCatcherTextSprite, 8000 },
#endif

	// Star Clear
#if defined(REGION_US)
	{ &llSC1PStageClear1StarClearTextSprite, 12000 },
#else
	{ &llSC1PStageClear1StarClearTextSprite, 8000 },
#endif

	// Vegetarian
#if defined(REGION_US)
	{ &llSC1PStageClear1VegetarianTextSprite, 9000 },
#else
	{ &llSC1PStageClear1VegetarianTextSprite, 5000 },
#endif

	// Heart Throb
#if defined(REGION_US)
	{ &llSC1PStageClear1HeartThrobTextSprite, 17000 },
#else
	{ &llSC1PStageClear1HeartThrobTextSprite, 8000 },
#endif

	// Throw Down
	{ &llSC1PStageClear1ThrowDownTextSprite, 2000 },

	// Smash Mania
#if defined(REGION_US)
	{ &llSC1PStageClear1SmashManiaTextSprite, 3500 },
#else
	{ &llSC1PStageClear1SmashManiaTextSprite, 3000 },
#endif

	// Smashless
#if defined(REGION_US)
	{ &llSC1PStageClear1SmashlessTextSprite, 5000 },
#else
	{ &llSC1PStageClear1SmashlessTextSprite, 3000 },
#endif

	// Special Move
#if defined(REGION_US)
	{ &llSC1PStageClear1SpecialMoveTextSprite, 5000 },
#else
	{ &llSC1PStageClear1SpecialMoveTextSprite, 5000 },
#endif

	// Single Move
	{ &llSC1PStageClear1SingleMoveTextSprite, 8000 },

	// Pokemon Finish
#if defined(REGION_US)
	{ &llSC1PStageClear1PokemonFinishTextSprite, 11000 },
#else
	{ &llSC1PStageClear1PokemonFinishTextSprite, 8000 },
#endif

	// Booby Trap
#if defined(REGION_US)
	{ &llSC1PStageClear1BoobyTrapTextSprite, 12000 },
#else
	{ &llSC1PStageClear1BoobyTrapTextSprite, 8000 },
#endif

	// Fighter Stance
	{ &llSC1PStageClear1FighterStanceTextSprite, 100 },

	// Mystic
#if defined(REGION_US)
	{ &llSC1PStageClear1MysticTextSprite, 7000 },
#else
	{ &llSC1PStageClear1MysticTextSprite, 6000 },
#endif

	// Comet Mystic
#if defined(REGION_US)
	{ &llSC1PStageClear1CometMysticTextSprite, 10000 },
#else
	{ &llSC1PStageClear1CometMysticTextSprite, 7000 },
#endif

	// Acid Clear
#if defined(REGION_US)
	{ &llSC1PStageClear1AcidClearTextSprite, 1500 },
#else
	{ &llSC1PStageClear1AcidClearTextSprite, 1000 },
#endif

	// Bumper Clear
#if defined(REGION_US)
	{ &llSC1PStageClear1BumperClearTextSprite, 10000 },
#else
	{ &llSC1PStageClear1BumperClearTextSprite, 3000 },
#endif

	// Tornado Clear
	{ &llSC1PStageClear1TornadoClearTextSprite, 3000 },

	// ARWING Clear
#if defined(REGION_US)
	{ &llSC1PStageClear1ArwingClearTextSprite, 4000 },
#else
	{ &llSC1PStageClear1ArwingClearTextSprite, 3000 },
#endif

	// Counter Attack
#if defined(REGION_US)
	{ &llSC1PStageClear1CounterAttackTextSprite, 0 },
#else
	{ &llSC1PStageClear1CounterAttackTextSprite, 5000 },
#endif

	// Meteor Smash
#if defined(REGION_US)
	{ &llSC1PStageClear1MeteorSmashTextSprite, 0 },
#else
	{ &llSC1PStageClear1MeteorSmashTextSprite, 6000 },
#endif

	// Aerial
#if defined(REGION_US)
	{ &llSC1PStageClear1AerialTextSprite, 0 },
#else
	{ &llSC1PStageClear1AerialTextSprite, 20000 },
#endif

	// Last Second
#if defined(REGION_US)
	{ &llSC1PStageClear1LastSecondTextSprite, 8000 },
#else
	{ &llSC1PStageClear1LastSecondTextSprite, 10000 },
#endif

	// Lucky 3
#if defined(REGION_US)
	{ &llSC1PStageClear1Lucky3TextSprite, 9990 },
#else
	{ &llSC1PStageClear1Lucky3TextSprite, 8000 },
#endif

	// Jackpot
#if defined(REGION_US)
	{ &llSC1PStageClear1JackpotTextSprite, 3330 },
#else
	{ &llSC1PStageClear1JackpotTextSprite, 5000 },
#endif

	// Yoshi Rainbow
#if defined(REGION_US)
	{ &llSC1PStageClear1YoshiRainbowTextSprite, 50000 },
#else
	{ &llSC1PStageClear1YoshiRainbowTextSprite, 15000 },
#endif

	// Kirby Ranks
#if defined(REGION_US)
	{ &llSC1PStageClear1KirbyRanksTextSprite, 25000 },
#else
	{ &llSC1PStageClear1KirbyRanksTextSprite, 12000 },
#endif

	// Bros. Calamity
#if defined(REGION_US)
	{ &llSC1PStageClear1BrosCalamityTextSprite, 25000 },
#else
	{ &llSC1PStageClear1BrosCalamityTextSprite, 12000 },
#endif

	// DK Defender
#if defined(REGION_US)
	{ &llSC1PStageClear1DKDefenderTextSprite, 10000 },
#else
	{ &llSC1PStageClear1DKDefenderTextSprite, 7000 },
#endif

	// DK Perfect
	{ &llSC1PStageClear1DKPerfectTextSprite, 50000 },

	// Good Friend
#if defined(REGION_US)
	{ &llSC1PStageClear1GoodFriendTextSprite, 8000 },
#else
	{ &llSC1PStageClear1GoodFriendTextSprite, 5000 },
#endif

	// True Friend
#if defined(REGION_US)
	{ &llSC1PStageClear1TrueFriendTextSprite, 25000 }
#else
	{ &llSC1PStageClear1TrueFriendTextSprite, 30000 },
#endif
};

#endif /* NDS_P2_1P_GAME */
