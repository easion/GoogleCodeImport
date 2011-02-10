#ifndef _EVAS_ENGINE_GIX_H
#define _EVAS_ENGINE_GIX_H

#include <gi/gi.h>

typedef struct _Evas_Engine_Info_Gix Evas_Engine_Info_Gix;

struct _Evas_Engine_Info_Gix
{
   Evas_Engine_Info magic;

   struct {     
      unsigned int                          drawable;
      unsigned int                          mask;
      gi_format_code_t                      gformat;
      int                                   rotation;

      // int                                   depth;
     //unsigned int                          shape_dither       : 1;
      //unsigned int                          track_mask_changes : 1;
      //unsigned int                          alloc_grayscale    : 1;
      unsigned int                          debug              : 1;
      unsigned int                          destination_alpha  : 1;
   } info;

};

#endif


