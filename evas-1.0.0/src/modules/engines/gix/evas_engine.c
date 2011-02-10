#include "evas_common.h"
#include "evas_private.h"

#include "Evas_Engine_Gix.h"
#include "evas_engine.h"
# include "evas_gix_outbuf.h"

int _evas_engine_gix_log_dom = -1;

/* function tables - filled in later (func and parent func) */
static Evas_Func func, pfunc;


/* engine struct data */
typedef struct _Render_Engine Render_Engine;

struct _Render_Engine
{
   Tilebuf      *tb;
   Outbuf       *ob;
   Tilebuf_Rect *rects;
   Eina_Inlist  *cur_rect;
   int           end : 1;  

};


static void *eng_gix_info(Evas *e);
static void eng_gix_info_free(Evas *e, void *info);
static int eng_gix_setup(Evas *e, void *info);
static void eng_gix_output_free(void *data);
static void eng_gix_output_resize(void *data, int w, int h);
static void eng_gix_output_tile_size_set(void *data, int w, int h);
static void eng_gix_output_redraws_rect_add(void *data, int x, int y, int w, int h);
static void eng_gix_output_redraws_rect_del(void *data, int x, int y, int w, int h);
static void eng_gix_output_redraws_clear(void *data);
static void *eng_gix_output_redraws_next_update_get(void *data, int *x, int *y, int *w, int *h, int *cx, int *cy, int *cw, int *ch);
static void eng_gix_output_redraws_next_update_push(void *data, void *surface, int x, int y, int w, int h);
static void eng_gix_output_flush(void *data);
static void eng_gix_output_idle_flush(void *data);


/* internal engine routines */

static void *
_output_gix_setup(int      w,
                   int      h,
                   int      rot,
                   gi_window_id_t draw,
                   gi_format_code_t      format,                                 
                   int      destination_alpha)
{
   Render_Engine *re;

   re = calloc(1, sizeof(Render_Engine));
   if (!re)
     return NULL;

   evas_gix_platform_outbuf_init();

#if 0    
	evas_common_font_dpi_set(re->xr.dpi / 1000);
#endif


  
   re->ob = evas_gix_platform_outbuf_setup_x(w,
									  h,
									  rot,
									  OUTBUF_DEPTH_INHERIT,                                              
									  draw,                                            
									  format,									  
									  destination_alpha);
   if (!re->ob)
     {
	free(re);
	return NULL;
     }
  

   re->tb = evas_common_tilebuf_new(w, h);
   if (!re->tb)
     {
	evas_gix_platform_outbuf_free(re->ob);
	free(re);
	return NULL;
     }
   /* in preliminary tests 16x16 gave highest framerates */
   evas_common_tilebuf_set_tile_size(re->tb, TILESIZE, TILESIZE);
   return re;
}



/* engine api this module provides */
static void *
eng_gix_info(Evas *e __UNUSED__)
{
   Evas_Engine_Info_Gix *info;
   info = calloc(1, sizeof(Evas_Engine_Info_Gix));
   if (!info) return NULL;
   info->magic.magic = rand();
   
   return info;
}

static void
eng_gix_info_free(Evas *e __UNUSED__, void *info)
{
   Evas_Engine_Info_Gix *in;
   in = (Evas_Engine_Info_Gix *)info;
   free(in);
}

static int
eng_gix_setup(Evas *e, void *in)
{
   Evas_Engine_Info_Gix *info;
   Render_Engine *re = NULL;

   info = (Evas_Engine_Info_Gix *)in;
   if (!e->engine.data.output)
     {
        /* if we haven't initialized - init (automatic abort if already done) */
        evas_common_cpu_init();
        evas_common_blend_init();
        evas_common_image_init();
        evas_common_convert_init();
        evas_common_scale_init();
        evas_common_rectangle_init();
        evas_common_polygon_init();
        evas_common_line_init();
        evas_common_font_init();
        evas_common_draw_init();
        evas_common_tilebuf_init();
  printf("%s got line%d\n",__FUNCTION__,__LINE__);

		 re = _output_gix_setup(e->output.w,
							 e->output.h,
							 info->info.rotation,								
							 info->info.drawable,			 
							 info->info.gformat,							 
							 info->info.destination_alpha);		

        e->engine.data.output = re;
     }
   else
     {
	int            ponebuf = 0;

	re = e->engine.data.output;
	ponebuf = re->ob->onebuf;

	 evas_gix_platform_outbuf_free(re->ob);
	 re->ob = evas_gix_platform_outbuf_setup_x(e->output.w,
											e->output.h,
											info->info.rotation,
											OUTBUF_DEPTH_INHERIT,												
											info->info.drawable,											
											info->info.gformat,											
											info->info.destination_alpha);

	re->ob->onebuf = ponebuf;
     }
   if (!e->engine.data.output) return 0;
   if (!e->engine.data.context)
     e->engine.data.context =
     e->engine.func->context_new(e->engine.data.output);

   re = e->engine.data.output;

   return 1;
}

static void
eng_gix_output_free(void *data)
{
   Render_Engine *re;

   if (!data) return;
   re = (Render_Engine *)data;
   evas_gix_platform_outbuf_free(re->ob);
   evas_common_tilebuf_free(re->tb);
   if (re->rects) evas_common_tilebuf_free_render_rects(re->rects);
   free(re);
   evas_common_font_shutdown();
   evas_common_image_shutdown();
}

