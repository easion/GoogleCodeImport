

#include "SDL_gi.h"

/* Sets the dll to use for OpenGL and loads it */
int sdl_netbas_gi_GL_LoadLibrary (_THIS, const char *path)
{
	return 0;
}

/* Retrieves the address of a function in the gl library */
void * sdl_netbas_gi_GL_GetProcAddress (_THIS, const char *proc)
{
	return NULL;
}

/* Get attribute information from the windowing system. */
int sdl_netbas_gi_GL_GetAttribute (_THIS, SDL_GLattr attrib, int *value)
{
	return 0;
}

/* Make the context associated with this driver current */
int sdl_netbas_gi_GL_MakeCurrent (_THIS)
{
	return 0;
}

/* Swap the current buffers in double buffer mode. */
void sdl_netbas_gi_GL_SwapBuffers (_THIS)
{
}
