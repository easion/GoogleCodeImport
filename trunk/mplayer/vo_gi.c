

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <gi/gi.h>

#include <gi/region.h>
#include <gi/property.h>
#include <gi/ezxml.h>

#include "mplayer.h"
#include "config.h"
#include "aspect.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "osdep/keycodes.h"
#include "osdep/getch2.h"
#include "libmpcodecs/vf_scale.h"
#include "input/input.h"
#include "input/mouse.h"
#define GI_DOUBLEBUFFER

static gi_window_id_t  window = 0;
static gi_gc_id_t  gc;
static gi_screen_info_t si;
static	gi_image_t *gi_bitmap;

#if 1

#define UNUSED(x) ((void)(x))

static void handler_set (void);
static uint32_t draw_image (mp_image_t *mpi);
static int query_format (uint32_t format);
static void draw_alpha (int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride);

static int image_x;
static int image_y;
static int image_x_full;
static int image_y_full;
static int image_width;
static int image_height;
static int image_width_full;
static int image_height_full;
static int image_aspect;
static int image_aspect_full;
static struct SwsContext *image_sws;
static uint8_t *image_scale_buf;
static int image_fullscreen;
static int image_format;

static vo_info_t info = {
	"Netbas GI Windowing System",
	"gix",
	"Easion <easion@gmail.com>",
	""
};

LIBVO_EXTERN(netbasgi);

static int preinit(const char *arg)
{
	int err;

	err = gi_init();	

	gi_get_screen_info(&si);
	
	if (err<0) {
		printf("%s:%d\n", __FILE__, __LINE__);
		exit(0);
	}	
	
	return 0;
}

