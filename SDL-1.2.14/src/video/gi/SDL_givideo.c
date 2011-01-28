
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_gi.h"


#include "SDL_mouse.h"
#include "../../events/SDL_events_c.h"
#include "../SDL_cursor_c.h"
#include "SDL_giyuv_c.h"

#define GIVID_DRIVER_NAME "GI"
extern gi_cliprect_t SDL_bounds;
//int sdl_netbas_gi_running;
int gi_sdl_fd;

VideoBootStrap GIX_bootstrap = {
	GIVID_DRIVER_NAME, "SDL GI video driver",
	sdl_netbas_gi_Available, sdl_netbas_gi_CreateDevice
};

int sdl_netbas_gi_Available(void)
{
	
	//const char *envr = getenv("SDL_VIDEODRIVER");
	//if ((envr) && (strcmp(envr, GIVID_DRIVER_NAME) == 0)) {
		return(1);
	//}

	//return(0);
}

void sdl_netbas_gi_DeleteDevice(SDL_VideoDevice *device)
{
	if (!device)
	{
		return;
	}
	if(device->hidden)
	free(device->hidden);
	free(device);
}

SDL_GrabMode GI_GrabInput(_THIS, SDL_GrabMode mode)
{
	if ( mode == SDL_GRAB_OFF ) {

		//gi_ungrab_pointer();

			
		gi_clip_cursor(NULL);
		if ( !(SDL_cursorstate & CURSOR_VISIBLE) ) {
		/*	 */
			/*POINT pt;
			int x, y;
			SDL_GetMouseState(&x,&y);
			pt.x = x;
			pt.y = y;
			ClientToScreen(SDL_Window, &pt);
			SetCursorPos(pt.x,pt.y);
			*/
		}
	}
	else{
		//gi_grab_pointer(SDL_Window,TRUE,FALSE,GI_MASK_BUTTON_DOWN,0,GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M);
		gi_clip_cursor(&SDL_bounds);
		if ( !(SDL_cursorstate & CURSOR_VISIBLE) ) {
		/*	 */
			/*POINT pt;
			pt.x = (SDL_VideoSurface->w/2);
			pt.y = (SDL_VideoSurface->h/2);
			ClientToScreen(SDL_Window, &pt);
			SetCursorPos(pt.x, pt.y);
			*/
		}

		//if ( !(this->screen->flags & SDL_FULLSCREEN) )
		//	XRaiseWindow(SDL_Display, WMwindow);
	}

	return(mode);
}

void GI_SetWMIcon(_THIS, SDL_Surface *icon, Uint8 *mask)
{
	SDL_Lock_EventThread();
	SDL_Unlock_EventThread();
}

int GI_IconifyWindow(_THIS)
{
	SDL_Lock_EventThread();
	gi_iconify_window( SDL_Window);
	SDL_Unlock_EventThread();
	//gi_show_window(SDL_Window);
	return(1);
}

int GI_GetVidModeGamma(_THIS, float *red, float *green, float *blue)
{
    int result;

    SDL_Lock_EventThread();
    result = -1;//X11_GetGammaNoLock(this, red, green, blue);
    SDL_Unlock_EventThread();

    return(result);
}

