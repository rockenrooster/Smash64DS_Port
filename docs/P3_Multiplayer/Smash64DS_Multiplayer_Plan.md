# P3 — Smash64DS Local Multiplayer Plan

**Status:** Proposed architecture and implementation plan; not a claim of implemented or hardware-validated multiplayer.

**Review baseline:** `master` at `806c007b88b3f753642925500d73314ed905e460`. At review, this was newer than `p2-pikachu` and `codex/r2-runtime2`.

**Intended location:** `docs/P3_Multiplayer/Multiplayer.md`

## 1. Decision

Build **routerless, multi-card, local DS wireless multiplayer using host-coordinated deterministic input lockstep**. Every console runs the whole match. The host owns session configuration and the ordered, confirmed input stream; it is not a server that streams fighter positions to passive clients.

Use a small, fixed input delay selected before each match. Do not depend on rollback, prediction, full-state replication, or mid-match state restoration for the first release.

This is the recommended architecture, subject to two independent feasibility gates: a supported wireless backend integrated with this project's SDK, and deterministic simulation from a common initial state and input stream.

## 2. Product scope

Preserve the original draft's flow:

- `VS Mode > VS Start > Host: Off / On`, default Off.
- `VS Mode > VS Join > Searching for host... > Select room > Lobby`.
- Two to four human players, each using one console and one copy of the game.
- Humans plus CPUs total no more than four fighters. Support teams and free-for-all.
- Host controls match settings. Each guest controls their own fighter selection and Ready state, subject to host validation.
- With fewer than two humans, use the ordinary offline path. Do not run the wireless session just to supply CPU opponents.

The initial supported product is nearby-console play without a router, account, server, Internet connection, or Download Play transfer. Internet play, spectators, late joining, reconnecting into an active match, host migration, and automatic CPU takeover are out of scope.

A human who loses all stocks remains connected and participates in synchronization until the match ends. Eliminated players do not become a new networking mode.

## 3. Repository evidence and integration boundaries

The existing product contract prioritizes stable 30 FPS, with a representative-gameplay P95 target around 1.12 million ARM9 timing ticks per presentation. Multiplayer must fit inside that contract rather than receive an additional frame budget. [R2]

The current runtime batches two 60 Hz source updates per presentation by default. It explicitly avoids compensating a missed retrace with an unbounded later simulation burst. Preserve that policy. A setting that merely changes the update count from two to one is documented as uncompensated half-speed, not a shipping optimization. [R4, R5]

`src/port/controller_backend.c` already supports an array of four playback pads and a connected-player mask. Live hardware input normally supplies slot zero. Its playback commit function currently updates a counter; it is not a production synchronization barrier. Generalize the input-provider boundary rather than replacing game mechanics with networking calls. [R3]

The source controller wrapper documents a real duplicate-publication problem: publishing an already-drained edge accumulator can erase button taps. Networking must preserve the source's read/publication contract and make retransmissions invisible to it. [R6]

The battle runs inside its own scene loop. A network pump added only to the outer `main()` loop is not sufficient. [R4, R7]

The existing Task 9 hash is useful diagnostic infrastructure, but it walks raw structures and deliberately normalizes multiple static pointer identities to the same value. It must not be relabeled as a complete multiplayer determinism proof. [R8]

## 4. Resolve the wireless-stack dependency first

### 4.1 Current blocker

This project uses a devkitPro build with Calico integration. In the reviewed upstream Calico header, `WlMgrMode_LocalComms` is explicitly marked **not yet supported**. An enum entry and raw packet API are not evidence that managed local host/client multiplayer works. [R9, E1]

BlocksDS provides a documented local multiplayer implementation, but its migration guide excludes Calico/libnds 2.x migration from its ordinary migration instructions. Do not assume BlocksDS headers or libraries are drop-in additions to this build. [E2, E3]

### 4.2 Recommended execution path

First bring up the official BlocksDS local multiplayer example as a separate, minimal test ROM. Establish discovery, joining, two-console data exchange, and then four-console group exchange. Pin the exact SDK/library revisions that pass.

Next, create a separate compatibility target or branch to evaluate running the existing game with that supported stack. Preserve the working devkitPro production target while measuring build compatibility, memory layout, audio, rendering, input, storage, and timing.

Prefer using an existing supported multiplayer implementation over writing a new wireless driver as the first project step. If integration requires a broader SDK migration, make that an explicit, reviewed work item. If maintaining Calico instead requires implementing or porting local multiplayer support, classify it as a real driver/SDK project, not a small networking wrapper.

