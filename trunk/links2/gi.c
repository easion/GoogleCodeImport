

#include "cfg.h"

#ifdef NCE_WINDOWS

#include "links.h"
#include "bits.h"

#include <ctype.h>
#include <gi/gi.h>
#include <gi/property.h>
//#define DEBUGF printf
#define DEBUGF

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

extern struct graphics_driver xynth_driver;


typedef struct lngi_device_s {
	char *title;
	gi_cliprect_t update;
	gi_gc_ptr_t gc;
	gi_image_t *img;
} lngi_device_t;



typedef struct lngi_gd_s {
	struct graphics_device *active;
} lngi_gd_t;

typedef struct lngi_root_s {	
	int window;
	lngi_gd_t *gd;
	gi_image_t *dummy_img;
} lngi_root_t;

static lngi_root_t *lngi_root = NULL;
static void lngi_set_title (struct graphics_device *dev, unsigned char *title);

static void lngi_surface_register_update (void *dev)
{
	lngi_device_t *wd;

    if (lngi_root->gd->active != dev) {
		return;
	}
	wd = (lngi_device_t *) ((struct graphics_device *) dev)->driver_data;
	gi_bitblt_image( wd->gc,wd->update.x, wd->update.y, wd->update.w, wd->update.h,wd->img,wd->update.x, wd->update.y);

	wd->update = (gi_cliprect_t) {-1, -1, -1, -1};
}


