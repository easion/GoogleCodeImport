/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2010-2011 Razor team
 * Authors:
 *   Christopher "VdoP" Regali
 *   Alexander Sokoloff <sokoloff.a@gmail.ru>
 *
 * This program or library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 *
 * END_COMMON_COPYRIGHT_HEADER */



#include <QtGui/QX11Info>
#include <QtCore/QList>
#include <QtGui/QApplication>
#include <QtCore/QDebug>
#include <QtGui/QDesktopWidget>

#include <stdint.h>
#include "xfitman.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <gi/gi.h>
#include <gi/region.h>
#include <gi/property.h>

#include <QtGui/QWidget>
#if 1 //FIXME DPP



extern "C" int gi_fetch_window_caption_name (gi_window_id_t w, char **name);
extern "C" int gi_get_transient_for_hint(gi_window_id_t w,gi_window_id_t * propWindow);



/**
 * @file xfitman.cpp
 * @brief implements class Xfitman
 * @author Christopher "VdoP" Regali
 */

/*
 Some requests from Clients include type of the Client, for example the _NET_ACTIVE_WINDOW
 message. Currently the types can be 1 for normal applications, and 2 for pagers.
 See http://standards.freedesktop.org/wm-spec/latest/ar01s09.html#sourceindication
 */
#define SOURCE_NORMAL   1
#define SOURCE_PAGER    2

/*
  _NET_WM_STATE actions
 */
#define _NET_WM_STATE_REMOVE    0    // remove/unset property
#define _NET_WM_STATE_ADD       1    // add/set property
#define _NET_WM_STATE_TOGGLE    2    // toggle property


const XfitMan&  xfitMan()
{
    static XfitMan instance;
    return instance;
}

/**
 * @brief constructor: gets Display vars and registers us
 */
XfitMan::XfitMan()
{
#if 0
    getAtoms();
#endif
    root = GI_DESKTOP_WINDOW_ID;
#if 0
    screencount = 1;//ScreenCount(QX11Info::display());
#endif
}

/**
 * @brief moves a window to a new position
 */

void XfitMan::moveWindow(gi_window_id_t _win, int _x, int _y) const
{
    gi_move_window( _win, _x, _y);
}

/************************************************

 ************************************************/
bool XfitMan::getWindowProperty(gi_window_id_t window,
                       gi_atom_id_t atom,               // property
                       gi_atom_id_t reqType,            // req_type
                       unsigned long* resultLen,// nitems_return
                       unsigned char** result   // prop_return
                      ) const
{
    int  format;
    unsigned long type, rest;
    return gi_get_window_property( window, atom, 0, 4096, false,
                              reqType, (gi_atom_id_t*)&type, &format, resultLen, &rest,
                              result)  == 0;
}


/************************************************

 ************************************************/
bool XfitMan::getRootWindowProperty(gi_atom_id_t atom,    // property
                           gi_atom_id_t reqType,            // req_type
                           unsigned long* resultLen,// nitems_return
                           unsigned char** result   // prop_return
                          ) const
{
    return getWindowProperty(root, atom, reqType, resultLen, result);
}


/**
 * @brief this one gets the active application window.
 */
gi_window_id_t XfitMan::getActiveAppWindow() const
{
    gi_window_id_t window = getActiveWindow();
    if (window == 0)
        return 0;

    if (acceptWindow(window))
        return window;

    gi_window_id_t transFor = 0;
    if (gi_get_transient_for_hint( window, &transFor))
        return transFor;

    return 0;
}



/**
 * @brief returns the window that currently has inputfocus
 */
extern "C" gi_window_id_t gi_wm_get_active_window(void);

gi_window_id_t XfitMan::getActiveWindow() const
{ 
    gi_window_id_t result = 0;
	result = gi_wm_get_active_window();
    
    return result;
}


/**
 * @brief Get the number of desktops
 */

int XfitMan::getNumDesktop() const
{
    /*unsigned long length, *data;
    getRootWindowProperty(atom("_NET_NUMBER_OF_DESKTOPS"), GA_CARDINAL, &length, (unsigned char**) &data);
    if (data)
    {
        int res = data[0];
        free(data);
        return res;
    }*/
    return 1;
}

