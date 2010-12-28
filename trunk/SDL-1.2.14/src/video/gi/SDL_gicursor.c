
#include "SDL_gi.h"


#include "SDL_config.h"


#include "SDL_mouse.h"
#include "../../events/SDL_events_c.h"
#include "../SDL_cursor_c.h"


int mouse_relative = 0;

#if 0
	#define DEBUG_OUT(a) printf(a);
#else
	#define DEBUG_OUT(a)
#endif

/* Free a window manager cursor
   This function can be NULL if CreateWMCursor is also NULL.
 */
void sdl_netbas_gi_FreeWMCursor (_THIS, WMcursor *cursor)
{
	SDL_Lock_EventThread();
	if(cursor)
	free(cursor);
	SDL_Unlock_EventThread();
	DEBUG_OUT("sdl_netbas_gi_FreeWMCursor\n");
}

/* If not NULL, create a black/white window manager cursor */
WMcursor * sdl_netbas_gi_CreateWMCursor (_THIS, Uint8 *data, Uint8 *mask, int w, int h, int hot_x, int hot_y)
{
	SDL_Lock_EventThread();
	WMcursor *cursor = (WMcursor *) malloc(sizeof(WMcursor));
	SDL_Unlock_EventThread();
	DEBUG_OUT("sdl_netbas_gi_CreateWMCursor\n");
	return cursor;
}

/* Show the specified cursor, or hide if cursor is NULL */
int sdl_netbas_gi_ShowWMCursor (_THIS, WMcursor *cursor)
{
	SDL_Lock_EventThread();
	if (cursor)
	{
		gi_load_cursor(SDL_Window,GI_CURSOR_ARROW);
	}
	else{
		gi_load_cursor(SDL_Window,GI_CURSOR_NO);
	}
	SDL_Unlock_EventThread();
	DEBUG_OUT("sdl_netbas_gi_ShowWMCursor\n");
	return 1;
}

/* Warp the window manager cursor to (x,y)
   If NULL, a mouse motion event is posted internally.
 */
void sdl_netbas_gi_WarpWMCursor (_THIS, Uint16 x, Uint16 y)
{
   gi_window_info_t info ;

    SDL_Lock_EventThread () ;

	if ( mouse_relative) {
		/*	RJR: March 28, 2000
			leave physical cursor at center of screen if
			mouse hidden and grabbed */
		SDL_PrivateMouseMotion(0, 0, x, y);
	} else{
    
    gi_get_window_info (SDL_Window, & info) ;
    gi_move_cursor (info.x + x, info.y + y) ;
	}

    SDL_Unlock_EventThread () ;

	//DEBUG_OUT("sdl_netbas_gi_WarpWMCursor\n");
}

/* If not NULL, this is called when a mouse motion event occurs */
void sdl_netbas_gi_MoveWMCursor (_THIS, int x, int y)
{
	DEBUG_OUT("sdl_netbas_gi_MoveWMCursor\n");
}

/* Determine whether the mouse should be in relative mode or not.
   This function is called when the input grab state or cursor
   visibility state changes.
   If the cursor is not visible, and the input is grabbed, the
   driver can place the mouse in relative mode, which may result
   in higher accuracy sampling of the pointer motion.
*/
void sdl_netbas_gi_CheckMouseMode (_THIS)
{
	SDL_Lock_EventThread();

	DEBUG_OUT("sdl_netbas_gi_CheckMouseMode\n");
	 /* If the mouse is hidden and input is grabbed, we use relative mode */
        if ( !(SDL_cursorstate & CURSOR_VISIBLE) &&
             (this->input_grab != SDL_GRAB_OFF) ) {
                mouse_relative = 1;
        } else {
                mouse_relative = 0;
        }
	SDL_Unlock_EventThread();
}
