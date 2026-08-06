#include "libultraship/bridge/gfxbridge.h"

#include "ship/Context.h"
#include "fast/Fast3dWindow.h"
#include "fast/interpreter.h"

// Set the dimensions for the VI mode that the console would be using
// (Usually 320x240 for lo-res and 640x480 for hi-res)
extern "C" void GfxSetNativeDimensions(uint32_t width, uint32_t height) {
    Fast::Interpreter* gfx = static_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow())
                                 ->GetInterpreterWeak()
                                 .lock()
                                 .get();
    gfx->SetNativeDimensions(width, height);
}

extern "C" void GfxGetPixelDepthPrepare(float x, float y) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    if (wnd == nullptr) {
        return;
    }
    return wnd->GetPixelDepthPrepare(x, y);
}

extern "C" uint16_t GfxGetPixelDepth(float x, float y) {
    auto wnd = std::dynamic_pointer_cast<Fast::Fast3dWindow>(Ship::Context::GetInstance()->GetWindow());
    if (wnd == nullptr) {
        return 0;
    }
    return wnd->GetPixelDepth(x, y);
}

extern "C" void GfxSetWidescreenActive(int active) {
    auto window = Ship::Context::GetInstance()->GetWindow();
    if (window == nullptr) {
        return;
    }
    auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(window);
    if (fast3d == nullptr) {
        return;
    }
    auto interp = fast3d->GetInterpreterWeak().lock();
    if (interp == nullptr) {
        return;
    }
    interp->SetWidescreenActive(active != 0);
}

extern "C" float GfxGetWidescreenClipXScale(void) {
    auto window = Ship::Context::GetInstance()->GetWindow();
    if (window == nullptr) {
        return 1.0f;
    }
    auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(window);
    if (fast3d == nullptr) {
        return 1.0f;
    }
    auto interp = fast3d->GetInterpreterWeak().lock();
    if (interp == nullptr) {
        return 1.0f;
    }
    return interp->GetWidescreenClipXScale();
}

extern "C" void GfxSetTight4_3ScissorWindow(int active) {
    auto window = Ship::Context::GetInstance()->GetWindow();
    if (window == nullptr) {
        return;
    }
    auto fast3d = std::dynamic_pointer_cast<Fast::Fast3dWindow>(window);
    if (fast3d == nullptr) {
        return;
    }
    auto interp = fast3d->GetInterpreterWeak().lock();
    if (interp == nullptr) {
        return;
    }
    interp->SetTight4_3ScissorWindow(active != 0);
}