QStringList XfitMan::getDesktopNames() const
{  
    QStringList ret;
    unsigned long length;
    unsigned char *data = 0;

    if (getRootWindowProperty(atom("_NET_DESKTOP_NAMES"), atom("UTF8_STRING"), &length, &data))
    {
        if (data)
        {
            char* c = (char*)data;
            char* end = (char*)data + length;
            while (c < end)
            {
                ret << QString::fromUtf8(c);
                c += strlen(c) + 1; // for trailing \0
            }

            free(data);
        }
    }


    return ret;
}


QString XfitMan::getDesktopName(int desktopNum, const QString &defaultName) const
{
    QStringList names = getDesktopNames();
    if (desktopNum<0 || desktopNum>names.count()-1)
        return defaultName;

    return names.at(desktopNum);
}

/**
 * @brief resizes a window to the given dimensions
 */
void XfitMan::resizeWindow(gi_window_id_t _wid, int _width, int _height) const
{
    gi_resize_window( _wid, _width, _height);
}

static void *
get_prop_data (gi_window_id_t win, gi_atom_id_t prop,
	gi_atom_id_t type, int *items)
{
	gi_atom_id_t type_ret=0;
	int format_ret=0;
	unsigned long items_ret=0;
	unsigned long after_ret=0;
	unsigned char *prop_data = NULL;
	int err;

	prop_data = 0;
	err = gi_get_window_property ( win, prop, 0, 0x7fffffff, FALSE,
							  type, &type_ret, &format_ret, &items_ret,
							  &after_ret, &prop_data);
	if (items)
		*items = items_ret;
	return prop_data;
}

/**
 * @brief gets a windowpixmap from a window
 */

bool XfitMan::getClientIcon(gi_window_id_t _wid, QPixmap& _pixreturn) const
{
  int num = 0;
  uint32_t *data=NULL;

  data = (uint32_t *)get_prop_data(_wid, GA_NET_WM_ICON, GA_CARDINAL, &num);

  if (!data) {
	  return false;
  }

    int i;
    uint32_t *datal = data;
    uint32_t w, h;
	int stride = 0;

    w = *datal++;
    h = *datal++;
	stride = w * sizeof(uint32_t);

    QImage img (w, h, QImage::Format_ARGB32);
    for (int i=0; i<img.byteCount()/4; ++i)
        ((uint*)img.bits())[i] = datal[i+2];

    _pixreturn = QPixmap::fromImage(img);
    free(data);

    return true;
}



/**
 * @brief destructor
 */
XfitMan::~XfitMan()
{
}



/**
 * @brief returns a windowname and sets _nameSource to the finally used gi_atom_id_t
 */

//i got the idea for this from taskbar-plugin of LXPanel - so credits fly out :)
QString XfitMan::getName(gi_window_id_t _wid) const
{
    QString name = "";
	void *data;
    //first try the modern net-wm ones
    
    if (name.isEmpty())
    {
        int ok = gi_fetch_window_caption_name( _wid, (char**) &data);
        name = QString((char*) data);
        if (0 != ok) free(data);
    }

    return name;
}


#if 0
/**
 * @brief this add(1) / removes (0) / toggles (2) the _NET_WM_STATE_XXXX flag for a
 *  specified window
 * @param[in] _wid windowId for the action
 * @param[in] _atomcode the QString-code for atomMap - state
 * @param[in] _action the action to do (add 1, remove 0, toggle 2)
 */
void XfitMan::setClientStateFlag(gi_window_id_t _wid, const QString & _atomcode, int _action) const
{
    clientMessage(_wid, atomMap["net_wm_state"],_action,atomMap[_atomcode],0,0,0);
}
#endif

/**
 * @brief sends a clientmessage to a window
 */
int XfitMan::clientMessage(gi_window_id_t _wid, gi_atom_id_t _msg,
                            unsigned long data0,
                            unsigned long data1,
                            unsigned long data2,
                            unsigned long data3,
                            unsigned long data4) const
{
    gi_msg_t msg;
    msg.ev_window = _wid;
    msg.type = GI_MSG_CLIENT_MSG;
    msg.body.client.client_type = _msg;
    //msg.send_event = true;
    //msg.display = QX11Info::display();
    msg.body.client.client_format = 32;
    msg.params[0] = data0;
    msg.params[1] = data1;
    msg.params[2] = data2;
    msg.params[3] = data3;
    msg.body.message[0] = data4;
    if (gi_send_event( root, FALSE, (GI_MASK_CLIENT_MSG) , (gi_msg_t *) &msg) == 0)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}



