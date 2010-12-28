
#include "SDL_config.h"

/* This is the XFree86 Xv extension implementation of YUV video overlays */

#include "SDL_video.h"
#include "SDL_gi.h"


extern SDL_Overlay *Gix_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display);

extern int Gix_LockYUVOverlay(_THIS, SDL_Overlay *overlay);

extern void Gix_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay);

extern int Gix_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *src, SDL_Rect *dst);

extern void Gix_FreeYUVOverlay(_THIS, SDL_Overlay *overlay);