static int control (uint32_t request, void *data, ...)
{
	switch (request) {
		case VOCTRL_QUERY_FORMAT:
			return query_format(*((uint32_t*) data));
		case VOCTRL_DRAW_IMAGE:
			return draw_image((mp_image_t *) data);
		case VOCTRL_PAUSE:
			//window->surface->mode = SURFACE_REAL;
			return VO_TRUE;
		case VOCTRL_RESUME:
			//window->surface->mode = SURFACE_VIRTUAL;
			return VO_TRUE;
		case VOCTRL_FULLSCREEN:
			image_fullscreen = (image_fullscreen + 1) % 2;
			if (image_fullscreen) {
				gi_fullscreen_window(window,1);
				//gi_resize_window(window,si.scr_width,si.scr_height);

				//s_window_set_coor(window, WINDOW_NOFORM, 0, 0, si.scr_width, si.scr_height);
				gi_set_window_keep_above(window, 1);
			} else {
				gi_fullscreen_window(window,0);

				//gi_move_window(window,image_x, image_y);
				//gi_resize_window(window,image_width, image_height);
				//s_window_set_coor(window, WINDOW_NOFORM, image_x, image_y, image_width, image_height);
				gi_set_window_keep_above(window, 0);
			}
			//s_fillbox(window->surface, 0, 0, window->surface->width, window->surface->height, 0);
			return VO_TRUE;
		/*
		     VOCTRL_GET_IMAGE
		     VOCTRL_RESET
		     VOCTRL_GUISUPPORT
		     VOCTRL_SET_EQUALIZER
		     VOCTRL_GET_EQUALIZER
		     VOCTRL_ONTOP
		*/
	}

	return VO_NOTIMPL;
}
static int fb_line_len;
static int fb_pixel_size;
static int config (uint32_t width, uint32_t height, uint32_t d_width,
                   uint32_t d_height, uint32_t flags, char *title,
                   uint32_t format)
{
	int x;
	int y;
	int req_w = (int) width;
	int req_h = (int) height;
	int req_bpp;
		int err;

	if (!IMGFMT_IS_RGB(format) && !IMGFMT_IS_BGR(format)) {
	assert(0);
	printf("gi rgb config error\n");
	return -1;
	}
	req_bpp = IMGFMT_BGR_DEPTH(format);

	if (vo_dbpp != 0 && vo_dbpp != req_bpp) {
	assert(0);
	printf("gi bpp config error\n");
	return -1;
	}

	if (GI_RENDER_FORMAT_BPP(si.format) != req_bpp) {
	exit(0);
	}

	image_width = req_w;
	image_height = req_h;

	image_fullscreen = 0;
	image_format = format;

	image_aspect = ((1 << 16) * image_width + image_height / 2) / image_height;
	image_width_full = si.scr_width;
	image_height_full = si.scr_height;

	image_aspect_full = (image_width_full * (1 << 16) + (image_height_full >> 1)) / image_height_full;
	if (image_aspect_full > image_aspect) {
	image_width_full = (image_height_full * image_aspect + (1 << 15)) >> 16;
	} else {
	image_height_full = ((image_width_full << 16) + (image_aspect >> 1)) / image_aspect;
	}

	image_x_full = (si.scr_width - image_width_full) / 2;
	image_y_full = (si.scr_height - image_height_full) / 2;

	image_scale_buf = (uint8_t *) malloc(image_width_full * image_height_full * (GI_RENDER_FORMAT_BPP(si.format)+7/8));
	image_sws = sws_getContextFromCmdLine(image_width, image_height, image_format, image_width_full, image_height_full, image_format);

	//if (vo_config_count <= 0) {
	char titlebuf[128];

	uint32_t wflags =	(GI_MASK_BUTTON_DOWN | GI_MASK_KEY_DOWN |
	GI_MASK_CONFIGURENOTIFY | GI_MASK_KEY_UP|GI_MASK_MOUSE_MOVE| GI_MASK_BUTTON_UP|GI_MASK_CLIENT_MSG
	| GI_MASK_EXPOSURE);

	handler_set();

	x = (si.scr_width - (int) image_width) / 2;
	if (x <= 5) x = 10;
	y = (si.scr_height - (int) image_height) / 2;
	if (y <= 5) y = 10;
	image_x = x;
	image_y = y;

	if (WinID > 0){
		gi_window_info_t winfo;
		gi_window_id_t *child = 0;
		gi_window_id_t parent;
        unsigned int n_children;

		window = WinID;

		gi_query_tree( WinID,  &parent, &child, &n_children);
        if (n_children)
            free(child);

		//window = parent;

		err = gi_get_window_info(window, &winfo);
		if (err < 0)
		{
			goto local_win;
		}
	}
	else{
local_win:
		window = gi_create_window(GI_DESKTOP_WINDOW_ID, x,y,image_width, 
			image_height,GI_RGB( 192, 192, 192 ),GI_FLAGS_TOPMOST_WINDOW);

		if (window<0)
		{
			printf("Create  MPlayer window error\n");
			return -1;
		}

		

		snprintf(titlebuf,sizeof(titlebuf),"MPlayer-1.0rc2 - %s", filename);	
		gi_set_window_utf8_caption(window,titlebuf);
	}

	gi_set_events_mask(window,  wflags);
	gc = gi_create_gc((window),NULL);
	gi_show_window(window);

	gi_bitmap = gi_create_image_depth(image_width, image_height,si.format);
	fb_pixel_size=GI_RENDER_FORMAT_BPP(si.format)/8;
	fb_line_len = image_width*fb_pixel_size;

	if (!gi_bitmap)
	{
		gi_exit();
		return 1;
	}
	//}
	return 0;
}



static int draw_slice(uint8_t *src[], int stride[], int w, int h, int x,
		int y)
{
	uint8_t *d;
	uint8_t *s;

	d = (char*)gi_bitmap->rgba + fb_line_len * y + fb_pixel_size * x;

	s = src[0];
	while (h) {
		fast_memcpy(d, s, w * fb_pixel_size);
		d += fb_line_len;
		s += stride[0];
		h--;
	}

	return 0;
}



static int draw_frame (uint8_t *src[])
{
	assert(0);
	UNUSED(src);
	return VO_ERROR;
}

