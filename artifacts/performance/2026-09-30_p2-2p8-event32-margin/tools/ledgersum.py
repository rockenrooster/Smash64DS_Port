"""event32 ledger extras per sweep arm: limit, high-water, failures, detaches."""
import json
import os
import sys

BASE = "D:/Stuff/DevFolder/Smash64DS_Port/artifacts/performance/2026-09-26_p2-2p8-ftr-item-tail/"
KEYS = ["gNdsAObjEvent32CapacityLimit", "gNdsAObjEvent32NormalizedHighWater",
        "gNdsAObjEvent32LiveHighWater", "gNdsAObjEvent32NormalizeFailCount",
        "gNdsAObjEvent32DetachCount", "gNdsAObjEvent32ForgetHoleRemovals",
        "gNdsAObjEvent32LedgerCompactions", "gNdsAObjEvent32RosterNeed",
        "gNdsAObjEvent32StageBoundLimit", "gNdsTaskmanGeneralHeapFreeMin",
        "gNdsRendererNativeFailure.count"]
SHORT = ["limit", "hw", "livehw", "fail", "detach", "removed", "compact", "need", "stage", "heapmin", "natfail"]
for arm in sys.argv[1].split(","):
    path = BASE + arm + ".json"
    if not os.path.exists(path):
        log = BASE + arm + "-run.log"
        fault = ""
        if os.path.exists(log):
            for line in open(log, encoding="utf-8", errors="replace"):
                if "TICKFAULT" in line:
                    fault = line.strip()[:120]
                    break
        print(f"{arm:10s} NO JSON {fault}")
        continue
    d = json.load(open(path))
    ex = {e["name"]: e["value"] for e in d["extras"]}
    vals = " ".join(f"{s}={ex.get(k)}" for k, s in zip(KEYS, SHORT) if k in ex)
    print(f"{arm:10s} {vals}")
