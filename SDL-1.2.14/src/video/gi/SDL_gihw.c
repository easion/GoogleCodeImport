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

/* Allocates a surface in video memory */
int sdl_netbas_gi_AllocHWSurface (_THIS, SDL_Surface *surface)
{
	return -1;
}

/* Sets the hardware accelerated blit function, if any, based
   on the current flags of the surface (colorkey, alpha, etc.)
 */
int sdl_netbas_gi_CheckHWBlit (_THIS, SDL_Surface *src, SDL_Surface *dst)
{
	return 0;
}

/* Fills a surface rectangle with the given color */
int sdl_netbas_gi_FillHWRect (_THIS, SDL_Surface *dst, SDL_Rect *rect, Uint32 color)
{
	return 0;
}

/* Sets video mem colorkey and accelerated blit function */
int sdl_netbas_gi_SetHWColorKey (_THIS, SDL_Surface *surface, Uint32 key)
{
	return 0;
}

/* Sets per surface hardware alpha value */
int sdl_netbas_gi_SetHWAlpha (_THIS, SDL_Surface *surface, Uint8 value)
{
	return 0;
}

/* Returns a readable/writable surface */
int sdl_netbas_gi_LockHWSurface (_THIS, SDL_Surface *surface)
{
	return 0;
}

void sdl_netbas_gi_UnlockHWSurface (_THIS, SDL_Surface *surface)
{
}

/* Performs hardware flipping */
int sdl_netbas_gi_FlipHWSurface (_THIS, SDL_Surface *surface)
{
	return 0;
}

/* Frees a previously allocated video surface */
void sdl_netbas_gi_FreeHWSurface (_THIS, SDL_Surface *surface)
{
}