/***********************************************

 ***********************************************/
WindowAllowedActions XfitMan::getAllowedActions(gi_window_id_t window) const
{
    WindowAllowedActions actions = { };
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
#if 0
    unsigned long len;
    unsigned long *data;
    if (getWindowProperty(window, atom("_NET_WM_ALLOWED_ACTIONS"), GA_ATOM, &len, (unsigned char**) &data))
    {
        for (unsigned long i=0; i<len; ++i)
        {
            if (data[i] == atom("_NET_WM_ACTION_MOVE"))             actions.Move = true;            else
            if (data[i] == atom("_NET_WM_ACTION_RESIZE"))           actions.Resize = true;          else
            if (data[i] == atom("_NET_WM_ACTION_MINIMIZE"))         actions.Minimize = true;        else
            if (data[i] == atom("_NET_WM_ACTION_SHADE"))            actions.Shade = true;           else
            if (data[i] == atom("_NET_WM_ACTION_STICK"))            actions.Stick = true;           else
            if (data[i] == atom("_NET_WM_ACTION_MAXIMIZE_HORZ"))    actions.MaximizeHoriz = true;   else
            if (data[i] == atom("_NET_WM_ACTION_MAXIMIZE_VERT"))    actions.MaximizeVert = true;    else
            if (data[i] == atom("_NET_WM_ACTION_FULLSCREEN"))       actions.FullScreen = true;      else
            if (data[i] == atom("_NET_WM_ACTION_CHANGE_DESKTOP"))   actions.ChangeDesktop = true;   else
            if (data[i] == atom("_NET_WM_ACTION_CLOSE"))            actions.Close = true;           else
            if (data[i] == atom("_NET_WM_ACTION_ABOVE"))            actions.AboveLayer = true;      else
            if (data[i] == atom("_NET_WM_ACTION_BELOW"))            actions.BelowLayer = true;
        }
        free(data);
    }
#endif

 actions.Move = true;            
 actions.Resize = true;          
 actions.Minimize = true;        
 //actions.Shade = true;           
 //actions.Stick = true;           
 actions.MaximizeHoriz = true;   
 //actions.MaximizeVert = true;    
 actions.FullScreen = true;      
 //actions.ChangeDesktop = true;   
 actions.Close = true;           
 actions.AboveLayer = true;      
 actions.BelowLayer = true;

    return actions;
}


WindowState XfitMan::getWindowState(gi_window_id_t window) const
{
    WindowState state = { };
	gi_window_info_t info;

	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
	gi_get_window_info(window,&info);
	if (info.flags & GI_FLAGS_TEMP_WINDOW)
	{
		state.Modal = true;  
		state.SkipTaskBar = true;  
	}

	if (!info.mapped )
	{
		state.Hidden = true;  
	}
#if 0
    unsigned long len;
    unsigned long *data;
    if (getWindowProperty(window, atom("_NET_WM_STATE"), GA_ATOM, &len, (unsigned char**) &data))
    {
        for (unsigned long i=0; i<len; ++i)
        {
            if (data[i] == atom("_NET_WM_STATE_MODAL"))             
				state.Modal = true;             else
            if (data[i] == atom("_NET_WM_STATE_STICKY"))            
				state.Sticky = true;            else
            if (data[i] == atom("_NET_WM_STATEZED_VERT"))    
				state.MaximizedVert = true;     else
            if (data[i] == atom("_NET_WM_STATE_MAXIMIZED_HORZ"))    
				state.MaximizedHoriz = true;    else
            if (data[i] == atom("_NET_WM_STATE_SHADED"))            
				state.Shaded = true;            else
            if (data[i] == atom("_NET_WM_STATE_SKIP_TASKBAR"))      
				state.SkipTaskBar = true;       else
            if (data[i] == atom("_NET_WM_STATE_SKIP_PAGER"))        
				state.SkipPager = true;         else
            if (data[i] == atom("_NET_WM_STATE_HIDDEN"))            
				state.Hidden = true;            else
            if (data[i] == atom("_NET_WM_STATE_FULLSCREEN"))        
				state.FullScreen = true;        else
            if (data[i] == atom("_NET_WM_STATE_ABOVE"))             
				state.AboveLayer = true;        else
            if (data[i] == atom("_NET_WM_STATE_BELOW"))             
				state.BelowLayer = true;        else
            if (data[i] == atom("_NET_WM_STATE_DEMANDS_ATTENTION")) 
				state.Attention = true;
        }
        free(data);
    }
#endif

    return state;

}



