//----------------------------------------------------------------------------
// Anti-Grain Geometry (AGG) - Version 2.5
// A high quality rendering engine for C++
// Copyright (C) 2002-2006 Maxim Shemanarev
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://antigrain.com
// 
// Copyright (C) 2011 Easion Deng
// Contact: easion@hanxuantech.com
//          easion@gmail.com
//          http://hanxuantech.com
// 
// AGG is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// AGG is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with AGG; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
// MA 02110-1301, USA.
//----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <gi/gi.h>
#include <gi/property.h>

#include "agg_basics.h"
#include "util/agg_color_conv_rgb8.h"
#include "platform/agg_platform_support.h"


namespace agg
{
    //------------------------------------------------------------------------
    class platform_specific
    {
    public:
        platform_specific(pix_format_e format, bool flip_y);
        ~platform_specific();
        
        void caption(const char* capt);
        void put_image(const rendering_buffer* src);
       
        pix_format_e         m_format;
        pix_format_e         m_sys_format;
        int                  m_byte_order;
        bool                 m_flip_y;
        unsigned             m_bpp;
        unsigned             m_sys_bpp;
        int             m_display;
        gi_screen_info_t              m_visual;
        gi_window_id_t               m_window;
        gi_gc_ptr_t                   m_gc;
        gi_image_t*              m_image;
        gi_window_info_t m_window_attributes;
        unsigned char*       m_buf_window;
        unsigned char*       m_buf_img[platform_support::max_images];
        unsigned             m_keymap[256];
       
        bool m_update_flag;
        bool m_resize_flag;
        bool m_initialized;
        //bool m_wait_mode;
        clock_t m_sw_start;
    };



    //------------------------------------------------------------------------
    platform_specific::platform_specific(pix_format_e format, bool flip_y) :
        m_format(format),
        m_sys_format(pix_format_undefined),
        m_flip_y(flip_y),
        m_bpp(0),
        m_sys_bpp(0),
        m_display(0),
        m_window(0),
        m_gc(0),
        m_image(0),

        m_buf_window(0),
        
        m_update_flag(true), 
        m_resize_flag(true),
        m_initialized(false)
        //m_wait_mode(true)
    {
        memset(m_buf_img, 0, sizeof(m_buf_img));

        unsigned i;
        for(i = 0; i < 256; i++)
        {
            m_keymap[i] = i;
        }

        m_keymap[G_KEY_PAUSE&0xFF] = key_pause;
        m_keymap[G_KEY_CLEAR&0xFF] = key_clear;

        m_keymap[G_KEY_0&0xFF] = key_kp0;
        m_keymap[G_KEY_1&0xFF] = key_kp1;
        m_keymap[G_KEY_2&0xFF] = key_kp2;
        m_keymap[G_KEY_3&0xFF] = key_kp3;
        m_keymap[G_KEY_4&0xFF] = key_kp4;
        m_keymap[G_KEY_5&0xFF] = key_kp5;
        m_keymap[G_KEY_6&0xFF] = key_kp6;
        m_keymap[G_KEY_7&0xFF] = key_kp7;
        m_keymap[G_KEY_8&0xFF] = key_kp8;
        m_keymap[G_KEY_9&0xFF] = key_kp9;

       // m_keymap[G_KEY_Insert&0xFF]    = key_kp0;
        m_keymap[G_KEY_END&0xFF]       = key_kp1;   
        //m_keymap[G_KEY_Down&0xFF]      = key_kp2;
        //m_keymap[G_KEY_Page_Down&0xFF] = key_kp3;
       // m_keymap[G_KEY_Left&0xFF]      = key_kp4;
        //m_keymap[G_KEY_Begin&0xFF]     = key_kp5;
       // m_keymap[G_KEY_Right&0xFF]     = key_kp6;
        m_keymap[G_KEY_HOME&0xFF]      = key_kp7;
        //m_keymap[G_KEY_Up&0xFF]        = key_kp8;
        //m_keymap[G_KEY_Page_Up&0xFF]   = key_kp9;
        m_keymap[G_KEY_DELETE&0xFF]    = key_kp_period;
        m_keymap[G_KEY_PERIOD&0xFF]   = key_kp_period;
        m_keymap[G_KEY_SLASH & 0xff ]    = key_kp_divide;
        m_keymap[G_KEY_KP_MULTIPLY & 0xff]  = key_kp_multiply;
        m_keymap[G_KEY_MINUS & 0xff]  = key_kp_minus;
        m_keymap[G_KEY_PLUS&0xff]       = key_kp_plus;
        m_keymap[G_KEY_RETURN&0xFF]     = key_kp_enter;
        m_keymap[G_KEY_EQUALS&0xFF]     = key_kp_equals;

        m_keymap[G_KEY_UP&0xFF]           = key_up;
        m_keymap[G_KEY_DOWN&0xFF]         = key_down;
        m_keymap[G_KEY_RIGHT&0xFF]        = key_right;
        m_keymap[G_KEY_LEFT&0xFF]         = key_left;
        m_keymap[G_KEY_INSERT&0xFF]       = key_insert;
        m_keymap[G_KEY_HOME&0xFF]         = key_delete;
        m_keymap[G_KEY_END&0xFF]          = key_end;
        m_keymap[G_KEY_PAGEUP&0xFF]      = key_page_up;
        m_keymap[G_KEY_PAGEDOWN&0xFF]    = key_page_down;

        m_keymap[G_KEY_F1&0xFF]           = key_f1;
        m_keymap[G_KEY_F2&0xFF]           = key_f2;
        m_keymap[G_KEY_F3&0xFF]           = key_f3;
        m_keymap[G_KEY_F4&0xFF]           = key_f4;
        m_keymap[G_KEY_F5&0xFF]           = key_f5;
        m_keymap[G_KEY_F6&0xFF]           = key_f6;
        m_keymap[G_KEY_F7&0xFF]           = key_f7;
        m_keymap[G_KEY_F8&0xFF]           = key_f8;
        m_keymap[G_KEY_F9&0xFF]           = key_f9;
        m_keymap[G_KEY_F10&0xFF]          = key_f10;
        m_keymap[G_KEY_F11&0xFF]          = key_f11;
        m_keymap[G_KEY_F12&0xFF]          = key_f12;
        m_keymap[G_KEY_F13&0xFF]          = key_f13;
        m_keymap[G_KEY_F14&0xFF]          = key_f14;
        m_keymap[G_KEY_F15&0xFF]          = key_f15;

        m_keymap[G_KEY_NUMLOCK&0xFF]     = key_numlock;
        m_keymap[G_KEY_CAPSLOCK&0xFF]    = key_capslock;
        m_keymap[G_KEY_SCROLLOCK&0xFF]  = key_scrollock;


        switch(m_format)
        {
        default: break;
        case pix_format_gray8:
            m_bpp = 8;
            break;

        case pix_format_rgb565:
        case pix_format_rgb555:
            m_bpp = 16;
            break;

        case pix_format_rgb24:
        case pix_format_bgr24:
            m_bpp = 24;
            break;

        case pix_format_bgra32:
        case pix_format_abgr32:
        case pix_format_argb32:
        case pix_format_rgba32:
            m_bpp = 32;
            break;
        }
        m_sw_start = clock();
    }