static void gi_process_events(void *data)
{
	gi_msg_t event;
	struct graphics_device *gd;
	int replay_event = 0;
	int flag = 0;
	int x,y;
	int k=0;

	do
	{
	replay_event=gi_get_message (&event,MSG_NO_WAIT);
	if (replay_event>0)
	{
		x=event.body.rect.x;
		y=event.body.rect.y;
		switch (event.type)
		{
	case GI_MSG_MOUSE_ENTER :
            {
                DEBUGF ("mouse enter\n") ;
                break ;
            }

            case GI_MSG_MOUSE_EXIT :
            {
                DEBUGF ("mouse exit\n") ;
                break ;
            }

                case GI_MSG_MOUSE_MOVE :
            {     
				flag |= B_MOVE;
				lngi_root->gd->active->mouse_handler(lngi_root->gd->active, x, y, flag);
                break ;
            }			

            case GI_MSG_BUTTON_DOWN :
            {
                int button = event.params[2] ;

				gi_set_focus_window(lngi_root->window);
                
                DEBUGF ("button down\n") ;

                switch (button) {
                    case GI_BUTTON_L :
						flag = B_LEFT; 
					flag |= B_DOWN;
                        break ;
                    case GI_BUTTON_M :
						flag = B_MIDDLE;
					flag |= B_DOWN;
                        break ;
                    case GI_BUTTON_R :
						flag = B_RIGHT;
					flag |= B_DOWN;
                        break ;
					case GI_BUTTON_WHEEL_UP:
						flag =  B_WHEELUP;
						flag |= B_MOVE;
						break;
					case GI_BUTTON_WHEEL_DOWN:
						flag =  B_WHEELDOWN;
						flag |= B_MOVE;
						break;
					case GI_BUTTON_WHEEL_LEFT:
						flag =  B_WHEELLEFT;
						flag |= B_MOVE;
						break;
					case GI_BUTTON_WHEEL_RIGHT:
						flag =  B_WHEELRIGHT;
						flag |= B_MOVE;
						break;
                    default:
						break;
                }

					
					lngi_root->gd->active->mouse_handler(lngi_root->gd->active, x, y, flag);
               
                break ;
            }

            // do not konw which button is released
            case GI_MSG_BUTTON_UP :
            {   
				flag |= B_UP;
				lngi_root->gd->active->mouse_handler(lngi_root->gd->active, x, y, flag);
                DEBUGF ("button up\n") ;                
                break ;
            }

            case GI_MSG_KEY_DOWN :
            {
				long mod=event.body.message[3];

				switch (event.params[3]) {
		case G_KEY_ENTER:	k = KBD_ENTER;	break;
		case G_KEY_RETURN:	k = KBD_ENTER;	break;
		case G_KEY_DELETE:	k = KBD_BS;	break;
		case G_KEY_TAB:	k = KBD_TAB;	break;
		case G_KEY_ESCAPE:	k = KBD_ESC;	break;
		case G_KEY_UP:	k = KBD_UP;	break;
		case G_KEY_DOWN:	k = KBD_DOWN;	break;
		case G_KEY_LEFT:	k = KBD_LEFT;	break;
		case G_KEY_RIGHT:	k = KBD_RIGHT;	break;
		case G_KEY_INSERT:	k = KBD_INS;	break;
		//case G_KEY_REMOVE:	k = KBD_DEL;	break;
		case G_KEY_HOME:	k = KBD_HOME;	break;
		case G_KEY_END:	k = KBD_END;	break;
		case G_KEY_PAGEUP:	k = KBD_PAGE_UP;break;
		case G_KEY_PAGEDOWN:k = KBD_PAGE_DOWN;break;
		case G_KEY_F1:	k = KBD_F1;	break;
		case G_KEY_F2:	k = KBD_F2;	break;
		case G_KEY_F3:	k = KBD_F3;	break;
		case G_KEY_F4:	k = KBD_F4;	break;
		case G_KEY_F5:	k = KBD_F5;	break;
		case G_KEY_F6:	k = KBD_F6;	break;
		case G_KEY_F7:	k = KBD_F7;	break;
		case G_KEY_F8:	k = KBD_F8;	break;
		case G_KEY_F9:	k = KBD_F9;	break;
		case G_KEY_F10:	k = KBD_F10;	break;
		case G_KEY_F11:	k = KBD_F11;	break;
		case G_KEY_F12:	k = KBD_F12;	break;
		default:
			k = event.params[3];
			if (!isprint(k)) {
				//return;
			}
			break;
	}

	if (mod & (G_MODIFIERS_CTRL)) {
		flag |= KBD_CTRL;
	}
	if (mod & (G_MODIFIERS_SHIFT)) {
	}
	if (mod & (G_MODIFIERS_ALT)) {
		flag |= KBD_ALT;
	}


				lngi_root->gd->active->keyboard_handler(lngi_root->gd->active, k, flag);
                break ;
            }

            case GI_MSG_KEY_UP :
            {
				
                break ;
            }

            case GI_MSG_WINDOW_DESTROY :
            {
                DEBUGF ("close require\n") ;
                break ;
            }

			case GI_MSG_CLIENT_MSG:
            {

				printf("links GI_MSG_CLIENT_MSG got exit message\n");

			if(event.body.client.client_type== GA_WM_PROTOCOLS
					&&event.params[0] == GA_WM_DELETE_WINDOW){
				
                lngi_root->gd->active->keyboard_handler(lngi_root->gd->active, KBD_CLOSE, 0);
				break;
				}
			}

            case GI_MSG_EXPOSURE :
            {				

				struct rect r;
				r.x1 = event.body.rect.x;
				r.y1 = event.body.rect.y;
				r.x2 = event.body.rect.x+event.body.rect.w;
				r.y2 = event.body.rect.y+event.body.rect.h;

				lngi_root->gd->active->redraw_handler(lngi_root->gd->active, &r);
                
                break ;
            }

			case GI_MSG_CONFIGURENOTIFY:
				if (lngi_root->gd->active->size.x2==event.body.rect.w&&lngi_root->gd->active->size.y2==event.body.rect.h)break;

			lngi_root->gd->active->size.x2=event.body.rect.w;
			lngi_root->gd->active->size.y2=event.body.rect.h;

			lngi_root->gd->active->resize_handler(lngi_root->gd->active);

				break;

		
		}
	}
	}
	while (replay_event>0);


}

static void lngi_surface_update (struct graphics_device *dev, int x, int y, int w, int h)
{
	gi_cliprect_t rect;
	lngi_device_t *wd;
	wd = (lngi_device_t *) dev->driver_data;
	rect = wd->update;
	if (rect.x >= 0) { wd->update.x = MIN(rect.x, x); } else { wd->update.x = x; }
	if (rect.y >= 0) { wd->update.y = MIN(rect.y, y); } else { wd->update.y = y; }
	if (rect.w >= 0) { wd->update.w = MAX(rect.x + rect.w, x + w) - wd->update.x; } else { wd->update.w = w; }
	if (rect.h >= 0) { wd->update.h = MAX(rect.y + rect.h, y + h) - wd->update.y; } else { wd->update.h = h; }
	register_bottom_half(lngi_surface_register_update, dev);
}