The transport gate passes only when one concrete approach works with the full game's renderer, sound, and memory allocation. A minimal demonstration on a different SDK proves the radio concept, not integration with Smash64DS.

Do not silently replace routerless multiplayer with router-based UDP to avoid this dependency. That would be a different product decision.

### 4.3 Transport selection

Prefer the managed host/client multiplayer transfer path. Configure at most three clients plus the host. Predeclare bounded packet capacities and use compact game messages. Use local-only initialization where supported, without starting an unnecessary TCP/IP stack. DSi-family testing must use the library's supported DS-compatible local-wireless path. [E2, E4]

Keep the native transport behind a small application interface, with a deterministic fake transport for tests. Do not introduce a general networking framework or let fighter code depend on the radio API.

## 5. Simulation and authority model

```text
Host keypad ----> local input provider ----\
Client 1 keypad -> wireless input ---------+--> Host confirms input batches
Client 2 keypad -> wireless input ---------+
Client 3 keypad -> wireless input ---------/
                                                   |
                             same confirmed input log to every console
                                                   |
                              each DS executes the same complete match
```

The host controls membership, stable slot ownership, rules, stage, CPU settings, final selection resolution, match seed, input ordering, and lifecycle commands.

All consoles simulate fighter behavior, collision, hitboxes, knockback, items, hazards, match timers, and CPU opponents. Do not network CPU controller decisions separately in version one: that creates another authoritative subsystem without removing the need for deterministic collision and game state.

Keep three concepts distinct: console identity, wireless association identifier, and SSB controller/fighter slot. A guest owning P3 remains P3 on every machine; their own console must not globally remap them to P1. CPU slots are fighter slots, not extra wireless peers. Keep human-input requirements separate from occupied-fighter masks.

The host submits its local input through the same delayed path as everyone else. It must not simulate its own buttons immediately while delaying guest buttons.

This model does not distribute the game's rendering or simulation cost between consoles. Every DS still needs to fit and run the full selected match.

## 6. Input clock, batching, and latency

### 6.1 Define the clocks

Use a monotonically increasing **source simulation tick** as the canonical gameplay index. With the current default, one presentation batch contains two source ticks. At nominal speed, these represent approximately 16.7 ms and 33.3 ms respectively. These are logical timing units, not guarantees about radio latency or actual wall-clock progress under load. [R4, R5]

The protocol must never use “frame” without specifying whether it means a simulation tick, an input batch, or a displayed frame.

Preserve existing input sampling behavior for the first implementation. Do not claim 60 independent hardware samples per second merely because the source simulation updates twice per presentation. Record the actual offline sample/read/publish timeline and make the network provider reproduce it.

### 6.2 Initial delay policy

For the first playable integration, evaluate **one presentation batch of added input delay**: two source ticks, nominally about 33 ms. Also test a two-batch profile, nominally about 67 ms. These are proposed test settings, not measured responsiveness claims.

Choose the lowest fixed setting that passes end-to-end delivery and playability tests. Measure sample-to-confirmed-input availability at every peer, including guest-to-host-to-other-guest routing, application scheduling, and stale preloaded replies. A favorable ping average alone is not admission evidence.

Keep delay fixed during a match. Any later adaptive-delay feature requires an explicit synchronized change that cannot duplicate or omit inputs. Do not add it to version one.

### 6.3 Bounded producer/consumer protocol

At entry to a new logical batch `b`, each peer captures its local input once for a future batch `b + D`, queues it for transmission, and then checks whether batch `b` is ready. Bootstrap the first `D` input batches with an explicitly agreed neutral sequence.

A sampled record becomes immutable. Stalling on batch `b` must not repeatedly overwrite the sample assigned to `b + D`, nor enqueue seconds of extra gameplay inputs. Physical input can still be sampled for local cancellation/UI, but that does not rewrite committed gameplay records.

The host confirms a batch only after receiving all required human inputs. It creates one immutable record containing the canonical inputs and any ordered control event. Every peer, including the host, consumes that record exactly once and in order.

Track separately what a peer has received and what it has executed. Cap execution skew; a conservative initial bound is one presentation batch, subject to measurement. Do not permit the host to run indefinitely ahead merely because it already has everyone's future inputs. The exact receive/execute acknowledgment state machine must have loss-injection tests.

### 6.4 Battle integration

