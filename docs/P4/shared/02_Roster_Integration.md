# Shared plan 02 — Expand identities without breaking the original game

**Status:** proposed. Parent: [master](../New_Characters.md).

## Preserve legacy meanings

Existing original-cast, boss, Metal Mario and polygon IDs, sentinels and serialized meanings must not move accidentally. The source-mirrored FTKind/header arrangement makes "insert 18 enum entries before the bosses" unsafe without coordinated changes. Audit every kind-indexed table, range predicate, raw mask and save/network serializer before release admission. Prior source inspection located both the mirrored enum and a Captain victim-kind offset table; those are starting points, not an exhaustive census.

Separate stable content identity, runtime fighter kind, native asset owner, four-instance player slot and CSS position. A compact generated translation table is sufficient. This is not a request for a runtime object framework. A child can share a native asset without becoming its donor for AI, records, copy behavior or result naming.

The eventual 30 selectable characters are not the total runtime kind count: legacy nonplayable variants already occupy IDs. Audit shifts and mask widths even when 30 seems to fit in 32 bits. Prefer bounded multiword bitsets or generated membership tables at the relevant seam; persist stable IDs rather than raw internal indices or CSS positions. Preserve existing save bytes through an explicit version/migration test, not by interpreting them under a new enum.

## Source and generated inventory

For each selection generate a descriptor naming source profile, effective actions/callbacks, native owner, costume and team mappings, UI/audio keys, AI contract, copy policy, resource fragments and supported modes. Source-qualified IDs remain explicit until converted. A costume count is not a guarantee all colors share the same model or palette footprint.

Treat the roster registry as the one admission authority. Dev-only incomplete selections may be enabled explicitly, but an enabled feature flag is not acceptance. Random selection, CSS navigation, proof builds and multiplayer descriptors must draw from the same admitted membership, not independent hard-coded lists.

## CSS and scene loading

Keep the source-derived portraits, names, stock icons, series emblems, announcer cues and actual preview behavior. Expand navigation/layout for the roster without preloading every fighter's battle closure. Use bounded scene-specific preview packs for the simultaneously visible selections and a measured cache/read policy appropriate to the existing lazy-CSS work.

Do not call a synchronous file read "asynchronous" merely because it was split into small requests. Long reads need the established DS I/O service/loading transition and an audio-safe presentation policy. Preview requests must be cancelable by generation: fast cursor movement may not install a stale selection's model. Preload results/entry resources in their correct scene or match epoch and measure transition peak memory.

Four player previews do not imply four independent copies of immutable data; four live fighters do imply four independent mutable states. Parent characters need not be selected for inherited data to be loaded.

## Interactions, CPU and copy powers

Audit the fighter as both attacker and victim: ordinary grabs, special captures, throws, inhale, egg/damage state, ledge/camera offsets and KO handling. Populate or replace victim-kind lookup domains intentionally. Reusing a parent's height/offset entry without checking the new topology can corrupt behavior even when attacks look correct.

Port each donor's required CPU attack and movement/recovery logic with the fighter. For example, Meta Knight includes CPU.asm; the main Remix characters reference AI/Attacks.asm. Effective behavior may also require shared AI hooks. Do not batch CPU support at the end or silently fall back to the parent forever.

Resolve the exact donor Kirby policy. It may be a shared power, a unique power or a source-defined no-copy outcome. The requirement is fidelity, not inventing eighteen new abilities. MRGAW's hat ID 0x08 is a mapping to resolve, not evidence that Chef must be copied. For unique abilities, keep logic/assets/article/state fragments separate from whole-fighter packs.

Prepare all copy abilities reachable in the selected match before GO, including resources that may remain needed after the donor dies. Use the actual copy/reset/transfer rules when determining reachability. Test multiple Kirbys independently, and do not unload a copied article's resources on a cosmetic hat change.

## P3 compatibility and authoritative state

P4 does not choose a new synchronization algorithm. Use P3's adopted protocol when available, with explicit build/content/profile compatibility. Send stable selection and costume identities, not process pointers or local native-owner addresses. Match-start compatibility must include all peers' required generated gameplay content and settings.

Make special state, buffers, article identity/generation, authoritative RNG and reset order explicit in replay/state checks. Keep authoritative decisions independent of local frame drops, optional rendering, audio completion and unordered allocation. Merely being designed deterministically is not a network pass: each character's P3 arm remains pending until tested under the actual protocol.

## Mode tiers and exit

Proposed first P4 tier is complete VS and Training; wireless is a separately tested P3 integration. Campaign/bonus/records expansion is a named later tier, not silently claimed done. Preserve the original campaign while adding P4.

Where donor content already supplies an assignment, audit it before inventing a replacement: Meta Knight explicitly selects Kirby bonus stages. That does not prove they are completable by the DS version. Metal Mario needs a clear distinction between selectable donor behavior and the existing campaign encounter.

Exit with registry-driven selection, stable legacy IDs/saves, generated UI/audio/AI/copy mappings, bounded preview residency, documented mode status and clean scene-generation ownership. All original-roster regression arms remain required.
