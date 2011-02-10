#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sys/time.h>
#include <sys/utsname.h>

#include "evas_common.h"
#include "evas_macros.h"
#include "evas_gix_outbuf.h"
#include "evas_gix_buffer.h"



typedef struct _Outbuf_Region   Outbuf_Region;

struct _Outbuf_Region
{
   Gix_Output_Buffer *xob;
   int              x;
   int              y;
   int              w;
   int              h;
};

static Eina_List *shmpool = NULL;


#define SHMPOOL_LOCK()
#define SHMPOOL_UNLOCK()

static Gix_Output_Buffer *
_find_xob(  gi_format_code_t format, int w, int h, void *data)
{
   return evas_gix_platform_output_buffer_new(  format, w, h,  data);
}

static void
_unfind_xob(Gix_Output_Buffer *xob, int sync)
{
     evas_gix_platform_output_buffer_free(xob, sync);
}

static void
_clear_xob(int sync)
{
   SHMPOOL_LOCK();
   while (shmpool)
     {
	Gix_Output_Buffer *xob;

	xob = shmpool->data;
	shmpool = eina_list_remove_list(shmpool, shmpool);
	evas_gix_platform_output_buffer_free(xob, sync);
     }
   SHMPOOL_UNLOCK();
}

void
evas_gix_platform_outbuf_init(void)
{
}

void
evas_gix_platform_outbuf_free(Outbuf *buf)
{
   while (buf->priv.pending_writes)
     {
	RGBA_Image *im;
	Outbuf_Region *obr;

	im = buf->priv.pending_writes->data;
	buf->priv.pending_writes = eina_list_remove_list(buf->priv.pending_writes, buf->priv.pending_writes);
	obr = im->extended_info;
	evas_cache_image_drop(&im->cache_entry);
	if (obr->xob) _unfind_xob(obr->xob, 0);
	free(obr);
     }

   evas_gix_platform_outbuf_idle_flush(buf);
   evas_gix_platform_outbuf_flush(buf);
   if (buf->priv.gix_platform.gc)
      gi_destroy_gc( buf->priv.gix_platform.gc); 

   free(buf);
   _clear_xob(0);
}

Outbuf *
evas_gix_platform_outbuf_setup_x(int w, int h, int rot, Outbuf_Depth depth,
				  gi_window_id_t draw, 
				  gi_format_code_t format,
				 int destination_alpha)
{
   Outbuf             *buf;
   gi_screen_info_t *vis;

   vis = gi_default_screen_info();
   buf = calloc(1, sizeof(Outbuf));
   if (!buf)
      return NULL;

   buf->w = w;
   buf->h = h;
   buf->depth = GI_RENDER_FORMAT_DEPTH(format);
   buf->rot = rot;
   buf->priv.gix_platform.ogformat = format;
   buf->priv.destination_alpha = destination_alpha;

   {
      Gfx_Func_Convert    conv_func;
      Gix_Output_Buffer    *xob;

      xob = evas_gix_platform_output_buffer_new(  buf->priv.gix_platform.ogformat,
						  1, 1,  NULL);
      conv_func = NULL;
      if (xob)
	{
#ifdef WORDS_BIGENDIAN
	     buf->priv.gix_platform.swap = 1;
	     buf->priv.gix_platform.bit_swap = 1;
#else
	     buf->priv.gix_platform.swap = 0;
	     buf->priv.gix_platform.bit_swap = 0;
#endif
	
	     {
		buf->priv.mask.r = (DATA32) vis->redmask;
		buf->priv.mask.g = (DATA32) vis->greenmask;
		buf->priv.mask.b = (DATA32) vis->bluemask;
		if (buf->priv.gix_platform.swap)
		  {
		     SWAP32(buf->priv.mask.r);
		     SWAP32(buf->priv.mask.g);
		     SWAP32(buf->priv.mask.b);
		  }
	     }
	   
	}
	
      evas_gix_platform_outbuf_drawable_set(buf, draw);
   }

   return buf;
}

