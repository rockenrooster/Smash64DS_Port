/*
 * Correctness-oriented frame-loop shape for current libnds-style projects.
 * The program samples input once per logical update and keeps VBlank work small.
 */
#include <nds.h>
#include <stdbool.h>
#include <stdint.h>

struct InputFrame {
    uint32_t down;
    uint32_t held;
    uint32_t up;
    touchPosition touch;
    bool has_touch;
};

struct GameState {
    int x;
    int y;
};

static struct InputFrame input_read_once(void)
{
    struct InputFrame input = {0};

    scanKeys();
    input.down = keysDown();
    input.held = keysHeld();
    input.up = keysUp();
    input.has_touch = (input.held & KEY_TOUCH) != 0;

    if (input.has_touch) {
        touchRead(&input.touch);
    }

    return input;
}

static void game_update(struct GameState *game, const struct InputFrame *input)
{
    game->x += ((input->held & KEY_RIGHT) != 0) -
               ((input->held & KEY_LEFT) != 0);
    game->y += ((input->held & KEY_DOWN) != 0) -
               ((input->held & KEY_UP) != 0);
}

static void render_prepare(const struct GameState *game)
{
    (void)game;
    // Build shadow OAM, BG dirty state, and a 3D draw list here.
    // Do not perform filesystem I/O or runtime asset conversion here.
}

static void render_submit(void)
{
    // Submit GX work here. Finalize the 3D frame exactly once if enabled.
}

static void video_commit_vblank(void)
{
    // Keep this bounded. Commit only subsystems initialized by this program.
    // oamUpdate(&oamMain);
    // oamUpdate(&oamSub);
    // bgUpdate();
}

int main(void)
{
    struct GameState game = {0};

    // Initialize display modes, VRAM, BG/OAM/GX, audio, and assets once through
    // the project's platform owners before entering the loop.

    while (pmMainLoop()) {
        const struct InputFrame input = input_read_once();

        if ((input.down & KEY_START) != 0) {
            break;
        }

        game_update(&game, &input);
        render_prepare(&game);
        render_submit();

        swiWaitForVBlank();
        video_commit_vblank();
    }

    return 0;
}