SDL_VideoDevice *sdl_netbas_gi_CreateDevice(int devindex)
{
	SDL_VideoDevice *device;

	/* Initialize all variables that we clean on shutdown */
	device = (SDL_VideoDevice *) malloc(sizeof(SDL_VideoDevice));
	if (device) {
		memset(device, 0, (sizeof *device));
		device->hidden = (struct SDL_PrivateVideoData *) malloc((sizeof *device->hidden));
	}
	if ((device == NULL) || (device->hidden == NULL)) {
		SDL_OutOfMemory();
		if (device) {
			free(device);
		}
		return(0);
	}
	memset(device->hidden, 0, (sizeof *device->hidden));

	device->VideoInit = sdl_netbas_gi_VideoInit;
	device->ListModes = sdl_netbas_gi_ListModes;
	device->SetVideoMode = sdl_netbas_gi_SetVideoMode;
	device->ToggleFullScreen = sdl_netbas_gi_ToggleFullScreen;
	device->UpdateMouse = sdl_netbas_gi_UpdateMouse;
	device->CreateYUVOverlay = Gix_CreateYUVOverlay;
	device->SetColors = sdl_netbas_gi_SetColors;
	device->UpdateRects = sdl_netbas_gi_UpdateRects;
	device->VideoQuit = sdl_netbas_gi_VideoQuit;

	device->AllocHWSurface = sdl_netbas_gi_AllocHWSurface;
	device->CheckHWBlit = NULL;
	device->FillHWRect = NULL;
	device->SetHWColorKey = NULL;
	device->SetHWAlpha = NULL;
	device->LockHWSurface = sdl_netbas_gi_LockHWSurface;
	device->UnlockHWSurface = sdl_netbas_gi_UnlockHWSurface;
	device->FlipHWSurface = NULL;
	device->FreeHWSurface = sdl_netbas_gi_FreeHWSurface;

	device->SetGamma = NULL;
	device->GetGamma = NULL;
	device->SetGammaRamp = NULL;
	device->GetGammaRamp = NULL;

	device->GL_LoadLibrary = NULL;
	device->GL_GetProcAddress = NULL;
	device->GL_GetAttribute = NULL;
	device->GL_MakeCurrent = NULL;
	device->GL_SwapBuffers = NULL;
	
	device->SetCaption = sdl_netbas_gi_SetCaption;
	device->SetIcon = GI_SetWMIcon; //add
	device->IconifyWindow = GI_IconifyWindow; //add
	device->GrabInput = GI_GrabInput; //add
	device->GetWMInfo = NULL;

	device->FreeWMCursor = sdl_netbas_gi_FreeWMCursor;
	device->CreateWMCursor = sdl_netbas_gi_CreateWMCursor;
	device->ShowWMCursor = sdl_netbas_gi_ShowWMCursor;
	device->WarpWMCursor = sdl_netbas_gi_WarpWMCursor;
	device->MoveWMCursor = sdl_netbas_gi_MoveWMCursor;
	device->CheckMouseMode = sdl_netbas_gi_CheckMouseMode;	
	
	device->InitOSKeymap = sdl_netbas_gi_InitOSKeymap;
	device->PumpEvents = sdl_netbas_gi_PumpEvents;

	device->free = sdl_netbas_gi_DeleteDevice;

	return device;
}
   static  gi_screen_info_t si ;

static void create_aux_windows (_THIS)
{
	uint32_t flags =	(GI_MASK_EXPOSURE       |
            GI_MASK_BUTTON_DOWN  | GI_MASK_BUTTON_UP  |
            GI_MASK_FOCUS_IN     | GI_MASK_FOCUS_OUT  |
            GI_MASK_KEY_DOWN     | GI_MASK_KEY_UP     |
		GI_MASK_CONFIGURENOTIFY     | GI_MASK_WINDOW_HIDE|GI_MASK_WINDOW_SHOW     |
            GI_MASK_MOUSE_ENTER  | GI_MASK_MOUSE_EXIT 
                  |  GI_MASK_MOUSE_MOVE     |   GI_MASK_CLIENT_MSG);

   // GR_WM_PROPERTIES props ;

    //Dprintf ("enter create_aux_windows\n") ;

  
    
    //if (FSwindow && FSwindow != GI_DESKTOP_WINDOW_ID) {
     //   gi_destroy_window (FSwindow) ;
    //}
    
    //FSwindow = gi_create_window(GI_DESKTOP_WINDOW_ID, 0,0,1, 1,   
	//	get_winxp_color(), GI_FLAGS_NON_FRAME);
	//GrNewWindow (GR_ROOT_WINDOW_ID, 0, 0, 1, 1, 0, BLACK, BLACK) ;
    //props.flags = GR_WM_FLAGS_PROPS ;
    //props.props = GR_WM_PROPS_NODECORATE ;
   // GrSetWMProperties (FSwindow, & props) ;

    	// gi_set_events_mask (this->hidden->window, flags) ;


    //Dprintf ("leave create_aux_windows\n") ;
}