Before beginning a presentation batch, verify that its complete required input set is available. During a ready batch, install the appropriate input latch before the existing source controller path consumes it. Preserve and test button edges across both source ticks.

When data is missing, do not call the gameplay update, advance the simulation clock, consume gameplay RNG, or run CPU decisions. Keep the previous presentation, service networking, maintain essential audio buffers, and display a separate waiting indicator when needed.

Do not repeatedly invoke the normal scene draw function during a stall until it has been proven free of gameplay-affecting side effects. A retained frame plus a small overlay is safer.

Do not block inside `osContGetReadData()`, an interrupt handler, or a source controller callback. The wait belongs at the battle scheduling boundary and must continue servicing communication. Stamp battle-controller publication with the accepted logical input index: background retraces or extra controller reads while waiting must not drain edges, publish a future input, or advance battle input history. Only output servicing that cannot mutate simulation-visible state may continue during a stall.

There is no unbounded catch-up simulation after a stall. Resume the ordinary fixed batch schedule. Missing input is never silently replaced with “hold the previous buttons” while other consoles continue: that would require prediction and correction, which this version does not have.

## 7. Determinism is a release prerequisite

### 7.1 Shared match description

Before setup, agree on a serialized match descriptor containing protocol version, executable/build compatibility identity, gameplay/content manifest identity, session and match generation, rules, stage, fighter slots, costumes, teams, CPU levels, handicaps, items, damage settings, seed, and input-delay profile.

Resolve random selections once, or resolve them through explicitly shared deterministic setup. Do not let menu dwell time, local save differences, RTC values, radio timings, or allocation history silently select gameplay state.

Set the agreed random state before the first setup operation that consumes gameplay randomness. Then compare a canonical post-setup state hash before permitting gameplay. Reset all scene-local and match-local state on rematch, not only the visible fighters.

Define a session unlock/availability policy explicitly. A proposed simple rule is that the host's unlocked choices govern the room, constrained by content supported by every build; guest save files are not modified. Whichever policy is approved becomes descriptor data, not a local override.

### 7.2 Audit boundaries

Audit random-number ownership, update order, initialization, controller edges, object creation/destruction, allocation failure, pending events, and any logic conditioned on visibility, camera, audio progress, or wall-clock time.

Separate presentation-only randomness from gameplay randomness where needed, but do not blindly split the original random stream and assume gameplay equivalence. Document and test changes to call order or ownership against source behavior.

Likewise, a dropped visual effect must not exhaust a shared pool differently and thereby prevent a gameplay object from spawning on only one console. Cosmetic degradation must have no gameplay consequences.

Identical CPUs and identical binaries help constrain the problem; they do not prove determinism. Floating point is not automatically disqualifying. Do not launch an all-fixed-point rewrite solely for networking. First eliminate timing dependencies, uninitialized data, undefined behavior, and inconsistent update paths.

### 7.3 Canonical hash and diagnostics

Create a typed gameplay-state traversal. Encode stable object identifiers and typed references, not guessed pointers based on numeric ranges. Exclude padding and machine addresses. Include every state category that can affect future gameplay: RNG state, fighters and action timers, collision-relevant transforms, percentages/stocks, CPU state, items/projectiles, stage hazards, match timers, controller history, pending events, and relevant object/process order.

Do not categorically exclude camera or animation data without checking whether gameplay consumes it. Exclude genuinely local presentation state only after establishing the boundary.

Use the existing Task 9 machinery as diagnostic reference, not unchanged network validation. In debug builds, capture detailed subsystem hashes and recent input records. In production, benchmark a bounded canonical checksum interval; an initial candidate is every 30 source ticks. A whole-state traversal is not free, and must fit the normal frame budget.

Hashes must refer to an explicitly named completed tick and be calculated from a coherent state boundary, not from a live state changing during an amortized scan. Never treat a truncated traversal or buffer overflow as a valid match.

On mismatch, stop the match, retain diagnostic evidence, and return an actionable error. A matching checksum is a detection aid, not a mathematical proof of complete state coverage. Do not “repair” divergence by copying fighter positions.

### 7.4 Replay-first proof

Feed the same descriptor, seed, and scripted input stream through the real DS simulation on two independent runs. Vary loading delays, menu histories, network arrival schedules, and permitted presentation behavior. Compare canonical states at the same source tick.

Start with the same ARM binary in emulator instances for debugging. Then repeat on independent consoles. Do not require a desktop-native floating-point build to match ARM bit-for-bit unless that separate portability property is intentionally implemented.