    //------------------------------------------------------------------------
    platform_specific::~platform_specific()
    {
    }

    //------------------------------------------------------------------------
    void platform_specific::caption(const char* capt)
    {
        gi_set_window_utf8_caption( m_window, capt);
    }

    
    //------------------------------------------------------------------------
    void platform_specific::put_image(const rendering_buffer* src)
    {    
        if(m_image == 0) return;
        m_image->rgba = (gi_color_t*)m_buf_window;
        
        if(m_format == m_sys_format)
        {
            gi_bitblt_image(                      
                      m_gc,0, 0, 
                      src->width(), 
                      src->height(),
                      m_image, 
                       0, 0);
        }
        else
        {
            int row_len = src->width() * m_sys_bpp / 8;
            unsigned char* buf_tmp = 
                new unsigned char[row_len * src->height()];
            
            rendering_buffer rbuf_tmp;
            rbuf_tmp.attach(buf_tmp, 
                            src->width(), 
                            src->height(), 
                            m_flip_y ? -row_len : row_len);

            switch(m_sys_format)            
            {
                default: break;
                case pix_format_rgb555:
                    switch(m_format)
                    {
                        default: break;
                        case pix_format_rgb555: color_conv(&rbuf_tmp, src, color_conv_rgb555_to_rgb555()); break;
                        case pix_format_rgb565: color_conv(&rbuf_tmp, src, color_conv_rgb565_to_rgb555()); break;
                        case pix_format_rgb24:  color_conv(&rbuf_tmp, src, color_conv_rgb24_to_rgb555());  break;
                        case pix_format_bgr24:  color_conv(&rbuf_tmp, src, color_conv_bgr24_to_rgb555());  break;
                        case pix_format_rgba32: color_conv(&rbuf_tmp, src, color_conv_rgba32_to_rgb555()); break;
                        case pix_format_argb32: color_conv(&rbuf_tmp, src, color_conv_argb32_to_rgb555()); break;
                        case pix_format_bgra32: color_conv(&rbuf_tmp, src, color_conv_bgra32_to_rgb555()); break;
                        case pix_format_abgr32: color_conv(&rbuf_tmp, src, color_conv_abgr32_to_rgb555()); break;
                    }
                    break;
                    
                case pix_format_rgb565:
                    switch(m_format)
                    {
                        default: break;
                        case pix_format_rgb555: color_conv(&rbuf_tmp, src, color_conv_rgb555_to_rgb565()); break;
                        case pix_format_rgb565: color_conv(&rbuf_tmp, src, color_conv_rgb565_to_rgb565()); break;
                        case pix_format_rgb24:  color_conv(&rbuf_tmp, src, color_conv_rgb24_to_rgb565());  break;
                        case pix_format_bgr24:  color_conv(&rbuf_tmp, src, color_conv_bgr24_to_rgb565());  break;
                        case pix_format_rgba32: color_conv(&rbuf_tmp, src, color_conv_rgba32_to_rgb565()); break;
                        case pix_format_argb32: color_conv(&rbuf_tmp, src, color_conv_argb32_to_rgb565()); break;
                        case pix_format_bgra32: color_conv(&rbuf_tmp, src, color_conv_bgra32_to_rgb565()); break;
                        case pix_format_abgr32: color_conv(&rbuf_tmp, src, color_conv_abgr32_to_rgb565()); break;
                    }
                    break;
                    
                case pix_format_rgba32:
                    switch(m_format)
                    {
                        default: break;
                        case pix_format_rgb555: color_conv(&rbuf_tmp, src, color_conv_rgb555_to_rgba32()); break;
                        case pix_format_rgb565: color_conv(&rbuf_tmp, src, color_conv_rgb565_to_rgba32()); break;
                        case pix_format_rgb24:  color_conv(&rbuf_tmp, src, color_conv_rgb24_to_rgba32());  break;
                        case pix_format_bgr24:  color_conv(&rbuf_tmp, src, color_conv_bgr24_to_rgba32());  break;
                        case pix_format_rgba32: color_conv(&rbuf_tmp, src, color_conv_rgba32_to_rgba32()); break;
                        case pix_format_argb32: color_conv(&rbuf_tmp, src, color_conv_argb32_to_rgba32()); break;
                        case pix_format_bgra32: color_conv(&rbuf_tmp, src, color_conv_bgra32_to_rgba32()); break;
                        case pix_format_abgr32: color_conv(&rbuf_tmp, src, color_conv_abgr32_to_rgba32()); break;
                    }
                    break;
                    
                case pix_format_abgr32:
                    switch(m_format)
                    {
                        default: break;
                        case pix_format_rgb555: color_conv(&rbuf_tmp, src, color_conv_rgb555_to_abgr32()); break;
                        case pix_format_rgb565: color_conv(&rbuf_tmp, src, color_conv_rgb565_to_abgr32()); break;
                        case pix_format_rgb24:  color_conv(&rbuf_tmp, src, color_conv_rgb24_to_abgr32());  break;
                        case pix_format_bgr24:  color_conv(&rbuf_tmp, src, color_conv_bgr24_to_abgr32());  break;
                        case pix_format_abgr32: color_conv(&rbuf_tmp, src, color_conv_abgr32_to_abgr32()); break;
                        case pix_format_rgba32: color_conv(&rbuf_tmp, src, color_conv_rgba32_to_abgr32()); break;
                        case pix_format_argb32: color_conv(&rbuf_tmp, src, color_conv_argb32_to_abgr32()); break;
                        case pix_format_bgra32: color_conv(&rbuf_tmp, src, color_conv_bgra32_to_abgr32()); break;
                    }
                    break;
                    
                case pix_format_argb32:
                    switch(m_format)
                    {
                        default: break;
                        case pix_format_rgb555: color_conv(&rbuf_tmp, src, color_conv_rgb555_to_argb32()); break;
                        case pix_format_rgb565: color_conv(&rbuf_tmp, src, color_conv_rgb565_to_argb32()); break;
                        case pix_format_rgb24:  color_conv(&rbuf_tmp, src, color_conv_rgb24_to_argb32());  break;
                        case pix_format_bgr24:  color_conv(&rbuf_tmp, src, color_conv_bgr24_to_argb32());  break;
                        case pix_format_rgba32: color_conv(&rbuf_tmp, src, color_conv_rgba32_to_argb32()); break;
                        case pix_format_argb32: color_conv(&rbuf_tmp, src, color_conv_argb32_to_argb32()); break;
                        case pix_format_abgr32: color_conv(&rbuf_tmp, src, color_conv_abgr32_to_argb32()); break;
                        case pix_format_bgra32: color_conv(&rbuf_tmp, src, color_conv_bgra32_to_argb32()); break;
                    }
                    break;
                    
                case pix_format_bgra32:
                    switch(m_format)
                    {
                        default: break;
                        case pix_format_rgb555: color_conv(&rbuf_tmp, src, color_conv_rgb555_to_bgra32()); break;
                        case pix_format_rgb565: color_conv(&rbuf_tmp, src, color_conv_rgb565_to_bgra32()); break;
                        case pix_format_rgb24:  color_conv(&rbuf_tmp, src, color_conv_rgb24_to_bgra32());  break;
                        case pix_format_bgr24:  color_conv(&rbuf_tmp, src, color_conv_bgr24_to_bgra32());  break;
                        case pix_format_rgba32: color_conv(&rbuf_tmp, src, color_conv_rgba32_to_bgra32()); break;
                        case pix_format_argb32: color_conv(&rbuf_tmp, src, color_conv_argb32_to_bgra32()); break;
                        case pix_format_abgr32: color_conv(&rbuf_tmp, src, color_conv_abgr32_to_bgra32()); break;
                        case pix_format_bgra32: color_conv(&rbuf_tmp, src, color_conv_bgra32_to_bgra32()); break;
                    }
                    break;
            }
            
          m_image->rgba = (gi_color_t*)buf_tmp;
           
		  gi_bitblt_image(                      
			  m_gc,0, 0, 
			  src->width(), 
			  src->height(),
			  m_image, 
			   0, 0);
            
            delete [] buf_tmp;
        }
    }
    

