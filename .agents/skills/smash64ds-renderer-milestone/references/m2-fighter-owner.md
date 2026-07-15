# M2 orientation — July 15 snapshot

Verify against the live native plan and ledger.

Current owner contract:

- 32 roots
- 25 Mario / 27 Fox joints
- 14 / 18 live bindings
- 49 epochs
- 67 runs
- 626 triangles
- 541 immutable vertices
- 44 cross-matrix triangles
- 11/11 pushes/pops, 10 stores, 84 restores

Current evidence that constrains the design:

- clean combined owner is roughly 412–414K ticks;
- CPU preorder saved only about 10.5K;
- per-joint GX hierarchy saved about 13.5K;
- split-matrix/hierarchy-only work is insufficient;
- hardware one-light shading misses the exact corpus, so CPU lighting remains;
- generated fractional-bias 20.12 texture matrices matched the current corpus;
- whole-owner FIFO packet regressed from 413,504 to 537,792 ticks;
- many small GX lists and generic packet/VM forms are already rejected.

The next packet must preflight and fuse source-exact local matrices, exact
lighting, live materials, and owner production. It must remove work rather than
copying an already-prepared output stream.
