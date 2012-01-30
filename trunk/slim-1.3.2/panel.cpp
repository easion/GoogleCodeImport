/* SLiM - Simple Login Manager
   Copyright (C) 1997, 1998 Per Liden
   Copyright (C) 2004-06 Simone Rota <sip@varlock.com>
   Copyright (C) 2004-06 Johannes Winkelmann <jw@tks6.net>
   Copyright (C) 2012 Easion <easion@hanxuantech.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include <sstream>
#include "panel.h"
#include "app.h"

using namespace std;
extern "C" int gi_ufont_param(gi_ufont_t* hfont,const char*str, int n, int *w,int *h);

void XftColorAllocName(const char *clrname, gi_color_t *cbin)
{
	gi_color_t c = 0;

	gi_parse_color(clrname, &c);
	*cbin = c;
	printf("XftColorAllocName: %s c=%x\n", clrname, c);
}

void XftColorFree(gi_color_t *cbin)
{
}

Panel::Panel(gi_window_id_t root, Cfg* config,
             const string& themedir) {
    // Set display
    //Dpy = dpy;
    //Scr = scr;
    Root = root;
    cfg = config;

    session = "";

    // Init GC
    //XGCValues gcv;
    //unsigned long gcm;
   // gcm = GCForeground|GCBackground|GCGraphicsExposures;
    //gcv.foreground = GetColor("black");
    //gcv.background = GetColor("white");
    //gcv.graphics_exposures = FALSE;
    //RootGC = gi_create_gc( Root, gcm, &gcv);
    RootGC = gi_create_gc( Root, NULL);

    font = gi_create_ufont( cfg->getOption("input_font").c_str());
    welcomefont = gi_create_ufont( cfg->getOption("welcome_font").c_str());
    introfont = gi_create_ufont( cfg->getOption("intro_font").c_str());
    enterfont = gi_create_ufont( cfg->getOption("username_font").c_str());
    msgfont = gi_create_ufont( cfg->getOption("msg_font").c_str());

    //Visual* visual = DefaultVisual( Scr);
    //Colormap colormap = DefaultColormap( Scr);
    // NOTE: using XftColorAllocValue() would be a better solution. Lazy me.
    XftColorAllocName( cfg->getOption("input_color").c_str(), &inputcolor);
    XftColorAllocName( cfg->getOption("input_shadow_color").c_str(), &inputshadowcolor);
    XftColorAllocName( cfg->getOption("welcome_color").c_str(), &welcomecolor);
    XftColorAllocName( cfg->getOption("welcome_shadow_color").c_str(), &welcomeshadowcolor);
    XftColorAllocName( cfg->getOption("username_color").c_str(), &entercolor);
    XftColorAllocName( cfg->getOption("username_shadow_color").c_str(), &entershadowcolor);
    XftColorAllocName( cfg->getOption("msg_color").c_str(), &msgcolor);
    XftColorAllocName( cfg->getOption("msg_shadow_color").c_str(), &msgshadowcolor);
    XftColorAllocName( cfg->getOption("intro_color").c_str(), &introcolor);
    XftColorAllocName( cfg->getOption("session_color").c_str(), &sessioncolor);
    XftColorAllocName( cfg->getOption("session_shadow_color").c_str(), &sessionshadowcolor);

    // Load properties from config / theme
    input_name_x = Cfg::string2int(cfg->getOption("input_name_x").c_str());
    input_name_y = Cfg::string2int(cfg->getOption("input_name_y").c_str());
    input_pass_x = Cfg::string2int(cfg->getOption("input_pass_x").c_str());
    input_pass_y = Cfg::string2int(cfg->getOption("input_pass_y").c_str());
    inputShadowXOffset =
        Cfg::string2int(cfg->getOption("input_shadow_xoffset").c_str());
    inputShadowYOffset =
        Cfg::string2int(cfg->getOption("input_shadow_yoffset").c_str());

    if (input_pass_x < 0 || input_pass_y < 0){ // single inputbox mode
        input_pass_x = input_name_x;
        input_pass_y = input_name_y;
    }

    // Load panel and background image
    string panelpng = "";
    panelpng = panelpng + themedir +"/panel.png";
    image = new Image;
    bool loaded = image->Read(panelpng.c_str());
    if (!loaded) { // try jpeg if png failed
        panelpng = themedir + "/panel.jpg";
        loaded = image->Read(panelpng.c_str());
        if (!loaded) {
            cerr << APPNAME
                 << ": could not load panel image for theme '"
                 << basename((char*)themedir.c_str()) << "'"
                 << endl;
            FAULT(ERR_EXIT);
        }
    }

    Image* bg = new Image();
    string bgstyle = cfg->getOption("background_style");
    if (bgstyle != "color") {
        panelpng = themedir +"/background.png";
        loaded = bg->Read(panelpng.c_str());
        if (!loaded) { // try jpeg if png failed
            panelpng = themedir + "/background.jpg";
            loaded = bg->Read(panelpng.c_str());
            if (!loaded){
                cerr << APPNAME
                     << ": could not load background image for theme '"
                     << basename((char*)themedir.c_str()) << "'"
                     << endl;
                FAULT(ERR_EXIT);
            }
        }
    }
    if (bgstyle == "stretch") {
        bg->Resize(gi_screen_width(), gi_screen_height());
    } else if (bgstyle == "tile") {
        bg->Tile(gi_screen_width(), gi_screen_height());
    } else if (bgstyle == "center") {
        string hexvalue = cfg->getOption("background_color");
        hexvalue = hexvalue.substr(1,6);
        bg->Center(gi_screen_width(),
                   gi_screen_height(),
                   hexvalue.c_str());
    } else { // plain color or error
        string hexvalue = cfg->getOption("background_color");
        hexvalue = hexvalue.substr(1,6);
        bg->Center(gi_screen_width(),
                   gi_screen_height(),
                   hexvalue.c_str());
    }

    string cfgX = cfg->getOption("input_panel_x");
    string cfgY = cfg->getOption("input_panel_y");
    X = Cfg::absolutepos(cfgX, gi_screen_width(), image->Width());
    Y = Cfg::absolutepos(cfgY, gi_screen_height(), image->Height());

    // Merge image into background
    image->Merge(bg, X, Y);
    delete bg;
    PanelPixmap = image->createPixmap(Root);

    // Read (and substitute vars in) the welcome message
    welcome_message = cfg->getWelcomeMessage();
    intro_message = cfg->getOption("intro_msg");
}

Panel::~Panel() {
    XftColorFree ( &inputcolor);
    XftColorFree ( &msgcolor);
    XftColorFree ( &welcomecolor);
    XftColorFree ( &entercolor);
    XftColorFree ( &sessioncolor);
    XftColorFree ( &sessionshadowcolor);
    gi_destroy_gc(RootGC);
    gi_delete_ufont( font);
    gi_delete_ufont( msgfont);
    gi_delete_ufont( introfont);
    gi_delete_ufont( welcomefont);
    gi_delete_ufont( enterfont);
    delete image;

}

void Panel::OpenPanel() {
    // Create window
    Win = gi_create_window( Root, X, Y,
                              image->Width(),
                              image->Height(),
                              GI_RGB( 128, 128  , 128   ), GI_FLAGS_NON_FRAME);

    // Events
    gi_set_events_mask(Win, GI_MASK_EXPOSURE | GI_MASK_KEY_DOWN|GI_MASK_KEY_UP);
    WinGC = gi_create_gc( Win, NULL);
	gi_set_focus_window(Win);

    // Set background
    gi_set_window_pixmap( Win, PanelPixmap,0);

    // Show window
    gi_show_window(Win);
    gi_move_window(Win, X, Y); // override wm positioning (for tests)

    // Grab keyboard
    //XGrabKeyboard( Win, FALSE, GrabModeAsync, GrabModeAsync, CurrentTime);

    //XFlush(Dpy);

}

void Panel::ClosePanel() {
    gi_ungrab_keyboard();
    gi_hide_window(Win);
    gi_destroy_window(Win);
    //XFlush(Dpy);
}

void Panel::ClearPanel() {
    session = "";
    Reset();
    gi_clear_window(Root, FALSE);
    gi_clear_window(Win,FALSE);
    Cursor(SHOW);
    ShowText();
    //XFlush(Dpy);
}

void Panel::Message(const string& text) {
    string cfgX, cfgY;
	int fw = 0,fh = 0;
    //XftDraw *draw = XftDrawCreate( Root,
    //                              DefaultVisual( Scr), 
	//	DefaultColormap( Scr));
    gi_ufont_param( msgfont, reinterpret_cast<const char*>(text.c_str()),
                    text.length(), &fw, &fh);
    cfgX = cfg->getOption("msg_x");
    cfgY = cfg->getOption("msg_y");
    int shadowXOffset =
        Cfg::string2int(cfg->getOption("msg_shadow_xoffset").c_str());
    int shadowYOffset =
        Cfg::string2int(cfg->getOption("msg_shadow_yoffset").c_str());

    int msg_x = Cfg::absolutepos(cfgX, gi_screen_width(), fw);
    int msg_y = Cfg::absolutepos(cfgY, gi_screen_height(), fh);

    SlimDrawString8 (RootGC, &msgcolor, msgfont, msg_x, msg_y,
                     text,
                     &msgshadowcolor,
                     shadowXOffset, shadowYOffset);
    //XFlush(Dpy);
    //XftDrawDestroy(draw);
}

void Panel::Error(const string& text) {
    ClosePanel();
    Message(text);
    sleep(ERROR_DURATION);
    OpenPanel();
    ClearPanel();
}


unsigned long Panel::GetColor(const char* colorname) {
    gi_color_t color;
    gi_window_info_t attributes;

    gi_get_window_info(Root, &attributes);
    color = 0;

    if(!gi_parse_color(  colorname, &color))
        cerr << APPNAME << ": can't parse color " << colorname << endl;
    //else if(!XAllocColor( attributes.colormap, &color))
    //    cerr << APPNAME << ": can't allocate color " << colorname << endl;

    return color;
}

void Panel::Cursor(int visible) {
    const char* text;
    int xx, yy, y2, cheight;
    const char* txth = "Wj"; // used to get cursor height

    switch(field) {
        case Get_Passwd:
            text = HiddenPasswdBuffer.c_str();
            xx = input_pass_x;
            yy = input_pass_y;
            break;

        case Get_Name:
            text = NameBuffer.c_str();
            xx = input_name_x;
            yy = input_name_y;
            break;
    }

	int fw = 0,fh = 0;
    gi_ufont_param( font, (char*)txth, strlen(txth), &fw,&fh);
    cheight = fh;
	int extents_y = 0;
	extents_y = gi_ufont_ascent_get(font)+ gi_ufont_descent_get(font);
    //y2 = yy - extents.y + fh;
    y2 = yy - extents_y + fh;
    gi_ufont_param( font, (char*)text, strlen(text), &fw,&fh);
    xx += fw;

    if(visible == SHOW) {
        gi_set_gc_foreground( WinGC,
                       GetColor(cfg->getOption("input_color").c_str()));
        //gi_draw_line( Win, RootGC,
        gi_draw_line(  WinGC,
                  xx+1, yy-cheight,
                  xx+1, y2);
    } else {
        gi_clear_area( Win, xx+1, yy-cheight,
                   1, y2-(yy-cheight)+1, false);
    }
}

void Panel::EventHandler(const Panel::FieldType& curfield) {
    gi_msg_t event;
    field=curfield;
    bool loop = true;
    OnExpose();
    while(loop) {
        gi_next_message( &event);

        switch(event.type) {
            case GI_MSG_EXPOSURE:
                OnExpose();
                break;

            case GI_MSG_KEY_DOWN:
                loop=OnKeyPress(event);
                break;
			case GI_MSG_KEY_UP:
				break;
        }
    }

    return;
}

void Panel::OnExpose(void) {
    //XftDraw *draw = XftDrawCreate( Win,
    //                    DefaultVisual( Scr), DefaultColormap( Scr));
    gi_clear_window( Win,FALSE);
    if (input_pass_x != input_name_x || input_pass_y != input_name_y){
        SlimDrawString8 (WinGC, &inputcolor, font, input_name_x, input_name_y,
                         NameBuffer,
                         &inputshadowcolor,
                         inputShadowXOffset, inputShadowYOffset);
        SlimDrawString8 (WinGC, &inputcolor, font, input_pass_x, input_pass_y,
                         HiddenPasswdBuffer,
                         &inputshadowcolor,
                         inputShadowXOffset, inputShadowYOffset);
    } else { //single input mode
        switch(field) {
            case Get_Passwd:
                SlimDrawString8 (WinGC, &inputcolor, font,
                                 input_pass_x, input_pass_y,
                                 HiddenPasswdBuffer,
                                 &inputshadowcolor,
                                 inputShadowXOffset, inputShadowYOffset);
                break;
            case Get_Name:
                SlimDrawString8 (WinGC, &inputcolor, font,
                                 input_name_x, input_name_y,
                                 NameBuffer,
                                 &inputshadowcolor,
                                 inputShadowXOffset, inputShadowYOffset);
                break;
        }
    }

    //XftDrawDestroy (draw);
    Cursor(SHOW);
    ShowText();
}

bool Panel::OnKeyPress(gi_msg_t& event) {
    char ascii;
    int keysym;
    //XComposeStatus compstatus;
    int xx;
    int yy;
    string text;
    string formerString = "";


	ascii = keysym = event.params[3];
    
    //XLookupString(&event.xkey, &ascii, 1, &keysym, &compstatus);
    switch(keysym){
        case G_KEY_F1:
            SwitchSession();
            return true;

        case G_KEY_F11:
            // Take a screenshot
            system(cfg->getOption("screenshot_cmd").c_str());
            return true;

        case G_KEY_RETURN:
        case G_KEY_ENTER:

            if (field==Get_Name){
                // Don't allow an empty username
                if (NameBuffer.empty()) return true;

                if (NameBuffer==CONSOLE_STR){
                    action = Console;
                } else if (NameBuffer==HALT_STR){
                    action = Halt;
                } else if (NameBuffer==REBOOT_STR){
                    action = Reboot;
                } else if (NameBuffer==SUSPEND_STR){
                    action = Suspend;
                } else if (NameBuffer==EXIT_STR){
                    action = Exit;
                } else{
                    action = Login;
                }
            };
            return false;
        default:
            break;
    };

    Cursor(HIDE);
    switch(keysym){
        case G_KEY_DELETE:
        case G_KEY_BACKSPACE:
            switch(field) {
                case GET_NAME:
                    if (! NameBuffer.empty()){
                        formerString=NameBuffer;
                        NameBuffer.erase(--NameBuffer.end());
                    };
                    break;
                case GET_PASSWD:
                    if (! PasswdBuffer.empty()){
                        formerString=HiddenPasswdBuffer;
                        PasswdBuffer.erase(--PasswdBuffer.end());
                        HiddenPasswdBuffer.erase(--HiddenPasswdBuffer.end());
                    };
                    break;
            };
            break;

        case G_KEY_w:
        case G_KEY_u:
            if (reinterpret_cast<gi_msg_t&>(event).body.message[3] & G_MODIFIERS_CTRL) {
                switch(field) {
                    case Get_Passwd:
                        formerString = HiddenPasswdBuffer;
                        HiddenPasswdBuffer.clear();
                        PasswdBuffer.clear();
                        break;

                    case Get_Name:
                        formerString = NameBuffer;
                        NameBuffer.clear();
                        break;
                };
                break;
            }
            // Deliberate fall-through
        
        default:
            if (isprint(ascii) ){
                switch(field) {
                    case GET_NAME:
                        formerString=NameBuffer;
                        if (NameBuffer.length() < INPUT_MAXLENGTH_NAME-1){
                            NameBuffer.append(&ascii,1);
                        };
                        break;
                    case GET_PASSWD:
                        formerString=HiddenPasswdBuffer;
                        if (PasswdBuffer.length() < INPUT_MAXLENGTH_PASSWD-1){
                            PasswdBuffer.append(&ascii,1);
                            HiddenPasswdBuffer.append("*");
                        };
                    break;
                };
            };
            break;
    };

	int fw = 0,fh = 0;
    //XftDraw *draw = XftDrawCreate( Win,
    //                              DefaultVisual( Scr), DefaultColormap( Scr));

   switch(field) {
        case Get_Name:
            text = NameBuffer;
            xx = input_name_x;
            yy = input_name_y;
            break;

        case Get_Passwd:
            text = HiddenPasswdBuffer;
            xx = input_pass_x;
            yy = input_pass_y;
            break;
    }

    if (!formerString.empty()){
        const char* txth = "Wj"; // get proper maximum height ?
        gi_ufont_param( font, reinterpret_cast<const char*>(txth), strlen(txth), &fw,&fh);
        int maxHeight = fh;

        gi_ufont_param( font, reinterpret_cast<const char*>(formerString.c_str()),
                        formerString.length(), &fw,&fh);
        int maxLength = fw;

        gi_clear_area( Win, xx-3, yy-maxHeight-3,
                   maxLength+6, maxHeight+6, false);
    }

    if (!text.empty()) {
        SlimDrawString8 (WinGC, &inputcolor, font, xx, yy,
                         text,
                         &inputshadowcolor,
                         inputShadowXOffset, inputShadowYOffset);
    }

    //XftDrawDestroy (draw);
    Cursor(SHOW);
    return true;
}

// Draw welcome and "enter username" message
void Panel::ShowText(){
    string cfgX, cfgY;
	int fw = 0,fh = 0;

    bool singleInputMode =
    input_name_x == input_pass_x &&
    input_name_y == input_pass_y;

    //XftDraw *draw = XftDrawCreate( Win,
     //                             DefaultVisual( Scr), DefaultColormap( Scr));
    /* welcome message */
    gi_ufont_param( welcomefont, (char*)welcome_message.c_str(),
                    strlen(welcome_message.c_str()), &fw, &fh);
    cfgX = cfg->getOption("welcome_x");
    cfgY = cfg->getOption("welcome_y");
    int shadowXOffset =
        Cfg::string2int(cfg->getOption("welcome_shadow_xoffset").c_str());
    int shadowYOffset =
        Cfg::string2int(cfg->getOption("welcome_shadow_yoffset").c_str());
    welcome_x = Cfg::absolutepos(cfgX, image->Width(), fw);
    welcome_y = Cfg::absolutepos(cfgY, image->Height(), fh);
    if (welcome_x >= 0 && welcome_y >= 0) {
        SlimDrawString8 (WinGC, &welcomecolor, welcomefont,
                         welcome_x, welcome_y,
                         welcome_message,
                         &welcomeshadowcolor, shadowXOffset, shadowYOffset);
    }

    /* Enter username-password message */
    string msg;
    if (!singleInputMode|| field == Get_Passwd ) {
        msg = cfg->getOption("password_msg");
        gi_ufont_param( enterfont, (char*)msg.c_str(),
                        strlen(msg.c_str()), &fw,&fh);
        cfgX = cfg->getOption("password_x");
        cfgY = cfg->getOption("password_y");
        int shadowXOffset =
            Cfg::string2int(cfg->getOption("username_shadow_xoffset").c_str());
        int shadowYOffset =
            Cfg::string2int(cfg->getOption("username_shadow_yoffset").c_str());
        password_x = Cfg::absolutepos(cfgX, image->Width(), fw);
        password_y = Cfg::absolutepos(cfgY, image->Height(), fh);
        if (password_x >= 0 && password_y >= 0){
            SlimDrawString8 (WinGC, &entercolor, enterfont, password_x, password_y,
                             msg, &entershadowcolor, shadowXOffset, shadowYOffset);
        }
    }
    if (!singleInputMode|| field == Get_Name ) {
        msg = cfg->getOption("username_msg");
        gi_ufont_param( enterfont, (char*)msg.c_str(),
                        strlen(msg.c_str()), &fw,&fh);
        cfgX = cfg->getOption("username_x");
        cfgY = cfg->getOption("username_y");
        int shadowXOffset =
            Cfg::string2int(cfg->getOption("username_shadow_xoffset").c_str());
        int shadowYOffset =
            Cfg::string2int(cfg->getOption("username_shadow_yoffset").c_str());
        username_x = Cfg::absolutepos(cfgX, image->Width(), fw);
        username_y = Cfg::absolutepos(cfgY, image->Height(), fh);
        if (username_x >= 0 && username_y >= 0){
            SlimDrawString8 (WinGC, &entercolor, enterfont, username_x, username_y,
                             msg, &entershadowcolor, shadowXOffset, shadowYOffset);
        }
    }
    //XftDrawDestroy(draw);
}