    //------------------------------------------------------------------------
    platform_support::platform_support(pix_format_e format, bool flip_y) :
        m_specific(new platform_specific(format, flip_y)),
        m_format(format),
        m_bpp(m_specific->m_bpp),
        m_wait_mode(true),
        m_flip_y(flip_y),
        m_initial_width(10),
        m_initial_height(10)
    {
        strcpy(m_caption, "AGG Application");
    }

    //------------------------------------------------------------------------
    platform_support::~platform_support()
    {
        delete m_specific;
    }



    //------------------------------------------------------------------------
    void platform_support::caption(const char* cap)
    {
        strcpy(m_caption, cap);
        if(m_specific->m_initialized)
        {
            m_specific->caption(cap);
        }
    }

   
    //------------------------------------------------------------------------
    
      unsigned long  xevent_mask = GI_MASK_CONFIGURENOTIFY
		| GI_MASK_KEY_DOWN | GI_MASK_KEY_UP
		| GI_MASK_MOUSE_MOVE | GI_MASK_BUTTON_UP
		| GI_MASK_CLIENT_MSG | GI_MASK_EXPOSURE
		| GI_MASK_BUTTON_DOWN ;


    //------------------------------------------------------------------------
    bool platform_support::init(unsigned width, unsigned height, unsigned flags)
    {
        //m_window_flags = flags;
        
        m_specific->m_display = gi_init();
        if(m_specific->m_display < 0) 
        {
            fprintf(stderr, "Unable to open DISPLAY!\n");
            return false;
        }

		memset(&m_specific->m_visual, 0, sizeof(gi_screen_info_t));

		gi_get_screen_info(&m_specific->m_visual);        
        
        unsigned long r_mask = m_specific->m_visual.redmask;
        unsigned long g_mask = m_specific->m_visual.greenmask;
        unsigned long b_mask = m_specific->m_visual.bluemask;
   

        if(GI_RENDER_FORMAT_BPP(m_specific->m_visual.format) < 15 ||
           r_mask == 0 || g_mask == 0 || b_mask == 0)
        {
            fprintf(stderr,
                   "There's no gi_screen_info_t compatible with minimal AGG requirements:\n"
                   "At least 15-bit color depth and True- or DirectColor class.\n\n");
            gi_exit();
            return false;
        }
        
        // Perceive SYS-format by mask
        switch(GI_RENDER_FORMAT_BPP(m_specific->m_visual.format))
        {
            case 15:
                m_specific->m_sys_bpp = 16;
                if(r_mask == 0x7C00 && g_mask == 0x3E0 && b_mask == 0x1F)
                {
                    m_specific->m_sys_format = pix_format_rgb555;
                }
                break;
                
            case 16:
                m_specific->m_sys_bpp = 16;
                if(r_mask == 0xF800 && g_mask == 0x7E0 && b_mask == 0x1F)
                {
                    m_specific->m_sys_format = pix_format_rgb565;
                }
                break;
                
            case 24:
            case 32:
                m_specific->m_sys_bpp = 32;
                if(g_mask == 0xFF00)
                {
                    if(r_mask == 0xFF && b_mask == 0xFF0000)
                    {
                        switch(m_specific->m_format)
                        {
                            case pix_format_rgba32:
                                m_specific->m_sys_format = pix_format_rgba32;
                                break;
                                
                            case pix_format_abgr32:
                                m_specific->m_sys_format = pix_format_abgr32;
                                break;

                            default:                            
                                m_specific->m_sys_format = 
                                    pix_format_rgba32 ;
                                    //pix_format_abgr32;
                                break;
                        }
                    }
                    
                    if(r_mask == 0xFF0000 && b_mask == 0xFF)
                    {
                        switch(m_specific->m_format)
                        {
                            case pix_format_argb32:
                                m_specific->m_sys_format = pix_format_argb32;
                                break;
                                
                            case pix_format_bgra32:
                                m_specific->m_sys_format = pix_format_bgra32;
                                break;

                            default:                            
                                m_specific->m_sys_format = 
                                    pix_format_argb32 ;
                                    //pix_format_bgra32;
                                break;
                        }
                    }
                }
                break;
        }
        
        if(m_specific->m_sys_format == pix_format_undefined)
        {
            fprintf(stderr,
                   "RGB masks are not compatible with AGG pixel formats:\n"
                   "R=%08x, R=%08x, B=%08x\n", r_mask, g_mask, b_mask);
            gi_exit();
            return false;
        }
                
        
        
        memset(&m_specific->m_window_attributes, 
               0, 
               sizeof(m_specific->m_window_attributes)); 
        
        
        m_specific->m_window = gi_create_window(GI_DESKTOP_WINDOW_ID, 
			0,0,width, height,
		GI_RGB( 240, 240, 242 ),
		0);

        gi_set_events_mask( 
                     m_specific->m_window, 
                     xevent_mask);   

        m_specific->m_gc = gi_create_gc( 
                                     m_specific->m_window, 
                                     NULL); 
        m_specific->m_buf_window = 
            new unsigned char[width * height * (m_bpp / 8)];

        memset(m_specific->m_buf_window, 255, width * height * (m_bpp / 8));
        
        m_rbuf_window.attach(m_specific->m_buf_window,
                             width,
                             height,
                             m_flip_y ? -width * (m_bpp / 8) : width * (m_bpp / 8));
            
        m_specific->m_image = gi_create_image_with_data(width, 
			height, m_specific->m_buf_window,(gi_format_code_t)m_specific->m_visual.format);
           

        m_specific->caption(m_caption); 
        m_initial_width = width;
        m_initial_height = height;
        
        if(!m_specific->m_initialized)
        {
            on_init();
            m_specific->m_initialized = true;
        }

        trans_affine_resizing(width, height);
        on_resize(width, height);
        m_specific->m_update_flag = true;       


        gi_show_window( 
                   m_specific->m_window);

    
       

        return true;
    }



