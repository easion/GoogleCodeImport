/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
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

 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * END_COMMON_COPYRIGHT_HEADER */

/********************************************************************
  Inspired by freedesktops tint2 ;)

*********************************************************************/


// Warning: order of those include is important.
#include <QApplication>
#include <QtDebug>
#include "trayicon.h"
#include <QX11Info>
#include "razortray.h"
#include <gi/gi.h>
#include <gi/property.h>
#include "razorqt/xfitman.h"


#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define XEMBED_EMBEDDED_NOTIFY  0
#define XEMBED_MAPPED          (1 << 0)


EXPORT_RAZOR_PANEL_PLUGIN_CPP(RazorTray)


/************************************************

 ************************************************/
RazorTray::RazorTray(const RazorPanelPluginStartInfo* startInfo, QWidget* parent):
    RazorPanelPlugin(startInfo, parent),
    mValid(false),
    mTrayId(0),
    mDamageEvent(0),
    mDamageError(0),
    mIconSize(TRAY_ICON_SIZE_DEFAULT, TRAY_ICON_SIZE_DEFAULT)
{
    setObjectName("Tray");
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    mValid = startTray();
}


/************************************************

 ************************************************/
RazorTray::~RazorTray()
{
    stopTray();
}


/************************************************

 ************************************************/
void RazorTray::pfEventFilter(gi_msg_t* event)
{
    TrayIcon* icon;

	return;

    switch (event->type)
    {
        case GI_MSG_CLIENT_MSG:
            clientMessageEvent(event);
            break;

//        case ConfigureNotify:
//            icon = findIcon(event->xconfigure.window);
//            if (icon)
//                icon->configureEvent(&(event->xconfigure));
//            break;

        case GI_MSG_WINDOW_DESTROY:
            icon = findIcon(event->ev_window);
            if (icon)
            {
                mIcons.removeAll(icon);
                delete icon;
            }
            break;

        default:
            /*if (event->type == mDamageEvent + XDamageNotify)
            {
                XDamageNotifyEvent* dmg = reinterpret_cast<XDamageNotifyEvent*>(event);
                icon = findIcon(dmg->drawable);
                if (icon)
                    icon->update();
            }*/
            break;
    }
}


/************************************************

 ************************************************/
void RazorTray::clientMessageEvent(gi_msg_t* e)
{
    unsigned long opcode;
    opcode = e->params[1];
    gi_window_id_t id;

    switch (opcode)
    {
        case SYSTEM_TRAY_REQUEST_DOCK:
            id = e->params[2];
            if (id)
                addIcon(id);
            break;


        case SYSTEM_TRAY_BEGIN_MESSAGE:
        case SYSTEM_TRAY_CANCEL_MESSAGE:
            qDebug() << "we don't show baloons messages.";
            break;


        default:
            //if (opcode == xfitMan().atom("_NET_SYSTEM_TRAY_MESSAGE_DATA"))
             //   qDebug() << "message from dockapp:" << e->ev_window;
//            else
//                qDebug() << "SYSTEM_TRAY : unknown message type" << opcode;
            break;
    }
}



/************************************************

 ************************************************/
TrayIcon* RazorTray::findIcon(gi_window_id_t id)
{
    foreach(TrayIcon* icon, mIcons)
    {
        if (icon->iconId() == id || icon->windowId() == id)
            return icon;
    }
    return 0;
}


/************************************************

************************************************/
void RazorTray::setIconSize(QSize iconSize)
{
    mIconSize = iconSize;
    foreach(TrayIcon* icon, mIcons)
        icon->setIconSize(mIconSize);
}


