#pragma once

#include <map>
#include <memory>
#include <string>
#include <cstdint>
#include <queue>
#include <vector>
#include <map>
#include "libultraship/libultra/controller.h"
#include "ship/utils/color.h"
#include <unordered_map>
#include "ship/controller/controldevice/controller/Controller.h"

namespace Ship {
class RaphnetTransport;  // ship/controller/raphnet/RaphnetTransport.h
}

namespace LUS {
class Controller : public Ship::Controller {
  public:
    Controller(uint8_t portIndex, std::vector<CONTROLLERBUTTONS_T> bitmasks);

    void ReadToPad(void* pad) override;

    // Bind this controller to a raphnet adapter channel. After this call,
    // ReadToOSContPad polls the transport for raw N64 SI status and writes
    // directly into pad — bypasses the SDL/keyboard/mouse mapping pipeline
    // AND bypasses mPadBuffer entirely (combining real adapter latency with
    // CVAR_SIMULATED_INPUT_LAG would be meaningless on this port).
    //
    // Pass an expired weak_ptr to clear the binding (revert to the mapping
    // pipeline), but in practice we only set it once at ControlDeck::Init.
    void SetRaphnetBinding(std::weak_ptr<Ship::RaphnetTransport> transport, uint8_t channel) override;

  private:
    void ReadToOSContPad(OSContPad* pad);

    std::deque<OSContPad> mPadBuffer;

    // Native raphnet binding state — see SetRaphnetBinding.
    std::weak_ptr<Ship::RaphnetTransport> mRaphnetTransport;
    uint8_t mRaphnetChannel = 0;
    bool mRaphnetBindingActive = false;
    // Auto-fallback: if Poll fails for this many consecutive frames (~1s
    // at 60 fps), drop the binding and let the SDL/keyboard mapping pipe-
    // line handle the port. Without this the user is stuck with a dead
    // controller until they edit gControllers.Raphnet.Enabled=0 and relaunch.
    uint32_t mRaphnetConsecutivePollFailures = 0;
};
} // namespace LUS