/* Initialize the native video subsystem, filling 'vformat' with the
   "best" display pixel format, returning 0 or -1 if there's an error.
 */
int sdl_netbas_gi_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
	gi_sdl_fd = gi_init ();

	if (gi_sdl_fd < 0) {
	SDL_SetError ("gi_init() fail") ;
	return -1 ;
	}

	gi_get_screen_info (& si) ;

	vformat->BitsPerPixel = GI_RENDER_FORMAT_BPP(si.format);
	vformat->BytesPerPixel = (vformat->BitsPerPixel)/8;
	this->info.wm_available = 1;

	create_aux_windows(this);

	return 0;
}

/* List the available video modes for the given pixel format, sorted
   from largest to smallest.
 */
SDL_Rect **sdl_netbas_gi_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
   	 return (SDL_Rect **) -1;
}



int GIX_LeaveFullScreen (_THIS)
{
    if (currently_fullscreen) {
        //gi_hide_window (FSwindow) ;
        //gi_show_window (SDL_Window) ;
        //gi_raise_window (SDL_Window) ;
        //gi_set_focus_window (SDL_Window) ;
		gi_fullscreen_window(SDL_Window,FALSE);
        currently_fullscreen = 0 ;
		GI_RefreshDisplay (this) ;
    }

    return 0 ;
}


int GIX_EnterFullScreen (_THIS)
{
    if (! currently_fullscreen) {
		gi_fullscreen_window(SDL_Window,TRUE);
        //gi_screen_info_t si ;

        //gi_get_screen_info (& si) ;
        //gi_resize_window (FSwindow, si.scr_width, si.scr_height) ;
        //gi_hide_window (SDL_Window) ;
        //gi_show_window (FSwindow) ;
        //gi_raise_window (FSwindow) ;
        //gi_set_focus_window (FSwindow) ;
        currently_fullscreen = 1 ; 
		GI_RefreshDisplay (this) ;
    }

    return 1 ;
}



static void GIX_DestroyWindow (_THIS, SDL_Surface * screen)
{
    //Dprintf ("enter GIX_DestroyWindow\n") ;

    if (SDL_Window)
	{
        if (screen && (screen -> flags & SDL_FULLSCREEN)) {
            screen -> flags &= ~ SDL_FULLSCREEN ;
            GIX_LeaveFullScreen (this) ;
        }

        // Destroy the output window
        if (SDL_Window && SDL_Window != GI_DESKTOP_WINDOW_ID) {
            gi_destroy_window (SDL_Window) ;
			//usleep(30000);
        }
    }
    
    // Free the graphics context
    if ( SDL_GC) {
        gi_destroy_gc (SDL_GC) ;
        SDL_GC = 0;
    }

	if (SDL_Image)
	{
		gi_release_bitmap_struct(SDL_Image);
		SDL_Image=NULL;
	}

    //Dprintf ("leave GIX_DestroyWindow\n") ;
}


/* Set the requested video mode, returning a surface which will be
   set to the SDL_VideoSurface.  The width and height will already
   be verified by ListModes(), and the video subsystem is free to
   set the mode to a supported bit depth different from the one
   specified -- the desired bpp will be emulated with a shadow
   surface if necessary.  If a new mode is returned, this function
   should take care of cleaning up the current mode.
 */