static void draw_osd (void)
{
	vo_draw_text(image_width, image_height, draw_alpha);
}

static void flip_page (void)
{
	gi_draw_image(gc,gi_bitmap,0,0);
}



extern int enable_mouse_movements;
static void check_events (void)
{
	gi_msg_t event;
	struct graphics_device *gd;
	int replay_event = 0;
	int ret = 0;
	int x,y;
	int k=0;

	//printf("event begin\n");
	do
	{
	replay_event = gi_get_message (&event,MSG_NO_WAIT);
	if (replay_event > 0)
	{
		x=event.body.rect.x;
		y=event.body.rect.y;

		switch (event.type)
		{	
            case GI_MSG_MOUSE_MOVE :
            {   
				if (enable_mouse_movements) {
                char cmd_str[40];
                snprintf(cmd_str, sizeof(cmd_str), "set_mouse_pos %i %i",
                        x, y);
                mp_input_queue_cmd(mp_input_parse_cmd(cmd_str));
               }
                break ;
            }

            case GI_MSG_BUTTON_DOWN :
            {
				int button = event.params[2] ;
				switch (button) {
                    case GI_BUTTON_L :
						if (!vo_nomouse_input)
						mplayer_put_key(MOUSE_BTN0);						
                        break ;
                    case GI_BUTTON_M :
						if (!vo_nomouse_input)
						 mplayer_put_key(MOUSE_BTN1);
                        break ;
                    case GI_BUTTON_R :
						if (!vo_nomouse_input)
						 mplayer_put_key(MOUSE_BTN2);
                        break ;
                    default :
						break;
                }
				 //mplayer_put_key((MOUSE_BTN0 + Event.xbutton.button -
                  //               1) | MP_KEY_DOWN);
                
                break ;
            }

			case GI_MSG_CONFIGURENOTIFY:
				break;

            // do not konw which button is released
            case GI_MSG_BUTTON_UP :
            {   
                //dbg_printf ("button up\n") ; 
				//mplayer_put_key(MOUSE_BTN0 + Event.xbutton.button - 1);
                break ;
            }

            case GI_MSG_KEY_DOWN :
            {
				int wParam=event.params[4];
				//vo_x11_putkey(key);

				switch (wParam) {
                case G_KEY_LEFT:    mplayer_put_key(KEY_LEFT);      break;
                case G_KEY_UP:      mplayer_put_key(KEY_UP);        break;
                case G_KEY_RIGHT:   mplayer_put_key(KEY_RIGHT);     break;
                case G_KEY_DOWN:    mplayer_put_key(KEY_DOWN);      break;
                case G_KEY_TAB:     mplayer_put_key(KEY_TAB);       break;
                //case G_KEY_CONTROL: mplayer_put_key(KEY_CTRL);      break;
                //case G_KEY_BACK:    mplayer_put_key(KEY_BS);        break;
                case G_KEY_DELETE:  mplayer_put_key(KEY_DELETE);    break;
                case G_KEY_INSERT:  mplayer_put_key(KEY_INSERT);    break;
                case G_KEY_HOME:    mplayer_put_key(KEY_HOME);      break;
                case G_KEY_END:     mplayer_put_key(KEY_END);       break;
                //case G_KEY_PRIOR:   mplayer_put_key(KEY_PAGE_UP);   break;
                //case G_KEY_NEXT:    mplayer_put_key(KEY_PAGE_DOWN); break;
                case G_KEY_ESCAPE:  mplayer_put_key(KEY_ESC);       break;
				default:
				mplayer_put_key(wParam );
					break;
            }
            break;

				

                break ;
            }

		case GI_MSG_KEY_UP :
		{				
		break ;
		}

		case GI_MSG_CLIENT_MSG:
		if(event.body.client.client_type == GA_WM_PROTOCOLS
		&&event.params[0] == GA_WM_DELETE_WINDOW){

		mplayer_put_key(KEY_CLOSE_WIN);
		}
		break;

		case GI_MSG_EXPOSURE :
		{	
		ret |= VO_EVENT_EXPOSE;
		printf("VO_EVENT_EXPOSE got\n");

		//vo_x11_clearwindow_part(mDisplay, vo_window, myximage->width,
		//               myximage->height, 0);
		flip_page();

		break ;
		}


		}
	}
	}
	while (replay_event>0);

	//printf("event done\n");



}

