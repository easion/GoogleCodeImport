#ifndef EVAS_GIGix_BUFFER_H
#define EVAS_GIGix_BUFFER_H


#include "evas_engine.h"

#define LSBFirst 0
#define MSBFirst 1

typedef struct _Gix_Output_Buffer Gix_Output_Buffer;

struct _Gix_Output_Buffer
{
   gi_image_t          *xim;
   gi_bool_t need_free;
   void            *data;
   int              w;
   int              h;
   int              bpl;
   int              psize;
};


Gix_Output_Buffer *evas_gix_platform_output_buffer_new (  gi_format_code_t format, int w, int h,  void *data);

void evas_gix_platform_output_buffer_free            (Gix_Output_Buffer *xob, int sync);

void evas_gix_platform_output_buffer_paste           (Gix_Output_Buffer *xob, gi_window_id_t d, gi_gc_ptr_t gc, int x, int y, int sync);

DATA8 *evas_gix_platform_output_buffer_data          (Gix_Output_Buffer *xob, int *bytes_per_line_ret);

int evas_gix_platform_output_buffer_depth            (Gix_Output_Buffer *xob);


#endif