string Panel::getSession() {
    return session;
}

// choose next available session type
void Panel::SwitchSession() {
    session = cfg->nextSession(session);
    if (session.size() > 0) {
        ShowSession();
    }
}

// Display session type on the screen
void Panel::ShowSession() {
	string msg_x, msg_y;
    gi_clear_window(Root, FALSE);
    string currsession = cfg->getOption("session_msg") + " " + session;
    //XGlyphInfo extents;
	int fw = 0,fh = 0;
	
	sessionfont = gi_create_ufont( cfg->getOption("session_font").c_str());
    
	//XftDraw *draw = XftDrawCreate( Root,
    //                              DefaultVisual( Scr), DefaultColormap( Scr));
    gi_ufont_param( sessionfont, reinterpret_cast<const char*>(currsession.c_str()),
                    currsession.length(), &fw, &fh);
    msg_x = cfg->getOption("session_x");
    msg_y = cfg->getOption("session_y");
    int x = Cfg::absolutepos(msg_x, gi_screen_width(), fw);
    int y = Cfg::absolutepos(msg_y, gi_screen_height(), fh);
    int shadowXOffset =
        Cfg::string2int(cfg->getOption("session_shadow_xoffset").c_str());
    int shadowYOffset =
        Cfg::string2int(cfg->getOption("session_shadow_yoffset").c_str());

    SlimDrawString8(RootGC, &sessioncolor, sessionfont, x, y,
                    currsession, 
                    &sessionshadowcolor,
                    shadowXOffset, shadowYOffset);
    //XFlush(Dpy);
    //XftDrawDestroy(draw);
}