static void uninit (void)
{	
	gi_exit();
}

/*****************************************************************************/

static int query_format (uint32_t format)
{
	int32_t req_bpp;
	int32_t flags;

	// only RGB modes supported
	if((!IMGFMT_IS_RGB(format)) && (!IMGFMT_IS_BGR(format)))
		return 0;
	// Reject different endian
#ifdef WORDS_BIGENDIAN
	if (IMGFMT_IS_BGR(format))
		return 0;
#else
	if (IMGFMT_IS_RGB(format))
		return 0;
#endif
	if ((format == IMGFMT_BGR4) || (format == IMGFMT_RGB4))
		return 0;
	req_bpp = IMGFMT_RGB_DEPTH(format);
	if ((vo_dbpp > 0) && (vo_dbpp != req_bpp))
		return 0;

	if (req_bpp == GI_RENDER_FORMAT_BPP(si.format)) {
		flags = VFCAP_CSP_SUPPORTED |
			VFCAP_CSP_SUPPORTED_BY_HW |
			VFCAP_ACCEPT_STRIDE |
			VFCAP_SWSCALE;
		if (req_bpp > 8)
			flags |= VFCAP_OSD;
		return flags;
	}

        return 0;
}

static void draw_alpha (int x0, int y0, int w, int h, unsigned char *src, unsigned char *srca, int stride)
{
        int bpp; 
	char *base;
	int image_w;
	gi_window_info_t winfo;

	gi_get_window_info(window, &winfo);

	if (image_fullscreen == 1) {
		x0 += image_x_full;
		y0 += image_y_full;
	}

	image_w = winfo.width;
	bpp = (GI_RENDER_FORMAT_BPP(si.format)+7/8);
	base = (char*)gi_bitmap->rgba + ((y0 * image_w) + x0) * bpp;

	switch (GI_RENDER_FORMAT_BPP(si.format)) {
		case 32:
			vo_draw_alpha_rgb32(w, h, src, srca, stride, base, image_w * bpp);
			break;
		case 24:
			vo_draw_alpha_rgb24(w, h, src, srca, stride, base, image_w * bpp);
			break;
		case 16:
			vo_draw_alpha_rgb16(w, h, src, srca, stride, base, image_w * bpp);
			break;
		case 15:
			vo_draw_alpha_rgb15(w, h, src, srca, stride, base, image_w * bpp);
			break;
	}


}

static uint32_t draw_image (mp_image_t *mpi)
{
	int x;
	int y;
	int w;
	int h;
	uint8_t *rgbplane;
	gi_image_t *img;	
	
	if (mpi->flags & MP_IMGFLAG_DIRECT) {
		assert((uint32_t) mpi->priv >= 0);
		return VO_TRUE; // it's already done
	}

	if (image_fullscreen == 1) {
		printf("draw_image error\n");
		return VO_FALSE;
	}

	x = mpi->x;
	y = mpi->y;
	w = mpi->w;
	h = mpi->h;
	rgbplane = mpi->planes[0] + y * mpi->stride[0] + (x * mpi->bpp) / 8;

	if (w == gi_bitmap->w && h==gi_bitmap->h)
	{
		memcpy(gi_bitmap->rgba,rgbplane,w*h*mpi->bpp/8);
		//fast_memcpy(gi_bitmap->rgba,rgbplane,w*h*mpi->bpp/8);
	}
	else{

		img = gi_create_image_with_data(w, h,rgbplane,gi_bitmap->format);
		if (!img)
		{
			return;
		}

		gi_image_copy_area(gi_bitmap,img,x, y, 0,0,w, h);

		gi_release_bitmap_struct(img);
	}


	return VO_TRUE;
}