/**
 * @brief returns true if a window has its hidden_flag set
 */

bool XfitMan::isHidden(gi_window_id_t _wid) const
{
    return getWindowState(_wid).Hidden;
}

#if 0
bool XfitMan::requiresAttention(gi_window_id_t _wid) const
{
    return getWindowState(_wid).Attention;
}
#endif


gi_atom_id_t XfitMan::atom(const char* atomName)
{
    static QHash<QString, gi_atom_id_t> hash;

    if (hash.contains(atomName))
        return hash.value(atomName);

    gi_atom_id_t atom = gi_intern_atom( atomName, false);
    hash[atomName] = atom;
    return atom;
}

#if 0
/**
 * @brief gets the used atoms into a QMap for further usage
 */
void XfitMan::getAtoms() const
{
    atomMap["net_wm_strut"] = gi_intern_atom( "_NET_WM_STRUT", FALSE);
    atomMap["net_wm_strut_partial"] = gi_intern_atom( "_NET_WM_STRUT_PARTIAL", FALSE);
    atomMap["net_client_list"] = gi_intern_atom( "_NET_CLIENT_LIST", FALSE);
    atomMap["net_wm_visible_name"] =gi_intern_atom( "_NET_WM_VISIBLE_NAME", FALSE);
    atomMap["net_wm_name"] =gi_intern_atom( "_NET_WM_NAME", FALSE);
    atomMap["xa_wm_name"] =gi_intern_atom( "GA_WM_NAME", FALSE);
    atomMap["utf8"] = gi_intern_atom( "UTF8_STRING", FALSE);
    atomMap["net_wm_icon"] = gi_intern_atom( "_NET_WM_ICON", FALSE);
    atomMap["net_wm_state_hidden"] = gi_intern_atom( "_NET_WM_STATE_HIDDEN", FALSE);
    atomMap["net_wm_state_fullscreen"] = gi_intern_atom( "_NET_WM_STATE_FULLSCREEN", FALSE);
    atomMap["net_wm_state"] = gi_intern_atom( "_NET_WM_STATE", FALSE);
    atomMap["net_current_desktop"] = gi_intern_atom( "_NET_CURRENT_DESKTOP", FALSE);
    atomMap["net_wm_desktop"] = gi_intern_atom("_NET_WM_DESKTOP", FALSE);
    atomMap["net_active_window"] = gi_intern_atom( "_NET_ACTIVE_WINDOW", FALSE);
    atomMap["_win_workspace"] = gi_intern_atom( "_WIN_WORKSPACE", FALSE);
    atomMap["net_number_of_desktops"] = gi_intern_atom( "_NET_NUMBER_OF_DESKTOPS", FALSE);

    atomMap["net_wm_window_type"] = gi_intern_atom( "_NET_WM_WINDOW_TYPE", FALSE);
    atomMap["net_wm_window_type_normal"] = gi_intern_atom( "_NET_WM_WINDOW_TYPE_NORMAL", FALSE);
    atomMap["net_wm_window_type_desktop"] = gi_intern_atom( "_NET_WM_WINDOW_TYPE_DESKTOP", FALSE);
    atomMap["net_wm_window_type_dock"] = gi_intern_atom( "_NET_WM_WINDOW_TYPE_DOCK", FALSE);
    atomMap["net_wm_window_type_splash"] = gi_intern_atom( "_NET_WM_WINDOW_TYPE_SPLASH", FALSE);
    atomMap["kde_net_wm_window_type_override"] = gi_intern_atom( "_KDE_NET_WM_WINDOW_TYPE_OVERRIDE", FALSE);

    char atomname[20] = {0};
    qsnprintf(atomname, 20, "_NET_SYSTEM_TRAY_S%d", 0);
    atomMap["net_system_tray"] = gi_intern_atom( atomname, FALSE);
    atomMap["net_system_tray_opcode"] = gi_intern_atom( "_NET_SYSTEM_TRAY_OPCODE", FALSE);
    atomMap["net_manager"] = gi_intern_atom( "MANAGER", FALSE);
    atomMap["net_message_data"] = gi_intern_atom( "_NET_SYSTEM_TRAY_MESSAGE_DATA", FALSE);
    atomMap["xrootpmap"] = gi_intern_atom( "_XROOTPMAP_ID", FALSE);
    atomMap["esetroot"] = gi_intern_atom( "ESETROOT_PMAP_ID", FALSE);
    atomMap["net_wm_window_demands_attention"] = gi_intern_atom( "_NET_WM_STATE_DEMANDS_ATTENTION", FALSE);
}
#endif

