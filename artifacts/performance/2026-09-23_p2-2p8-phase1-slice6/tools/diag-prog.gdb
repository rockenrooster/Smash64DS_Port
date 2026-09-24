break ndsRendererNativeFighterSetRootProgram if $r0 == 0 && gNdsBattlePlayablePacingPresentedFrames >= 670 && gNdsBattlePlayablePacingPresentedFrames <= 750
commands
silent
printf "PROG f=%u owner=%u program=%u lr=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0, $r1, $lr
continue
end
break *0x2022e50 if gNdsBattlePlayablePacingPresentedFrames <= 750
commands
silent
printf "OTHER f=%u slot=%u root=%u op=%x idx=%u lean=%08x rec=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0, $r2, $r3, $r4, $r5, $r6
continue
end
