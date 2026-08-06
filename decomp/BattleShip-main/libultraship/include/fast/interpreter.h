#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <unordered_map>
#include <map>
#include <list>
#include <cstddef>
#include <vector>
#include <stack>
#include <string>
#include <string_view>

#include "fast/lus_gbi.h"
#include "fast/types.h"
#include "fast/ucodehandlers.h"
#include "backends/gfx_rendering_api.h"
#include "postprocess/PostProcessChain.h"

#include "fast/resource/type/Texture.h"
#include "ship/resource/Resource.h"

// TODO figure out why changing these to 640x480 makes the game only render in a quarter of the window
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <compare>
#endif

/*enum {
    CC_0,
    CC_TEXEL0,
    CC_TEXEL1,
    CC_PRIM,
    CC_SHADE,
    CC_ENV,
    CC_TEXEL0A,
    CC_LOD
};*/

enum {
    SHADER_0,
    SHADER_INPUT_1,
    SHADER_INPUT_2,
    SHADER_INPUT_3,
    SHADER_INPUT_4,
    SHADER_INPUT_5,
    SHADER_INPUT_6,
    SHADER_INPUT_7,
    SHADER_TEXEL0,
    SHADER_TEXEL0A,
    SHADER_TEXEL1,
    SHADER_TEXEL1A,
    SHADER_1,
    SHADER_COMBINED,
    SHADER_NOISE
};

#ifdef __cplusplus
enum class ShaderOpts {
    ALPHA,
    FOG,
    TEXTURE_EDGE,
    NOISE,
    _2CYC,
    ALPHA_THRESHOLD,
    INVISIBLE,
    GRAYSCALE,
    TEXEL0_CLAMP_S,
    TEXEL0_CLAMP_T,
    TEXEL1_CLAMP_S,
    TEXEL1_CLAMP_T,
    TEXEL0_MASK,
    TEXEL1_MASK,
    TEXEL0_BLEND,
    TEXEL1_BLEND,
    PRISM_SHADER, // 16-bit width
    MAX
};

#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))
#endif

struct ColorCombinerKey {
    uint64_t combine_mode;
    uint64_t options;
    uint64_t shader_id;

#ifdef __cplusplus
    auto operator<=>(const ColorCombinerKey&) const = default;
#endif
};

#define SHADER_MAX_TEXTURES 6
#define SHADER_FIRST_TEXTURE 0
#define SHADER_FIRST_MASK_TEXTURE 2
#define SHADER_FIRST_REPLACEMENT_TEXTURE 4

struct CCFeatures {
    int c[2][2][4];
    bool opt_alpha;
    bool opt_fog;
    bool opt_texture_edge;
    bool opt_noise;
    bool opt_2cyc;
    bool opt_alpha_threshold;
    bool opt_invisible;
    bool opt_grayscale;
    bool usedTextures[2];
    bool used_masks[2];
    bool used_blend[2];
    bool clamp[2][2];
    int numInputs;
    bool do_single[2][2];
    bool do_multiply[2][2];
    bool do_mix[2][2];
    bool color_alpha_same[2];
    int16_t shader_id;
};

void gfx_cc_get_features(uint64_t shader_id0, uint64_t shader_id1, struct CCFeatures* cc_features);

union Gfx;

namespace Fast {

class GfxRenderingAPI;
class GfxWindowBackend;

constexpr size_t MAX_SEGMENT_POINTERS = 16;
constexpr size_t SHADER_ID_SHIFT = 16;
constexpr int16_t ShaderIdUnmask(int id) {
    return (id >> SHADER_ID_SHIFT) & 0xFFFF;
}

struct GfxExecStack {
    // This is a dlist stack used to handle dlist calls.
    std::stack<F3DGfx*> cmd_stack = {};
    // This is also a dlist stack but a std::vector is used to make it possible
    // to iterate on the elements.
    // The purpose of this is to identify an instruction at a poin in time
    // which would not be possible with just a F3DGfx* because a dlist can be called multiple times
    // what we do instead is store the call path that leads to the instruction (including branches)
    std::vector<const F3DGfx*> gfx_path = {};
    struct CodeDisp {
        const char* file;
        int line;
    };
    // stack for OpenDisp/CloseDisps
    std::vector<CodeDisp> disp_stack{};

