"""The one list of native-owner arrays that ship as a NitroFS image.

Two generators have to agree about this set and they cannot import each other:
`generate_nds_native_owner_images.py` builds the image struct FROM an owner
context, and `generate_nds_native_owners.py` emits the in-binary arrays that
the image replaces. If the two lists ever disagree, the failure is silent in
the worst possible way -- an array present in both places wastes the arena it
was moved to save, and an array present in neither is a dangling table pointer
in a draw path. So the list lives here, in the module both import, and each
generator asserts its own view against it.

The names are the owner generator's array-name stems: the text between the
`sNdsNative<Owner>Fighter` prefix and the `Low` detail suffix.
"""

NATIVE_OWNER_IMAGE_ARRAYS = (
    "StateDeltas",
    "StateSequence",
    "VertexActions",
    "EpochDirectPolicy",
    "DenseVertices",
    "DenseNormals",
    # PreparedDense is the draw path's MUTABLE GX-packed vertex scratch, but
    # its INITIAL bytes are fully determined at generation time (baked GX
    # positions, zeroed UV/color fields) and the buffer it lives in is already
    # scene-owned arena: the image payload is read into a taskman-arena buffer
    # at fighter construction and the scene manager rewinds the arena between
    # scenes. Keeping the initial bytes in the image removes one static
    # initialized array per owner+detail from ARM9 binaries for scenes that
    # never use that owner, while the runtime keeps writing the resident
    # image copy exactly as it wrote the static one. No in-battle paging: the
    # whole array loads once with the rest of its owner image.
    "PreparedDense",
    "ActionDenseSpans",
    "DenseColorSource",
    "PackedCorners",
    "RunFirstCorner",
    "RunFirstUnique",
    "RunUniqueCount",
    "RunUniqueDense",
    "Triangles",
    "Runs",
    "PrimitiveGroupFirst",
    "PrimitiveGroupCount",
    "PrimitiveGroupType",
    "PrimitiveGroupFirstVertex",
    "PrimitiveGroupVertexCount",
    "PrimitiveVertices",
    "Epochs",
)

# Still NOT imaged: `Roots` and `CrossPaletteSlots` belong to the owner rather
# than to its table set, and the light preambles are shared across owners.
# (`PreparedDense` used to be listed here as resident scratch; it moved into
# the image with its initial bytes: this trades permanent RAM for the same
# number of bytes in each loaded owner's scene-owned writable buffer.)
NATIVE_OWNER_RESIDENT_ARRAYS = (
    "Roots",
    "CrossPaletteSlots",
)