static unsigned char * lngi_init_driver (unsigned char *param, unsigned char *display)
{
 		gi_screen_info_t si ;
       int bpp;
        int Bpp;
		uint32_t flags =	(GI_MASK_EXPOSURE       |
            GI_MASK_BUTTON_DOWN  | GI_MASK_BUTTON_UP  |    GI_MASK_CONFIGURENOTIFY       |
            GI_MASK_KEY_DOWN     | GI_MASK_KEY_UP     |            
               GI_MASK_MOUSE_MOVE     |   GI_MASK_WINDOW_DESTROY|GI_MASK_CLIENT_MSG);


	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	lngi_root = (lngi_root_t *) malloc(sizeof(lngi_root_t));
	lngi_root->gd = (lngi_gd_t *) malloc(sizeof(lngi_gd_t));
	lngi_root->gd->active = NULL;

	gi_init ();
	gi_get_screen_info (& si) ;
	bpp=GI_RENDER_FORMAT_BPP(si.format);
		Bpp=bpp/8;

		lngi_root->dummy_img = gi_create_image_depth(1,1,si.format);

		set_handlers(gi_get_connection_fd(),gi_process_events,0,0,0);

	lngi_root->window =gi_create_window(GI_DESKTOP_WINDOW_ID, 10,0,si.scr_width-10, si.scr_height-30,   get_winxp_color(), 0);
	 gi_set_events_mask (lngi_root->window, flags) ;
	 if (!lngi_root->window)
	{
		printf("Test window error\n");
		return -1;
	}
	
	

	gi_set_window_caption(lngi_root->window,"Links");
	 gi_show_window(lngi_root->window);


		

        if (bpp ==  32) {
		bpp = 24;
	}
	xynth_driver.depth = (bpp << 3) | Bpp;

	register_bottom_half(gi_process_events, NULL);

	return NULL;
}

static struct graphics_device * lngi_init_device (void)
{
	gi_window_info_t winfo;
	lngi_device_t *wd;
	struct graphics_device *gd;

	gi_get_window_info(lngi_root->window, &winfo);


	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	wd = (lngi_device_t *) malloc(sizeof(lngi_device_t));
	wd->update = (gi_cliprect_t) {-1, -1, -1, -1};
	gd = (struct graphics_device *) malloc(sizeof(struct graphics_device));

	wd->title = NULL;
	

	wd->gc = gi_create_gc(lngi_root->window,NULL);

	wd->img =  gi_create_image_depth(winfo.width, winfo.height,gi_screen_format());

	if (!wd->img)
	{
		gi_exit();
		exit(1);
	}

	gd->size.x1 = 0;
	gd->size.x2 = wd->img->w;
	gd->size.y1 = 0;
	gd->size.y2 = wd->img->h;
	gd->clip.x1 = 0;
	gd->clip.x2 = gd->size.x2;
	gd->clip.y1 = 0;
	gd->clip.y2 = gd->size.y2;

	gd->drv = &xynth_driver;
	gd->driver_data = wd;
	gd->user_data = NULL;

	lngi_root->gd->active = gd;
	register_bottom_half(gi_process_events, NULL);

	return gd;
}

static void lngi_shutdown_device (struct graphics_device *dev)
{
        int acc = 0;
        int pos = 0;
	lngi_device_t *wd;
	struct graphics_device *gd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	
	unregister_bottom_half(lngi_surface_register_update, dev);
	wd = (lngi_device_t *) dev->driver_data;
	free(wd->title);
	gi_destroy_image(wd->img);
	free(wd);
	free(dev);
	

	register_bottom_half(gi_process_events, NULL);
}

static void lngi_shutdown_driver (void)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	free(lngi_root->gd);

	free(lngi_root);
}

static unsigned char * lngi_driver_param (void)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static int lngi_get_empty_bitmap (struct bitmap *dest)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	dest->data = (char *) malloc(dest->x * dest->y * gi_screen_depth()/8);
	dest->skip = dest->x * (gi_screen_depth()/8);
	dest->flags = 0;

	return 0;
}