SDL_Surface * sdl_netbas_gi_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags)
{
	int err;
	uint32_t wflags =	(GI_MASK_EXPOSURE       |
            GI_MASK_BUTTON_DOWN  | GI_MASK_BUTTON_UP  |
            GI_MASK_FOCUS_IN     | GI_MASK_FOCUS_OUT  |
            GI_MASK_KEY_DOWN     | GI_MASK_KEY_UP     |
		 GI_MASK_CONFIGURENOTIFY     | GI_MASK_WINDOW_HIDE|GI_MASK_WINDOW_SHOW     |
            GI_MASK_MOUSE_ENTER  | GI_MASK_MOUSE_EXIT 
                  |  GI_MASK_MOUSE_MOVE     |   GI_MASK_CLIENT_MSG);

	SDL_Lock_EventThread();

	if (SDL_Window && SDL_Window != GI_DESKTOP_WINDOW_ID) {
        GIX_DestroyWindow (this, current) ;
    }

	if (this->hidden->buffer) {
		free(this->hidden->buffer);
		this->hidden->buffer=NULL;
	}

	bpp = GI_RENDER_FORMAT_BPP(si.format);
	if (bpp == 15) {
		bpp = 16;
	}
        
	this->hidden->buffer = malloc(width * height * (bpp / 8));
    this->hidden->bpp = bpp;
	memset(this->hidden->buffer, 0, width * height * (bpp / 8));

	if (!SDL_ReallocFormat(current, bpp, 0, 0, 0, 0)) {
		free(this->hidden->buffer);
		this->hidden->buffer = NULL;
		SDL_SetError("Couldn't allocate new pixel format for requested mode");
		SDL_Unlock_EventThread();
		return NULL;
	}

	//current->flags = flags & SDL_FULLSCREEN;
	current->flags = flags ;
	this->hidden->w = current->w = width;
	this->hidden->h = current->h = height;
	current->pitch = current->w * (bpp / 8);
	current->pixels = this->hidden->buffer;

	this->hidden->window =gi_create_window(GI_DESKTOP_WINDOW_ID, 0,0,width, height, 
		get_winxp_color(), 0);
	

	if (!this->hidden->window)
	{
		SDL_Unlock_EventThread();
		printf("Test window error\n");
		return NULL;
	}
	 gi_set_events_mask (this->hidden->window, wflags) ; 
	SDL_GC = gi_create_gc(this->hidden->window,NULL);
	SDL_Image =  gi_create_image_with_data(width, height,this->hidden->buffer,si.format);

	if (!SDL_Image)
	{
		SDL_Unlock_EventThread();
		printf("gi_create_image_with_data error\n");
		return NULL;
	}

	printf("sdl_netbas_gi_SetVideoMode: gi_create_window %x OK\n", this->hidden->window);


	
	sdl_netbas_gi_SetCaption(this, this->wm_title, this->wm_icon);
	usleep(100000); //wait for wm
	err = gi_show_window(this->hidden->window);
	if (err)
	{
		printf("gi_show_window %x faied\n",this->hidden->window);
	}

	if ( flags & SDL_FULLSCREEN ) {
		printf("got full screen\n");
		current->flags |= SDL_FULLSCREEN;
		GIX_EnterFullScreen(this);
	} else {
		current->flags &= ~SDL_FULLSCREEN;
	}

	gi_window_info_t info;

	err = gi_get_window_info(this->hidden->window,&info);
	if (!err)
	{
	SDL_bounds.x=info.x;
	SDL_bounds.y=info.y;
	SDL_bounds.w=info.width;
	SDL_bounds.h=info.height;
	}
	else{
	printf("got full gi_get_window_info %x error\n",this->hidden->window);
	}

	SDL_Unlock_EventThread();
	return current ;
}

