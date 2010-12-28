
#include "SDL_config.h"

#include <gi/gi.h>
#include <gi/gv.h>
#include <gi/property.h>


#include "SDL_giyuv_c.h"
#include "../SDL_yuvfuncs.h"

#define None 0

/* Workaround when pitch != width */
#define PITCH_WORKAROUND



/* Fix for the NVidia GeForce 2 - use the last available adaptor */
/*#define USE_LAST_ADAPTOR*/  /* Apparently the NVidia drivers are fixed */

/* The functions used to manipulate software video overlays */
static struct private_yuvhwfuncs x11_yuvfuncs = {
	Gix_LockYUVOverlay,
	Gix_UnlockYUVOverlay,
	Gix_DisplayYUVOverlay,
	Gix_FreeYUVOverlay
};

struct private_yuvhwdata {
	int port;

	XvImage *image;
};

static int xv_error;

SDL_Overlay *Gix_CreateYUVOverlay(_THIS, int width, int height, Uint32 format, SDL_Surface *display)
{
	SDL_Overlay *overlay;
	struct private_yuvhwdata *hwdata;
	int xv_port;
	unsigned int i, j, k;
	unsigned int adaptors;
	XvAdaptorInfo *ainfo;
	int bpp;


	/* Look for the XVideo extension with a valid port for this format */
	xv_port = -1;
	if ( 
	     (0 == gv_query_adaptors(GI_DESKTOP_WINDOW_ID,
	                                 &adaptors, &ainfo)) ) {
#ifdef USE_LAST_ADAPTOR
		for ( i=0; i < adaptors; ++i )
#else
		for ( i=0; (i < adaptors) && (xv_port == -1); ++i )
#endif /* USE_LAST_ADAPTOR */
		{
			#if 0
			/* Check to see if the visual can be used */
			if ( BUGGY_XFREE86(<=, 4001) ) {
				int visual_ok = 0;
				for ( j=0; j<ainfo[i].num_formats; ++j ) {
					if ( ainfo[i].formats[j].visual_id ==
							SDL_Visual->visualid ) {
						visual_ok = 1;
						break;
					}
				}
				if ( ! visual_ok ) {
					continue;
				}
			}
			#endif

			if ( (ainfo[i].type & 2) &&
			     (ainfo[i].type & 1) ) {
				int num_formats;
				XF86ImageRec *formats;

				formats = gv_list_image_formats(
				              ainfo[i].base_id, &num_formats);
#ifdef USE_LAST_ADAPTOR
				for ( j=0; j < num_formats; ++j )
#else
				for ( j=0; (j < num_formats) && (xv_port == -1); ++j )
#endif /* USE_LAST_ADAPTOR */
				{

					if ( (Uint32)formats[j].id == format ) {
						for ( k=0; k < ainfo[i].num_ports; ++k ) {
							if ( 0 == gv_grab_port( ainfo[i].base_id+k, 0) ) {
								xv_port = ainfo[i].base_id+k;
								break;
							}
						}
					}
				}
				if ( formats ) {
					free(formats);
				}
			}
		}
		gv_free_adaptorInfo(ainfo);
	}

	/* Precalculate the bpp for the pitch workaround below */
	switch (format) {
	    /* Add any other cases we need to support... */
	    case SDL_YUY2_OVERLAY:
	    case SDL_UYVY_OVERLAY:
	    case SDL_YVYU_OVERLAY:
		bpp = 2;
		break;
	    default:
		bpp = 1;
		break;
	}

	if ( xv_port == -1 ) {
		SDL_SetError("No available video ports for requested format");
		return(NULL);
	}

	

	/* Enable auto-painting of the overlay colorkey */
	{
		static const char *attr[] = { "XV_AUTOPAINT_COLORKEY", "XV_AUTOPAINT_COLOURKEY" };
		unsigned int i;

		//SDL_NAME(XvSelectPortNotify)(GFX_Display, xv_port, TRUE);
		//X_handler = XSetErrorHandler(xv_errhandler);
		for ( i=0; i < sizeof(attr)/(sizeof attr[0]); ++i ) {
			gi_atom_id_t a;

			xv_error = FALSE;
			a = gi_intern_atom( attr[i], TRUE);
			if ( a != None ) {
     				gv_set_port_attribute( xv_port, a, 1);
				//XSync(GFX_Display, TRUE);
				if ( ! xv_error ) {
					break;
				}
			}
		}
		//XSetErrorHandler(X_handler);
		//SDL_NAME(XvSelectPortNotify)(GFX_Display, xv_port, FALSE);
	}

	/* Create the overlay structure */
	overlay = (SDL_Overlay *)SDL_malloc(sizeof *overlay);
	if ( overlay == NULL ) {
		gv_ungrab_port( xv_port, 0);
		SDL_OutOfMemory();
		return(NULL);
	}
	SDL_memset(overlay, 0, (sizeof *overlay));

	/* Fill in the basic members */
	overlay->format = format;
	overlay->w = width;
	overlay->h = height;

	/* Set up the YUV surface function structure */
	overlay->hwfuncs = &x11_yuvfuncs;
	overlay->hw_overlay = 1;

	/* Create the pixel data and lookup tables */
	hwdata = (struct private_yuvhwdata *)SDL_malloc(sizeof *hwdata);
	overlay->hwdata = hwdata;
	if ( hwdata == NULL ) {
		gv_ungrab_port( xv_port, 0);
		SDL_OutOfMemory();
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}
	hwdata->port = xv_port;

	{
		hwdata->image = gv_create_image( xv_port, format,
							0, width, height);

#ifdef PITCH_WORKAROUND
		if ( hwdata->image != NULL && hwdata->image->pitches[0] != (width*bpp) ) {
			/* Ajust overlay width according to pitch */ 
			free(hwdata->image);
			width = hwdata->image->pitches[0] / bpp;
			hwdata->image = gv_create_image( xv_port, format,
								0, width, height);
		}
#endif /* PITCH_WORKAROUND */
		if ( hwdata->image == NULL ) {
			SDL_SetError("Couldn't create XVideo image");
			SDL_FreeYUVOverlay(overlay);
			return(NULL);
		}
		hwdata->image->data = SDL_malloc(hwdata->image->data_size);
		if ( hwdata->image->data == NULL ) {
			SDL_OutOfMemory();
			SDL_FreeYUVOverlay(overlay);
			return(NULL);
		}
	}

	/* Find the pitch and offset values for the overlay */
	overlay->planes = hwdata->image->num_planes;
	overlay->pitches = (Uint16 *)SDL_malloc(overlay->planes * sizeof(Uint16));
	overlay->pixels = (Uint8 **)SDL_malloc(overlay->planes * sizeof(Uint8 *));
	if ( !overlay->pitches || !overlay->pixels ) {
		SDL_OutOfMemory();
		SDL_FreeYUVOverlay(overlay);
		return(NULL);
	}
	for ( i=0; i<overlay->planes; ++i ) {
		overlay->pitches[i] = hwdata->image->pitches[i];
		overlay->pixels[i] = (Uint8 *)hwdata->image->data +
		                              hwdata->image->offsets[i];
	}

	/* We're all done.. */
	return(overlay);
}

int Gix_LockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	return(0);
}

void Gix_UnlockYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	return;
}

int Gix_DisplayYUVOverlay(_THIS, SDL_Overlay *overlay, SDL_Rect *src, SDL_Rect *dst)
{
	struct private_yuvhwdata *hwdata;

	hwdata = overlay->hwdata;

	gv_put_image( hwdata->port, SDL_Window, SDL_GC,
				 hwdata->image,
						 src->x, src->y, src->w, src->h,
						 dst->x, dst->y, dst->w, dst->h);
	//XSync(GFX_Display, FALSE);
	return(0);
}

void Gix_FreeYUVOverlay(_THIS, SDL_Overlay *overlay)
{
	struct private_yuvhwdata *hwdata;

	hwdata = overlay->hwdata;
	if ( hwdata ) {
		gv_ungrab_port( hwdata->port, 0);

		if ( hwdata->image ) {
			free(hwdata->image);
		}
		SDL_free(hwdata);
	}
	if ( overlay->pitches ) {
		SDL_free(overlay->pitches);
		overlay->pitches = NULL;
	}
	if ( overlay->pixels ) {
		SDL_free(overlay->pixels);
		overlay->pixels = NULL;
	}

}


