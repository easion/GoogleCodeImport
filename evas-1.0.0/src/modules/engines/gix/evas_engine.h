#ifndef EVAS_ENGINE_H
#define EVAS_ENGINE_H

#include <sys/ipc.h>
#include <sys/shm.h>


#include <gi/gi.h>
#include <gi/property.h>
#include <gi/region.h>

//typedef void* Colormap;

extern int _evas_engine_gix_log_dom;
#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_gix_log_dom, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_gix_log_dom, __VA_ARGS__)

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_evas_engine_gix_log_dom, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_gix_log_dom, __VA_ARGS__)

#ifdef CRIT
# undef CRIT
#endif
#define CRIT(...) EINA_LOG_DOM_CRIT(_evas_engine_gix_log_dom, __VA_ARGS__)

typedef enum   _Outbuf_Depth    Outbuf_Depth;

enum _Outbuf_Depth
{
   OUTBUF_DEPTH_NONE,
   OUTBUF_DEPTH_INHERIT,
   OUTBUF_DEPTH_RGB_16BPP_565_565_DITHERED,
   OUTBUF_DEPTH_RGB_16BPP_555_555_DITHERED,
   OUTBUF_DEPTH_RGB_16BPP_444_444_DITHERED,
   OUTBUF_DEPTH_RGB_16BPP_565_444_DITHERED,
   OUTBUF_DEPTH_RGB_32BPP_888_8888,
   OUTBUF_DEPTH_LAST
};

typedef struct _Outbuf          Outbuf;

struct _Outbuf
{
   Outbuf_Depth    depth;
   int             w, h;
   int             rot;
   int             onebuf;

   struct {

         struct {
            gi_window_id_t            win;
            gi_gc_ptr_t                gc;
			gi_format_code_t ogformat;
            unsigned char     swap     : 1;
            unsigned char     bit_swap : 1;
         } gix_platform;



      struct {
	 DATA32    r, g, b;
      } mask;

      /* 1 big buffer for updates - flush on idle_flush */
      RGBA_Image  *onebuf;
      Eina_List   *onebuf_regions;

      /* a list of pending regions to write to the target */
      Eina_List   *pending_writes;
      /* a list of previous frame pending regions to write to the target */
      Eina_List   *prev_pending_writes;


      unsigned char destination_alpha : 1;
      unsigned char synced : 1;
   } priv;
};




#endif
