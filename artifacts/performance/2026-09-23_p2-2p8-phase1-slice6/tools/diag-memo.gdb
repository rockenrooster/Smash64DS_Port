break *0x2028660 if $r4 == 0 && gNdsBattlePlayablePacingPresentedFrames >= 675 && gNdsBattlePlayablePacingPresentedFrames <= 677
commands
silent
printf "HIT f=%u run=%u slot=%u entry=%08x name=%u\n", gNdsBattlePlayablePacingPresentedFrames, $r0, $r4, $r5, $r1
continue
end
break ndsRendererR2RunTextureMemoFill if gNdsBattlePlayablePacingPresentedFrames >= 675 && gNdsBattlePlayablePacingPresentedFrames <= 677
commands
silent
printf "FILL f=%u run=%u r1=%08x r2=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0, $r1, $r2
continue
end
break ndsRendererNativeFighterSetRootProgram if $r0 == 0 && gNdsBattlePlayablePacingPresentedFrames >= 675 && gNdsBattlePlayablePacingPresentedFrames <= 677
commands
silent
printf "PROG f=%u program=%u lr=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r1, $lr
continue
end
