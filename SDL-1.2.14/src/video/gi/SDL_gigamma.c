/***************************************************************************
    begin                : Thu Jan 22 2004
    copyright            : (C) 2004 - 2007 by Alper Akcan
    email                : distchx@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation; either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 ***************************************************************************/

#include "SDL_gi.h"

/* Set the gamma correction directly (emulated with gamma ramps) */
int sdl_netbas_gi_SetGamma (_THIS, float red, float green, float blue)
{
	SDL_Lock_EventThread();
    SDL_Unlock_EventThread();
	return 0;
}

/* Get the gamma correction directly (emulated with gamma ramps) */
int sdl_netbas_gi_GetGamma (_THIS, float *red, float *green, float *blue)
{
    int result;

    SDL_Lock_EventThread();
    result = -1;//X11_GetGammaNoLock(this, red, green, blue);
    SDL_Unlock_EventThread();

    return(result);
}

/* Set the gamma ramp */
int sdl_netbas_gi_SetGammaRamp (_THIS, Uint16 *ramp)
{
	return 0;
}

/* Get the gamma ramp */
int sdl_netbas_gi_GetGammaRamp (_THIS, Uint16 *ramp)
{
	return 0;
}
