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

#ifndef _PANEL_H_
#define _PANEL_H_

#include <gi/gi.h>
#include <gi/user_font.h>
//#include <X11/keysym.h>
//#include <X11/Xft/Xft.h>
//#include <X11/cursorfont.h>
//#include <X11/Xmu/WinUtil.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <string>

#ifdef NEEDS_BASENAME
#include <libgen.h>
#endif

#include "switchuser.h"
#include "const.h"
#include "image.h"


class Panel {
public:
    enum ActionType {
        Login,
        Console,
        Reboot,
        Halt,
        Exit,
        Suspend
    };
    enum FieldType {
        Get_Name,
        Get_Passwd
    };


    Panel(  gi_window_id_t root, Cfg* config,
          const std::string& themed);
    ~Panel();
    void OpenPanel();
    void ClosePanel();
    void ClearPanel();
    void Message(const std::string& text);
    void Error(const std::string& text);
    void EventHandler(const FieldType& curfield);
    std::string getSession();
    ActionType getAction(void) const;

    void Reset(void);
    void ResetName(void);
    void ResetPasswd(void);
    void SetName(const std::string& name);
    const std::string& GetName(void) const;
    const std::string& GetPasswd(void) const;
private:
    Panel();
    void Cursor(int visible);
    unsigned long GetColor(const char* colorname);
    void OnExpose(void);
    bool OnKeyPress(gi_msg_t& event);
    void ShowText();
    void SwitchSession();
    void ShowSession();

    void SlimDrawString8(gi_gc_ptr_t d, gi_color_t *color, gi_ufont_t *font,
                            int x, int y, const std::string& str,
                            gi_color_t* shadowColor,
                            int xOffset, int yOffset);

    Cfg* cfg;

    // Private data
    gi_window_id_t Win;
    gi_window_id_t Root;
    //Display* Dpy;
    int Scr;
    int X, Y;
    gi_gc_ptr_t RootGC;
    gi_gc_ptr_t WinGC;
    gi_ufont_t* font;
    gi_color_t inputshadowcolor;
    gi_color_t inputcolor;
    gi_color_t msgcolor;
    gi_color_t msgshadowcolor;
    gi_ufont_t* msgfont;
    gi_color_t introcolor;
    gi_ufont_t* introfont;
    gi_ufont_t* welcomefont;
    gi_color_t welcomecolor;
    gi_ufont_t* sessionfont;
    gi_color_t sessioncolor;
    gi_color_t sessionshadowcolor;
    gi_color_t welcomeshadowcolor;
    gi_ufont_t* enterfont;
    gi_color_t entercolor;
    gi_color_t entershadowcolor;
    ActionType action;
    FieldType field;
    
    // Username/Password
    std::string NameBuffer;
    std::string PasswdBuffer;
    std::string HiddenPasswdBuffer;

    // Configuration
    int input_name_x;
    int input_name_y;
    int input_pass_x;
    int input_pass_y;
    int inputShadowXOffset;
    int inputShadowYOffset;
    int input_cursor_height;
    int welcome_x;
    int welcome_y;
    int welcome_shadow_xoffset;
    int welcome_shadow_yoffset;
    int session_shadow_xoffset;
    int session_shadow_yoffset;
    int intro_x;
    int intro_y;
    int username_x;
    int username_y;
    int username_shadow_xoffset;
    int username_shadow_yoffset;
    int password_x;
    int password_y;
    std::string welcome_message;
    std::string intro_message;

    // Pixmap data
    gi_window_id_t PanelPixmap;

    Image* image;

    // For thesting themes
    bool testing;
    std::string themedir;

    // Session handling
    std::string session;

};

#endif


