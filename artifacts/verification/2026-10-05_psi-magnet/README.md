# 2026-10-05 Ness's PSI Magnet draws over Ness

Owner (BUGS.md, Ness): "Down B needs shield guard z move treatment (so it draws
over player)".

The magnet is a model effect (`efManagerNessPsychicMagnetMakeEffect`) whose root
follows Ness's TopN joint through the 0x50 joint-attach matrix. On the DS the
opaque body won the depth test and hid most of the bubble. The maker now
publishes the first display list of its tree (`gNdsEffectTowardEyeDL`,
`src/import/battleship_efmanager.c`), and `ndsRendererAdapterBuildJointAttachTraMtx`
(`src/port/renderer_adapter_matrix.c`) moves a root whose tree leads with that
list 150 units toward the camera eye -- the shield's floor bias. Translation of
the drawn matrix only; the source position, pose and lifetime are untouched.

Capture: clean four-CPU lab ROM with the lab input knob (`build-lab-clean1005x`),
4 x Ness on Dream Land, player 1 forced to hold Down + B for 60 of every 120
ticks (`gNdsLabForceInputSlots=1`, `gNdsLabForceInput=2952806400`,
`gNdsLabForceInputPeriod=3932280`); gdb stops at the effect's maker after
presented frame 500 and captures the next three frames (`scratchpad magcap.ps1`,
tag `magnet4`; `gNdsEffectTowardEyeDL` non-NULL on each). Ness stands on the top
platform inside the bubble; only his shoes show below it. Captures are local
(`artifacts/visibility/2026-10-05_playtest/magnet4-c*.png`), not committed.

Official gate with this change: replay digest identical
(`../../performance/2026-10-05_playtest-gate/README.md`).