Cover every admitted fighter, stage, item family, CPU level behavior, teams, grabs/throws, projectiles, deaths, respawns, sudden death, and repeated matches. Determinism failures block multiplayer admission for the affected content; a narrow passing demo is not proof of complete-game support.

## 8. Packet and reliability design

### 8.1 Small explicit wire format

A canonical human pad sample needs four payload bytes: a 16-bit held-button field and two signed 8-bit stick axes, matching the useful fields at the existing controller boundary. [R3]

Serialize fields explicitly with defined byte order. Do not transmit `sizeof(OSContPad)`, pointer-bearing structures, C bitfields, or padding. Derive edges on consumption using the audited controller path.

Use fixed maximum sizes, proposed initially as 256 bytes for a host game message and 128 bytes for a client game message. Finalize these against the selected driver's limits and measured airtime. Larger loading/configuration exchanges use bounded control-message chunks outside active gameplay.

The header identifies message kind, protocol version, session, match generation, sequence, tick range, and acknowledgments. Validate lengths, counts, ownership, and tick windows before touching a ring buffer. The transport-assigned peer identity, not a packet's self-asserted slot, controls authorization.

### 8.2 Reliability

Send a small history of recent input samples and confirmed batches, initially four to eight records as packet capacity permits. Ignore identical duplicates. Reject conflicting values for an already accepted record. Retain unacknowledged original inputs and confirmations in bounded rings.

Recent redundancy is not the complete recovery algorithm. If an older hole falls outside the recent window, explicitly retransmit the oldest missing records. Never overwrite unacknowledged data; a full retention window triggers a bounded stall or clean abort, not silent loss.

Use reliable, ordered, idempotent control transactions for configuration, readiness, load completion, starting, pausing, ending, and rematching. Keep live-input recovery independent of a long lobby/control message.

Wireless acknowledgments, application receipt, and completed simulation are different facts. Record all three separately where available. The library's preloaded client replies can be stale; every input and acknowledgment therefore carries its own logical identity. A hardware reply alone cannot prove the application consumed a newly received host command. [E4]

Use a new match generation for every match and rematch. Old packets and old callbacks must not be accepted into a new match. A CRC can aid corruption diagnostics, but it is not authentication or anti-cheat security.

## 9. DS resource ownership

Use fixed-capacity application input histories, confirmation histories, packet queues, and telemetry rings. Perform match allocations before GO. Do not grow packet queues during active gameplay.

An illustrative upper-rate calculation is `4 players × 4 bytes × 60 ticks = 960 bytes/second` of unique raw controller samples, before headers, routing, acknowledgments, and redundancy. This is arithmetic for sizing, not measured radio traffic.

A 128-source-tick four-player raw input history is 2,048 bytes. Metadata, separate retention queues, packet staging, and checksums add to that. Set an initial **16 KiB application-protocol RAM target**, excluding diagnostic captures and the wireless stack, and revise it only with an explicit ledger.

Account separately for driver code/data, ARM7 and ARM9 stacks, shared buffers, packet pools, SDK migration layout changes, and cache effects. Small game packets do not imply a small total wireless footprint. Do not fund the stack from an unmeasured assumption about spare main RAM or optional expansion RAM. Networking itself should not introduce a new expansion-memory requirement.

Maintain a timer/DMA/interrupt/shared-memory ownership table for the selected SDK. Integrate the library's ARM7 requirements without replacing or breaking existing audio and storage services. Follow its cache-coherence and buffer-ownership rules rather than hand-converting aliases. Receive callbacks only validate/copy bounded data into queues; simulation and heavy parsing run at a safe main-thread boundary.

Pump communication during the active battle, loading barriers, waiting states, and results. Long synchronous loading must either yield safely or be explicitly supported by a loading heartbeat/progress policy. Do not create an interrupt that calls into non-reentrant filesystem or gameplay code.

Measure full-game performance with Wi-Fi off, initialized-idle, active two-player, and active four-player operation. Include hashing, interrupts, memory contention, audio, and retransmission pressure. Report compute cost separately from network waiting and visible missed presentation deadlines.

## 10. Lobby, loading, and match lifecycle

Use an explicit session state machine:

```text
OFFLINE -> DISCOVER/HOST -> LOBBY -> PREPARE -> LOAD -> READY_TO_START
        -> RUNNING -> RESULTS -> LOBBY
                    \-> ABORT -> LOBBY or OFFLINE
```