    void start(F3DGfx* dlist);
    void stop();
    F3DGfx*& currCmd();
    void openDisp(const char* file, int line);
    void closeDisp();
    const std::vector<CodeDisp>& getDisp() const;
    void branch(F3DGfx* caller);
    void call(F3DGfx* caller, F3DGfx* callee);
    F3DGfx* ret();
};

struct XYWidthHeight {
    int16_t x, y;
    uint32_t width, height;
};

struct GfxDimensions {
    float internal_mul;
    uint32_t width, height;
    float aspect_ratio;
};

struct TextureCacheKey {
    const uint8_t* texture_addr;
    const uint8_t* palette_addrs[2];
    uint8_t fmt, siz;
    uint8_t palette_index;
    uint32_t size_bytes;
    uint8_t masks, maskt;
    uint16_t tile_width, tile_height;

    bool operator==(const TextureCacheKey&) const noexcept = default;

    struct Hasher {
        size_t operator()(const TextureCacheKey& key) const noexcept {
            // FNV-1a over every field operator== compares. Hashing only the
            // address degrades the map to a linked list when the game's bump
            // heaps cycle many textures through the same addresses.
            uint64_t h = 1469598103934665603ULL;
            auto mix = [&h](uint64_t v) {
                h ^= v;
                h *= 1099511628211ULL;
            };
            mix((uint64_t)(uintptr_t)key.texture_addr);
            mix((uint64_t)(uintptr_t)key.palette_addrs[0]);
            mix((uint64_t)(uintptr_t)key.palette_addrs[1]);
            mix((uint64_t)key.fmt | ((uint64_t)key.siz << 8) | ((uint64_t)key.palette_index << 16) |
                ((uint64_t)key.masks << 24) | ((uint64_t)key.maskt << 32));
            mix((uint64_t)key.size_bytes | ((uint64_t)key.tile_width << 32) | ((uint64_t)key.tile_height << 48));
            return (size_t)h;
        }
    };
};

typedef std::unordered_map<TextureCacheKey, struct TextureCacheValue, TextureCacheKey::Hasher> TextureCacheMap;
typedef std::pair<const TextureCacheKey, struct TextureCacheValue> TextureCacheNode;

struct TextureCacheValue {
    uint32_t texture_id;
    uint8_t cms, cmt;
    bool linear_filter;

    std::list<struct TextureCacheMapIter>::iterator lru_location;
};

struct TextureCacheMapIter {
    TextureCacheMap::iterator it;
};

struct RGBA {
    uint8_t r, g, b, a;
};

struct LoadedVertex {
    float x, y, z, w;
    float u, v;
    struct RGBA color;
    uint8_t clip_rej;
};

struct RawTexMetadata {
    uint16_t width, height;
    float h_byte_scale = 1, v_pixel_scale = 1;
    std::shared_ptr<Fast::Texture> resource;
    Fast::TextureType type;
};

#define MAX_LIGHTS 32
#define MAX_VERTICES 64

struct RSP {
    float modelview_matrix_stack[11][4][4];
    uint8_t modelview_matrix_stack_size;

    float MP_matrix[4][4];
    float P_matrix[4][4];

    F3DLight_t lookat[2];
    F3DLight current_lights[MAX_LIGHTS + 1];
    float current_lights_coeffs[MAX_LIGHTS][3];
    float current_lookat_coeffs[2][3]; // lookat_x, lookat_y
    uint8_t current_num_lights;        // includes ambient light
    bool lights_changed;

    uint32_t geometry_mode;
    int16_t fog_mul, fog_offset;

    uint32_t extra_geometry_mode;

    struct {
        // U0.16
        uint16_t s, t;
    } texture_scaling_factor;