static void lngi_register_bitmap (struct bitmap *bmp)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
}

static void * lngi_prepare_strip (struct bitmap *bmp, int top, int lines)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lngi_commit_strip (struct bitmap *bmp, int top, int lines)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lngi_unregister_bitmap (struct bitmap *bmp)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	free(bmp->data);
}

#define	CLIP_PREFACE \
	if ((x >= dev->clip.x2) || ((x + xs) <= dev->clip.x1)) return;\
	if ((y >= dev->clip.y2) || ((y + ys) <= dev->clip.y1)) return;\
	if ((x + xs) > dev->clip.x2) xs = dev->clip.x2 - x;\
	if ((y + ys) > dev->clip.y2) ys = dev->clip.y2 - y;\
	if ((dev->clip.x1 - x) > 0){\
		xs -= (dev->clip.x1 - x);\
		data += (gi_screen_depth()/8) * (dev->clip.x1 - x);\
		x = dev->clip.x1;\
	}\
	if ((dev->clip.y1 - y) > 0) {\
		ys -= (dev->clip.y1 - y);\
		data += hndl->skip * (dev->clip.y1 - y);\
		y = dev->clip.y1;\
	}

static void lngi_draw_bitmap (struct graphics_device *dev, struct bitmap *hndl, int x, int y)
{
	lngi_device_t *wd;
	int xs = hndl->x;
	int ys = hndl->y;
	char *data = hndl->data;
	gi_image_t *img;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	CLIP_PREFACE;

       // if (lngi_root->running == 0) return;
        //s_thread_mutex_lock(lngi_root->gd->mut);
        if (lngi_root->gd->active != dev) {
		//s_thread_mutex_unlock(lngi_root->gd->mut);
		return;
	}

	wd = (lngi_device_t *) dev->driver_data;

	//gi_bitblt_bitmap( wd->gc,x, y, xs, ys,wd->img,hndl->x, hndl->y);

	img = gi_create_image_with_data(hndl->x, hndl->y,data,wd->img->format);
	if (!img)
	{
		return;
	}

	gi_image_copy_area(wd->img,img,x, y, 0,0,xs, ys);

	gi_release_bitmap_struct(img);

	lngi_surface_update(dev, x, y, xs, ys);
}

static void lngi_draw_bitmaps (struct graphics_device *dev, struct bitmap **hndls, int n, int x, int y)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static long lngi_get_color (int rgb)
{
	lngi_device_t *wd;
	lngi_gd_t *gd;
	long c;

	gd = lngi_root->gd->active ;
	wd = (lngi_device_t *) ((struct graphics_device *) gd)->driver_data;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	c = gi_surface_rgb_color(&lngi_root->dummy_img->s, (rgb >> 0x10) & 0xFF,
	                                               (rgb >> 0x08) & 0xFF,
	                                               (rgb << 0x00) & 0xFF);

	return c;
}

#define FILL_CLIP_PREFACE \
	if ((left >= right) || (top >= bottom)) return;\
	if ((left >= dev->clip.x2) || (right <= dev->clip.x1) || (top >= dev->clip.y2) || (bottom <= dev->clip.y1)) return;\
	if (left < dev->clip.x1) left = dev->clip.x1;\
	if (right > dev->clip.x2) right = dev->clip.x2;\
	if (top < dev->clip.y1) top = dev->clip.y1;\
	if (bottom > dev->clip.y2) bottom = dev->clip.y2;

static void lngi_fill_area (struct graphics_device *dev, int left, int top, int right, int bottom, long color)
{
	lngi_device_t *wd;


	FILL_CLIP_PREFACE;

        if (lngi_root->gd->active != dev) {
		return;
	}

	wd = (lngi_device_t *) dev->driver_data;
	gi_image_fill_rectangle(wd->img, left, top, right - left, bottom - top, color);
	lngi_surface_update(dev, left, top, right - left, bottom - top);
}

