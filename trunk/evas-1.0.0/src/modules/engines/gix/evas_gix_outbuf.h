#ifndef EVAS_GIGix_OUTBUF_H
#define EVAS_GIGix_OUTBUF_H


#include "evas_engine.h"


void         evas_gix_platform_outbuf_init (void);

void         evas_gix_platform_outbuf_free (Outbuf *buf);

Outbuf      *evas_gix_platform_outbuf_setup_x (int          w,
                                                int          h,
                                                int          rot,
                                                Outbuf_Depth depth,                                                
                                                gi_window_id_t     draw,                                              
                                                gi_format_code_t          x_depth,                                                                                             
                                                int          destination_alpha);


RGBA_Image  *evas_gix_platform_outbuf_new_region_for_update (Outbuf *buf,
                                                              int     x,
                                                              int     y,
                                                              int     w,
                                                              int     h,
                                                              int    *cx,
                                                              int    *cy,
                                                              int    *cw,
                                                              int    *ch);

void         evas_gix_platform_outbuf_free_region_for_update (Outbuf     *buf,
                                                               RGBA_Image *update);

void         evas_gix_platform_outbuf_flush (Outbuf *buf);

void         evas_gix_platform_outbuf_idle_flush (Outbuf *buf);

void         evas_gix_platform_outbuf_push_updated_region (Outbuf     *buf,
                                                            RGBA_Image *update,
                                                            int         x,
                                                            int         y,
                                                            int         w,
                                                            int         h);

void         evas_gix_platform_outbuf_reconfigure (Outbuf      *buf,
                                                    int          w,
                                                    int          h,
                                                    int          rot,
                                                    Outbuf_Depth depth);

int          evas_gix_platform_outbuf_get_width (Outbuf *buf);

int          evas_gix_platform_outbuf_get_height (Outbuf *buf);

Outbuf_Depth evas_gix_platform_outbuf_get_depth (Outbuf *buf);

int          evas_gix_platform_outbuf_get_rot (Outbuf *buf);

void         evas_gix_platform_outbuf_drawable_set (Outbuf  *buf,
                                                     gi_window_id_t draw);

void         evas_gix_platform_outbuf_mask_set (Outbuf *buf,
                                                 gi_window_id_t mask);

void         evas_gix_platform_outbuf_rotation_set (Outbuf *buf,
                                                     int     rot);



#endif