Lobby edits are host-validated proposals. The host publishes a revisioned snapshot. Changes that affect the match clear readiness. Display player slot, fighter, team, Ready state, and link status; do not lockstep cosmetic menu cursor animation.

Before loading, freeze membership and the descriptor. Each console loads its own assets and reports success, content identity, and initial state. If any console cannot admit the selected match, return to the lobby with a reason.

A start transaction is acknowledged by all peers and uses a shared logical epoch and input bootstrap. Reliable start retransmission and progress acknowledgments are required. Do not assume sending one GO packet starts every console at the same physical instant. Bound start/execution skew with the ordinary input protocol.

Pause must be a synchronized host-controlled action, not a local source pause. Schedule entry at a common confirmed boundary. Resume uses the reliable session-control channel, which continues while simulation ticks are stopped; it must not wait for an unreachable future gameplay tick. The first experimental two-player slice may temporarily disable network pause, but the completed product must make the chosen policy explicit.

End-of-match state and results are based on the confirmed simulation. Confirm the terminal tick/hash before publishing a shared result. Rematch resets controller edge history, RNG/bootstrap state, acknowledgments, rings, match generation, scene state, and readiness/load barriers.

Treat disconnects conservatively in the first release. Brief loss produces bounded waiting; sustained loss or lack of simulation progress ends the match without inventing a winner. Suggested initial UX thresholds are a waiting overlay after roughly 250 ms and abort after roughly 3 seconds without recoverable progress; these are tuning proposals, not radio guarantees. Distinguish slow loading from active-match loss.

Do not silently turn a missing human into a CPU or neutral controller. Host loss ends the session; client loss during an active match ends that match. A local Quit request is a session event. Closing the lid must lead to an explicitly handled pause/termination policy or safe timeout, not a promise that communication continues during sleep.

Keep personal saves local. Multiplayer errors must not create bogus wins or partial progression writes. Commit permitted records only after a confirmed legitimate result.

## 11. Suggested source organization

Keep the integration small and compile-time selectable:

| File or area | Responsibility |
| --- | --- |
| `src/port/controller_backend.c` | Shared live/replay/network input provider; stable four-slot publication. |
| `src/import/battleship_sys_controller.c` | Preserve and test source read/edge-publication semantics. |
| `src/nds/r2/nds_r2_battle.c` | Readiness gate and bounded network-stall path. |
| `src/port/taskman_seam_battle_host.c` | Tick/latch integration and explicit clock ownership. |
| `src/nds/nds_menu_shell_*` | Host/Join, revisioned lobby, loading and results UI. |
| `src/nds/net/nds_net_session.c` | Membership, descriptor and lifecycle state machine. |
| `src/nds/net/nds_net_protocol.c` | Serialization, validation and reliable control transactions. |
| `src/nds/net/nds_net_inputs.c` | Immutable input rings, confirmation log and execution acknowledgments. |
| `src/nds/net/nds_net_transport_*.c` | One shipping radio adapter plus a fake test adapter. |
| `src/nds/net/nds_net_state_hash.c` | Typed canonical gameplay hash and mismatch diagnostics. |

These new names are proposed, not existing functionality. Inspect the reference `decomp/BattleShip-main/decomp/src/sys/netinput.c` for useful input and metadata concepts, but keep decomp read-only and do not wholesale import its prediction and replay-history machinery. Its presence does not prove it is the shipping DS multiplayer path. [R10]

## 12. Milestones and acceptance evidence

| Milestone | Deliverable | Evidence required before advancing |
| --- | --- | --- |
| P3-0A — Transport | Two-console, then four-console managed local-wireless probe; pinned backend/SDK decision. | Discovery, stable exchanges, stale-reply handling, loss recovery, measured RAM and timing; subsequently full-game coexistence. |
| P3-0B — Determinism | Common descriptor, input provider, replay harness, typed hashes. | Independent runs consume the same records once and produce matching state despite different wall-clock schedules. |
| P3-1 — Minimal integration | Two humans, one admitted stage, no items; host uses the same delayed path. | Correct controls, no tap loss, no advance without inputs, no unbounded catch-up, consistent terminal state. |
| P3-2 — Reliability | Confirmation/recovery protocol, fixed delay, progress tracking and controlled abort. | Injected loss, duplication, reordering, delayed delivery, stale generations, conflicting inputs, queue pressure and forced divergence. |
| P3-3 — Complete battle support | Two to four humans; CPU fill; all admitted fighters/stages/items; teams and FFA. | Four-real-console sessions including worst-case content, every console taking a host turn, synchronized special cases and acceptable responsiveness. |
| P3-4 — Product lifecycle | Host/Join UX, readiness, loading, pause policy, results, rematches, exit and save policy. | Repeated host/join/leave/rematch cycles; load failure and lid/disconnect handling; no leaked resources or stale input. |