    //------------------------------------------------------------------------
    void platform_support::update_window()
    {
        m_specific->put_image(&m_rbuf_window);      
    }


    //------------------------------------------------------------------------
    int platform_support::run()
    {
       
        bool quit = false;
        unsigned flags;
        int cur_x;
        int cur_y;

        while(!quit)
        {
            if(m_specific->m_update_flag)
            {
                on_draw();
                update_window();
                m_specific->m_update_flag = false;
            }

            if(!m_wait_mode)
            {
                if(gi_get_event_count() == 0)
                {
                    on_idle();
                    continue;
                }
            }

            gi_msg_t x_event;
            gi_get_message( &x_event, MSG_ALWAYS_WAIT);
 
			#if 0
            // In the Idle mode discard all intermediate MotionNotify events
            if(!m_wait_mode && x_event.type == MotionNotify)
            {
                gi_msg_t te = x_event;
                for(;;)
                {
                    if(gi_get_event_count()==0) break;
                    //XNextEvent( &te);
					gi_get_message( &te, MSG_ALWAYS_WAIT);
                    if(te.type != MotionNotify) break;
                }
                x_event = te;
            }
#endif

            switch(x_event.type) 
            {
            case GI_MSG_CONFIGURENOTIFY: 
                {
                    if(x_event.body.rect.w  != int(m_rbuf_window.width()) ||
                       x_event.body.rect.h != int(m_rbuf_window.height()))
                    {
                        int width  = x_event.body.rect.w;
                        int height = x_event.body.rect.h;

                        delete [] m_specific->m_buf_window;
                        m_specific->m_image->rgba = 0;
                        gi_destroy_image(m_specific->m_image);

                        m_specific->m_buf_window = 
                            new unsigned char[width * height * (m_bpp / 8)];

                        m_rbuf_window.attach(m_specific->m_buf_window,
                                             width,
                                             height,
                                             m_flip_y ? 
                                             -width * (m_bpp / 8) : 
                                             width * (m_bpp / 8));
            
                        m_specific->m_image = gi_create_image_with_data(width, 
						height,m_specific->m_buf_window, (gi_format_code_t)m_specific->m_visual.format);
                        

                        trans_affine_resizing(width, height);
                        on_resize(width, height);
                        on_draw();
                        update_window();
                    }
                }
                break;

            case GI_MSG_EXPOSURE:
                m_specific->put_image(&m_rbuf_window);
                
                break;

            case GI_MSG_KEY_DOWN:
                {
                    int key = x_event.params[3];
					int state = x_event.body.message[3];
                    flags = 0;
                    /*if(x_event.xkey.state & Button1Mask) flags |= mouse_left;
                    if(x_event.xkey.state & Button3Mask) flags |= mouse_right;*/
                    if(state & G_MODIFIERS_SHIFT)   flags |= kbd_shift;
                    if(state & G_MODIFIERS_CTRL) flags |= kbd_ctrl;
					

                    bool left  = false;
                    bool up    = false;
                    bool right = false;
                    bool down  = false;

                    switch(m_specific->m_keymap[key & 0xFF])
                    {
                    case key_left:
                        left = true;
                        break;

                    case key_up:
                        up = true;
                        break;

                    case key_right:
                        right = true;
                        break;

                    case key_down:
                        down = true;
                        break;

                    case key_f2:                        
                        copy_window_to_img(max_images - 1);
                        save_img(max_images - 1, "screenshot");
                        break;
                    }

                    if(m_ctrls.on_arrow_keys(left, right, down, up))
                    {
                        on_ctrl_change();
                        force_redraw();
                    }
                    else
                    {
                        on_key(x_event.body.rect.x, 
                               m_flip_y ? 
                                   m_rbuf_window.height() - x_event.body.rect.y :
                                   x_event.body.rect.y,
                               m_specific->m_keymap[key & 0xFF],
                               flags);
                    }
                }
                break;


            case GI_MSG_BUTTON_DOWN:
                {
                    flags = 0;
                    //if(x_event.xbutton.state & G_MODIFIERS_SHIFT)   flags |= kbd_shift;
                    //if(x_event.xbutton.state & G_MODIFIERS_CTRL) flags |= kbd_ctrl;
                    if(x_event.params[2] & GI_BUTTON_L)   flags |= mouse_left;
                    if(x_event.params[2] & GI_BUTTON_R)   flags |= mouse_right;

                    cur_x = x_event.body.rect.x;
                    cur_y = m_flip_y ? m_rbuf_window.height() - x_event.body.rect.y :
                                       x_event.body.rect.y;

                    if(flags & mouse_left)
                    {
                        if(m_ctrls.on_mouse_button_down(cur_x, cur_y))
                        {
                            m_ctrls.set_cur(cur_x, cur_y);
                            on_ctrl_change();
                            force_redraw();
                        }
                        else
                        {
                            if(m_ctrls.in_rect(cur_x, cur_y))
                            {
                                if(m_ctrls.set_cur(cur_x, cur_y))
                                {
                                    on_ctrl_change();
                                    force_redraw();
                                }
                            }
                            else
                            {
                                on_mouse_button_down(cur_x, cur_y, flags);
                            }
                        }
                    }
                    if(flags & mouse_right)
                    {
                        on_mouse_button_down(cur_x, cur_y, flags);
                    }
                    //m_specific->m_wait_mode = m_wait_mode;
                    //m_wait_mode = true;
                }
                break;

                
            case GI_MSG_MOUSE_MOVE:
                {
                    flags = 0;
                    if(x_event.params[3] & GI_BUTTON_L) flags |= mouse_left;
                    if(x_event.params[3] & GI_BUTTON_R) flags |= mouse_right;
                    //if(x_event.xmotion.state & ShiftMask)   flags |= kbd_shift;
                    //if(x_event.xmotion.state & ControlMask) flags |= kbd_ctrl;

                    cur_x = x_event.body.rect.x;
                    cur_y = m_flip_y ? m_rbuf_window.height() - x_event.body.rect.y :
                                       x_event.body.rect.y;

                    if(m_ctrls.on_mouse_move(cur_x, cur_y, (flags & mouse_left) != 0))
                    {
                        on_ctrl_change();
                        force_redraw();
                    }
                    else
                    {
                        if(!m_ctrls.in_rect(cur_x, cur_y))
                        {
                            on_mouse_move(cur_x, cur_y, flags);
                        }
                    }
                }
                break;
                
            case GI_MSG_BUTTON_UP:
                {
                    flags = 0;
                    //if(x_event.xbutton.state & ShiftMask)   flags |= kbd_shift;
                    //if(x_event.xbutton.state & ControlMask) flags |= kbd_ctrl;
                    if(x_event.params[3] & GI_BUTTON_L)   flags |= mouse_left;
                    if(x_event.params[3] & GI_BUTTON_R)   flags |= mouse_right;

                    cur_x = x_event.body.rect.x;
                    cur_y = m_flip_y ? m_rbuf_window.height() - x_event.body.rect.y :
                                       x_event.body.rect.y;

                    if(flags & mouse_left)
                    {
                        if(m_ctrls.on_mouse_button_up(cur_x, cur_y))
                        {
                            on_ctrl_change();
                            force_redraw();
                        }
                    }
                    if(flags & (mouse_left | mouse_right))
                    {
                        on_mouse_button_up(cur_x, cur_y, flags);
                    }
                }
                //m_wait_mode = m_specific->m_wait_mode;
                break;

			case GI_MSG_CLIENT_MSG:
				if(x_event.body.client.client_type==GA_WM_PROTOCOLS
						&&x_event.params[0] == GA_WM_DELETE_WINDOW){
				quit = true;				
				gi_quit_dispatch();
			}
			break;
            }           
        }


        unsigned i = platform_support::max_images;
        while(i--)
        {
            if(m_specific->m_buf_img[i]) 
            {
                delete [] m_specific->m_buf_img[i];
            }
        }

        delete [] m_specific->m_buf_window;
        m_specific->m_image->rgba = 0;
        gi_destroy_image(m_specific->m_image);
        gi_destroy_gc( m_specific->m_gc);
        gi_destroy_window( m_specific->m_window);
        gi_exit();
        
        return 0;
    }



