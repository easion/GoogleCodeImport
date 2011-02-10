#include "evas_common.h"

#include "evas_gix_buffer.h"



Gix_Output_Buffer *
evas_gix_platform_output_buffer_new(  gi_format_code_t format, int w, int h,  void *data)
{
   Gix_Output_Buffer *xob;
   int pitch = 0;

   xob = calloc(1, sizeof(Gix_Output_Buffer));
   if (!xob) return NULL;

   xob->xim = NULL;
   xob->w = w;
   xob->h = h;


   if (format == GI_RENDER_a8 && data)
   {
	   data = bitmap_create_from_data(data, w,h,FALSE,&pitch);
  //printf("%s got line%d format%x | %d,%d %p\n",__FUNCTION__,__LINE__,format,w,h,data);
   }

   xob->need_free = FALSE;

  if (data){
   xob->xim = gi_create_image_with_data( w, h,data, format);
  }
  else{
   xob->need_free = TRUE;
   xob->xim = gi_create_image_depth( w, h, format);
   data = xob->xim->rgba;
  }

   if (!xob->xim)
     {
	free(xob);
  printf("%s got line%d ERROR!!!\n",__FUNCTION__,__LINE__);
	return NULL;
     }

   xob->data = data;
 
   xob->bpl = xob->xim->pitch;
   xob->psize = xob->bpl * xob->h;
   return xob;
}

void
evas_gix_platform_output_buffer_free(Gix_Output_Buffer *xob, int sync)
{
  if (xob->data && !xob->need_free){
	  xob->xim->rgba = NULL; //free?
  }
  gi_destroy_image(xob->xim);
  free(xob);
}

void
evas_gix_platform_output_buffer_paste(Gix_Output_Buffer *xob, gi_window_id_t d, gi_gc_ptr_t gc, int x, int y, int sync)
{
  int err;

  gi_gc_attch_window(gc,d);
  err = gi_bitblt_image(gc,0, 0,xob->w, xob->h, xob->xim,x,y );
  if (err) {
  printf("%s got line%d (gc%x,d%x) (%d,%d,%d,%d)\n",__FUNCTION__,__LINE__, gc,d,xob->w, xob->h,x,y);
	  perror("evas_gix_platform_output_buffer_paste");
  }
}

DATA8 *
evas_gix_platform_output_buffer_data(Gix_Output_Buffer *xob, int *bytes_per_line_ret)
{
   if (bytes_per_line_ret) *bytes_per_line_ret = xob->xim->pitch;
   return (DATA8 *)xob->xim->rgba;
}

int
evas_gix_platform_output_buffer_depth(Gix_Output_Buffer *xob)
{
   return GI_RENDER_FORMAT_BPP(xob->xim->format);
}
