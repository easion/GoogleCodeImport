

#include "SDL_gi.h"
#include <gi/property.h>

#define dbg_printf 
static SDLKey sdl_netbas_gi_keymap[128];


SDL_keysym * sdl_netbas_gi_translatekey(gi_msg_t *event, SDL_keysym *keysym)
{
	int k;
	long mod = event->params[2]>>16;
	int key = event->params[3];
	char ch = key;

	keysym->mod = KMOD_NONE;

	if (mod & G_MODIFIERS_LSHIFT)
		keysym -> mod |= KMOD_LSHIFT ;
	if (mod & G_MODIFIERS_RSHIFT)
		keysym -> mod |= KMOD_RSHIFT ;

	if (mod & G_MODIFIERS_LCTRL)
		keysym -> mod |= KMOD_LCTRL ;
	if (mod & G_MODIFIERS_RCTRL)
		keysym -> mod |= KMOD_RCTRL ;

	if (mod & G_MODIFIERS_LALT)
		keysym -> mod |= KMOD_LALT ;
	if (mod & G_MODIFIERS_RALT)
		keysym -> mod |= KMOD_RALT ;

	if (mod & G_MODIFIERS_LMETA)
		keysym -> mod |= KMOD_LMETA ;
	if (mod & G_MODIFIERS_RMETA)
		keysym -> mod |= KMOD_RMETA ;

	if (mod & G_MODIFIERS_NUMLOCK)
		keysym -> mod |= KMOD_NUM ;
	if (mod & G_MODIFIERS_CAPSLOCK)
		keysym -> mod |= KMOD_CAPS ;
	//if (mod & G_MODIFIERS_ALTGR)
	//	keysym -> mod |= KMOD_MODE ;

	keysym->scancode = ch; //event->keybd->scancode;
	keysym->sym = key;// (event->keybd->keycode == S_KEYCODE_NOCODE) ? SDLK_UNKNOWN : sdl_netbas_gi_keymap[event->keybd->keycode];
	keysym->unicode =  key; //event->keybd->ascii;

	
	//keysym->scancode = key; //event->keybd->scancode;
	//keysym->sym =ch;// (event->keybd->keycode == S_KEYCODE_NOCODE) ? SDLK_UNKNOWN : sdl_netbas_gi_keymap[event->keybd->keycode];
	//keysym->unicode =  ch; //event->keybd->ascii;
	
	if (SDL_TranslateUNICODE) {
	}
	
	return keysym;
}

void GI_RefreshDisplay (_THIS)
{

    // Don't refresh a display that doesn't have an image (like GL)
    if (! SDL_Image) {
    printf ("GI_RefreshDisplay empty\n") ;
        return;
    }


    /*    if (currently_fullscreen) {
            gi_bitblt_bitmap (FSwindow, SDL_GC, OffsetX, OffsetY, this -> screen -> w, 
                this -> screen -> h, SDL_Image, 0,0) ;
	} else {
           gi_bitblt_bitmap (SDL_Window, SDL_GC, 0, 0, this -> screen -> w, 
                this -> screen -> h, SDL_Image, 0,0) ;
	}*/
	gi_gc_attch_window(SDL_GC, SDL_Window);
	gi_draw_image(SDL_GC,SDL_Image, 0, 0);

    gi_flush();

   // printf ("leave GI_RefreshDisplay\n") ;
}
extern int mouse_relative ;

