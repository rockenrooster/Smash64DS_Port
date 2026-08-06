#include "enhancements.h"
#include <libultraship/bridge/consolevariablebridge.h>

namespace ssb64 {
    namespace enhancements {

        const char* ClassicCoopCVarName() {
            return "gEnhancements.ClassicCoop";
        }

        const char* ClassicCoopFriendlyFireCVarName() {
            return "gEnhancements.ClassicCoopFriendlyFire";
        }

    } // namespace enhancements
} // namespace ssb64

// Latched at boot (port_classic_coop_latch) so mid-session toggles only take
// effect after a restart, matching the "(Needs reload)" menu label. -1 = not
// yet latched; falls back to a live read if queried before the latch runs.
static int sClassicCoopLatched = -1;

// Set while the VS CSS overlay is serving Classic mode (between the 1P-mode
// redirect and the CSS handing off to nSCKind1PGame / backing out). Keyed
// explicitly at the redirect site so BootToVSCSS — which also enters
// nSCKindPlayersVS — never trips it.
static int sClassicCoopContext = 0;

extern "C" void port_classic_coop_latch(void) {
    sClassicCoopLatched = CVarGetInteger(ssb64::enhancements::ClassicCoopCVarName(), 1) != 0;
}

extern "C" int port_enhancement_classic_coop(void) {
    if (sClassicCoopLatched < 0) {
        port_classic_coop_latch();
    }
    return sClassicCoopLatched;
}

extern "C" int port_classic_coop_context(void) {
    return sClassicCoopContext;
}

// Live read (no latch): consumed once per stage setup, so a mid-run toggle
// simply applies from the next stage.
extern "C" int port_classic_coop_friendly_fire(void) {
    return CVarGetInteger(ssb64::enhancements::ClassicCoopFriendlyFireCVarName(), 0) != 0;
}

extern "C" void port_classic_coop_set_context(int active) {
    sClassicCoopContext = active != 0;
}
