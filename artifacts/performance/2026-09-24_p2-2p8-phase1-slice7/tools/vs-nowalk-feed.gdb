set $phase=0
set $tick=0
set $css_visits=0
set $sss_visits=0
set $res_visits=0
break *ndsMenuShellRunTitle
commands
silent
set $phase=1
set $tick=0
continue
end
break *ndsMenuShellRunModeSelect
commands
silent
set $phase=2
set $tick=0
continue
end
break *ndsMenuShellRunVSMode
commands
silent
set $phase=3
set $tick=0
continue
end
break *mnPlayersVSStartScene
commands
silent
set $phase=4
set $tick=0
set $css_visits=$css_visits+1
printf "FEED css-enter visit=%u\n",$css_visits
continue
end
break *mnMapsStartScene
commands
silent
set $phase=5
set $tick=0
set $sss_visits=$sss_visits+1
printf "FEED sss-enter visit=%u\n",$sss_visits
continue
end
break *mnVSResultsStartScene
commands
silent
set $phase=6
set $tick=0
set $res_visits=$res_visits+1
printf "FEED results-enter visit=%u\n",$res_visits
continue
end
break *(keysHeld+4)
commands
silent
set $tick=$tick+1
set $raw_keys=0
if $phase==1 && ($tick==140 || $tick==141)
set $raw_keys=8
end
if $phase==2
if $tick==12
set $raw_keys=128
end
if $tick==24 || $tick==25
set $raw_keys=1
end
end
if $phase==3 && ($tick==24 || $tick==25)
set $raw_keys=1
end
if $phase==4 && $css_visits==1
if $tick==80
printf "FEED css-start fkind=%u,%u selected=%u,%u pkind=%08x\n",sCssFkind[0],sCssFkind[1],sCssSelected[0],sCssSelected[1],*(unsigned int*)sCssPkind
end
if $tick==80 || $tick==81
set $raw_keys=8
end
end
if $phase==5 && $sss_visits==1 && ($tick==60 || $tick==61)
set $raw_keys=1
end
if $phase==6 && $tick>=440 && (($tick & 15) >= 8) && (($tick & 15) < 12)
set $raw_keys=8
end
set variable $r0=$raw_keys
continue
end