gi_cliprect_t SDL_bounds;
/* Handle any queued OS events */
void sdl_netbas_gi_PumpEvents(_THIS)
{
    gi_msg_t         event ;
    static long last_button_down = 0 ;
	int err;
	extern int client_id;
	int posted = 0;
	err=1;

    while (err == 1) {		

    err = gi_get_message (&event,MSG_NO_WAIT) ;

	if (err<0)
	{
		perror("NETBASGUI_PumpEvents");
		return;
	}
	else if(err == 0){

	break;
	}
	else{
	}
		
	// dispatch event
	switch (event.type) {	

	case GI_MSG_MOUSE_ENTER :
	{
		dbg_printf ("mouse enter\n") ;
		SDL_PrivateAppActive (1, SDL_APPMOUSEFOCUS) ;
		break ;
	}

	case GI_MSG_MOUSE_EXIT :
	{
		dbg_printf ("mouse exit\n") ;
		SDL_PrivateAppActive (0, SDL_APPMOUSEFOCUS) ;
		break ;
	}

	case GI_MSG_FOCUS_IN :
	{
		dbg_printf ("focus in\n") ;
		SDL_PrivateAppActive (1, SDL_APPINPUTFOCUS) ;
		break ;
	}

	case GI_MSG_FOCUS_OUT :
	{
		dbg_printf ("focus out\n") ;
		SDL_PrivateAppActive (0, SDL_APPINPUTFOCUS) ;
		break ;
	}

	case GI_MSG_MOUSE_MOVE :
	{               
		dbg_printf ("mouse motion\n") ;

		if (SDL_VideoSurface) {
		   /* if (currently_fullscreen) {
				if (check_boundary (this, event.body.rect.x, event.body.rect.y)) {
					SDL_PrivateMouseMotion (0, 0, event.body.rect.x - OffsetX, 
						event.body.rect.y - OffsetY) ;
				}
			} else {*/
				if ( mouse_relative ) {
					SDL_PrivateMouseMotion (0, 0, event.body.rect.x, event.body.rect.y) ;
				//SDL_PrivateMouseMotion (0, 1, event.params[0], event.params[1]) ;
				//posted = X11_WarpedMotion(this,&xevent);
				}
				else{
				SDL_PrivateMouseMotion (0, 0, event.body.rect.x, event.body.rect.y) ;
				}
			//}
		}
		break ;
	}

	case GI_MSG_BUTTON_DOWN :
	{
		int button = event.params[2] ;
		
		//printf ("button down\n") ;

		if (button & GI_BUTTON_L)
			button = 1 ;
		else if (button & GI_BUTTON_M)
			button = 2 ;
		else if (button & GI_BUTTON_R)
			button = 3 ;
		else if (button & GI_BUTTON_WHEEL_UP)
			button = SDL_BUTTON_WHEELUP ;
		else if (button & GI_BUTTON_WHEEL_DOWN)
			button = SDL_BUTTON_WHEELDOWN ;
		else
			button = 0;

		last_button_down = button ;
		
		/*if (currently_fullscreen) {
			if (check_boundary (this, event.body.rect.x, event.body.rect.y)) {
				SDL_PrivateMouseButton (SDL_PRESSED, button, 
					event.body.rect.x - OffsetX, event.body.rect.y - OffsetY) ;
			}
		} else {*/

		//printf("SDL_PrivateMouseButton xxx %d %d\n", event.body.rect.x, event.body.rect.y);
			SDL_PrivateMouseButton (SDL_PRESSED, button,event.body.rect.x, event.body.rect.y) ;

			if (button==SDL_BUTTON_WHEELUP || button==SDL_BUTTON_WHEELDOWN)
			{
				SDL_PrivateMouseButton( SDL_RELEASED, button, 0, 0);
			}
	   // }
		break ;
	}

	// do not konw which button is released
	case GI_MSG_BUTTON_UP :
	{   
		dbg_printf ("button up\n") ;

		/*if (currently_fullscreen) {
			if (check_boundary (this, event.body.rect.x, event.body.rect.y)) {
				SDL_PrivateMouseButton (SDL_RELEASED, last_button_down, 
					event.body.rect.x - OffsetX, event.body.rect.y - OffsetY) ;
			}
		} else {*/
		if(last_button_down)
			SDL_PrivateMouseButton (SDL_RELEASED, last_button_down, 
				event.body.rect.x, event.body.rect.y) ;
		//}
		last_button_down = 0 ;
		break ;
	}

	case GI_MSG_KEY_DOWN :
	{
		SDL_keysym keysym ;
		 //SDL_keysym key;

		  // kbd_interrupt(ev);

		   SDL_PrivateKeyboard(SDL_PRESSED, sdl_netbas_gi_translatekey(&event, &keysym));

		//dbg_printf ("key down\n") ;
		//SDL_PrivateKeyboard (SDL_PRESSED,
		 //   GI_TranslateKey (& event, & keysym)) ;
		break ;
	}

	case GI_MSG_KEY_UP :
	{
		SDL_keysym keysym ;

		//SDL_keysym keysym ;

	   // dbg_printf ("key up\n") ;
		SDL_PrivateKeyboard(SDL_RELEASED, sdl_netbas_gi_translatekey(&event, &keysym));
		break ;
	}


	case GI_MSG_WINDOW_DESTROY :
	{
	
		if (event.ev_window==SDL_Window){}
	
		break;
	}
	

	case GI_MSG_CLIENT_MSG:
	{
		if(event.body.client.client_type == GA_WM_PROTOCOLS
			&& event.params[0] == GA_WM_DELETE_WINDOW){
			printf("close require\n");
		SDL_PrivateQuit () ;
		}
		
		break ;
	}

	case GI_MSG_EXPOSURE :
	{
		//if (SDL_VideoSurface) {
		//printf ("event_type_exposure\n") ;
			GI_RefreshDisplay (this) ;//, & event.exposure) ;
	   // }
		break ;
	}

	case GI_MSG_CONFIGURENOTIFY:
	{
		gi_window_info_t info;

		gi_get_window_info(event.ev_window,&info);

		SDL_bounds.x=info.x;
		SDL_bounds.y=info.y;
		SDL_bounds.w=info.width;
		SDL_bounds.h=info.height;

		if (event.params[0] == GI_STRUCT_CHANGE_RESIZE){
			//printf("sdl got resize\n");
		SDL_PrivateResize (event.body.rect.w, event.body.rect.h) ;
		}


		if ( this->input_grab != SDL_GRAB_OFF ) {
			gi_clip_cursor(&SDL_bounds);
		}
	}
	break;

	case GI_MSG_WINDOW_HIDE:
		 if (SDL_GetAppState () & SDL_APPACTIVE) {
					// Send an internal deactivate event
					SDL_PrivateAppActive (0, SDL_APPACTIVE | SDL_APPINPUTFOCUS) ;
				}

				gi_clip_cursor(NULL);
		break;

	case GI_MSG_WINDOW_SHOW:
		 if (!(SDL_GetAppState () & SDL_APPACTIVE)) {
					// Send an internal activate event
					SDL_PrivateAppActive (1, SDL_APPACTIVE) ;
				}
				if (SDL_VideoSurface) {
					GI_RefreshDisplay (this) ;
				}
		break;

		default:
			break;

        }

	
    }

	dbg_printf("%s %d returned\n",__FUNCTION__,event.type);
}


