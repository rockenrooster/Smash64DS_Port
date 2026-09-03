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

# Deliberately NOT imaged: `PreparedDense` is mutable draw scratch rather than
# content, `Roots` and `CrossPaletteSlots` belong to the owner rather than to
# its table set, and the light preambles are shared across owners.
NATIVE_OWNER_RESIDENT_ARRAYS = (
    "PreparedDense",
    "Roots",
    "CrossPaletteSlots",
)
