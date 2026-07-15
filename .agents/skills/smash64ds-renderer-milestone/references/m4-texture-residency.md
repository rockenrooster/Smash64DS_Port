# M4 orientation — July 15 snapshot

Verify every number against the live memory state.

Current planning evidence:

- ordinary gameplay conversion remains roughly a 189K-class wall;
- conservative static census is about 69 keys / 179,328 texture bytes;
- two live water owners raise the simple estimate to about 216,192 bytes;
- current texture VRAM mapping is 262,144 bytes;
- main-RAM reserve must remain at least 131,072 bytes;
- current cache cardinality is too small for the proposed manifest;
- exact water corpus cannot fit as simple fully resident RGB256/palette output.

Manifest keys must include all fields that determine exact visible output, not
only image/TLUT offsets. Arm a post-GO fence and require zero critical-path
conversion, preparation, allocation, I/O, replacement, refresh, and upload work
for the supported set. Treat water as a separate representation experiment.