AtomList XfitMan::getWindowType(gi_window_id_t window) const
{
    AtomList result;

    unsigned long length, *data;
    length=0;
    if (!getWindowProperty(window, atom("_NET_WM_WINDOW_TYPE"), 
		(gi_atom_id_t)G_ANY_PROP_TYPE, &length, (unsigned char**) &data))
        return result;
#if 1
    for (unsigned int i = 0; i < length; i++)
        result.append(data[i]);
#endif

    free(data);
    return result;
}


/**
 * @brief rejects a window from beeing listed
 */
bool XfitMan::acceptWindow(gi_window_id_t window) const
{
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
	//return true;
    {
        AtomList types = getWindowType(window);
        AtomList ignoreList;
        ignoreList  << atom("_NET_WM_WINDOW_TYPE_DESKTOP")
                    << atom("_NET_WM_WINDOW_TYPE_DOCK")
                    << atom("_NET_WM_WINDOW_TYPE_SPLASH")
                    << atom("_NET_WM_WINDOW_TYPE_TOOLBAR")
                    << atom("_NET_WM_WINDOW_TYPE_MENU")
                    << atom("_KDE_NET_WM_WINDOW_TYPE_OVERRIDE");
                    
        foreach (gi_atom_id_t i, ignoreList)
        {
            if (types.contains(i))
                return false;
        }

        WindowState state = getWindowState(window);
        if (state.SkipTaskBar)  return false;
    }

    gi_window_id_t transFor = 0;
    // WM_TRANSIENT_FOR hint not set - normal window
    if (!gi_get_transient_for_hint( window, &transFor))
        return true;

    if (transFor == 0)      return true;
    if (transFor == window) return true;
    if (transFor == root)   return true;

     AtomList transForTypes = getWindowType(transFor);
     //return !transForTypes.contains(atom("_NET_WM_WINDOW_TYPE_NORMAL"));
     return false;
}

int netwm_get_mapped_windows(gi_window_id_t **windows)
{
  unsigned long n=0, extra;

  gi_atom_id_t real;
  int format;
  unsigned char* prop = 0;

  int status = gi_get_window_property( GI_DESKTOP_WINDOW_ID, 
      GA_NET_CLIENT_LIST, 0L, 0x7fffffff, FALSE, GA_WINDOW, &real, &format, &n, &extra, 
      (unsigned char**)&prop);

  if(status != 0 || !prop)
    return 0;

  *windows = (gi_window_id_t*)prop;

  return n;
}



/**
 * @brief gets a client list
 */
QList<gi_window_id_t> XfitMan::getClientList() const
{
    //initialize the parameters for the gi_get_window_property call
    unsigned long length;//, *data;
	gi_window_id_t *win = NULL;
    length=0;

    /**
     * @todo maybe support multiple desktops here!
     */
    QList<gi_window_id_t> output;
	length = netwm_get_mapped_windows(&win);
	for (unsigned int i = 0; i < length; i += 2){
        output.append(win[i]);
		printf("dpp client append = %d/%d, %d,%d\n",i,length, win[i],win[i+1] );
	}
	free(win);

    return output;
}


/**
 * @brief returns the current desktop
 */
int XfitMan::getActiveDesktop() const
{
    int res = -2;
    unsigned long length, *data;
    if (getRootWindowProperty(atom("_NET_CURRENT_DESKTOP"), GA_CARDINAL, &length, (unsigned char**) &data))
    {
        if (data)
        {
            res = data[0];
            free(data);
        }
    }

    return res;
}


/**
 * @brief gets the desktop of the windows _wid
 */
