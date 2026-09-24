break *0x2022e50
commands
silent
printf "OTHER f=%u slot=%u root=%u op=%x idx=%u lean=%08x rec=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0, $r2, $r3, $r4, $r5, $r6
continue
end
break *0x2022928 if $r1 == 5
commands
silent
printf "OTHERB f=%u slot=%u root=%u op=%x idx=%u lean=%08x rec=%08x\n", gNdsBattlePlayablePacingPresentedFrames, $r0, $r2, $r3, $r4, $r5, $r6
continue
end