#define HLINE_CLIP_PREFACE \
	if (left >= right) return;\
	if ((y < dev->clip.y1) || (y >= dev->clip.y2) || (right <= dev->clip.x1) || (left >= dev->clip.x2)) return;\
	if (left < dev->clip.x1) left = dev->clip.x1;\
	if (right > dev->clip.x2) right = dev->clip.x2;

static void lngi_draw_hline (struct graphics_device *dev, int left, int y, int right, long color)
{
	lngi_device_t *wd;

	HLINE_CLIP_PREFACE;

      
    if (lngi_root->gd->active != dev) {
		return;
	}

	wd = (lngi_device_t *) dev->driver_data;
	if (left>right)
	{
		int tmp;
		tmp = left;
		left = right;
		right=tmp;
	}
	gi_image_fill_span(wd->img, left, y, right-left+1, color);

	lngi_surface_update(dev, left, y, right - left, 1);
}

#define VLINE_CLIP_PREFACE \
	if (top >= bottom) return;\
	if ((x < dev->clip.x1) || (x >= dev->clip.x2) || (top >= dev->clip.y2) || (bottom <= dev->clip.y1)) return;\
	if (top < dev->clip.y1) top = dev->clip.y1;\
	if (bottom > dev->clip.y2) bottom = dev->clip.y2;

static void lngi_draw_vline (struct graphics_device *dev, int x, int top, int bottom, long color)
{
	lngi_device_t *wd;

	VLINE_CLIP_PREFACE;

    if (lngi_root->gd->active != dev) {
		return;
	}

	wd = (lngi_device_t *) dev->driver_data;

	if (top>bottom)
	{
		int tmp;
		tmp = top;
		top = bottom;
		bottom=tmp;
	}

	gi_image_vline(wd->img, x, top, bottom-top+1, color);
	lngi_surface_update(dev, x, top, 1, bottom - top);
}

#define HSCROLL_CLIP_PREFACE \
	if (!sc) return 0;\
	if (sc > (dev->clip.x2 - dev->clip.x1) || -sc > (dev->clip.x2 - dev->clip.x1))\
		return 1;

static int lngi_hscroll (struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	gi_cliprect_t rect;
	lngi_device_t *wd;

	

	HSCROLL_CLIP_PREFACE;

     if (lngi_root->gd->active != dev) {
		return;
	}

#if 0
	wd = (lngi_device_t *) dev->driver_data;

	ignore = NULL;

	rect.x = dev->clip.x1;
	rect.y = dev->clip.y1;
	rect.w = dev->clip.x2 - dev->clip.x1;
	rect.h = dev->clip.y2 - dev->clip.y1;

	gi_image_t *img;

	img=gi_image_get_area(wd->img,rect.x, rect.y, rect.w, rect.h);
	if (!img)
	{
		return;
	}

	gi_image_copy_area(wd->img,img,rect.x + sc, rect.y,0, 0, rect.w, rect.h);
	gi_destroy_image(img);

	lngi_surface_update(dev, rect.x, rect.y, rect.w, rect.h);
#else

gi_copy_area(
		lngi_root->window,
		lngi_root->window,
		wd->gc,
		dev->clip.x1,dev->clip.y1,
		dev->clip.x2-dev->clip.x1,dev->clip.y2-dev->clip.y1,
		dev->clip.x1+sc,dev->clip.y1
	);
#endif

	printf ("%s (line%d) src %d,%d wh %d,%d dst %d,%d\n", __FUNCTION__,  __LINE__,dev->clip.x1,dev->clip.y1,
		dev->clip.x2-dev->clip.x1,dev->clip.y2-dev->clip.y1,
		dev->clip.x1+sc,dev->clip.y1);

	register_bottom_half(gi_process_events, NULL);
	//gi_clear_window(lngi_root->window,TRUE);

        return 1;
}

#define VSCROLL_CLIP_PREFACE \
	if (!sc) return 0;\
	if (sc > dev->clip.y2 - dev->clip.y1 || -sc > dev->clip.y2 - dev->clip.y1) return 1;