int XfitMan::getWindowDesktop(gi_window_id_t _wid) const
{
    int  res = -1;
    unsigned long length, *data;
    // so we try to use net_wm_desktop first, but if the
    // system does not use net_wm standard we use win_workspace!
    if (getWindowProperty(_wid, atom("_NET_WM_DESKTOP"), GA_CARDINAL, &length, (unsigned char**) &data))
    {
        if (!data)
            return res;
        res = data[0];
        free(data);
    }
    else
    {
        if (getWindowProperty(_wid, atom("_WIN_WORKSPACE"), GA_CARDINAL, &length, (unsigned char**) &data))
        {
            if (!data)
                return res;
            res = data[0];
            free(data);
        }
    }

    return res;
}


/**
 * @brief moves a window to a specified desktop
 */

void XfitMan::moveWindowToDesktop(gi_window_id_t _wid, int _display) const
{
    //clientMessage(_wid, atom("_NET_WM_DESKTOP"), (unsigned long) _display,0,0,0,0);
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
}


/**
 * @brief raises windows _wid
 */
void XfitMan::raiseWindow(gi_window_id_t _wid) const
{
	gi_raise_window(_wid);
	gi_show_window(_wid);
	//gi_wm_activate_window(_wid);
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
    //clientMessage(_wid, atom("_NET_ACTIVE_WINDOW"),
     //             SOURCE_PAGER);
}


/************************************************

 ************************************************/
void XfitMan::minimizeWindow(gi_window_id_t _wid) const
{
 	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
   //clientMessage(_wid, atom("WM_CHANGE_STATE"),
     //             IconicState);
	 gi_hide_window(_wid);
}


/************************************************

 ************************************************/
void XfitMan::maximizeWindow(gi_window_id_t _wid, MaximizeDirection direction) const
{
    /*gi_atom_id_t atom1 = 0, atom2= 0;
    switch (direction)
    {
        case MaximizeHoriz:
            atom1 = atom("_NET_WM_STATE_MAXIMIZED_HORZ");
            break;

        case MaximizeVert:
            atom1 = atom("_NET_WM_STATE_MAXIMIZED_VERT");
            break;

        case MaximizeBoth:
            atom1 = atom("_NET_WM_STATE_MAXIMIZED_VERT");
            atom2 = atom("_NET_WM_STATE_MAXIMIZED_HORZ");
            break;

    }

    clientMessage(_wid, atom("_NET_WM_STATE"),
                  _NET_WM_STATE_ADD,
                  atom1, atom2,
                  SOURCE_PAGER);
	*/
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
	gi_wm_window_maximize(_wid,TRUE);
	gi_show_window(_wid);
    raiseWindow(_wid);
}


/************************************************

 ************************************************/
void XfitMan::deMaximizeWindow(gi_window_id_t _wid) const
{
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
	gi_wm_window_maximize(_wid,FALSE);
	gi_show_window(_wid);
    /*clientMessage(_wid, atom("_NET_WM_STATE"),
                  _NET_WM_STATE_REMOVE,
                  atom("_NET_WM_STATE_MAXIMIZED_VERT"),
                  atom("_NET_WM_STATE_MAXIMIZED_HORZ"),
                  SOURCE_PAGER);
				  */
}

/************************************************

 ************************************************/
void XfitMan::shadeWindow(gi_window_id_t _wid, bool shade) const
{
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
    clientMessage(_wid, atom("_NET_WM_STATE"),
                  shade ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE,
                  atom("_NET_WM_STATE_SHADED"),
                  0,
                  SOURCE_PAGER);
}


/************************************************

 ************************************************/
void XfitMan::closeWindow(gi_window_id_t _wid) const
{
	gi_post_quit_message(_wid);
    //clientMessage(_wid, atom("_NET_CLOSE_WINDOW"),
     //             0, // Timestamp
    //              SOURCE_PAGER);
}


/************************************************

 ************************************************/
void XfitMan::setWindowLayer(gi_window_id_t _wid, XfitMan::Layer layer) const
{
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);

    /*ulong aboveAction = (layer == LayerAbove) ?
        _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;

    ulong belowAction = (layer == LayerBelow) ?
        _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;

    clientMessage(_wid, atom("_NET_WM_STATE"),
                  aboveAction,
                  atom("_NET_WM_STATE_ABOVE"),
                  0,
                  SOURCE_PAGER);

    clientMessage(_wid, atom("_NET_WM_STATE"),
                  belowAction,
                  atom("_NET_WM_STATE_BELOW"),
                  0,
                  SOURCE_PAGER);
				  */
}

/**
 * @brief changes active desktop to _desktop
 */