/* Initialize keyboard mapping for this driver */
void sdl_netbas_gi_InitOSKeymap (_THIS)
{
	int i;
	
	for (i = 0; i < SDL_TABLESIZE(sdl_netbas_gi_keymap); i++) {
		sdl_netbas_gi_keymap[i] = SDLK_UNKNOWN;
	}
#if 0	
	sdl_netbas_gi_keymap[NCE_KEY_ESCAPE] = SDLK_ESCAPE;
	sdl_netbas_gi_keymap[NCE_KEY_ONE] = SDLK_1;
	sdl_netbas_gi_keymap[NCE_KEY_TWO] = SDLK_2;
	sdl_netbas_gi_keymap[NCE_KEY_THREE] = SDLK_3;
	sdl_netbas_gi_keymap[NCE_KEY_FOUR] = SDLK_4;
	sdl_netbas_gi_keymap[NCE_KEY_FIVE] = SDLK_5;
	sdl_netbas_gi_keymap[NCE_KEY_SIX] = SDLK_6;
	sdl_netbas_gi_keymap[NCE_KEY_SEVEN] = SDLK_7;
	sdl_netbas_gi_keymap[NCE_KEY_EIGHT] = SDLK_8;
	sdl_netbas_gi_keymap[NCE_KEY_NINE] = SDLK_9;
	sdl_netbas_gi_keymap[NCE_KEY_ZERO] = SDLK_0;
	sdl_netbas_gi_keymap[NCE_KEY_MINUS] = SDLK_MINUS;
	sdl_netbas_gi_keymap[NCE_KEY_EQUAL] = SDLK_EQUALS;
	sdl_netbas_gi_keymap[NCE_KEY_DELETE] = SDLK_BACKSPACE;
	sdl_netbas_gi_keymap[NCE_KEY_TAB] = SDLK_TAB;
	sdl_netbas_gi_keymap[NCE_KEY_q] = SDLK_q;
	sdl_netbas_gi_keymap[NCE_KEY_w] = SDLK_w;
	sdl_netbas_gi_keymap[NCE_KEY_e] = SDLK_e;
	sdl_netbas_gi_keymap[NCE_KEY_r] = SDLK_r;
	sdl_netbas_gi_keymap[NCE_KEY_t] = SDLK_t;
	sdl_netbas_gi_keymap[NCE_KEY_y] = SDLK_y;
	sdl_netbas_gi_keymap[NCE_KEY_u] = SDLK_u;
	sdl_netbas_gi_keymap[NCE_KEY_i] = SDLK_i;
	sdl_netbas_gi_keymap[NCE_KEY_o] = SDLK_o;
	sdl_netbas_gi_keymap[NCE_KEY_p] = SDLK_p;
	sdl_netbas_gi_keymap[NCE_KEY_BRACKETLEFT] = SDLK_LEFTBRACKET;
	sdl_netbas_gi_keymap[NCE_KEY_BRACKETRIGHT] = SDLK_RIGHTBRACKET;
	sdl_netbas_gi_keymap[NCE_KEY_RETURN] = SDLK_RETURN;
	sdl_netbas_gi_keymap[NCE_KEY_LEFTCONTROL] = SDLK_LCTRL;
	sdl_netbas_gi_keymap[NCE_KEY_a] = SDLK_a;
	sdl_netbas_gi_keymap[NCE_KEY_s] = SDLK_s;
	sdl_netbas_gi_keymap[NCE_KEY_d] = SDLK_d;
	sdl_netbas_gi_keymap[NCE_KEY_f] = SDLK_f;
	sdl_netbas_gi_keymap[NCE_KEY_g] = SDLK_g;
	sdl_netbas_gi_keymap[NCE_KEY_h] = SDLK_h;
	sdl_netbas_gi_keymap[NCE_KEY_j] = SDLK_j;
	sdl_netbas_gi_keymap[NCE_KEY_k] = SDLK_k;
	sdl_netbas_gi_keymap[NCE_KEY_l] = SDLK_l;
	sdl_netbas_gi_keymap[NCE_KEY_SEMICOLON] = SDLK_SEMICOLON;
	sdl_netbas_gi_keymap[NCE_KEY_APOSTROPHE] = SDLK_QUOTE;
	sdl_netbas_gi_keymap[NCE_KEY_GRAVE] = SDLK_BACKQUOTE;
	sdl_netbas_gi_keymap[NCE_KEY_LEFTSHIFT] = SDLK_LSHIFT;
	sdl_netbas_gi_keymap[NCE_KEY_BACKSLASH] = SDLK_BACKSLASH;
	sdl_netbas_gi_keymap[NCE_KEY_z] = SDLK_z;
	sdl_netbas_gi_keymap[NCE_KEY_x] = SDLK_x;
	sdl_netbas_gi_keymap[NCE_KEY_c] = SDLK_c;
	sdl_netbas_gi_keymap[NCE_KEY_v] = SDLK_v;
	sdl_netbas_gi_keymap[NCE_KEY_b] = SDLK_b;
	sdl_netbas_gi_keymap[NCE_KEY_n] = SDLK_n;
	sdl_netbas_gi_keymap[NCE_KEY_m] = SDLK_m;
	sdl_netbas_gi_keymap[NCE_KEY_COMMA] = SDLK_COMMA;
	sdl_netbas_gi_keymap[NCE_KEY_PERIOD] = SDLK_PERIOD;
	sdl_netbas_gi_keymap[NCE_KEY_SLASH] = SDLK_SLASH;
	sdl_netbas_gi_keymap[NCE_KEY_RIGHTSHIFT] = SDLK_RSHIFT;
	sdl_netbas_gi_keymap[NCE_KEY_KP_MULTIPLY] = SDLK_KP_MULTIPLY;
	sdl_netbas_gi_keymap[NCE_KEY_ALT] = SDLK_LALT;
	sdl_netbas_gi_keymap[NCE_KEY_SPACE] = SDLK_SPACE;
	sdl_netbas_gi_keymap[NCE_KEY_CAPS_LOCK] = SDLK_CAPSLOCK;
	sdl_netbas_gi_keymap[NCE_KEY_F1] = SDLK_F1;
	sdl_netbas_gi_keymap[NCE_KEY_F2] = SDLK_F2;
	sdl_netbas_gi_keymap[NCE_KEY_F3] = SDLK_F3;
	sdl_netbas_gi_keymap[NCE_KEY_F4] = SDLK_F4;
	sdl_netbas_gi_keymap[NCE_KEY_F5] = SDLK_F5;
	sdl_netbas_gi_keymap[NCE_KEY_F6] = SDLK_F6;
	sdl_netbas_gi_keymap[NCE_KEY_F7] = SDLK_F7;
	sdl_netbas_gi_keymap[NCE_KEY_F8] = SDLK_F8;
	sdl_netbas_gi_keymap[NCE_KEY_F9] = SDLK_F9;
	sdl_netbas_gi_keymap[NCE_KEY_F10] = SDLK_F10;
	sdl_netbas_gi_keymap[NCE_KEY_NUM_LOCK] = SDLK_NUMLOCK;
	sdl_netbas_gi_keymap[NCE_KEY_SCROLL_LOCK] = SDLK_SCROLLOCK;
	sdl_netbas_gi_keymap[NCE_KEY_KP_7] = SDLK_KP7;
	sdl_netbas_gi_keymap[NCE_KEY_KP_8] = SDLK_KP8;
	sdl_netbas_gi_keymap[NCE_KEY_KP_9] = SDLK_KP9;
	sdl_netbas_gi_keymap[NCE_KEY_KP_SUBTRACT] = SDLK_KP_MINUS;
	sdl_netbas_gi_keymap[NCE_KEY_KP_4] = SDLK_KP4;
	sdl_netbas_gi_keymap[NCE_KEY_KP_5] = SDLK_KP5;
	sdl_netbas_gi_keymap[NCE_KEY_KP_6] = SDLK_KP6;
	sdl_netbas_gi_keymap[NCE_KEY_KP_ADD] = SDLK_KP_PLUS;
	sdl_netbas_gi_keymap[NCE_KEY_KP_1] = SDLK_KP1;
	sdl_netbas_gi_keymap[NCE_KEY_KP_2] = SDLK_KP2;
	sdl_netbas_gi_keymap[NCE_KEY_KP_3] = SDLK_KP3;
	sdl_netbas_gi_keymap[NCE_KEY_KP_0] = SDLK_KP0;
	sdl_netbas_gi_keymap[NCE_KEY_KP_PERIOD] = SDLK_KP_PERIOD;
	sdl_netbas_gi_keymap[NCE_KEY_LESS] = SDLK_LESS;
	sdl_netbas_gi_keymap[NCE_KEY_F11] = SDLK_F11;
	sdl_netbas_gi_keymap[NCE_KEY_F12] = SDLK_F12;
	sdl_netbas_gi_keymap[NCE_KEY_KP_ENTER] = SDLK_KP_ENTER;
	sdl_netbas_gi_keymap[NCE_KEY_RIGHTCONTROL] = SDLK_RCTRL;
	sdl_netbas_gi_keymap[NCE_KEY_KP_DIVIDE] = SDLK_KP_DIVIDE;
	sdl_netbas_gi_keymap[NCE_KEY_VOIDSYMBOL] = SDLK_PRINT;
	sdl_netbas_gi_keymap[NCE_KEY_ALTGR] = SDLK_RALT;
	sdl_netbas_gi_keymap[NCE_KEY_BREAK] = SDLK_BREAK;
	sdl_netbas_gi_keymap[NCE_KEY_HOME] = SDLK_HOME;
	sdl_netbas_gi_keymap[NCE_KEY_UP] = SDLK_UP;
	sdl_netbas_gi_keymap[NCE_KEY_PAGEUP] = SDLK_PAGEUP;
	sdl_netbas_gi_keymap[NCE_KEY_LEFT] = SDLK_LEFT;
	sdl_netbas_gi_keymap[NCE_KEY_RIGHT] = SDLK_RIGHT;
	sdl_netbas_gi_keymap[NCE_KEY_END] = SDLK_END;
	sdl_netbas_gi_keymap[NCE_KEY_DOWN] = SDLK_DOWN;
	sdl_netbas_gi_keymap[NCE_KEY_PAGEDOWN] = SDLK_PAGEDOWN;
	sdl_netbas_gi_keymap[NCE_KEY_INSERT] = SDLK_INSERT;
	sdl_netbas_gi_keymap[NCE_KEY_REMOVE] = SDLK_DELETE;
	sdl_netbas_gi_keymap[NCE_KEY_PAUSE] = SDLK_PAUSE;
#endif

}