void Panel::SlimDrawString8(gi_gc_ptr_t gc, gi_color_t *color, gi_ufont_t *font,
                            int x, int y, const string& str,
                            gi_color_t* shadowColor,
                            int xOffset, int yOffset)
{
	//printf("SlimDrawString8 called: %s c=%x %d,%d \n", str.c_str(),*color,x,y );

	if (xOffset && yOffset) {
		gi_ufont_set_text_attr(gc,font,FALSE,*shadowColor,0);
		gi_set_gc_foreground(gc,*shadowColor);
		gi_ufont_draw(gc, font,
			reinterpret_cast<const char*>(str.c_str()),
			x+xOffset, y+yOffset,  str.length());
	}

	gi_ufont_set_text_attr(gc,font,FALSE,*color,0);
	gi_set_gc_foreground(gc,*color);
	gi_ufont_draw(gc, font,
		reinterpret_cast<const char*>(str.c_str()), x, y,  str.length());

#if 0
    if (xOffset && yOffset) {
        XftDrawString8(d, shadowColor, font, x+xOffset, y+yOffset,
                       reinterpret_cast<const FcChar8*>(str.c_str()),
			str.length());
    }
    XftDrawString8(d, color, font, x, y, 
		reinterpret_cast<const FcChar8*>(str.c_str()), str.length());
#endif
}


Panel::ActionType Panel::getAction(void) const{
    return action;
};


void Panel::Reset(void){
    ResetName();
    ResetPasswd();
};

void Panel::ResetName(void){
    NameBuffer.clear();
};

void Panel::ResetPasswd(void){
    PasswdBuffer.clear();
    HiddenPasswdBuffer.clear();
};

void Panel::SetName(const string& name){
    NameBuffer=name;
    return;
};
const string& Panel::GetName(void) const{
    return NameBuffer;
};
const string& Panel::GetPasswd(void) const{
    return PasswdBuffer;
};