static int lngi_vscroll (struct graphics_device *dev, struct rect_set **ignore, int sc)
{
	gi_cliprect_t rect;
	lngi_device_t *wd;

	//printf ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	//return 1;

	VSCROLL_CLIP_PREFACE;

    if (lngi_root->gd->active != dev) {
		return 0;
	}

	wd = (lngi_device_t *) dev->driver_data;

#if 0
	ignore = NULL;

	rect.x = dev->clip.x1;
	rect.y = dev->clip.y1;
	rect.w = dev->clip.x2 - dev->clip.x1;
	rect.h = dev->clip.y2 - dev->clip.y1;

	gi_image_t *img;
	printf ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	img=gi_image_get_area(wd->img,rect.x, rect.y, rect.w, rect.h);
	if (!img)
	{
		return 0;
	}

	printf ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	gi_image_copy_area(wd->img,img,rect.x, rect.y + sc,0, 0, rect.w, rect.h);
	gi_destroy_image(img);
	

	lngi_surface_update(dev, rect.x, rect.y, rect.w, rect.h);
#else

	

gi_copy_area(
		lngi_root->window,
		lngi_root->window,
		wd->gc,
		dev->clip.x1,
		dev->clip.y1,
		dev->clip.x2-dev->clip.x1,
		dev->clip.y2-dev->clip.y1,
		dev->clip.x1,
		dev->clip.y1+sc
	);

printf ("%s (line%d) src %d,%d wh %d,%d dst %d,%d\n", __FUNCTION__,  __LINE__,dev->clip.x1,
		dev->clip.y1,
		dev->clip.x2-dev->clip.x1,
		dev->clip.y2-dev->clip.y1,
		dev->clip.x1,
		dev->clip.y1+sc);
#endif

	register_bottom_half(gi_process_events, NULL);
	//gi_clear_window(lngi_root->window,TRUE);

	return 1;
}

static void lngi_set_clip_area (struct graphics_device *dev, struct rect *r)
{
	lngi_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

     if (lngi_root->gd->active != dev) {
		return;
	}

	wd = (lngi_device_t *) dev->driver_data;

	memcpy(&dev->clip, r, sizeof(struct rect));
	if ((dev->clip.x1 >= dev->clip.x2) ||
	    (dev->clip.y2 <= dev->clip.y1) ||
	    (dev->clip.y2 <= 0) ||
	    (dev->clip.x2 <= 0) ||
	    (dev->clip.x1 >= wd->img->w) ||
	    (dev->clip.y1 >= wd->img->h)) {
		dev->clip.x1 = dev->clip.x2 = dev->clip.y1 = dev->clip.y2 = 0;
	} else {
		if (dev->clip.x1 < 0) dev->clip.x1 = 0;
		if (dev->clip.x2 > wd->img->w) dev->clip.x2 = wd->img->w;
		if (dev->clip.y1 < 0) dev->clip.y1 = 0;
		if (dev->clip.y2 > wd->img->h) dev->clip.y2 = wd->img->h;
	}
}

static int lngi_block (struct graphics_device *dev)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lngi_unblock (struct graphics_device *dev)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

static void lngi_set_title (struct graphics_device *dev, unsigned char *title)
{
	lngi_device_t *wd;

	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);

	wd = (lngi_device_t *) dev->driver_data;
        free(wd->title);
        wd->title = strdup(title);

    if (lngi_root->gd->active != dev) {
		return;
	}

	gi_set_window_caption(lngi_root->window, title);

	 gi_show_window(lngi_root->window);

	 register_bottom_half(gi_process_events, NULL);
}

static int lngi_exec (unsigned char *command, int flag)
{
	DEBUGF ("%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__);
	exit(0);
}

struct graphics_driver xynth_driver = {
	"xynth",
	lngi_init_driver,
	lngi_init_device,
	lngi_shutdown_device,
	lngi_shutdown_driver,
	lngi_driver_param,
	lngi_get_empty_bitmap,
	lngi_register_bitmap,
	lngi_prepare_strip,
	lngi_commit_strip,
	lngi_unregister_bitmap,
	lngi_draw_bitmap,
	lngi_draw_bitmaps,
	lngi_get_color,
	lngi_fill_area,
	lngi_draw_hline,
	lngi_draw_vline,
	lngi_hscroll,
	lngi_vscroll,
	lngi_set_clip_area,
	lngi_block,
	lngi_unblock,
	lngi_set_title,
	lngi_exec,
	0,
	0,
	0,
	0,
};

#endif