static void
eng_gix_output_resize(void *data, int w, int h)
{
   Render_Engine *re;
  printf("%s got line%d\n",__FUNCTION__,__LINE__);

   re = (Render_Engine *)data;
   evas_gix_platform_outbuf_reconfigure(re->ob, w, h,
                          evas_gix_platform_outbuf_get_rot(re->ob),
                          OUTBUF_DEPTH_INHERIT);
   evas_common_tilebuf_free(re->tb);
   re->tb = evas_common_tilebuf_new(w, h);
   if (re->tb)
     evas_common_tilebuf_set_tile_size(re->tb, TILESIZE, TILESIZE);
}

static void
eng_gix_output_tile_size_set(void *data, int w, int h)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   evas_common_tilebuf_set_tile_size(re->tb, w, h);
}

static void
eng_gix_output_redraws_rect_add(void *data, int x, int y, int w, int h)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   evas_common_tilebuf_add_redraw(re->tb, x, y, w, h);
}

static void
eng_gix_output_redraws_rect_del(void *data, int x, int y, int w, int h)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   evas_common_tilebuf_del_redraw(re->tb, x, y, w, h);
}

static void
eng_gix_output_redraws_clear(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   evas_common_tilebuf_clear(re->tb);
}

static void *
eng_gix_output_redraws_next_update_get(void *data, int *x, int *y, int *w, int *h, int *cx, int *cy, int *cw, int *ch)
{
   Render_Engine *re;
   RGBA_Image *surface;
   Tilebuf_Rect *rect;
   int ux, uy, uw, uh;

   re = (Render_Engine *)data;
   if (re->end)
     {
	re->end = 0;
	return NULL;
     }
   if (!re->rects)
     {
	re->rects = evas_common_tilebuf_get_render_rects(re->tb);
	re->cur_rect = EINA_INLIST_GET(re->rects);
     }
   if (!re->cur_rect) return NULL;
   rect = (Tilebuf_Rect *)re->cur_rect;
   ux = rect->x; uy = rect->y; uw = rect->w; uh = rect->h;
   re->cur_rect = re->cur_rect->next;
   if (!re->cur_rect)
     {
	evas_common_tilebuf_free_render_rects(re->rects);
	re->rects = NULL;
	re->end = 1;
     }

   surface = evas_gix_platform_outbuf_new_region_for_update (re->ob, ux, uy, uw, uh, cx, cy, cw, ch);
   *x = ux; *y = uy; *w = uw; *h = uh;
   return surface;
}

static void
eng_gix_output_redraws_next_update_push(void *data, void *surface, int x, int y, int w, int h)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
#if defined(BUILD_PIPE_RENDER) && !defined(EVAS_FRAME_QUEUING)
   evas_common_pipe_map4_begin(surface);
#endif /* BUILD_PIPE_RENDER  && !EVAS_FRAME_QUEUING*/

   evas_gix_platform_outbuf_push_updated_region(re->ob, surface, x, y, w, h);
   evas_gix_platform_outbuf_free_region_for_update(re->ob, surface);
   evas_common_cpu_end_opt();
}


static void
eng_gix_output_flush(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
     {
        evas_gix_platform_outbuf_flush(re->ob);
     }
}

static void
eng_gix_output_idle_flush(void *data)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   evas_gix_platform_outbuf_idle_flush(re->ob);
}

static Eina_Bool
eng_gix_canvas_alpha_get(void *data, void *context __UNUSED__)
{
   Render_Engine *re;

   re = (Render_Engine *)data;
   return (re->ob->priv.destination_alpha) ;
}


/* module advertising code */
static int
module_open(Evas_Module *em)
{
   if (!em) return 0;

   /* get whatever engine module we inherit from */
   if (!_evas_module_engine_inherit(&pfunc, "software_generic")) return 0;
   _evas_engine_gix_log_dom = eina_log_domain_register
     ("evas-gix", EVAS_DEFAULT_LOG_COLOR);
   if (_evas_engine_gix_log_dom < 0)
     {
        EINA_LOG_ERR("Can not create a module log domain.");
        return 0;
     }
   /* store it for later use */
   func = pfunc;
   /* now to override methods */
#define ORD(f) EVAS_API_OVERRIDE(f, &func, eng_gix_)
   ORD(info);
   ORD(info_free);
   ORD(setup);
   ORD(canvas_alpha_get);
   ORD(output_free);
   ORD(output_resize);
   ORD(output_tile_size_set);
   ORD(output_redraws_rect_add);
   ORD(output_redraws_rect_del);
   ORD(output_redraws_clear);
   ORD(output_redraws_next_update_get);
   ORD(output_redraws_next_update_push);
   ORD(output_flush);
   ORD(output_idle_flush);

   em->functions = (void *)(&func);
   return 1;
}

static void
module_close(Evas_Module *em __UNUSED__)
{
  eina_log_domain_unregister(_evas_engine_gix_log_dom);
}

static Evas_Module_Api evas_modapi =
{
   EVAS_MODULE_API_VERSION,
   "gix",
   "gix embedded systems",
   {
     module_open,
     module_close
   }
};

EVAS_MODULE_DEFINE(EVAS_MODULE_TYPE_ENGINE, engine, gix);

#ifndef EVAS_STATIC_BUILD_GIX
EVAS_EINA_MODULE_DEFINE(engine, gix);
#endif