    struct LoadedVertex loaded_vertices[MAX_VERTICES + 4];
};

struct RDP {
    const uint8_t* palettes[2];
    // Original DRAM source address of the most recent TLUT load per palette half.
    // Used in texture cache keys instead of palettes[] (which always points to staging).
    const uint8_t* palette_dram_addr[2];
    // CI4 palette staging buffer: N64 TMEM holds up to 16 CI4 palettes (16 entries x 2 bytes each = 32 bytes per
    // palette). palettes[0] covers indices 0-7 (256 bytes), palettes[1] covers 8-15 (256 bytes). GfxDpLoadTlut copies
    // TLUT data here at the correct offset so multi-palette CI4 models work.
    uint8_t palette_staging[2][256];
    struct {
        const uint8_t* addr;
        uint8_t siz;
        uint32_t width;
        uint32_t tex_flags;
        struct RawTexMetadata raw_tex_metadata;
    } texture_to_load;
    struct {
        const uint8_t* addr;
        uint32_t orig_size_bytes;
        uint32_t size_bytes;
        uint32_t full_image_line_size_bytes;
        uint32_t line_size_bytes;
        uint32_t tex_flags;
        struct RawTexMetadata raw_tex_metadata;
        bool masked;
        bool blended;
    } loaded_texture[2];
    struct {
        uint8_t fmt;
        uint8_t siz;
        uint8_t cms, cmt;
        uint8_t shifts, shiftt;
        uint8_t masks, maskt; // PORT: wrap masks (N64 mask_s, mask_t) — 0 means no mask
        float uls, ult, lrs, lrt;
        uint16_t tmem; // 0-511, in 64-bit word units
        uint32_t line_size_bytes;
        uint8_t palette;
        uint8_t tmem_index; // 0 or 1 for offset 0 kB or offset 2 kB, respectively
    } texture_tile[8];
    bool textures_changed[2];

    uint8_t first_tile_index;

    uint32_t other_mode_l, other_mode_h;
    uint64_t combine_mode;
    bool grayscale;

    uint8_t prim_lod_fraction;
    // PORT: G_SETPRIMDEPTH stores a constant Z (and dz) used when other_mode_l has
    // G_ZS_PRIM set. Upstream Fast3D never wired this up — game sprites rendered
    // with gDPSetDepthSource(G_ZS_PRIM) ended up at vertex Z (typically near plane),
    // so 2D backgrounds stomped 3D foreground geometry across the port (SSB64
    // wallpaper occluding the explosion transition, fighter description-scene 2D
    // logo drawing in front of the model, etc.).
    uint16_t prim_depth_z;
    uint16_t prim_depth_dz;
    struct RGBA env_color, prim_color, fog_color, blend_color, fill_color, grayscale_color;
    struct XYWidthHeight viewport, scissor;
    bool viewport_or_scissor_changed;
    void* z_buf_address;
    void* color_image_address;
};

typedef enum Attribute {
    MTX_PROJECTION,
    MTX_LOAD,
    MTX_PUSH,
    MTX_NOPUSH,
    CULL_FRONT,
    CULL_BACK,
    CULL_BOTH,
    MV_VIEWPORT,
    MV_LIGHT,
} Attribute;

extern GfxExecStack g_exec_stack;

struct GfxTextureCache {
    TextureCacheMap map;
    std::list<TextureCacheMapIter> lru;
    std::vector<uint32_t> free_texture_ids;
};

struct ColorCombiner {
    uint64_t shader_id0;
    uint64_t shader_id1;
    bool usedTextures[2];
    struct ShaderProgram* prg[16];
    uint8_t shader_input_mapping[2][7];
};

struct RenderingState {
    uint8_t depth_test_and_mask; // 1: depth test, 2: depth mask
    bool decal_mode;
    bool alpha_blend;
    struct XYWidthHeight viewport, scissor;
    struct ShaderProgram* mShaderProgram;
    TextureCacheNode* mTextures[SHADER_MAX_TEXTURES];
};

struct FBInfo {
    uint32_t orig_width, orig_height;       // Original shape
    uint32_t applied_width, applied_height; // Up-scaled for the viewport
    uint32_t native_width, native_height;   // Max "native" size of the screen, used for up-scaling
    bool resize;                            // Scale to match the viewport
};

struct MaskedTextureEntry {
    uint8_t* mask;
    uint8_t* replacementData;
};

class Interpreter {
  public:
    Interpreter();
    ~Interpreter();

