# Minimal Calico PXI request/reply components

`arm9.c` and `arm7_service.c` are **components**, not two complete replacement
programs. Add them to a matching libnds 2.x / Calico ARM9/ARM7 application. Keep
the application's existing ARM7 initialization, touch, power, RTC, sound, storage,
and other services. Start from the compatible official PXI template when a new
paired application is actually required. Source pins: `../../references/SOURCES.md`.

Reserve `PxiChannel_User0` on both CPUs; choose the same unused user channel in
both source files when User0 already has an owner. Do not use a system channel.

## Wiring

After the ARM7 runtime/services initialize, call
`nds_pxi_demo_server_start()` once from its main thread and check the result.
Let that main thread continue to use its normal blocking `pmMainLoop`/VBlank
path so the lower-priority server can run. Registration happens in the server
thread, after its mailbox and stack exist.

On ARM9, call `nds_pxi_demo_connect()` once at startup. From the single owning
requester thread, use `nds_pxi_demo_echo(value, &reply)` for a value in
0..`NDS_PXI_DEMO_VALUE_MAX`. It validates the echoed reply. During coordinated
shutdown, send `nds_pxi_demo_stop()` and check the acknowledgement. Send no more
requests after STOP. ARM7 can join the stopped server before reclaiming any
service-owned state; all storage here is static and the service is start-once.

## Contracts that make this small example safe

A simple PXI immediate carries 26 bits. This protocol uses a 24-bit echo payload,
one stop command, and one error reply. Unknown commands receive an error so a
valid requester is not left waiting. Neither side shares payload pointers or
needs cache flushes for these value messages.

At most **one request is outstanding**, and only simple-message APIs use the
channel. The reviewed `pxiSetMailbox` adapter calls `mailboxTrySend` and discards
its failure result. A full mailbox can therefore drop a message, not apply
backpressure. Four slots plus one-request discipline prevent that in this
example. A burst/extended-message protocol needs capacity/credits/framing; merely
increasing a queue is not a proof of safety.

`pxiSendAndReceive` waits for a reply and has no timeout parameter. `pxiWaitRemote`
also waits. `pxiSend` avoids waiting for a reply but its transport can still block
on a mutex/FIFO. Do not call these from an IRQ or with interrupts disabled. This
is a startup/control demonstration, not a fault-tolerant frame-critical RPC layer.
A missing/broken peer can leave it waiting indefinitely; that is an explicit
integration precondition, not hidden timeout handling.

The server worker uses no C library functions or thread-local variables, so it
does not attach TLS. Adding libc calls requires checking the CPU/runtime support
and worker storage. Do not copy ARM9 TLS assumptions blindly to ARM7.

## Verification

`protocol.h` is exercised by host tests. `tests/run_target_checks.py` compiles
both CPU components with a real installed SDK when available. Header/protocol
tests do not establish PXI delivery, timing, real scheduler behavior, or lifecycle
correctness on a device. Test the paired image, not one object in isolation.
