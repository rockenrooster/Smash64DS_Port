break ndsFtrLeanMaterialize if ($r1 & 0x100) != 0
commands
silent
printf "WIDE f=%u slot=%u entry=%x\n", gNdsBattlePlayablePacingPresentedFrames, $r0, $r1
continue
end
break ndsFtrLeanBuildJoints if gNdsBattlePlayablePacingPresentedFrames == 105
commands
silent
printf "BUILDJOINTS inst=%08x root=%08x bindings=%08x count=%u\n", $r0, $r1, $r2, $r3
x/12wx $r1
continue
end
break __excpt_entry
commands
printf "EXCEPTION f=%u\n", gNdsBattlePlayablePacingPresentedFrames
x/8wx 0x02295680
printf "framebuffer set %08x..%08x\n", &gSYFramebufferSets, ((char*)&gSYFramebufferSets) + sizeof(gSYFramebufferSets)
quit
end