P3-0A and P3-0B can progress independently. Do not implement elaborate lobby UI before the transport and determinism risks are understood.

### Release validation

Retain the project's existing compute-performance contract; do not quietly tighten it or treat network stalls as zero-cost frames. Add separately reported input-delivery latency, stall time, execution skew, frame intervals, packet/queue high-water marks, and full memory low-water measurements.

Retain the project's established performance-harness authority for compute-budget acceptance; radio validation is a separate requirement, not a replacement performance authority. Use emulator instances for debugging and deterministic fault injection. Actual nearby-radio compatibility is a separate test property: emulator success alone cannot establish it. A recommended release test set includes four physical consoles, host rotation, representative audio/effects-heavy matches, a continuous-session soak, and repeated rematches. Suggested initial lab goals are a 30-minute representative session and 100 automated lifecycle cycles, with test duration distinguished from coverage.

Test DS/DS Lite as the baseline. List DSi-family launch configurations separately as admitted only after they pass. Do not use DSi-only resources for the ordinary-DS networking baseline or claim a four-human feature based on a two-console test.

## 13. Non-goals and future options

Do not build full-state serialization or rollback just to get the first local match working. Rollback requires complete save/load state and replaying additional simulation; those capabilities must be measured and validated before making them a product dependency. [E5]

After local play passes, a bounded rollback experiment could be considered if input delay remains unacceptable and a compact, correct snapshot plus resimulation budget can be demonstrated. This is an evidence-based later decision, not a statement that rollback is inherently impossible on DS.

A router-based UDP or Internet adapter can reuse the logical input protocol later, but neither is a substitute for proving routerless local play. A different transport also needs its own latency and failure-policy admission.

## 14. First implementation tickets

**Ticket A — Deterministic input/replay seam:** Add a production-safe four-slot input provider and named simulation/input indices. Preserve offline controls exactly. Test duplicate packet delivery without double button edges. Add canonical hashes and independently replay the same match under varied scheduling.

**Ticket B — Native wireless feasibility:** Run the supported local-multiplayer example on two then four DS consoles; measure transfer/acknowledgment behavior and resource use. Establish the concrete SDK integration path and test it with the existing renderer/audio before approving wider migration.

The plan is successful when every console is executing the same complete match with a small, measured input delay, within the existing DS resource budget—not merely when packets appear on a screen.

## Sources

Repository sources are pinned to the review commit. External library capabilities describe the reviewed upstream versions, not necessarily the user's locally installed SDK.

- [R1 — Original multiplayer draft](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/docs/P3_Multiplayer/Multiplayer.md)
- [R2 — Project goal](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/PROJECT_GOAL.md)
- [R3 — Controller backend](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/src/port/controller_backend.c)
- [R4 — Runtime 2 battle loop](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/src/nds/r2/nds_r2_battle.c)
- [R5 — Battle host seam](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/src/port/taskman_seam_battle_host.c)
- [R6 — Controller publication wrapper](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/src/import/battleship_sys_controller.c)
- [R7 — Main loop](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/src/nds/main.c)
- [R8 — Existing Task 9 hash](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/src/nds/nds_task9_state_hash.c)
- [R9 — Makefile](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/Makefile)
- [R10 — Reference network input implementation](https://github.com/rockenrooster/Smash64DS_Port/blob/806c007b88b3f753642925500d73314ed905e460/decomp/BattleShip-main/decomp/src/sys/netinput.c)
- [E1 — Calico wireless-manager header, reviewed revision](https://github.com/devkitPro/calico/blob/81b75e314d57ed1784545e28554e567f26f572f1/include/calico/nds/wlmgr.h)
- [E2 — BlocksDS local multiplayer guide](https://blocksds.skylyrac.net/dswifi/md_documentation_2local__multiplayer__mode.html)
- [E3 — BlocksDS migration guide](https://blocksds.skylyrac.net/docs/guides/devkitarm_porting_guide/)
- [E4 — BlocksDS multiplayer API](https://blocksds.skylyrac.net/dswifi/group__dswifi9__mp.html)
- [E5 — GGPO integration requirements](https://www.ggpo.net/)