    void Init(GfxWindowBackend* wapi, class GfxRenderingAPI* rapi, const char* game_name, bool start_in_fullscreen,
              uint32_t width, uint32_t height, uint32_t posX, uint32_t posY);
    void Destroy();
    void GetDimensions(uint32_t* width, uint32_t* height, int32_t* posX, int32_t* posY);
    GfxRenderingAPI* GetCurrentRenderingAPI();
    void StartFrame();
    void RunGuiOnly();
    void Run(Gfx* commands, const std::unordered_map<Mtx*, MtxF>& mtx_replacements);
    void PresentCurrentFramebuffer();
    void EndFrame();
    void HandleWindowEvents();
    bool IsFrameReady();
    bool ViewportMatchesRendererResolution();
    void SetForceRenderToFb(bool force);
    int GetTargetFps();
    void SetTargetFps(int fps);
    void SetMaxFrameLatency(int latency);
    int CreateFrameBuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                          uint8_t resize);
    void SetFrameBuffer(int fb, float noiseScale);
    void CopyFrameBuffer(int fb_dst_id, int fb_src_id, bool copyOnce, bool* hasCopiedPtr);
    void ResetFrameBuffer();
    void AdjustPixelDepthCoordinates(float& x, float& y);
    void GetPixelDepthPrepare(float x, float y);
    uint16_t GetPixelDepth(float x, float y);
    void RegisterBlendedTexture(const char* name, uint8_t* mask, uint8_t* replacement);
    void UnregisterBlendedTexture(const char* name);

    // Register a CPU address range as a mirror of a GPU framebuffer's sub-rect.
    // When ImportTexture sees a gsDPSetTextureImage(cpuAddr) where cpuAddr falls
    // inside [base, base+sizeBytes), it binds the registered FB via
    // SelectTextureFb AND remaps the consumer's local UV (0..1) to the FB
    // sub-rect [u0,u1] x [v0,v1] -- so a multi-tile sprite that samples its
    // own 300x6 row-stripe at UV (0..1) ends up sampling a 300x6 slice of the
    // bigger captured FB instead of the whole thing.
    //
    // u0/v0/u1/v1 are normalized [0,1] coordinates of the source FB.
    // For a 1:1 substitution (single quad covering the whole FB) pass
    // (0, 0, 1, 1).
    //
    // Used by the SSB64 framebuffer-capture trick (1P stage clear wallpaper
    // = ~37 row-stripes, each registered with its slice of the photo region;
    // lbtransition photo wipe -> VS results screen = single-tile photo heap).
    // The UV-remap plumbing in GfxSpTri1 makes this work for any N64 game
    // that samples a captured framebuffer via N>=1 tile loads (i.e. all of
    // them, since N64 TMEM is 4 KB and any FB-sized region must be tiled).
    void RegisterFbTexture(const void* base, size_t sizeBytes, int fbId,
                           float u0, float v0, float u1, float v1);
    void UnregisterFbTexture(const void* base);
    void ClearFbTextures();

    void SetNativeDimensions(float width, float height);
    void SetResolutionMultiplier(float multiplier);
    void SetMsaaLevel(uint32_t level);
    void GetCurDimensions(uint32_t* width, uint32_t* height);

    // Port hook: when set true, AdjXForAspectRatio compresses post-projection
    // clip-space X by (4/3) / window_aspect, expanding the visible 4:3
    // frustum into the wider window. Refreshed per-frame by the game's port
    // glue from a CVar; default false means non-widescreen-aware ports get
    // exactly the prior unconditional behaviour as a no-op fallback.
    void SetWidescreenActive(bool active) { mWidescreenActive = active; }

    // Returns the same scale factor AdjXForAspectRatio applies to clip-space X.
    // Port glue uses this to compress game-side world-to-screen projection
    // results so HUD sprites that attach to 3D characters track them after
    // the camera frustum widens. Returns 1.0f when widescreen is off or when
    // the window is not wider than 4:3.
    float GetWidescreenClipXScale() const;

    // Port hook: when set true AND widescreen is active, the GPU scissor is
    // narrowed to the original 4:3 sub-region of the wider FB. Game code
    // flips this on for scene-specific effects whose mesh geometry would
    // otherwise expose perspective-foreshortened slants in widescreen.
    void SetTight4_3ScissorWindow(bool active) { mTight4_3ScissorWindow = active; }

