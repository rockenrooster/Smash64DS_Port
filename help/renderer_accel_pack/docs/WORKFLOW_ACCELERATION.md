# Workflow acceleration recommendations

The project is now past bootstrapping. The main blocker is no longer “can original code run?” but “can DS backend contracts render what original code submits?”

## Replace open-ended `/goal` work with boundary packets

For every Codex session, use:

```text
current boundary:
next boundary:
allowed files/systems:
forbidden expansions:
verification markers:
stop condition:
```

The stop condition is the most important part. Without it, the agent will recursively import unrelated dependencies.

## Bigger slices, but still bounded

Bigger slices are useful now, but only if they are backend slices, not gameplay slices.

Good bigger slices:

- renderer state machine for known GBI commands
- one DObj/MObj material path
- one texture format family
- one camera/matrix path
- one VRAM upload/cache prototype

Bad bigger slices:

- import all Title/menu code and all renderer code at once
- import fighters while renderer is still primitive-only
- run full `gcDrawAll` continuously without command budgets
- import full `lbcommon.c` if it pulls fighter/ground/full display dependencies unexpectedly

## Two-agent workflow

Use one active agent and one parallel prep lane.

Active agent:

- modifies verified runtime
- keeps build/verifier green
- advances one runtime boundary

Parallel prep agent / this pack:

- writes docs, manifests, helper APIs, conversion code, scripts
- does not change verified runtime
- prepares later drop-ins

## Renderer-first ordering

Fastest route to visible progress:

1. strengthen current Startup logo and Opening Room preview
2. implement command-state tracking for the already-parsed DObj path
3. add matrix/camera transform for that same selected path
4. add texture-source discovery and decode for one texture-bearing MObj
5. add DS texture upload/cache for that one texture
6. broaden from one DObj path to a small bounded list
7. then import real Title scene slice
8. then stage/fighter probes

## Verification improvement

Every renderer claim should have numeric verifier evidence:

- commands parsed
- branches resolved
- vertices loaded
- triangles submitted
- matrix stack max depth
- texture source pointer and size
- decoded texels count
- VRAM allocation bytes
- nonzero output pixels
- first unsupported opcode

Screenshots are good, but they should supplement marker checks, not replace them.