RGBA_Image *
evas_gix_platform_outbuf_new_region_for_update(Outbuf *buf, int x, int y, int w, int h, int *cx, int *cy, int *cw, int *ch)
{
   RGBA_Image         *im;
   Outbuf_Region      *obr;
   int                 bpl = 0;
   int                 alpha;

   if ((buf->onebuf))// && (buf->priv.gix_platform.shm))
     {
	Eina_Rectangle *rect;

	RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, buf->w, buf->h);
	rect = eina_rectangle_new(x, y, w, h);
	if (!rect) return NULL;

	buf->priv.onebuf_regions = eina_list_append(buf->priv.onebuf_regions, rect);
	if (buf->priv.onebuf)
	  {
	     *cx = x;
	     *cy = y;
	     *cw = w;
	     *ch = h;
	     if (!buf->priv.synced)
	       {
		  buf->priv.synced = 1;
	       }
	     return buf->priv.onebuf;
	  }
	obr = calloc(1, sizeof(Outbuf_Region));
	obr->x = 0;
	obr->y = 0;
	obr->w = buf->w;
	obr->h = buf->h;
	*cx = x;
	*cy = y;
	*cw = w;
	*ch = h;

    alpha = (  (buf->priv.destination_alpha));

	if ((buf->rot == 0) &&
	    (buf->priv.mask.r == 0xff0000) &&
	    (buf->priv.mask.g == 0x00ff00) &&
	    (buf->priv.mask.b == 0x0000ff))
	  {
	     obr->xob = evas_gix_platform_output_buffer_new( buf->priv.gix_platform.ogformat,
							      buf->w, buf->h,							      
							      NULL);
             im = (RGBA_Image *) evas_cache_image_data(evas_common_image_cache_get(),
                                                       buf->w, buf->h,
                                                       (DATA32 *) evas_gix_platform_output_buffer_data(obr->xob, &bpl),
                                                       alpha, EVAS_COLORSPACE_ARGB8888);
	     im->extended_info = obr;	     
	  }
	else
	  {
	     im = (RGBA_Image *) evas_cache_image_empty(evas_common_image_cache_get());
             im->cache_entry.flags.alpha |= alpha ? 1 : 0;
             evas_cache_image_surface_alloc(&im->cache_entry, buf->w, buf->h);
	     im->extended_info = obr;
	     if ((buf->rot == 0) || (buf->rot == 180))
               {
                  obr->xob = evas_gix_platform_output_buffer_new(buf->priv.gix_platform.ogformat,
                                                                    buf->w, buf->h,                                                                    
                                                                    NULL);                  
               }
	     else if ((buf->rot == 90) || (buf->rot == 270))
               {
                  obr->xob = evas_gix_platform_output_buffer_new( buf->priv.gix_platform.ogformat,
                                                                    buf->h, buf->w,                                                                    
                                                                    NULL);                  
               }
	  }
	if (alpha)
          memset(im->image.data, 0, w * h * sizeof(DATA32));

        buf->priv.onebuf = im;
	return im;
     }

   obr = calloc(1, sizeof(Outbuf_Region));
   obr->x = x;
   obr->y = y;
   obr->w = w;
   obr->h = h;
   *cx = 0;
   *cy = 0;
   *cw = w;
   *ch = h;

   alpha = ( (buf->priv.destination_alpha));

   if ((buf->rot == 0) &&
       (buf->priv.mask.r == 0xff0000) &&
       (buf->priv.mask.g == 0x00ff00) &&
       (buf->priv.mask.b == 0x0000ff))
     {
	  obr->xob = _find_xob(buf->priv.gix_platform.ogformat, w, h, NULL);
      im = (RGBA_Image *) evas_cache_image_data(evas_common_image_cache_get(),
                                                  w, h,
                                                  (DATA32 *) evas_gix_platform_output_buffer_data(obr->xob, &bpl),
                                                  alpha, EVAS_COLORSPACE_ARGB8888);
	im->extended_info = obr;
	
     }
   else
     {
        im = (RGBA_Image *) evas_cache_image_empty(evas_common_image_cache_get());
        im->cache_entry.flags.alpha |= alpha ? 1 : 0;
        evas_cache_image_surface_alloc(&im->cache_entry, w, h);
	im->extended_info = obr;
	if ((buf->rot == 0) || (buf->rot == 180))
          {
             obr->xob = _find_xob(buf->priv.gix_platform.ogformat,
                                  w, h, 
                                  NULL);
             
          }

	else if ((buf->rot == 90) || (buf->rot == 270))
          {
             obr->xob = _find_xob( buf->priv.gix_platform.ogformat,
                                  h, w,                                  
                                  NULL);             
          }

     }
   
   if ( (buf->priv.destination_alpha))
     memset(im->image.data, 0, w * h * sizeof(DATA32));

      buf->priv.pending_writes = eina_list_append(buf->priv.pending_writes, im);
   return im;
}

