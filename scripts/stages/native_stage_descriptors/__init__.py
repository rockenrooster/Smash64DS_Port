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


_REGISTRY: dict[str, object] | None = None


def _registry() -> dict:
    global _REGISTRY
    if _REGISTRY is None:
        _REGISTRY = {"dreamland": _dreamland(), "yoster": _yoster()}
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
