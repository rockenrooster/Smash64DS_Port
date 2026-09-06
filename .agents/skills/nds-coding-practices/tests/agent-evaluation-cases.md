# Skill evaluation cases

These are proposed model/harness checks, not claimed evaluation results. Run
old and revised packs with the same model, prompts, repository, tools, and token
budget. Score correct builds/behavior and unnecessary work, not confident prose.
Do not load this file during unrelated feature implementation.

| Prompt fixture | Required outcome | Failure to catch |
|---|---|---|
| Upload a palette range that may have zero entries. | Zero is a no-op before DMA/cache/MMIO; unit/count validation. | Starts zero-count DMA. |
| DMA a flushed local array; linker puts stack in DTCM. | Identifies inaccessible memory; CPU copy or accessible staging. | Only adds a wait or flush. |
| Add a profiler to Calico-based libnds. | Uses tick units correctly or reserves a free timer. | Reprograms runtime timers 2/3. |
| Decode a file on a Calico TickTask callback. | Bounded IRQ request; decode in main/worker context. | Filesystem/decode inside the callback. |
| Load packed GX commands whose first word is byte size. | Corrects to payload word count; checks actual wrapper and DMA ownership. | Leaves byte count or assumes glCallList is nonblocking. |
| Some geometry disappears only in large scenes. | Checks DISP3DCNT overflow and geometry counts. | Reads only GXSTAT or blames missing glEnd. |
| Submit single-screen 3D every other refresh. | Does not require capture solely for the lower rate; retains referenced textures. | Allocates an unnecessary capture bank or frees live textures. |
| Optimize a negative fixed-point multiply. | Preserves range and named rounding; checks generated code. | Narrows away required intermediate or changes negative rounding. |
| Add scrolling to a small static tiled background. | Uses existing BG ownership and scroll updates. | Rebuilds bitmap per frame or writes a new streaming framework. |
| Add a feature to frozen legacy libnds or BlocksDS. | Stays within the provided SDK; checks its headers. | Unrequested Calico migration or guessed signatures. |


| Send a 32-bit value through current PXI. | Checks the simple 26-bit limit; uses a bounded encoding or matched extended protocol. | Silent high-bit truncation or obsolete FIFO API. |
| Burst requests through pxiSetMailbox. | Accounts for adapter drops on full; uses credits/capacity or one request in flight. | Assumes the transport backpressures the application mailbox. |
| High-priority producer waits for a lower-priority worker with threadYield. | Replaces the dependency spin with a blocking wait and a bounded queue policy. | Retains starvation while claiming yielding fixes it. |
| Add libc work to a native Calico ARM9 thread. | Attaches/budgets TLS and preserves the stack through join. | Missing TLS or freed worker stack. |
| Fit a 3.8 MiB ordinary ARM9 image under the stock Calico DS linker. | Separates physical RAM from the actual linker/runtime budget. | Budgets all 4 MiB or edits only an assertion. |
| Convert normalized vectors to NORMAL_PACK. | Uses signed ten-bit components scaled by 512 and checks range. | Uses 1024 because the header says .10. |
| Compile six examples to validate all headers. | Adds nonconstant exported call sites for actual helper codegen. | Treats unused headers or empty optimized objects as evidence. |

For each fixture record: functional result, compiled target, affected resource
contracts, changed files, tool calls, tokens, time, and any measured runtime cost.
Correctness failures override speed/token improvements. Re-run multiple trials
before drawing conclusions about a new model.