static void atexit_handler ()
{
	//if (running) {
		exit_player(NULL);
	//}
}

#if 0
static void keybd_handler_left		() { mplayer_put_key(KEY_LEFT); }	/* seek +10		*/
static void keybd_handler_right		() { mplayer_put_key(KEY_RIGHT); }	/* seek -10		*/
static void keybd_handler_down		() { mplayer_put_key(KEY_DOWN); }	/* seek +60		*/
static void keybd_handler_up		() { mplayer_put_key(KEY_UP); }	/* seek -60		*/
static void keybd_handler_pagedown	() { mplayer_put_key(KEY_PAGE_DOWN); }	/* seek +600	*/
static void keybd_handler_pageup	() { mplayer_put_key(KEY_PAGE_UP); }/* seek -600		*/
static void keybd_handler_minus		() { mplayer_put_key('-'); }	/* audio delay +1.000	*/
static void keybd_handler_add		() { mplayer_put_key('+'); }	/* audio delay -1.000	*/
static void keybd_handler_q		() { mplayer_put_key('q'); }	/* quit			*/
static void keybd_handler_esc		() { mplayer_put_key(KEY_ESC); }	/* quit			*/
static void keybd_handler_p		() { mplayer_put_key(' '); }	/* pause		*/
static void keybd_handler_space		() { mplayer_put_key(' '); }	/* pause		*/
static void keybd_handler_o		() { mplayer_put_key('o'); }	/* osd			*/
static void keybd_handler_z		() { mplayer_put_key('z'); }	/* sub delay -0.1	*/
static void keybd_handler_x		() { mplayer_put_key('x'); }	/* sub delay +0.1	*/
static void keybd_handler_g		() { mplayer_put_key('g'); }	/* sub step -1		*/
static void keybd_handler_y		() { mplayer_put_key('y'); }	/* sub step +1		*/
static void keybd_handler_9		() { mplayer_put_key('9'); }	/* volume -1		*/
static void keybd_handler_divide	() { mplayer_put_key('/'); }	/* volume -1		*/
static void keybd_handler_0		() { mplayer_put_key('0'); }	/* volume +1		*/
static void keybd_handler_multiply	() { mplayer_put_key('*'); }	/* volume +1		*/
static void keybd_handler_m		() { mplayer_put_key('m'); }	/* mute			*/
static void keybd_handler_1		() { mplayer_put_key('1'); }	/* contrast -1		*/
static void keybd_handler_2		() { mplayer_put_key('2'); }	/* contrast +1		*/
static void keybd_handler_3		() { mplayer_put_key('3'); }	/* brightness -1	*/
static void keybd_handler_4		() { mplayer_put_key('4'); }	/* brightness +1	*/
static void keybd_handler_5		() { mplayer_put_key('5'); }	/* hue -1		*/
static void keybd_handler_6		() { mplayer_put_key('6'); }	/* hue +1		*/
static void keybd_handler_7		() { mplayer_put_key('7'); }	/* saturation -1	*/
static void keybd_handler_8		() { mplayer_put_key('8'); }	/* saturation +1	*/
static void keybd_handler_d		() { mplayer_put_key('d'); }	/* frame drop		*/
static void keybd_handler_r		() { mplayer_put_key('r'); }	/* sub pos -1		*/
static void keybd_handler_t		() { mplayer_put_key('t'); }	/* sub pos +1		*/
static void keybd_handler_a		() { mplayer_put_key('a'); }	/* sub alignment	*/
static void keybd_handler_v		() { mplayer_put_key('v'); }	/* sub visibility	*/
static void keybd_handler_j		() { mplayer_put_key('j'); }	/* vobsub lang		*/
static void keybd_handler_s		() { mplayer_put_key('s'); }	/* screen shot		*/
static void keybd_handler_w		() { mplayer_put_key('w'); }	/* panscan -0.1		*/
static void keybd_handler_e		() { mplayer_put_key('e'); }	/* panscan +0.1		*/
static void keybd_handler_f		() { mplayer_put_key('f'); }	/* vo fullscreen	*/