    //------------------------------------------------------------------------
    const char* platform_support::img_ext() const { return ".ppm"; }

    //------------------------------------------------------------------------
    const char* platform_support::full_file_name(const char* file_name)
    {
        return file_name;
    }

    //------------------------------------------------------------------------
    bool platform_support::load_img(unsigned idx, const char* file)
    {
        if(idx < max_images)
        {
            char buf[1024];
            strcpy(buf, file);
            int len = strlen(buf);
            if(len < 4 || strcasecmp(buf + len - 4, ".ppm") != 0)
            {
                strcat(buf, ".ppm");
            }
            
            FILE* fd = fopen(buf, "rb");
            if(fd == 0) return false;

            if((len = fread(buf, 1, 1022, fd)) == 0)
            {
                fclose(fd);
                return false;
            }
            buf[len] = 0;
            
            if(buf[0] != 'P' && buf[1] != '6')
            {
                fclose(fd);
                return false;
            }
            
            char* ptr = buf + 2;
            
            while(*ptr && !isdigit(*ptr)) ptr++;
            if(*ptr == 0)
            {
                fclose(fd);
                return false;
            }
            
            unsigned width = atoi(ptr);
            if(width == 0 || width > 4096)
            {
                fclose(fd);
                return false;
            }
            while(*ptr && isdigit(*ptr)) ptr++;
            while(*ptr && !isdigit(*ptr)) ptr++;
            if(*ptr == 0)
            {
                fclose(fd);
                return false;
            }
            unsigned height = atoi(ptr);
            if(height == 0 || height > 4096)
            {
                fclose(fd);
                return false;
            }
            while(*ptr && isdigit(*ptr)) ptr++;
            while(*ptr && !isdigit(*ptr)) ptr++;
            if(atoi(ptr) != 255)
            {
                fclose(fd);
                return false;
            }
            while(*ptr && isdigit(*ptr)) ptr++;
            if(*ptr == 0)
            {
                fclose(fd);
                return false;
            }
            ptr++;
            fseek(fd, long(ptr - buf), SEEK_SET);
            
            create_img(idx, width, height);
            bool ret = true;
            
            if(m_format == pix_format_rgb24)
            {
                fread(m_specific->m_buf_img[idx], 1, width * height * 3, fd);
            }
            else
            {
                unsigned char* buf_img = new unsigned char [width * height * 3];
                rendering_buffer rbuf_img;
                rbuf_img.attach(buf_img,
                                width,
                                height,
                                m_flip_y ?
                                  -width * 3 :
                                   width * 3);
                
                fread(buf_img, 1, width * height * 3, fd);
                
                switch(m_format)
                {
                    case pix_format_rgb555:
                        color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_rgb555());
                        break;
                        
                    case pix_format_rgb565:
                        color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_rgb565());
                        break;
                        
                    case pix_format_bgr24:
                        color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_bgr24());
                        break;
                        
                    case pix_format_rgba32:
                        color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_rgba32());
                        break;
                        