    // private: TODO make these private
    void Flush();
    // End-of-frame composition step: resolves MSAA (when applicable),
    // runs the post-process chain (when active), and sets mGfxFrameBuffer
    // to the texture handle the GUI samples. Called from Run() and
    // RunGuiOnly() once GBI execution is complete.
    void ComposeFinalFrame();
    // Reconcile the post-process chain with CVAR_POSTPROCESS_ENABLED /
    // _SHADER. Compiles / unloads the active shader only on transitions —
    // a no-op when both CVars are unchanged from the prior frame.
    void UpdatePostProcessFromCVars();
    ShaderProgram* LookupOrCreateShaderProgram(uint64_t id0, uint64_t id1);
    ColorCombiner* LookupOrCreateColorCombiner(const ColorCombinerKey& key);
    void TextureCacheClear();
    bool TextureCacheLookup(int i, const TextureCacheKey& key);
    void TextureCacheDelete(const uint8_t* origAddr);
    void TextureCacheDeleteRange(const uint8_t* base, size_t size);
    void ResetRdpTextureState();
    void ImportTextureRgba16(int tile, bool importReplacement);
    void ImportTextureRgba32(int tile, bool importReplacement);
    void ImportTextureIA4(int tile, bool importReplacement);
    void ImportTextureIA8(int tile, bool importReplacement);
    void ImportTextureIA16(int tile, bool importReplacement);
    void ImportTextureI4(int tile, bool importReplacement);
    void ImportTextureI8(int tile, bool importReplacement);
    void ImportTextureCi4(int tile, bool importReplacement);
    void ImportTextureCi8(int tile, bool importReplacement);
    void ImportTextureRaw(int tile, bool importReplacement);
    void ImportTextureImg(int tile, bool importReplacement);
    void ImportTexture(int i, int tile, bool importReplacement);
    void ImportTextureMask(int i, int tile);
    void CalculateNormalDir(const F3DLight_t*, float coeffs[3]);

    void GfxSpMatrix(uint8_t params, const int32_t* addr);
    void GfxSpPopMatrix(uint32_t count);
    void GfxSpVertex(size_t numVertices, size_t destIndex, const F3DVtx* vertices);
    void GfxSpModifyVertex(uint16_t vtxIdx, uint8_t where, uint32_t val);
    void GfxSpTri1(uint8_t vtx1Idx, uint8_t vtx2Idx, uint8_t vtx3Idx, bool isRect);
    void GfxSpGeometryMode(uint32_t clear, uint32_t set);
    void GfxSpExtraGeometryMode(uint32_t clear, uint32_t set);
    void GfxSpMovememF3dex2(uint8_t index, uint8_t offset, const void* data);
    void GfxSpMovememF3d(uint8_t index, uint8_t offset, const void* data);
    void GfxSpMovewordF3dex2(uint8_t index, uint16_t offset, uintptr_t data);
    void GfxSpMovewordF3d(uint8_t index, uint16_t offset, uintptr_t data);
    void GfxSpTexture(uint16_t sc, uint16_t tc, uint8_t level, uint8_t tile, uint8_t on);
    void GfxDpSetScissor(uint32_t mode, uint32_t ulx, uint32_t uly, uint32_t lrx, uint32_t lry);
    void GfxDpSetTextureImage(uint32_t format, uint32_t size, uint32_t width, const char* texPath, uint32_t texFlags,
                              RawTexMetadata rawTexMetdata, const void* addr);
    void GfxDpSetTile(uint8_t fmt, uint32_t siz, uint32_t line, uint32_t tmem, uint8_t tile, uint32_t palette,
                      uint32_t cmt, uint32_t maskt, uint32_t shiftt, uint32_t cms, uint32_t masks, uint32_t shifts);
    void GfxDpSetTileSize(uint8_t tile, uint16_t uls, uint16_t ult, uint16_t lrs, uint16_t lrt);
    void GfxDpLoadTlut(uint8_t tile, uint32_t high_index);
    void GfxDpLoadBlock(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t dxt);
    void GfxDpLoadTile(uint8_t tile, uint32_t uls, uint32_t ult, uint32_t lrs, uint32_t lrt);
    void GfxDpSetCombineMode(uint32_t rgb, uint32_t alpha, uint32_t rgb_cyc2, uint32_t alpha_cyc2);
    void GfxDpSetGrayscaleColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetEnvColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetPrimColor(uint8_t m, uint8_t r, uint8_t l, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetFogColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetBlendColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    void GfxDpSetFillColor(uint32_t pickedColor);
    void GfxDrawRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
    void GfxDpTextureRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry, uint8_t tile, int16_t uls,
                               int16_t ult, int16_t dsdx, int16_t dtdy, bool flip);
    void GfxDpImageRectangle(int32_t tile, int32_t w, int32_t h, int32_t ulx, int32_t uly, int16_t uls, int16_t ult,
                             int32_t lrx, int32_t lry, int16_t lrs, int16_t lrt);
    void GfxDpFillRectangle(int32_t ulx, int32_t uly, int32_t lrx, int32_t lry);
    void GfxDpSetZImage(void* zBufAddr);
    void GfxDpSetColorImage(uint32_t format, uint32_t size, uint32_t width, void* address);
    void GfxSpSetOtherMode(uint32_t shift, uint32_t num_bits, uint64_t mode);
    void GfxDpSetOtherMode(uint32_t h, uint32_t l);