#define keybd_handler_set_(a, b)\
	{\
		s_handler_init(&handler);\
		handler->type = KEYBD_HANDLER;\
		handler->keybd.button = a;\
		handler->keybd.p = b;\
		s_handler_add(window, handler);\
	}

#endif

static void keybd_handler_set (void)
{
#if 0
	s_handler_t *handler;
	
	keybd_handler_set_(G_KEY_RIGHT, keybd_handler_right);
	keybd_handler_set_(G_KEY_LEFT, keybd_handler_left);
	keybd_handler_set_(G_KEY_UP, keybd_handler_up);
	keybd_handler_set_(G_KEY_DOWN, keybd_handler_down);
	keybd_handler_set_(G_KEY_PAGEUP, keybd_handler_pageup);
	keybd_handler_set_(G_KEY_PAGEDOWN, keybd_handler_pagedown);
	keybd_handler_set_(G_KEY_KP_SUBTRACT, keybd_handler_minus);
	keybd_handler_set_(G_KEY_KP_ADD, keybd_handler_add);
	keybd_handler_set_(G_KEY_q, keybd_handler_q);
	keybd_handler_set_(G_KEY_ESCAPE, keybd_handler_esc);
	keybd_handler_set_(G_KEY_p, keybd_handler_p);
	keybd_handler_set_(G_KEY_SPACE, keybd_handler_space);
	keybd_handler_set_(G_KEY_o, keybd_handler_o);
	keybd_handler_set_(G_KEY_z, keybd_handler_z);
	keybd_handler_set_(G_KEY_x, keybd_handler_x);
	keybd_handler_set_(G_KEY_g, keybd_handler_g);
	keybd_handler_set_(G_KEY_y, keybd_handler_y);
	keybd_handler_set_(G_KEY_NINE, keybd_handler_9);
	keybd_handler_set_(G_KEY_KP_DIVIDE, keybd_handler_divide);
	keybd_handler_set_(G_KEY_ZERO, keybd_handler_0);
	keybd_handler_set_(G_KEY_KP_MULTIPLY, keybd_handler_multiply);
	keybd_handler_set_(G_KEY_m, keybd_handler_m);
	keybd_handler_set_(G_KEY_ONE, keybd_handler_1);
	keybd_handler_set_(G_KEY_TWO, keybd_handler_2);
	keybd_handler_set_(G_KEY_THREE, keybd_handler_3);
	keybd_handler_set_(G_KEY_FOUR, keybd_handler_4);
	keybd_handler_set_(G_KEY_FIVE, keybd_handler_5);
	keybd_handler_set_(G_KEY_SIX, keybd_handler_6);
	keybd_handler_set_(G_KEY_SEVEN, keybd_handler_7);
	keybd_handler_set_(G_KEY_EIGHT, keybd_handler_8);
	keybd_handler_set_(G_KEY_d, keybd_handler_d);
	keybd_handler_set_(G_KEY_r, keybd_handler_r);
	keybd_handler_set_(G_KEY_t, keybd_handler_t);
	keybd_handler_set_(G_KEY_a, keybd_handler_a);
	keybd_handler_set_(G_KEY_v, keybd_handler_v);
	keybd_handler_set_(G_KEY_j, keybd_handler_j);
	keybd_handler_set_(G_KEY_f, keybd_handler_f);
	keybd_handler_set_(G_KEY_s, keybd_handler_s);
	keybd_handler_set_(G_KEY_w, keybd_handler_w);
	keybd_handler_set_(G_KEY_e, keybd_handler_e);
#endif
}
static void handler_set (void)
{
	keybd_handler_set();
}
#endif