                    case pix_format_argb32:
                        color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_argb32());
                        break;
                        
                    case pix_format_bgra32:
                        color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_bgra32());
                        break;
                        
                    case pix_format_abgr32:
                        color_conv(m_rbuf_img+idx, &rbuf_img, color_conv_rgb24_to_abgr32());
                        break;
                        
                    default:
                        ret = false;
                }
                delete [] buf_img;
            }
                        
            fclose(fd);
            return ret;
        }
        return false;
    }




    //------------------------------------------------------------------------
    bool platform_support::save_img(unsigned idx, const char* file)
    {
        if(idx < max_images &&  rbuf_img(idx).buf())
        {
            char buf[1024];
            strcpy(buf, file);
            int len = strlen(buf);
            if(len < 4 || strcasecmp(buf + len - 4, ".ppm") != 0)
            {
                strcat(buf, ".ppm");
            }
            
            FILE* fd = fopen(buf, "wb");
            if(fd == 0) return false;
            
            unsigned w = rbuf_img(idx).width();
            unsigned h = rbuf_img(idx).height();
            
            fprintf(fd, "P6\n%d %d\n255\n", w, h);
                
            unsigned y; 
            unsigned char* tmp_buf = new unsigned char [w * 3];
            for(y = 0; y < rbuf_img(idx).height(); y++)
            {
                const unsigned char* src = rbuf_img(idx).row_ptr(m_flip_y ? h - 1 - y : y);
                switch(m_format)
                {
                    default: break;
                    case pix_format_rgb555:
                        color_conv_row(tmp_buf, src, w, color_conv_rgb555_to_rgb24());
                        break;
                        
                    case pix_format_rgb565:
                        color_conv_row(tmp_buf, src, w, color_conv_rgb565_to_rgb24());
                        break;
                        
                    case pix_format_bgr24:
                        color_conv_row(tmp_buf, src, w, color_conv_bgr24_to_rgb24());
                        break;
                        
                    case pix_format_rgb24:
                        color_conv_row(tmp_buf, src, w, color_conv_rgb24_to_rgb24());
                        break;
                       
                    case pix_format_rgba32:
                        color_conv_row(tmp_buf, src, w, color_conv_rgba32_to_rgb24());
                        break;
                        
                    case pix_format_argb32:
                        color_conv_row(tmp_buf, src, w, color_conv_argb32_to_rgb24());
                        break;
                        
                    case pix_format_bgra32:
                        color_conv_row(tmp_buf, src, w, color_conv_bgra32_to_rgb24());
                        break;
                        
                    case pix_format_abgr32:
                        color_conv_row(tmp_buf, src, w, color_conv_abgr32_to_rgb24());
                        break;
                }
                fwrite(tmp_buf, 1, w * 3, fd);
            }
            delete [] tmp_buf;
            fclose(fd);
            return true;
        }
        return false;
    }



    //------------------------------------------------------------------------
    bool platform_support::create_img(unsigned idx, unsigned width, unsigned height)
    {
        if(idx < max_images)
        {
            if(width  == 0) width  = rbuf_window().width();
            if(height == 0) height = rbuf_window().height();
            delete [] m_specific->m_buf_img[idx];
            m_specific->m_buf_img[idx] = 
                new unsigned char[width * height * (m_bpp / 8)];

            m_rbuf_img[idx].attach(m_specific->m_buf_img[idx],
                                   width,
                                   height,
                                   m_flip_y ? 
                                       -width * (m_bpp / 8) : 
                                        width * (m_bpp / 8));
            return true;
        }
        return false;
    }


    //------------------------------------------------------------------------
    void platform_support::force_redraw()
    {
        m_specific->m_update_flag = true;
    }


    //------------------------------------------------------------------------
    void platform_support::message(const char* msg)
    {
        fprintf(stderr, "%s\n", msg);
    }

    //------------------------------------------------------------------------
    void platform_support::start_timer()
    {
        m_specific->m_sw_start = clock();
    }

    //------------------------------------------------------------------------
    double platform_support::elapsed_time() const
    {
        clock_t stop = clock();
        return double(stop - m_specific->m_sw_start) * 1000.0 / CLOCKS_PER_SEC;
    }


    //------------------------------------------------------------------------
    void platform_support::on_init() {}
    void platform_support::on_resize(int sx, int sy) {}
    void platform_support::on_idle() {}
    void platform_support::on_mouse_move(int x, int y, unsigned flags) {}
    void platform_support::on_mouse_button_down(int x, int y, unsigned flags) {}
    void platform_support::on_mouse_button_up(int x, int y, unsigned flags) {}
    void platform_support::on_key(int x, int y, unsigned key, unsigned flags) {}
    void platform_support::on_ctrl_change() {}
    void platform_support::on_draw() {}
    void platform_support::on_post_draw(void* raw_handler) {}



}


int agg_main(int argc, char* argv[]);


int main(int argc, char* argv[])
{
    return agg_main(argc, argv);
}






