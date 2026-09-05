"""Per-stage descriptors for the native stage packet generator.

Step 1 of the P2-4n1 parameterisation: the stage is a parameter, with
Dream Land as the default. Each descriptor freezes one stage's pinned
inputs, owners, materials, expected counts and segment-0 goldens exactly
as the generator previously hardcoded them. No numeric value here is new;
Dream Land reads byte-for-byte as the former module constants.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Union


@dataclass(frozen=True)
class StageDescriptor:
    """All per-stage inputs the generator used to hold as globals."""

    name: str
    include_sha: str
    generated_segment_index: int
    # P2-4n1 step 5 (BLOCKER A). Two stage packets land in ONE translation
    # unit (`src/nds/nds_renderer_assets.c` includes them back to back), so a
    # second include may not re-emit the shared struct typedefs, may not
    # redefine the `NDS_NATIVE_STAGE_*` macros and may not redefine the
    # `sNdsNativeStage*` / `gNdsNativeStage*` objects. A non-empty prefix pair
    # switches the emitter to namespaced output. Dream Land keeps both empty,
    # which makes the namespacing pass a provable no-op on the frozen include.
    symbol_prefix: str = ""
    macro_prefix: str = ""
    expected_counts: dict = field(default_factory=dict)
    o2r_inputs: dict = field(default_factory=dict)
    text_inputs: dict = field(default_factory=dict)
    text_contract_tokens: dict = field(default_factory=dict)
    map_constructor_text_key: str = ""
    map_constructor_token: str = ""
    map_constructor_min_count: int = 0
    asset_order: tuple = ()
    owner_specs: tuple = ()
    material_sources: tuple = ()
    material_command_partition: tuple = ()
    segment_partition: tuple = ()
    callback_partition: tuple = ()
    segment0: dict = field(default_factory=dict)
    # P2-4n1 step 3: the native stage checker half. Step 2 moved the
    # runtime adapter's five counts plus its asset id/size tables into a
    # per-stage C descriptor; the same values move here so a second stage
    # can supply its own without editing the checker. Counts mirror the
    # C maxima (segment/dobj/binding/asset/material); the asset tables
    # mirror sNdsRendererAdapterNativeStageDreamLand verbatim.
    adapter_segment_count: int = 0
    adapter_dobj_count: int = 0
    adapter_binding_count: int = 0
    adapter_asset_count: int = 0
    adapter_material_count: int = 0
    adapter_asset_ids: tuple = ()
    adapter_asset_sizes: tuple = ()


def _dreamland() -> StageDescriptor:
    from native_stage_descriptors import dreamland as _dl

    return _dl.DESCRIPTOR


def _yoster() -> StageDescriptor:
    from native_stage_descriptors import yoster as _yo

    return _yo.DESCRIPTOR


def _castle() -> StageDescriptor:
    from native_stage_descriptors import castle as _ca

    return _ca.DESCRIPTOR


def _jungle() -> StageDescriptor:
    from native_stage_descriptors import jungle as _ju

    return _ju.DESCRIPTOR


def _sector() -> StageDescriptor:
    from native_stage_descriptors import sector

    return sector.DESCRIPTOR


def _hyrule() -> StageDescriptor:
    from native_stage_descriptors import hyrule

    return hyrule.DESCRIPTOR


def _zebes() -> StageDescriptor:
    from native_stage_descriptors import zebes

    return zebes.DESCRIPTOR


def _yamabuki() -> StageDescriptor:
    from native_stage_descriptors import yamabuki

    return yamabuki.DESCRIPTOR


def _inishie() -> StageDescriptor:
    from native_stage_descriptors import inishie

    return inishie.DESCRIPTOR


def _metal() -> StageDescriptor:
    from native_stage_descriptors import metal

    return metal.DESCRIPTOR


def _last() -> StageDescriptor:
    from native_stage_descriptors import last

    return last.DESCRIPTOR


def _zako() -> StageDescriptor:
    from native_stage_descriptors import zako

    return zako.DESCRIPTOR


def _yostersmall() -> StageDescriptor:
    from native_stage_descriptors import yostersmall

    return yostersmall.DESCRIPTOR


def _pupupusmall() -> StageDescriptor:
    from native_stage_descriptors import pupupusmall

    return pupupusmall.DESCRIPTOR


def _bonus1_mario() -> StageDescriptor:
    from native_stage_descriptors import bonus1_mario

    return bonus1_mario.DESCRIPTOR


def _bonus2_mario() -> StageDescriptor:
    from native_stage_descriptors import bonus2_mario

    return bonus2_mario.DESCRIPTOR


def _bonus2_fox() -> StageDescriptor:
    from native_stage_descriptors import bonus2_fox

    return bonus2_fox.DESCRIPTOR


def _bonus2_donkey() -> StageDescriptor:
    from native_stage_descriptors import bonus2_donkey

    return bonus2_donkey.DESCRIPTOR


def _bonus2_samus() -> StageDescriptor:
    from native_stage_descriptors import bonus2_samus

    return bonus2_samus.DESCRIPTOR


def _bonus2_luigi() -> StageDescriptor:
    from native_stage_descriptors import bonus2_luigi

    return bonus2_luigi.DESCRIPTOR


def _bonus2_link() -> StageDescriptor:
    from native_stage_descriptors import bonus2_link

    return bonus2_link.DESCRIPTOR


def _bonus2_yoshi() -> StageDescriptor:
    from native_stage_descriptors import bonus2_yoshi

    return bonus2_yoshi.DESCRIPTOR


def _bonus2_captain() -> StageDescriptor:
    from native_stage_descriptors import bonus2_captain

    return bonus2_captain.DESCRIPTOR


def _bonus2_kirby() -> StageDescriptor:
    from native_stage_descriptors import bonus2_kirby

    return bonus2_kirby.DESCRIPTOR


def _bonus2_pikachu() -> StageDescriptor:
    from native_stage_descriptors import bonus2_pikachu

    return bonus2_pikachu.DESCRIPTOR


def _bonus2_purin() -> StageDescriptor:
    from native_stage_descriptors import bonus2_purin

    return bonus2_purin.DESCRIPTOR


def _bonus2_ness() -> StageDescriptor:
    from native_stage_descriptors import bonus2_ness

    return bonus2_ness.DESCRIPTOR


def _bonus3() -> StageDescriptor:
    from native_stage_descriptors import bonus3

    return bonus3.DESCRIPTOR


def _bonus1_fox() -> StageDescriptor:
    from native_stage_descriptors import bonus1_fox

    return bonus1_fox.DESCRIPTOR


def _bonus1_donkey() -> StageDescriptor:
    from native_stage_descriptors import bonus1_donkey

    return bonus1_donkey.DESCRIPTOR


def _bonus1_samus() -> StageDescriptor:
    from native_stage_descriptors import bonus1_samus

    return bonus1_samus.DESCRIPTOR


def _bonus1_luigi() -> StageDescriptor:
    from native_stage_descriptors import bonus1_luigi

    return bonus1_luigi.DESCRIPTOR


def _bonus1_link() -> StageDescriptor:
    from native_stage_descriptors import bonus1_link

    return bonus1_link.DESCRIPTOR


def _bonus1_yoshi() -> StageDescriptor:
    from native_stage_descriptors import bonus1_yoshi

    return bonus1_yoshi.DESCRIPTOR


def _bonus1_captain() -> StageDescriptor:
    from native_stage_descriptors import bonus1_captain

    return bonus1_captain.DESCRIPTOR


def _bonus1_kirby() -> StageDescriptor:
    from native_stage_descriptors import bonus1_kirby

    return bonus1_kirby.DESCRIPTOR


def _bonus1_pikachu() -> StageDescriptor:
    from native_stage_descriptors import bonus1_pikachu

    return bonus1_pikachu.DESCRIPTOR


def _bonus1_purin() -> StageDescriptor:
    from native_stage_descriptors import bonus1_purin

    return bonus1_purin.DESCRIPTOR


def _bonus1_ness() -> StageDescriptor:
    from native_stage_descriptors import bonus1_ness

    return bonus1_ness.DESCRIPTOR


_REGISTRY: dict[str, object] | None = None


def _registry() -> dict:
    global _REGISTRY
    if _REGISTRY is None:
        _REGISTRY = {"dreamland": _dreamland(), "yoster": _yoster(),
                     "castle": _castle(), "jungle": _jungle(),
                     "sector": _sector(), "hyrule": _hyrule(),
                     "zebes": _zebes(),
                     "yamabuki": _yamabuki(),
                     "inishie": _inishie(),
                     "metal": _metal(), "last": _last(), "zako": _zako(),
                      "yostersmall": _yostersmall(),
                      "pupupusmall": _pupupusmall(),
                       "bonus1_mario": _bonus1_mario(),
                       "bonus1_fox": _bonus1_fox(),
                       "bonus1_donkey": _bonus1_donkey(),
                       "bonus1_samus": _bonus1_samus(),
                       "bonus1_luigi": _bonus1_luigi(),
                       "bonus1_link": _bonus1_link(),
                       "bonus1_yoshi": _bonus1_yoshi(),
                       "bonus1_captain": _bonus1_captain(),
                       "bonus1_kirby": _bonus1_kirby(),
                       "bonus1_pikachu": _bonus1_pikachu(),
                       "bonus1_purin": _bonus1_purin(),
                       "bonus1_ness": _bonus1_ness(),
                       "bonus2_mario": _bonus2_mario(),
                       "bonus2_fox": _bonus2_fox(),
                       "bonus2_donkey": _bonus2_donkey(),
                       "bonus2_samus": _bonus2_samus(),
                       "bonus2_luigi": _bonus2_luigi(),
                       "bonus2_link": _bonus2_link(),
                       "bonus2_yoshi": _bonus2_yoshi(),
                       "bonus2_captain": _bonus2_captain(),
                       "bonus2_kirby": _bonus2_kirby(),
                       "bonus2_pikachu": _bonus2_pikachu(),
                       "bonus2_purin": _bonus2_purin(),
                       "bonus2_ness": _bonus2_ness(),
                       "bonus3": _bonus3()}
    return _REGISTRY  # type: ignore[return-value]


def list_stages() -> tuple[str, ...]:
    return tuple(sorted(_registry().keys()))


def get_descriptor(stage: Union[str, StageDescriptor, None] = "dreamland") -> StageDescriptor:
    """Resolve a stage name (default ``dreamland``) to its descriptor."""
    if stage is None:
        stage = "dreamland"
    if isinstance(stage, StageDescriptor):
        return stage
    registry = _registry()
    if stage not in registry:
        raise ValueError(
            f"unknown stage {stage!r}; known stages: {sorted(registry.keys())}"
        )
    return registry[stage]  # type: ignore[return-value]