void XfitMan::setActiveDesktop(int _desktop) const
{
    clientMessage(root, atom("_NET_CURRENT_DESKTOP"), (unsigned long) _desktop,0,0,0,0);
}

#if 0
/**
 * @brief this sets a window as selection owner for a specified atom - checks for success then sends the clientmessage
 */
void XfitMan::setSelectionOwner(gi_window_id_t _wid, const QString & _selection, const QString & _manager) const
{
    if (getSelectionOwner(_selection))
    {
        qWarning() << "Another tray started";
        return;
    }

    gi_set_selection_owner( atomMap.value(_selection), _wid, CurrentTime);
    if (getSelectionOwner(_selection)== _wid)
        clientMessage(QApplication::desktop()->winId(),atomMap.value(_manager),CurrentTime,atomMap.value(_selection),_wid,0,0);
}
#endif

#if 0
/**
 * @brief returns the owning window of selection _selection
 */
gi_window_id_t XfitMan::getSelectionOwner(const QString & _selection) const
{
    return gi_get_selection_owner( atomMap.value(_selection));
}
#endif

/**
 * @brief sets net_wm_strut_partial = our reserved panelspace for the mainbar!
 */
void XfitMan::setStrut(gi_window_id_t _wid,
                       int left, int right,
                       int top,  int bottom,

                       int leftStartY,   int leftEndY,
                       int rightStartY,  int rightEndY,
                       int topStartX,    int topEndX,
                       int bottomStartX, int bottomEndX
                       ) const
{
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
    //qDebug() << "XfitMan: Trying to set STRUT_PARTIAL for panel!";
    //prepare strutsize
    memset(desstrut,0,sizeof(desstrut));
    //prepare the array
    //it has format:
    /*
     * left, right, top, bottom, left_start_y, left_end_y,
     * right_start_y, right_end_y, top_start_x, top_end_x, bottom_start_x,
     * bottom_end_x
     *
     */

    //so we take our panelsize from the bottom up
    desstrut[0] = left; desstrut[1] = right;
    desstrut[2] = top;  desstrut[3] = bottom;

    desstrut[4] = leftStartY;    desstrut[5] = leftEndY;
    desstrut[6] = rightStartY;   desstrut[7] = rightEndY;
    desstrut[8] = topStartX;     desstrut[9] = topEndX;
    desstrut[10] = bottomStartX; desstrut[11] = bottomEndX;

    //now we can change that property right
    gi_change_property( _wid , atom("_NET_WM_STRUT_PARTIAL"),
                    GA_CARDINAL, 32, G_PROP_MODE_Replace,  (unsigned char *) desstrut, 12  );

    //now some wm do not support net_wm_strut_partial but only net_wm_strut, so we also
    // send that one too xdg-std says: if you get a strut_partial ignore all following
    // struts! so if this msg is recognized its ok if not, we dont care either

    gi_change_property( _wid, atom("_NET_WM_STRUT"),
                    GA_CARDINAL, 32, G_PROP_MODE_Replace, (unsigned char*) desstrut, 4);
}

#if 0
/**
 * @brief this unsets the strut set for panel
 */
void XfitMan::unsetStrut(gi_window_id_t _wid) const
{
    gi_delete_property( _wid, atom("_NET_WM_STRUT"));
    gi_delete_property( _wid, atom("_NET_WM_STRUT_PARTIAL"));
}
#endif

#ifdef DEBUG
/************************************************

 ************************************************/
QString XfitMan::debugWindow(gi_window_id_t wnd)
{
    if (!wnd)
        return QString("[%1]").arg(wnd,8, 16);

    QString typeStr;
    int  format;
    unsigned long type, length, rest, *data;
    length=0;
    if (gi_get_window_property( wnd, gi_intern_atom( "_NET_WM_WINDOW_TYPE", FALSE),
                           0, 4096, FALSE, G_ANY_PROP_TYPE, &type, &format,
                           &length, &rest,(unsigned char**) &data) == 0)
    {
        for (unsigned int i = 0; i < length; i++)
        {
            char* aname = gi_intern_atom( data[i]);
            typeStr = typeStr + " " + aname;
            free(aname);
        }
    }
    else
        typeStr ="ERROR";

    return QString("[%1] %2 %3").arg(wnd,8, 16).arg(xfitMan().getName(wnd)).arg(typeStr);
}
#endif


/************************************************

 ************************************************/