/* Toggle the fullscreen mode */
int sdl_netbas_gi_ToggleFullScreen (_THIS, int on)
{
    SDL_Rect rect ;
	Uint32 event_thread;

	/* Don't lock if we are the event thread */
	event_thread = SDL_EventThreadID();
	if ( event_thread && (SDL_ThreadID() == event_thread) ) {
		event_thread = 0;
	}
	if ( event_thread ) {
		SDL_Lock_EventThread();
	}


    if (on) {
		
        GIX_EnterFullScreen (this) ;
		 this -> screen -> flags |=  SDL_FULLSCREEN ;
    } else {
        this -> screen -> flags &= ~ SDL_FULLSCREEN ;
        GIX_LeaveFullScreen (this) ;
	   
    }

    rect.x = rect.y = 0 ;
    rect.w = this -> screen -> w, rect.h = this -> screen -> h ;
    sdl_netbas_gi_UpdateRects (this, 1, & rect) ;

	GI_RefreshDisplay (this) ;

	if ( event_thread ) {
		SDL_Unlock_EventThread();
	}

	//return -1;
}

/* This is called after the video mode has been set, to get the
   initial mouse state.  It should queue events as necessary to
   properly represent the current mouse focus and position.
 */
void sdl_netbas_gi_UpdateMouse (_THIS)
{
    int            x, y,_x,_y ;
    gi_window_info_t info ;
	gi_window_id_t win;
    //gi_screen_info_t si ;

   /* Lock the event thread, in multi-threading environments */
	SDL_Lock_EventThread();
    
    gi_get_mouse_pos (GI_DESKTOP_WINDOW_ID,&win, &_x,&_y) ;
    gi_get_window_info (SDL_Window, & info) ;
#if 1
    x = _x - info.x ;
    y = _y - info.y ;
#endif
    if (x >= 0 && x <= info.width && y >= 0 && y <= info.height) {
        SDL_PrivateAppActive (1, SDL_APPMOUSEFOCUS) ;
        SDL_PrivateMouseMotion (0, 0, x, y);
		//printf("SDL_PrivateMouseButton yyy %d %d\n", x, y);
    } else {
        SDL_PrivateAppActive (0, SDL_APPMOUSEFOCUS) ;
    }

    SDL_Unlock_EventThread () ;
    //Dprintf ("leave GIX_UpdateMouse\n") ;
}

/* Create a YUV video surface (possibly overlay) of the given
   format.  The hardware should be able to perform at least 2x
   scaling on display.
 */
SDL_Overlay * sdl_netbas_gi_CreateYUVOverlay (_THIS, int width, int height, Uint32 format, SDL_Surface *display)
{
	return NULL;
}

/* Sets the color entries { firstcolor .. (firstcolor+ncolors-1) }
   of the physical palette to those in 'colors'. If the device is
   using a software palette (SDL_HWPALETTE not set), then the
   changes are reflected in the logical palette of the screen
   as well.
   The return value is 1 if all entries could be set properly
   or 0 otherwise.
 */
int sdl_netbas_gi_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
	return 1;
}

/* This pointer should exist in the native video subsystem and should
   point to an appropriate update function for the current video mode
 */
void sdl_netbas_gi_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
	char *buf;
	int n = numrects;
	SDL_Rect *r = rects;

	gi_gc_attch_window(SDL_GC, SDL_Window);

	while (n--) {
		gi_bitblt_bitmap( SDL_GC,r->x,r->y,r->w,r->h,SDL_Image,r->x,r->y);
		r++;
	}
	
}

/* Reverse the effects VideoInit() -- called if VideoInit() fails
   or if the application is shutting down the video subsystem.
 */
void sdl_netbas_gi_VideoQuit(_THIS)
{
	
   // Start shutting down the windows
   // GIX_DestroyImage (this, this -> screen) ;
   
    GIX_DestroyWindow (this, this -> screen) ;
    //if (FSwindow && FSwindow != GI_DESKTOP_WINDOW_ID) {
      //  gi_destroy_window (FSwindow) ;
    //}
    //GIX_FreeVideoModes (this) ;
	
	if (this->screen->pixels != NULL)
	{
		free(this->screen->pixels);
		this->screen->pixels = NULL;
	}

	printf("sdl_netbas_gi_VideoQuit called\n");
	gi_exit();
	
	
}