void
evas_gix_platform_outbuf_free_region_for_update(Outbuf *buf __UNUSED__, RGBA_Image *update __UNUSED__)
{
   /* no need to do anything - they are cleaned up on flush */
}

void
evas_gix_platform_outbuf_flush(Outbuf *buf)
{
   Eina_List *l;
   RGBA_Image *im;
   Outbuf_Region *obr;

   if ((buf->priv.onebuf) && (buf->priv.onebuf_regions))
     {
	gi_region_ptr_t tmpr;

	im = buf->priv.onebuf;
	obr = im->extended_info;
	tmpr = XCreateRegion();
	while (buf->priv.onebuf_regions)
	  {
	     Eina_Rectangle *rect;
	     gi_cliprect_t xr;

	     rect = buf->priv.onebuf_regions->data;
	     buf->priv.onebuf_regions = eina_list_remove_list(buf->priv.onebuf_regions, buf->priv.onebuf_regions);
	     xr.x = rect->x;
	     xr.y = rect->y;
	     xr.w = rect->w;
	     xr.h = rect->h;
	     XUnionRectWithRegion(&xr, tmpr, tmpr);	     
	     eina_rectangle_free(rect);
	  }
	gi_set_gc_clip_rectangles( buf->priv.gix_platform.gc, tmpr->rects,tmpr->numRects);
	evas_gix_platform_output_buffer_paste(obr->xob, buf->priv.gix_platform.win,
						buf->priv.gix_platform.gc,
						0, 0, 0);
	
	XDestroyRegion(tmpr);
	buf->priv.synced = 0;
     }
   else
     {

	EINA_LIST_FOREACH(buf->priv.pending_writes, l, im)
	  {
	     obr = im->extended_info;
	     
	     evas_gix_platform_output_buffer_paste(obr->xob, buf->priv.gix_platform.win,
						     buf->priv.gix_platform.gc,
						     obr->x, obr->y, 0);	     
	  }

	while (buf->priv.prev_pending_writes)
	  {
	     im = buf->priv.prev_pending_writes->data;
	     buf->priv.prev_pending_writes =
	       eina_list_remove_list(buf->priv.prev_pending_writes,
				     buf->priv.prev_pending_writes);
	     obr = im->extended_info;
	     evas_cache_image_drop(&im->cache_entry);
	     if (obr->xob) _unfind_xob(obr->xob, 0);

	     free(obr);
	  }

	buf->priv.prev_pending_writes = buf->priv.pending_writes;
	buf->priv.pending_writes = NULL;
     }
   evas_common_cpu_end_opt();
}

void
evas_gix_platform_outbuf_idle_flush(Outbuf *buf)
{
   if (buf->priv.onebuf)
     {
        RGBA_Image *im;
	Outbuf_Region *obr;

	im = buf->priv.onebuf;
	buf->priv.onebuf = NULL;
	obr = im->extended_info;
	if (obr->xob) evas_gix_platform_output_buffer_free(obr->xob, 0);
	free(obr);
	evas_cache_image_drop(&im->cache_entry);
     }
   else
     {
	while (buf->priv.prev_pending_writes)
	  {
	     RGBA_Image *im;
	     Outbuf_Region *obr;

	     im = buf->priv.prev_pending_writes->data;
	     buf->priv.prev_pending_writes =
	       eina_list_remove_list(buf->priv.prev_pending_writes,
				     buf->priv.prev_pending_writes);
	     obr = im->extended_info;
	     evas_cache_image_drop(&im->cache_entry);
	     if (obr->xob) _unfind_xob(obr->xob, 0);

	     free(obr);
	  }

	_clear_xob(0);
     }
}