    void Gfxs2dexBgCopy(F3DuObjBg* bg);
    void Gfxs2dexBg1cyc(F3DuObjBg* bg);
    void Gfxs2dexRecyCopy(F3DuObjSprite* spr);

    void AdjustWidthHeightForScale(uint32_t& width, uint32_t& height, uint32_t nativeWidth,
                                   uint32_t nativeHeight) const;
    float AdjXForAspectRatio(float x) const;
    void AdjustVIewportOrScissor(XYWidthHeight* area);
    void CalcAndSetViewport(const F3DVp_t* viewport);

    void SpReset();
    void* SegAddr(uintptr_t w1);

    static const char* CCMUXtoStr(uint32_t ccmux);
    static const char* ACMUXtoStr(uint32_t acmux);
    static void GenerateCC(ColorCombiner* comb, const ColorCombinerKey& key);
    static std::string_view GetBaseTexturePath(std::string_view path);
    static void NormalizeVector(float v[3]);
    static void TransposedMatrixMul(float res[3], const float a[3], const float b[4][4]);
    static void MatrixMul(float res[4][4], const float a[4][4], const float b[4][4]);

    RSP* mRsp;
    RDP* mRdp;
    RenderingState mRenderingState{};

    GfxTextureCache mTextureCache{};
    std::map<ColorCombinerKey, ColorCombiner> mColorCombinerPool; // color_combiner_pool;
    std::map<ColorCombinerKey, ColorCombiner>::iterator mPrevCombiner = mColorCombinerPool.end();
    uint8_t* mTexUploadBuffer = nullptr;

    GfxDimensions mGfxCurrentWindowDimensions{}; // gfx_current_window_dimensions;
    int32_t mCurWindowPosX{};
    int32_t mCurWindowPosY{};
    GfxDimensions mCurDimensions{};        // gfx_current_dimensions;
    GfxDimensions mPrvDimensions{};        // gfx_prev_dimensions;
    XYWidthHeight mGameWindowViewport{};   // gfx_current_game_window_viewport;
    XYWidthHeight mNativeDimensions{};     // gfx_native_dimensions;
    XYWidthHeight mPrevNativeDimensions{}; // gfx_prev_native_dimensions;
    uintptr_t mGfxFrameBuffer{};

    unsigned int mMsaaLevel = 1;
    bool mDroppedFrame{};
    float* mBufVbo; // 3 vertices in a triangle and 32 floats per vtx
    size_t mBufVboLen{};
    size_t mBufVboNumTris{};
    GfxWindowBackend* mWapi = nullptr;
    GfxRenderingAPI* mRapi = nullptr;

    uintptr_t mSegmentPointers[MAX_SEGMENT_POINTERS]{};

