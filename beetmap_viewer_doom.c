#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "SDL.h"

#include "doomtype.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_video.h"

void D_DoomMain();

void beetmap_viewer_doom_loop() {
    myargc = 1;
    myargv = malloc(sizeof(char*));
    assert(myargv != NULL);

    myargv[0] = "";

    M_AddLooseFiles();

    M_FindResponseFile();
    M_SetExeDir();

#ifdef SDL_HINT_NO_SIGNAL_HANDLERS
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
#endif

    fullscreen = false;

    // start doom
    D_DoomMain();
}
