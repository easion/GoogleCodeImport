
#include "SDL_config.h"

#include "SDL_syswm.h"
#include "../../events/SDL_events_c.h"

#include "SDL_gi.h"

/* Set the title and icon text */
void sdl_netbas_gi_SetCaption (_THIS, const char *title, const char *icon)
{
	SDL_Lock_EventThread();
	gi_set_window_caption(this->hidden->window, title);
	gi_set_window_icon_name(this->hidden->window,icon);
	SDL_Unlock_EventThread();

	printf("sdl_netbas_gi_SetCaption title = %s\n",title);
	 //gi_show_window(this->hidden->window);

	//s_window_set_title(this->hidden->window, (char *) title);
}


/* Iconify the window.
   This function returns 1 if there is a window manager and the
   window was actually iconified, it returns 0 otherwise.
*/

/* Grab or ungrab keyboard and mouse input */
SDL_GrabMode sdl_netbas_gi_GrabInput (_THIS, SDL_GrabMode mode)
{
	SDL_Lock_EventThread();
	mode = SDL_GRAB_OFF;
	//mode = X11_GrabInputNoLock(this, mode);
	SDL_Unlock_EventThread();
	return mode;
}



/* Get some platform dependent window information */
int sdl_netbas_gi_GetWMInfo(_THIS, SDL_SysWMinfo *info)
{
    if (info->version.major <= SDL_MAJOR_VERSION) {
        info->window = SDL_Window ;
        return 1 ;
    } else {
        SDL_SetError("Application not compiled with SDL %d.%d\n",
            SDL_MAJOR_VERSION, SDL_MINOR_VERSION) ;
        return -1 ;
    }

	return 0;
}