void
evas_gix_platform_outbuf_push_updated_region(Outbuf *buf, RGBA_Image *update, int x, int y, int w, int h)
{
   Gfx_Func_Convert    conv_func = NULL;
   Outbuf_Region      *obr;
   DATA32             *src_data;
   void               *data;
   int                 bpl = 0, yy;

   obr = update->extended_info;
  
     {
	if ((buf->rot == 0) || (buf->rot == 180))
	  conv_func = evas_common_convert_func_get(0, w, h,
						   evas_gix_platform_output_buffer_depth
						   (obr->xob), buf->priv.mask.r,
						   buf->priv.mask.g, buf->priv.mask.b,
						   PAL_MODE_NONE, buf->rot);
	else if ((buf->rot == 90) || (buf->rot == 270))
	  conv_func = evas_common_convert_func_get(0, h, w,
						   evas_gix_platform_output_buffer_depth
						   (obr->xob), buf->priv.mask.r,
						   buf->priv.mask.g, buf->priv.mask.b,
						   PAL_MODE_NONE, buf->rot);
     }
   if (!conv_func) return;

   data = evas_gix_platform_output_buffer_data(obr->xob, &bpl);
   src_data = update->image.data;
   if (buf->rot == 0)
     {
	obr->x = x;
	obr->y = y;
     }
   else if (buf->rot == 90)
     {
	obr->x = y;
	obr->y = buf->w - x - w;
     }
   else if (buf->rot == 180)
     {
	obr->x = buf->w - x - w;
	obr->y = buf->h - y - h;
     }
   else if (buf->rot == 270)
     {
	obr->x = buf->h - y - h;
	obr->y = x;
     }
   if ((buf->rot == 0) || (buf->rot == 180))
     {
	obr->w = w;
	obr->h = h;
     }
   else if ((buf->rot == 90) || (buf->rot == 270))
     {
	obr->w = h;
	obr->h = w;
     }
   
     {
	if (data != src_data)
	  conv_func(src_data, data,
		    0,
		    bpl /
		    ((evas_gix_platform_output_buffer_depth(obr->xob) /
		      8)) - obr->w, obr->w, obr->h, x, y, NULL);
     }   

}

void
evas_gix_platform_outbuf_reconfigure(Outbuf * buf, int w, int h, int rot,
				     Outbuf_Depth depth)
{
   if ((w == buf->w) &&
       (h == buf->h) &&
       (rot == buf->rot) &&
       (depth == buf->depth)) return;
   buf->w = w;
   buf->h = h;
   buf->rot = rot;
   evas_gix_platform_outbuf_idle_flush(buf);
}

int
evas_gix_platform_outbuf_get_width(Outbuf * buf)
{
   return buf->w;
}

int
evas_gix_platform_outbuf_get_height(Outbuf * buf)
{
   return buf->h;
}

Outbuf_Depth
evas_gix_platform_outbuf_get_depth(Outbuf * buf)
{
   return buf->depth;
}

int
evas_gix_platform_outbuf_get_rot(Outbuf * buf)
{
   return buf->rot;
}

void
evas_gix_platform_outbuf_drawable_set(Outbuf * buf, gi_window_id_t draw)
{

   if (buf->priv.gix_platform.win == draw) return;
   if (buf->priv.gix_platform.gc)
     {
	gi_destroy_gc( buf->priv.gix_platform.gc);
	buf->priv.gix_platform.gc = NULL;
     }
   buf->priv.gix_platform.win = draw;
   buf->priv.gix_platform.gc = gi_create_gc( buf->priv.gix_platform.win, 0);
}


