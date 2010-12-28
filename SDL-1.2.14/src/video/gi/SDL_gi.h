
#ifndef SDL_netbas_gi_h
#define SDL_netbas_gi_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include <gi/gi.h>



#include "SDL.h"
#include "SDL_error.h"
#include "SDL_mouse.h"
#include "SDL_mutex.h"
#include "../SDL_sysvideo.h"
#include "../../events/SDL_sysevents.h"
#include "../../events/SDL_events_c.h"

#define _THIS	SDL_VideoDevice *this


struct WMcursor {
	int unused;
};

struct SDL_PrivateVideoData {
    int w;
    int h;
    int bpp;
    void *buffer;
   // s_thread_t *tid;
    gi_window_id_t window;
    //gi_window_id_t fullscr_window;
	gi_gc_ptr_t gc;
	gi_image_t *img;
	int currently_fullscreen;
	int             OffsetX, OffsetY ;
};

/* cursor.c */
void sdl_netbas_gi_FreeWMCursor (_THIS, WMcursor *cursor);
WMcursor * sdl_netbas_gi_CreateWMCursor (_THIS, Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y);
int sdl_netbas_gi_ShowWMCursor (_THIS, WMcursor *cursor);
void sdl_netbas_gi_WarpWMCursor (_THIS, Uint16 x, Uint16 y);
void sdl_netbas_gi_MoveWMCursor (_THIS, int x, int y);
void sdl_netbas_gi_CheckMouseMode (_THIS);

/* event.c */
//void sdl_netbas_gi_atexit (s_window_t *window);
//void sdl_netbas_gi_atevent (s_window_t *window, gi_msg_t *event);
void sdl_netbas_gi_InitOSKeymap (_THIS);
void sdl_netbas_gi_PumpEvents (_THIS);
//SDL_keysym * sdl_netbas_gi_translatekey(gi_msg_t *event, SDL_keysym *keysym);

/* gamma.c */
int sdl_netbas_gi_SetGamma (_THIS, float red, float green, float blue);
int sdl_netbas_gi_GetGamma (_THIS, float *red, float *green, float *blue);
int sdl_netbas_gi_SetGammaRamp (_THIS, Uint16 *ramp);
int sdl_netbas_gi_GetGammaRamp (_THIS, Uint16 *ramp);

/* gl.c */
int sdl_netbas_gi_GL_LoadLibrary (_THIS, const char *path);
void* sdl_netbas_gi_GL_GetProcAddress (_THIS, const char *proc);
int sdl_netbas_gi_GL_GetAttribute (_THIS, SDL_GLattr attrib, int* value);
int sdl_netbas_gi_GL_MakeCurrent (_THIS);
void sdl_netbas_gi_GL_SwapBuffers (_THIS);

/* hw.c */
int sdl_netbas_gi_AllocHWSurface (_THIS, SDL_Surface *surface);
int sdl_netbas_gi_CheckHWBlit (_THIS, SDL_Surface *src, SDL_Surface *dst);
int sdl_netbas_gi_FillHWRect (_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color);
int sdl_netbas_gi_SetHWColorKey (_THIS, SDL_Surface *surface, Uint32 key);
int sdl_netbas_gi_SetHWAlpha (_THIS, SDL_Surface *surface, Uint8 value);
int sdl_netbas_gi_LockHWSurface (_THIS, SDL_Surface *surface);
void sdl_netbas_gi_UnlockHWSurface (_THIS, SDL_Surface *surface);
int sdl_netbas_gi_FlipHWSurface (_THIS, SDL_Surface *surface);
void sdl_netbas_gi_FreeHWSurface (_THIS, SDL_Surface *surface);

/* video.c */
int sdl_netbas_gi_Available(void);
void sdl_netbas_gi_DeleteDevice(SDL_VideoDevice *device);
SDL_VideoDevice *sdl_netbas_gi_CreateDevice(int devindex);
int sdl_netbas_gi_VideoInit (_THIS, SDL_PixelFormat *vformat);
SDL_Rect ** sdl_netbas_gi_ListModes (_THIS, SDL_PixelFormat *format, Uint32 flags);
SDL_Surface * sdl_netbas_gi_SetVideoMode (_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
int sdl_netbas_gi_ToggleFullScreen (_THIS, int on);
void sdl_netbas_gi_UpdateMouse (_THIS);
SDL_Overlay * sdl_netbas_gi_CreateYUVOverlay (_THIS, int width, int height, Uint32 format, SDL_Surface *display);
int sdl_netbas_gi_SetColors (_THIS, int firstcolor, int ncolors, SDL_Color *colors);
void sdl_netbas_gi_UpdateRects (_THIS, int numrects, SDL_Rect *rects);
void sdl_netbas_gi_VideoQuit (_THIS);

/* wm.c */
void sdl_netbas_gi_SetCaption (_THIS, const char *title, const char *icon);
SDL_GrabMode sdl_netbas_gi_GrabInput (_THIS, SDL_GrabMode mode);
int sdl_netbas_gi_GetWMInfo (_THIS, SDL_SysWMinfo *info);


#define SDL_Window           (this -> hidden -> window)
#define SDL_Image            (this -> hidden -> img)
//#define FSwindow             (this -> hidden -> fullscr_window)
#define SDL_GC               (this -> hidden -> gc)
#define currently_fullscreen (this -> hidden -> currently_fullscreen)
#define OffsetX              (this -> hidden -> OffsetX)
#define OffsetY              (this -> hidden -> OffsetY)

#if 0
#define SDL_windowid         (this -> hidden -> SDL_windowid)
#define Image_buff           (this -> hidden -> Image_buff)
#define Clientfb             (this -> hidden -> Clientfb)
#define SDL_Visual           (this -> hidden -> SDL_Visual)
#define SDL_modelist         (this -> hidden -> modelist)

#define GammaRamp_R          (this -> hidden -> GammaRamp_R)
#define GammaRamp_G          (this -> hidden -> GammaRamp_G)
#define GammaRamp_B          (this -> hidden -> GammaRamp_B)
#define pixel_type           (this -> hidden -> pixel_type)
#define fbinfo               (this -> hidden -> fbinfo)
#endif

#endif /* SDL_netbas_gi_h */