    bool mFbActive{};
    bool mRendersToFb{}; // game_renders_to_framebuffer;
    // When true, ViewportMatchesRendererResolution() always returns false,
    // which pins mRendersToFb=true and keeps mGameFb populated every frame.
    // Required by GPU-readback consumers (e.g. SSB64's stage-clear / scene-
    // transition wallpaper capture) on backends where FB 0 is the swap-
    // chain back buffer with undefined post-Present contents (D3D11 FLIP_-
    // DISCARD on Windows). On macOS the same effect is achieved
    // unconditionally via the __APPLE__ guard inside that method.
    bool mForceRenderToFb{};
    // SSB64 port widescreen toggle — see SetWidescreenActive() doc.
    bool mWidescreenActive{};
    bool mTight4_3ScissorWindow{};
    std::map<int, FBInfo>::iterator mActiveFrameBuffer;
    std::map<int, FBInfo> mFrameBuffers;

    int mGameFb{};             // game_framebuffer;
    int mGameFbMsaaResolved{}; // game_framebuffer_msaa_resolved;

    // Post-process / user-shader pipeline. Owns its own FBO(s) and the
    // currently-loaded compiled program. Inactive (passthrough) until a
    // shader is loaded via PostProcessChain::LoadShader.
    PostProcessChain mPostProcessChain{};
    uint32_t mFrameCounter{};      // Monotonic, wraps. Fed to shader uniforms.
    bool mPostProcessEnabled{};    // Last-seen CVAR_POSTPROCESS_ENABLED.
    std::string mPostProcessName;  // Last-seen CVAR_POSTPROCESS_SHADER.
    // Latches when the current (name, enabled) combo failed to load,
    // so the per-frame UpdatePostProcessFromCVars doesn't retry the
    // same broken preset 60× per second. Cleared on cvar change or
    // when an attempted load succeeds.
    bool mPostProcessLoadFailed{};

    std::set<std::pair<float, float>> mGetPixelDepthPending; // get_pixel_depth_pending;
    std::unordered_map<std::pair<float, float>, uint16_t, hash_pair_ff> mGetPixelDepthCached; // get_pixel_depth_cached;
    std::map<std::string, MaskedTextureEntry, std::less<>> mMaskedTextures;
    // base addr -> (end addr exclusive, GPU FB id, source-FB UV sub-rect).
    // Ordered so range lookup can do an upper_bound + step-back walk.
    // See RegisterFbTexture.
    struct FbTextureRange {
        uintptr_t end;
        int fbId;
        float u0, v0, u1, v1;
    };
    std::map<uintptr_t, FbTextureRange> mFbTextures;

    // Per-tile-slot UV transform applied in GfxSpTri1 to remap a tile's local
    // UV (0..1) into the registered FB's sub-rect [u0,u1] x [v0,v1].
    // Identity (scale=1, offset=0) when the bound texture isn't an FB mirror,
    // which is the common case. Set by ImportTexture's FB-mirror hook on hit
    // and reset to identity on every miss so a previous hit's transform never
    // leaks into a fresh non-FB binding.
    struct FbUvTransform {
        float scaleU, scaleV;
        float offsetU, offsetV;
    };
    FbUvTransform mFbUvTransform[2] = { { 1.0f, 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f, 0.0f } };

    const std::unordered_map<Mtx*, MtxF>* mCurMtxReplacements;
    bool mMarkerOn; // This was originally a debug feature. Now it seems to control s2dex?
    std::unordered_map<size_t, const char*> mShaders;

    typedef size_t ShaderId;
    std::stack<ShaderId> mShaderStack;
    size_t mShadersIndex;
    int mInterpolationIndex;
    int mInterpolationIndexTarget;
};

void gfx_set_target_ucode(UcodeHandlers ucode);
void gfx_push_current_dir(char* path);
int32_t gfx_check_image_signature(const char* imgData);
const char* gfx_get_shader(int16_t id);
const char* GfxGetOpcodeName(int8_t opcode);