/************************************************

************************************************/
#if 0
Visual* RazorTray::getVisual()
{
    //Display* dsp = QX11Info::display();

    XVisualInfo templ;
    templ.screen=QX11Info::appScreen();
    templ.depth=32;
    templ.c_class=TrueColor;

    int nvi;
    XVisualInfo* xvi = XGetVisualInfo(dsp, VisualScreenMask|VisualDepthMask|VisualClassMask, &templ, &nvi);

    Visual* visual = 0;
    if (xvi)
    {
        int i;
        XRenderPictFormat* format;
        for (i = 0; i < nvi; i++)
        {
            format = XRenderFindVisualFormat(dsp, xvi[i].visual);
            if (format->type == PictTypeDirect && format->direct.alphaMask)
            {
                visual = xvi[i].visual;
                break;
            }
        }
        free (xvi);
    }

    return visual;
    // check composite manager
//    gi_window_id_t composite_manager = gi_get_selection_owner(dsp, xfitMan().atom("_NET_WM_CM_S0"));

   // if (mColormap)
   //   XFreeColormap(dsp, mColormap);

   // if (mColormap32)
   //     XFreeColormap(dsp, mColormap32);

//    if (visual)
//    {
//        mVisual32 = visual;
//        mColormap32 = XCreateColormap(dsp, GI_DESKTOP_WINDOW_ID, visual, AllocNone);
//    }

//    if (visual && composite_manager != 0)
//    {
//        XSetWindowAttributes attrs;
//        attrs.event_mask = StructureNotifyMask;
//        XChangeWindowAttributes (dsp, composite_manager, CWEventMask, &attrs);

//        //server.real_transparency = 1;
//        mDepth = 32;
//        qDebug() << "Real transparency on... depth:" << mDepth;
//        mColormap = XCreateColormap(dsp, GI_DESKTOP_WINDOW_ID, visual, AllocNone);
//        mVisual = visual;
//    }
//    else
//    {
        // no composite manager or snapshot mode => fake transparency
        //server.real_transparency = 0;
//        mDepth = DefaultDepth(dsp, QX11Info::appScreen());
//        qDebug() << "Real transparency off.... depth:" << mDepth;
//        mColormap = DefaultColormap(dsp, QX11Info::appScreen());
//        mVisual = DefaultVisual(dsp, QX11Info::appScreen());
//    }
}
#endif


/************************************************
   freedesktop systray specification
 ************************************************/
bool RazorTray::startTray()
{
    //Display* dsp = QX11Info::display();
    gi_window_id_t root = GI_DESKTOP_WINDOW_ID;

    QString s = QString("_NET_SYSTEM_TRAY_S%1").arg(0);
    gi_atom_id_t _NET_SYSTEM_TRAY_S = xfitMan().atom(s.toAscii());

    if (gi_get_selection_owner( _NET_SYSTEM_TRAY_S) != 0)
    {
        qWarning() << "Another systray is running";
        return false;
    }

    // init systray protocol
    mTrayId = gi_create_window( GI_DESKTOP_WINDOW_ID, -1, -1, 1, 1,  GI_RGB( 192, 192, 192 ), GI_FLAGS_NON_FRAME);


    int orientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
    gi_change_property(
                    mTrayId,
                    xfitMan().atom("_NET_SYSTEM_TRAY_ORIENTATION"),
                    GA_CARDINAL,
                    32,
                    G_PROP_MODE_Replace,
                    (unsigned char *) &orientation,
                    1);
#if 0
    // ** Visual ********************************
    Visual* visual = getVisual();
    if (visual)
    {
        VisualID vid = XVisualIDFromVisual(visual);
        gi_change_property(QX11Info::display(),
                        mTrayId,
                        xfitMan().atom("_NET_SYSTEM_TRAY_VISUAL"),
                        GA_VISUALID,
                        32,
                        G_PROP_MODE_Replace,
                        (unsigned char*)&vid,
                        1);
    }
    // ******************************************
#endif

    gi_set_selection_owner( _NET_SYSTEM_TRAY_S, mTrayId, 0);

    if (gi_get_selection_owner( _NET_SYSTEM_TRAY_S) != mTrayId) {
        stopTray();
        qWarning() << "Can't get systray manager";
        return false;
    }

    gi_msg_t ev;
    ev.type = GI_MSG_CLIENT_MSG;
    ev.ev_window = root;
    ev.body.client.client_type = xfitMan().atom("MANAGER");
    ev.body.client.client_format = 32;
    ev.params[0] = 0;
    ev.params[1] = _NET_SYSTEM_TRAY_S;
    ev.params[2] = mTrayId;
    ev.params[3] = 0;
    //ev.params[4] = 0;
    gi_send_event(root, FALSE, GI_MASK_CLIENT_MSG, (gi_msg_t*)&ev);

    //XDamageQueryExtension(QX11Info::display(), &mDamageEvent, &mDamageError);

    qDebug() << "Systray started";
    return true;
}


/************************************************

 ************************************************/
void RazorTray::stopTray()
{
    qDeleteAll(mIcons);
    if (mTrayId) {
        gi_destroy_window( mTrayId);
        mTrayId = 0;
    }
    mValid = false;
}


/************************************************

 ************************************************/
void RazorTray::addIcon(gi_window_id_t winId)
{
    TrayIcon* icon = new TrayIcon(winId, this);
    if (!icon->isValid())
    {
        delete icon;
        return;
    }

    icon->setIconSize(mIconSize);
    mIcons.append(icon);
    addWidget(icon);
}