const QRect XfitMan::availableGeometry(int screen) const
{
    QDesktopWidget *d = QApplication::desktop();

    if (screen < 0 || screen >= d->screenCount())
        screen = d->primaryScreen();
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);

    QRect available = d->screenGeometry(screen);

    // Iterate over all the client windows and subtract from the available
    // area the space they reserved on the edges (struts).
    // Note: _NET_WORKAREA is not reliable as it exposes only one
    // rectangular area spanning all screens.
    //Display *display = QX11Info::display();
    //int x11Screen = d->isVirtualDesktop() ? DefaultScreen(display) : screen;

    gi_atom_id_t ret;
    int format, status;
    uchar* data = 0;
    ulong nitems, after;

    status = gi_get_window_property( GI_DESKTOP_WINDOW_ID,
                                atom("_NET_CLIENT_LIST"), 0L, ~0L, FALSE, GA_WINDOW,
                                &ret, &format, &nitems, &after, &data);

    if (status == 0 && ret == GA_WINDOW && format == 32 && nitems)
    {
        const QRect desktopGeometry = d->rect();

        gi_window_id_t* xids = (gi_window_id_t*) data;
        for (quint32 i = 0; i < nitems; ++i)
        {
            ulong nitems2;
            uchar* data2 = 0;
            status = gi_get_window_property(xids[i],
                                        atom("_NET_WM_STRUT_PARTIAL"), 0, 12, FALSE, GA_CARDINAL,
                                        &ret, &format, &nitems2, &after, &data2);

            if (status == 0 && ret == GA_CARDINAL && format == 32 && nitems2 == 12)
            {
                ulong* struts = (ulong*) data2;

                QRect left(desktopGeometry.x(),
                           desktopGeometry.y() + struts[4],
                           struts[0],
                           struts[5] - struts[4]);
                if (available.intersects(left))
                    available.setX(left.width());

                QRect right(desktopGeometry.x() + desktopGeometry.width() - struts[1],
                            desktopGeometry.y() + struts[6],
                            struts[1],
                            struts[7] - struts[6]);
                if (available.intersects(right))
                    available.setWidth(right.x() - available.x());

                QRect top(desktopGeometry.x() + struts[8],
                          desktopGeometry.y(),
                          struts[9] - struts[8],
                          struts[2]);
                if (available.intersects(top))
                    available.setY(top.height());

                QRect bottom(desktopGeometry.x() + struts[10],
                             desktopGeometry.y() + desktopGeometry.height() - struts[3],
                             struts[11] - struts[10],
                             struts[3]);
                if (available.intersects(bottom))
                    available.setHeight(bottom.y() - available.y());
            }
            if (data2)
                free(data2);
        }
    }
    if (data)
        free(data);

    return available;
}


/************************************************

 ************************************************/
const QRect XfitMan::availableGeometry(const QWidget *widget) const
{
    if (!widget)
    {
        qWarning("XfitMan::availableGeometry(): Attempt "
                 "to get the available geometry of a null widget");
        return QRect();
    }

    return availableGeometry(QApplication::desktop()->screenNumber(widget));
}


/************************************************

 ************************************************/
const QRect XfitMan::availableGeometry(const QPoint &point) const
{
    return availableGeometry(QApplication::desktop()->screenNumber(point));
}


/************************************************
 The gi_window_id_t Manager MUST set this property on the root window to be the ID of a child
 window created by himself, to indicate that a compliant window manager is active.

 http://standards.freedesktop.org/wm-spec/wm-spec-latest.html#id2550836
 ************************************************/
bool XfitMan::isWindowManagerActive() const
{
    //gi_window_id_t *wins;

    //getRootWindowProperty(atom("_NET_SUPPORTING_WM_CHECK"), GA_WINDOW, &length, (unsigned char**) &wins);
#if 0
    gi_atom_id_t type;
    unsigned long length;
    gi_window_id_t *wins;
    int format;
    unsigned long after;

    gi_get_window_property( root, atom("_NET_SUPPORTING_WM_CHECK"),
                       0, LONG_MAX,
                       false, GA_WINDOW, &type, &format, &length,
                       &after, (unsigned char **)&wins);

    if ( type == GA_WINDOW && length > 0 && wins[0] != 0 )
    {
        free(wins);
        return true;
    }
    return false;
#else
	return true;
#endif
}

#endif