/* ─── Game-specific DL safety hooks ──────────────────────────────────────
 * libultraship's GFX walker can be given two optional callbacks by the
 * embedding game so it can defend against malformed DLs without dragging
 * game-specific symbols into libultraship.
 *
 * Use case: SSB64's per-MObj material-setup DLs are built at runtime
 * directly into the scene heap and sometimes lack `gsSPEndDisplayList`,
 * causing gfx_step to walk off the buffer into adjacent memory. The game
 * registers a bounds-check that consults its own DL-source range registry
 * (scene arena + reloc files); libultraship calls it before deref / push
 * and skips the walk frame if the cmd has escaped any known DL allocation.
 *
 * Both callbacks are optional. With none registered, gfx_step behaves
 * exactly as before. */

/* DL walker bounds-check.
 *   Returns 0 (UNKNOWN) — let through; libultraship makes no decision.
 *   Returns 1 (IN_RANGE) — confirmed valid; allow.
 *   Returns 2 (WALKED_PAST) — addr is just past a registered range; reject. */
using DLBoundsCheckFn = int (*)(uintptr_t addr);
void RegisterDLBoundsCheck(DLBoundsCheckFn fn);

/* Address classifier for diag dumps. Writes a human-readable label
 * (e.g. "scene_arena+0x4528") into buf. Returns nonzero if classified. */
using AddressClassifierFn = int (*)(uintptr_t addr, char* buf, size_t buf_size);
void RegisterAddressClassifier(AddressClassifierFn fn);

/* Dump the recent-DL-pushes and recent-segment-writes ring buffers via
 * SPDLOG_CRITICAL. Called from SIGSEGV handlers (CrashHandler.cpp,
 * port_watchdog.cpp) — `badCmd` is best-effort, typically `siginfo->si_addr`.
 * Safe to call multiple times; the buffers are static and not mutated by
 * dumping. */
void DumpDLDiag(void* badCmd, const char* reason);

} // namespace Fast

extern "C" void gfx_texture_cache_clear();
extern "C" int gfx_create_framebuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                                      uint8_t resize);

/* GBI trace callback — called for every command during interpreter execution.
 * Parameters: w0, w1 (raw command words), dl_depth (call stack depth).
 * Set to NULL to disable. */
typedef void (*GbiTraceCallbackFn)(uintptr_t w0, uintptr_t w1, int dl_depth);
extern "C" void gfx_set_trace_callback(GbiTraceCallbackFn callback);

/* Hi-res texture pack hook. Fires once per cache-miss texture upload, after
 * the format-specific N64 decode has produced a tightly-packed RGBA8 buffer
 * (mTexUploadBuffer) of `width * height * 4` bytes that the GPU is about to
 * receive. The host may hash the decoded pixels and substitute a pre-decoded
 * RGBA8 replacement at any resolution; the interpreter then uploads that
 * buffer instead of the original.
 *
 * Hashing post-decode (rather than over the raw N64 source bytes) makes the
 * key independent of source byte layout — Sprite-fixup, Bitmap-fixup, raw
 * paths, and CI-with-palette all collapse to the same RGBA8 image and the
 * same hash. The offline pack-conversion tool computes the same CRC32-IEEE
 * over the decoded pixels to name pack PNGs, so a runtime hit ⇔ matching PNG.
 *
 * fmt: G_IM_FMT_*  (RGBA=0, YUV=1, CI=2, IA=3, I=4) — informational only;
 *   the decoded RGBA8 alone is the hash input.
 * siz: G_IM_SIZ_*  (4b=0, 8b=1, 16b=2, 32b=3) — informational only.
 * rgba8: pointer to the decoded RGBA8 buffer (mTexUploadBuffer). Layout is
 *   tightly packed, row-major, 4 bytes per pixel, `width * height * 4` total.
 * width / height: pixel dimensions of the decoded buffer.
 * On hit, hook owns *outBuf and guarantees it stays valid through the
 *   immediately-following synchronous UploadTexture call. outW / outH carry
 *   the substitute dimensions. Returns true.
 * On miss/disabled, returns false; the decoded N64 pixels upload unchanged.
 */
typedef bool (*GfxHiResHookFn)(uint8_t fmt, uint8_t siz,
                               const uint8_t* rgba8, uint16_t width, uint16_t height,
                               const uint8_t** outBuf, uint16_t* outW, uint16_t* outH);
extern "C" void gfx_register_hires_hook(GfxHiResHookFn callback);
