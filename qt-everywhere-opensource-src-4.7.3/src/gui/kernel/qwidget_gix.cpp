/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** Copyright (C) 2011 www.hanxuantech.com. The Gix parts. 
** Written by Easion <easion@hanxuantech.com> 
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/


#include "qwidget_gix.h"
#include <qdebug.h>
#include "qapplication_p.h"
#include "private/qkeymapper_p.h"
#include "qdesktopwidget.h"
#include "qwidget.h"
#include "qwidget_p.h"
#include "qtextcodec.h"

#include <assert.h>
#include <stdio.h>
#include "qevent_p.h"
#include "private/qwindowsurface_raster_p.h"
#include "qapplication.h"
#include "qfileinfo.h"
#include <gi/gi.h>
#include <private/qbackingstore_p.h>

#include "qinputcontext.h"
#include "qinputcontextfactory.h"
#include "qmenu.h"
#include "private/qmenu_p.h"


QWidget *QWidgetPrivate::mouseGrabber = 0;
QWidget *QWidgetPrivate::keyboardGrabber = 0;

extern bool qt_nograb();
extern bool dndEnable(QWidget* w, bool on);

const uint stdWidgetEventMask =                        // X event mask
        (uint)(
            GI_MASK_KEY_DOWN | GI_MASK_KEY_UP |
            GI_MASK_BUTTON_DOWN | GI_MASK_BUTTON_UP |
            GI_MASK_KEYMAPNOTIFY |
            GI_MASK_MOUSE_MOVE | 
            GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT |
            GI_MASK_FOCUS_IN|GI_MASK_FOCUS_OUT |
            GI_MASK_EXPOSURE | GI_MSG_WINDOW_DESTROY|
            GI_MASK_PROPERTYNOTIFY |
			GI_MASK_CONFIGURENOTIFY |
            GI_MASK_CLIENT_MSG
       );

const uint stdDesktopEventMask =                        // X event mask
       (uint)(
           GI_MASK_KEYMAPNOTIFY |
           GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT | GI_MASK_PROPERTYNOTIFY
      );




void qt_XDestroyWindow( gi_window_id_t window)
{
    if (window)
        gi_destroy_window( window);
}

void qt_gix_enforce_cursor(QWidget * w, bool force)
{
    if (!w->testAttribute(Qt::WA_WState_Created))
        return;

    static QPointer<QWidget> lastUnderMouse = 0;
    if (force) {
        lastUnderMouse = w;
    } else if (lastUnderMouse && lastUnderMouse->effectiveWinId() == w->effectiveWinId()) {
        w = lastUnderMouse;
    } else if (!w->internalWinId()) {
        return; //the mouse is not under this widget, and it's not native, so don't change it
    }

    while (!w->internalWinId() && w->parentWidget() && !w->isWindow() && !w->testAttribute(Qt::WA_SetCursor))
        w = w->parentWidget();

    QWidget *nativeParent = w;
    if (!w->internalWinId())
        nativeParent = w->nativeParentWidget();
    // This does the same as effectiveWinId(), but since it is possible
    // to not have a native parent widget due to a special hack in
    // qwidget for reparenting widgets to a different X11 screen,
    // added additional check to make sure native parent widget exists.
    if (!nativeParent || !nativeParent->internalWinId())
        return;
    WId winid = nativeParent->internalWinId();

    if (w->isWindow() || w->testAttribute(Qt::WA_SetCursor)) {
#ifndef QT_NO_CURSOR
	//gi_setup_cursor
        QCursor *oc = QApplication::overrideCursor();
        if (oc) {
            gi_load_cursor( winid, oc->handle());
        } else if (w->isEnabled()) {
            gi_load_cursor( winid, w->cursor().handle());
        } else {
            // enforce the windows behavior of clearing the cursor on
            // disabled widgets
            gi_load_cursor( winid, GI_CURSOR_NONE);
        }
#endif
    } else {
        gi_load_cursor( winid, GI_CURSOR_NONE);
    }
}

Q_GUI_EXPORT void qt_gix_enforce_cursor(QWidget * w)
{
    qt_gix_enforce_cursor(w, false);
}

gi_window_id_t qt_XCreateSimpleWindow(const QWidget *,  gi_window_id_t parent,
                               int x, int y, uint w, uint h, long style)
{
	gi_window_id_t id;
	uint32_t background = GI_RGB(240,240,242);
	id = gi_create_window( parent, x, y, w, h, 
                                 background, style);

	//printf("qt XCreateSimpleWindow id = %d\n",id);
    return id;
}

// Qt reimplementation
#define XCOORD_MAX 16383
void QWidgetPrivate::create_sys(WId window, bool initializeWindow, bool destroyOldWindow)
{
    Q_Q(QWidget);
    Qt::WindowType type = q->windowType();
    Qt::WindowFlags &flags = data.window_flags;
    QWidget *parentWidget = q->parentWidget();
	long style = 0;

    if (type == Qt::ToolTip){
        flags |= Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint ;  
		style |= (GI_FLAGS_TASKBAR_WINDOW);
	}

	if (flags & Qt::FramelessWindowHint) {
		style |= (GI_FLAGS_NON_FRAME);
	}

	if (flags & Qt::WindowStaysOnBottomHint) {
		style |= (GI_FLAGS_DESKTOP_WINDOW);
	}

    bool topLevel = (flags & Qt::Window);
    bool popup = (type == Qt::Popup);
    bool dialog = (type == Qt::Dialog
                   || type == Qt::Sheet);
    bool desktop = (type == Qt::Desktop);
    bool tool = (type == Qt::Tool || type == Qt::SplashScreen
                 || type == Qt::ToolTip || type == Qt::Drawer);

	if(dialog) style |= (GI_FLAGS_NORESIZE|GI_FLAGS_TEMP_WINDOW|GI_FLAGS_TOPMOST_WINDOW);
	if(popup) style |= (GI_FLAGS_NON_FRAME | GI_FLAGS_MENU_WINDOW);
	if(tool) style |= (GI_FLAGS_TEMP_WINDOW | GI_FLAGS_NON_FRAME);

#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::create_sys START:" << q << "topLevel?" << topLevel << "type?" << type << "WId:"
             << window << "initializeWindow:" << initializeWindow << "destroyOldWindow" << destroyOldWindow;
#endif
    if (topLevel) {
        if (parentWidget) { // if our parent stays on top, so must we
            QWidget *ptl = parentWidget->window();
            if(ptl && (ptl->windowFlags() & Qt::WindowStaysOnTopHint))
                flags |= Qt::WindowStaysOnTopHint;
        }

		if (flags & Qt::WindowStaysOnTopHint)
		{
			style |= (GI_FLAGS_TASKBAR_WINDOW);
		}

        if (type == Qt::SplashScreen) {
			style |= (GI_FLAGS_NON_FRAME | GI_FLAGS_MENU_WINDOW) ;            
        }
        // All these buttons depend on the system menu, so we enable it
        if (flags & (Qt::WindowMinimizeButtonHint
                     | Qt::WindowMaximizeButtonHint
                     | Qt::WindowContextHelpButtonHint))
            flags |= Qt::WindowSystemMenuHint;
    }


    gi_window_id_t parentw, destroyw = 0;
    WId id = 0;

    // always initialize
    if (!window)
        initializeWindow = true;

    int sw = gi_screen_width();
    int sh = gi_screen_height();

    if (desktop) {                                // desktop widget
        dialog = popup = false;                        // force these flags off
        data.crect.setRect(0, 0, sw, sh);
    } else if (topLevel && !q->testAttribute(Qt::WA_Resized)) {
        QDesktopWidget *desktopWidget = qApp->desktop();
        if (desktopWidget->isVirtualDesktop()) {
            QRect r = desktopWidget->screenGeometry();
            sw = r.width();
            sh = r.height();
        }

        int width = sw / 2;
        int height = 4 * sh / 10;
        if (extra) {
            width = qMax(qMin(width, extra->maxw), extra->minw);
            height = qMax(qMin(height, extra->maxh), extra->minh);
        }
        data.crect.setSize(QSize(width, height));
    }

    parentw = topLevel ? GI_DESKTOP_WINDOW_ID : parentWidget->effectiveWinId();

    if (window) {                                // override the old window
        if (destroyOldWindow) {
            if (topLevel)
              dndEnable(q, false);
            destroyw = data.winid;
        }
        id = window;
        setWinId(window);
        gi_window_info_t a;
        gi_get_window_info( window, &a);
        data.crect.setRect(a.x, a.y, a.width, a.height);

        if (a.mapped )
            q->setAttribute(Qt::WA_WState_Visible, false);
        else
            q->setAttribute(Qt::WA_WState_Visible);

        //qt_x11_getX11InfoForWindow(&xinfo,a);

    } else if (desktop) {                        // desktop widget

        //id = (WId)parentw;                       // id = root window
        id = (WId)GI_DESKTOP_WINDOW_ID;                       // id = root window
        setWinId(id);
    } else if (topLevel || q->testAttribute(Qt::WA_NativeWindow) || paintOnScreen()) {

        QRect safeRect = data.crect; //##### must handle huge sizes as well.... i.e. wrect
        if (safeRect.width() < 1|| safeRect.height() < 1) {
            if (topLevel) {
                // top-levels must be at least 1x1
                safeRect.setSize(safeRect.size().expandedTo(QSize(1, 1)));
            } else {
                // create it way off screen, and rely on
                safeRect = QRect(-1000,-1000,1,1);
            }
        }

#define Q_WIDGET_WITH_DND_FLAGS 0x10
		if (QTLWExtra *topData = maybeTopData()) {
            if (topData->winFlags & Q_WIDGET_WITH_DND_FLAGS) {
                style |= (GI_FLAGS_NON_FRAME | GI_FLAGS_TEMP_WINDOW);
			}
        }

		id = (WId)qt_XCreateSimpleWindow(q,  parentw,
							 safeRect.left(), safeRect.top(),
							 safeRect.width(), safeRect.height(),
							 style);       
		if (!id)
		{
			printf("qt CreateSimpleWindow failed\n");
		} 
        setWinId(id);                                // set widget id/handle + hd
	}

    if (topLevel) {
        ulong wsa_mask = 0;
        if (type != Qt::SplashScreen) { // && customize) {

            //bool customize = flags & Qt::CustomizeWindowHint;
            //if (flags & Qt::FramelessWindowHint) {
			//	style |= (GI_FLAGS_NON_FRAME);
			//}
				//&& !(customize && !(flags & Qt::WindowTitleHint))) {
            //}
        } else {
            // if type == Qt::SplashScreen
            //mwmhints.decorations = MWM_DECOR_ALL;
        }
        
        if (wsa_mask && initializeWindow) {
            Q_ASSERT(id);
            //XChangeWindowAttributes(dpy, id, wsa_mask, &wsa);
        }      

    }

    if (!initializeWindow) {
        // do no initialization
    } else if (popup) {                        // popup widget
        // set EWMH window types
       
        Q_ASSERT(id);
        //XChangeWindowAttributes(dpy, id, CWOverrideRedirect | CWSaveUnder,
        //                        &wsa);
    } else if (topLevel && !desktop) {        // top-level widget
       
        //XClassHint class_hint;
        QByteArray appName = qAppName().toLatin1();       

        gi_resize_window( id,
                      qBound(1, data.crect.width(), XCOORD_MAX),
                      qBound(1, data.crect.height(), XCOORD_MAX));
        gi_set_window_utf8_caption( id, (const char*)appName.data());     

        // set _NET_WM_PID
        long curr_pid = getpid();
        gi_change_property( id, ATOM(_NET_WM_PID), GA_CARDINAL,
			32, G_PROP_MODE_Replace,
                        (unsigned char *) &curr_pid, 1);

        // when we create a toplevel widget, the frame strut should be dirty
        data.fstrut_dirty = 1;

        // declare the widget's window role
        if (QTLWExtra *topData = maybeTopData()) {
            if (!topData->role.isEmpty()) {
                QByteArray windowRole = topData->role.toUtf8();
                gi_change_property( id,
                                ATOM(WM_WINDOW_ROLE), GA_STRING, 8, G_PROP_MODE_Replace,
                                (unsigned char *)windowRole.constData(), windowRole.length());
            }
        }
       
    } else {
        // non-toplevel widgets don't have a frame, so no need to
        // update the strut
        data.fstrut_dirty = 0;
    }

    if (initializeWindow && q->internalWinId()) {
        // don't erase when resizing
        //wsa.bit_gravity = QApplication::isRightToLeft() ? NorthEastGravity : NorthWestGravity;
        Q_ASSERT(id);
        //XChangeWindowAttributes(dpy, id, CWBitGravity, &wsa);
    }

    // set X11 event mask
    if (desktop) {
        gi_set_events_mask( id, stdDesktopEventMask);
    } else if (q->internalWinId()) {
        gi_set_events_mask( id, stdWidgetEventMask);
    }

    if (desktop) {
        q->setAttribute(Qt::WA_WState_Visible);
    } else if (topLevel) {                        // set X cursor
        if (initializeWindow) {
            qt_gix_enforce_cursor(q);

            if (QTLWExtra *topData = maybeTopData())
                if (!topData->caption.isEmpty())
                    setWindowTitle_helper(topData->caption);

            //always enable dnd: it's not worth the effort to maintain the state
            // NOTE: this always creates topData()
            dndEnable(q, true);

            if (maybeTopData() && maybeTopData()->opacity != 255)
                q->setWindowOpacity(maybeTopData()->opacity/255.);

        }
    } else if (q->internalWinId()) {
        qt_gix_enforce_cursor(q);
        if (QWidget *p = q->parentWidget()) // reset the cursor on the native parent
            qt_gix_enforce_cursor(p);
    }    

    if (q->hasFocus() && q->testAttribute(Qt::WA_InputMethodEnabled)) {
        QInputContext *inputContext = q->inputContext();
        if (inputContext)
            inputContext->setFocusWidget(q);
    }

    if (destroyw)
        qt_XDestroyWindow(  destroyw);

    // newly created windows are positioned at the window system's
    // (0,0) position. If the parent uses wrect mapping to expand the
    // coordinate system, we must also adjust this widget's window
    // system position
    if (!topLevel && !parentWidget->data->wrect.topLeft().isNull())
        setWSGeometry();
    else if (topLevel && (data.crect.width() == 0 || data.crect.height() == 0))
        q->setAttribute(Qt::WA_OutsideWSRange, true);

    if (!topLevel && q->testAttribute(Qt::WA_NativeWindow) 
		&& q->testAttribute(Qt::WA_Mapped)) {
        Q_ASSERT(q->internalWinId());
        gi_show_window( q->internalWinId());
        // Ensure that mapped alien widgets are flushed immediately when re-created as native widgets.
        if (QWindowSurface *surface = q->windowSurface())
            surface->flush(q, q->rect(), q->mapTo(surface->window(), QPoint()));
    }
    q->setAttribute(Qt::WA_WState_Created); // accept move/resize events

#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::create_sys END:" << q;
#endif
}


void QWidgetPrivate::hide_sys()
{
	Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    deactivateWidgetCleanup();
   
    /*if (q->isWindow()) {
       // X11->deferred_map.removeAll(q);
        if (q->internalWinId()) // in nsplugin, may be 0
			gi_hide_window( q->internalWinId());
           // XWithdrawWindow( q->internalWinId(), xinfo.screen());
        //XFlush(X11->display);
    } else {
        invalidateBuffer(q->rect());
        if (q->internalWinId()) // in nsplugin, may be 0
            gi_hide_window( q->internalWinId());
    }*/

	if (q->windowFlags() != Qt::Desktop) {
		if (q->internalWinId()) // in nsplugin, may be 0
            gi_hide_window( q->internalWinId());
	}

	if (q->isWindow()) {
        if (QWidgetBackingStore *bs = maybeBackingStore())
            bs->releaseBuffer();
    } else {
        invalidateBuffer(q->rect());
    }

    q->setAttribute(Qt::WA_Mapped, false);
}


// Returns true if we should set WM_TRANSIENT_FOR on \a w
static inline bool isTransient(const QWidget *w)
{
    return ((w->windowType() == Qt::Dialog
             || w->windowType() == Qt::Sheet
             || w->windowType() == Qt::Tool
             || w->windowType() == Qt::SplashScreen
             || w->windowType() == Qt::ToolTip
             || w->windowType() == Qt::Drawer
             || w->windowType() == Qt::Popup)
            && !w->testAttribute(Qt::WA_X11BypassTransientForHint));
}


void QWidgetPrivate::show_sys()
{
	Q_Q(QWidget);
	Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    if (q->testAttribute(Qt::WA_OutsideWSRange))
        return;
    q->setAttribute(Qt::WA_Mapped);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));

    if (q->testAttribute(Qt::WA_DontShowOnScreen)) {
        invalidateBuffer(q->rect());
        return;
    }

	if (q->isWindow()) {
		if (q->isMinimized()) {
		   // fl = SWP_MINIMIZE;
		} else if (q->isMaximized()) {
			//fl |= SWP_MAXIMIZE;
		} else {
		   // fl |= SWP_RESTORE;
		}

		if (!(q->testAttribute(Qt::WA_ShowWithoutActivating)
			  || (q->windowType() == Qt::Popup)
			  || (q->windowType() == Qt::ToolTip)
			  || (q->windowType() == Qt::Tool))) {
		   // fl |= SWP_ACTIVATE;
		}
	}

    if (q->internalWinId())
	  gi_show_window( q->internalWinId());			
	invalidateBuffer(q->rect());   
}

bool QWidget::pfWinEvent(gi_msg_t *message, long *result)
{
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}


void QWidgetPrivate::reparentChildren()
{
    Q_Q(QWidget);
    QObjectList chlist = q->children();
    for(int i = 0; i < chlist.size(); ++i) { // reparent children
        QObject *obj = chlist.at(i);
        if (obj->isWidgetType()) {
            QWidget *w = (QWidget *)obj;
            if ((w->windowType() == Qt::Popup)) {
                ;
            } else if (w->isWindow()) {
                bool showIt = w->isVisible();
                QPoint old_pos = w->pos();
                w->setParent(q, w->windowFlags());
                w->move(old_pos);
                if (showIt)
                    w->show();
            } else {
                w->d_func()->invalidateBuffer(w->rect());
                gi_reparent_window(w->effectiveWinId(), q->effectiveWinId(),0,0);
                w->d_func()->reparentChildren();
            }
        }
    }
}



//FIXME DPP
void QWidgetPrivate::setParent_sys(QWidget* parent, Qt::WindowFlags f)
{
	Q_Q(QWidget);
#if 1	
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);
    if (q->isVisible() && q->parentWidget() && parent != q->parentWidget())
        q->parentWidget()->d_func()->invalidateBuffer(q->geometry());

    WId old_winid = data.winid;
	bool old_w = q->isWindow();
    bool widgetTypeChanged = false;

    if (wasCreated && q->isVisible()) {
		if (q->isWindow())
			gi_hide_window(data.winid);
    }

    bool dropSiteWasRegistered = false;
    if (q->testAttribute(Qt::WA_DropSiteRegistered)) {
        dropSiteWasRegistered = true;
        q->setAttribute(Qt::WA_DropSiteRegistered, false); // ole dnd unregister (we will register again below)
    }

    if ((q->windowType() == Qt::Desktop))
        old_winid = 0;

    QObjectPrivate::setParent_helper(parent);

    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    data.window_flags = f;
    data.fstrut_dirty = true;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);
    adjustFlags(data.window_flags, q);
	widgetTypeChanged = old_w != q->isWindow(); // widget type changed (widget<->window)

#else
    bool wasCreated = q->testAttribute(Qt::WA_WState_Created);
    if (q->isVisible() && q->parentWidget() && parent != q->parentWidget())
        q->parentWidget()->d_func()->invalidateBuffer(effectiveRectFor(q->geometry()));

    WId old_winid = data.winid;

    // hide and reparent our own window away. Otherwise we might get
    // destroyed when emitting the child remove event below. See QWorkspace.
    if (q->isVisible() && old_winid != 0) {
        //qt_WinSetWindowPos(old_winid, 0, 0, 0, 0, 0, SWP_HIDE);
        //WinSetParent(old_winid, HWND_OBJECT, FALSE);
        //WinSetOwner(old_winid, 0);
		gi_hide_window(old_winid);
		gi_reparent_window(old_winid, GI_DESKTOP_WINDOW_ID,0,0);
    }
    bool dropSiteWasRegistered = false;
    if (q->testAttribute(Qt::WA_DropSiteRegistered)) {
        dropSiteWasRegistered = true;
        q->setAttribute(Qt::WA_DropSiteRegistered, false); // ole dnd unregister (we will register again below)
    }

	if (q->isWindow() && wasCreated)
        dndEnable(q, false);

    if ((q->windowType() == Qt::Desktop))
        old_winid = 0;

    //if (extra && extra->topextra)
    //    extra->topextra->fId = 0;
    setWinId(0);

    QObjectPrivate::setParent_helper(parent);
    bool explicitlyHidden = q->testAttribute(Qt::WA_WState_Hidden) && q->testAttribute(Qt::WA_WState_ExplicitShowHide);

    data.window_flags = f;
    data.fstrut_dirty = true;
    q->setAttribute(Qt::WA_WState_Created, false);
    q->setAttribute(Qt::WA_WState_Visible, false);
    q->setAttribute(Qt::WA_WState_Hidden, false);
    adjustFlags(data.window_flags, q);
    // keep compatibility with previous versions, we need to preserve the created state
    // (but we recreate the winId for the widget being reparented, again for compatibility)
    if (wasCreated || (!q->isWindow() && parent->testAttribute(Qt::WA_WState_Created)))
        createWinId();
    if (q->isWindow() || (!parent || parent->isVisible()) || explicitlyHidden)
        q->setAttribute(Qt::WA_WState_Hidden);
    q->setAttribute(Qt::WA_WState_ExplicitShowHide, explicitlyHidden);

    if (wasCreated) {
        reparentChildren();
    }

    if (extra && !extra->mask.isEmpty()) {
        QRegion r = extra->mask;
        extra->mask = QRegion();
        q->setMask(r);
    }
    if (extra && extra->topextra && !extra->topextra->caption.isEmpty()) {
        setWindowIcon_sys(true);
        setWindowTitle_helper(extra->topextra->caption);
    }

    if (old_winid != 0 && q->windowType() != Qt::Desktop) {
#if defined(QT_DEBUGWINCREATEDESTROY)
            qDebug() << "|Destroying window (reparent)" << q
                     << "\n|  hwnd"  << qDebugFmtHex(old_winid);
#endif
        qt_XDestroyWindow(old_winid);
    }

#endif
    if (q->testAttribute(Qt::WA_AcceptDrops) || dropSiteWasRegistered
        || (!q->isWindow() && q->parentWidget() && q->parentWidget()->testAttribute(Qt::WA_DropSiteRegistered)))
        q->setAttribute(Qt::WA_DropSiteRegistered, true);

    invalidateBuffer(q->rect());
#ifdef ALIEN_DEBUG
    qDebug() << "QWidgetPrivate::setParent_sys END" << q;
#endif

}

void QWidgetPrivate::raise_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId())
        gi_raise_window( q->internalWinId());
}

void QWidgetPrivate::lower_sys()
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId())
        gi_lower_window( q->internalWinId());
    if(!q->isWindow())
        invalidateBuffer(q->rect());
}

void QWidgetPrivate::stackUnder_sys(QWidget* w)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (q->internalWinId() && w->internalWinId()) {
        gi_window_id_t stack[2];
        stack[0] = w->internalWinId();;
        stack[1] = q->internalWinId();
		printf("QWidgetPrivate::stackUnder_sys called\n");
        gi_restack_windows( stack, 2);
    }
    if(!q->isWindow() || !w->internalWinId())
        invalidateBuffer(q->rect());
//	Q_Q(QWidget);
//	qDebug() << "Unimplemented: QWidgetPrivate::stackUnder_sys() "<<  q << "	top:"<<widget;
}

void QWidgetPrivate::setWindowIcon_sys(bool forceReset)
{
    Q_Q(QWidget);
	gi_image_t *img;
	int iw = 0, ih = 0;

    if (!q->testAttribute(Qt::WA_WState_Created))
        return;
    if ( !q->isWindow())
        return;
	if (!q->internalWinId())
		return;

    QTLWExtra *topData = this->topData();
    if (topData->icon_small != NULL && !forceReset)
         //already been set
        return;

    QIcon icon = q->windowIcon();

    if (topData->icon_small != NULL)
        gi_destroy_image(topData->icon_small);

	img = QPixmap::to_gix_icon(icon);
    topData->icon_small = img;

	if(img){
		gi_set_window_icon(q->internalWinId(),
			(uint32_t*)img->rgba, img->w, img->h);
	}
}

void QWidgetPrivate::setWindowTitle_sys(const QString &caption)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
    if (!q->internalWinId())
        return;
    QByteArray net_wm_name = caption.toUtf8();
	gi_set_window_utf8_caption(q->internalWinId(), (const char *)net_wm_name.data());
}

void QWidgetPrivate::updateSystemBackground()
{
#if 0
   Q_Q(QWidget);
    if (!q->testAttribute(Qt::WA_WState_Created) || !q->internalWinId())
        return;
    QBrush brush = q->palette().brush(QPalette::Active, q->backgroundRole());
    Qt::WindowType type = q->windowType();
    if (brush.style() == Qt::NoBrush
        || q->testAttribute(Qt::WA_NoSystemBackground)
        || q->testAttribute(Qt::WA_UpdatesDisabled)
        || type == Qt::Popup || type == Qt::ToolTip
        )
        gi_set_window_pixmap( q->internalWinId(), 0,0);
    else if (brush.style() == Qt::SolidPattern && brush.isOpaque())
        gi_set_window_background( q->internalWinId(),
                             QColormap::instance(0).pixel(brush.color()), 0);
    else if (isBackgroundInherited())
        gi_set_window_pixmap( q->internalWinId(), 0,0 );//ParentRelative
    else if (brush.style() == Qt::TexturePattern) {
        extern QPixmap qt_toX11Pixmap(const QPixmap &pixmap); // qpixmap_x11.cpp
        gi_set_window_pixmap( q->internalWinId(),
            static_cast<QX11PixmapData*>(qt_toX11Pixmap(brush.texture()).data.data())->x11ConvertToDefaultDepth()
			,0);
    } else
        gi_set_window_background(q->internalWinId(),
                             QColormap::instance(0).pixel(brush.color()), 0);
#endif
//	Q_Q(QWidget);
//	qDebug() << "Unimplemented: QWidgetPrivate::UpdateSystemBackground() "<<  q ;
}

void QWidgetPrivate::setModal_sys()
{
	Q_Q(QWidget);
}

void QWidgetPrivate::setConstraints_sys()
{
	Q_Q(QWidget);

//	qDebug() << "QWidgetPrivate::setConstraints_sys() "<<  q ;
	if (!q->isWindow() || !q->testAttribute(Qt::WA_WState_Created) )
		return;
}

void QWidgetPrivate::setCursor_sys(const QCursor& cursor)
{
	Q_Q(QWidget);
    qt_gix_enforce_cursor(q);
    //Q_UNUSED(cursor);
}

void QWidgetPrivate::unsetCursor_sys()
{
    Q_Q(QWidget);
    qt_gix_enforce_cursor(q);
}

void QWidgetPrivate::setWSGeometry(bool dontShow, const QRect &)
{
	Q_UNUSED(dontShow);
	Q_Q(QWidget);
//	qDebug() << "Unimplemented: QWidgetPrivate::setWSGeomerty() "<<  q ;
}
void qWinRequestConfig(WId id, int req, int x, int y, int w, int h);

void QWidgetPrivate::setGeometry_sys(int x,int y,int w,int h,bool  isMove)
{
    Q_Q(QWidget);
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));


 //os2 style
   if (extra) { // any size restrictions?
        w = qMin(w,extra->maxw);
        h = qMin(h,extra->maxh);
        w = qMax(w,extra->minw);
        h = qMax(h,extra->minh);
    }
    if (q->isWindow())
        topData()->normalGeometry = QRect(0, 0, -1, -1);

    QSize  oldSize(q->size());
    QPoint oldPos(q->pos());

    if (!q->isWindow())
        isMove = (data.crect.topLeft() != QPoint(x, y));
    bool isResize = w != oldSize.width() || h != oldSize.height();

    if (!isMove && !isResize)
        return;

    gi_window_id_t fId = q->internalWinId();

    if (isResize && !q->testAttribute(Qt::WA_StaticContents) &&
        q->internalWinId() != 0) {
		gi_clear_area(q->internalWinId(),0, 0, data.crect.width(), data.crect.height(),TRUE);
        //RECTL rcl = { 0, 0, data.crect.width(), data.crect.height() };
        //WinValidateRect(q->internalWinId(), &rcl, FALSE);
    }

    if (isResize)
        data.window_state &= ~Qt::WindowMaximized;

    if (data.window_state & Qt::WindowFullScreen) {
        data.window_state &= ~Qt::WindowFullScreen;
    }

    QTLWExtra *tlwExtra = q->window()->d_func()->maybeTopData();
    const bool inTopLevelResize = tlwExtra ? tlwExtra->inTopLevelResize : false;

    if (q->testAttribute(Qt::WA_WState_ConfigPending)) {        // processing config event
        if (q->internalWinId() != 0){
			//fixme
            qWinRequestConfig(q->internalWinId(), isMove ? 2 : 1, x, y, w, h);
		}
    } else {
        if (!q->testAttribute(Qt::WA_DontShowOnScreen))
            q->setAttribute(Qt::WA_WState_ConfigPending);
        if (q->windowType() == Qt::Desktop) {
            data.crect.setRect(x, y, w, h);
        } else if (q->isWindow()) {
            //int sh = gi_screen_height();
            QRect fs(frameStrut());
            if (extra) {
                fs.setLeft(x - fs.left());
                fs.setTop(y - fs.top());
                fs.setRight((x + w - 1) + fs.right());
                fs.setBottom((y + h - 1) + fs.bottom());
            }
            if (w == 0 || h == 0) {
                q->setAttribute(Qt::WA_OutsideWSRange, true);
                if (q->isVisible() && q->testAttribute(Qt::WA_Mapped))
                    hide_sys();
                data.crect = QRect(x, y, w, h);
            } else if (q->isVisible() && q->testAttribute(Qt::WA_OutsideWSRange)) {
                q->setAttribute(Qt::WA_OutsideWSRange, false);

					//FIXME
                gi_move_window( q->internalWinId(), fs.x(), fs.y());
                gi_resize_window( q->internalWinId(),  fs.width(), fs.height());

                // put the window in its place and show it
                //WinSetWindowPos(fId, 0, fs.x(),
                //                // flip y coordinate
                 //               sh - (fs.y() + fs.height()),
                  //              fs.width(), fs.height(), SWP_MOVE | SWP_SIZE);
                data.crect.setRect(x, y, w, h);

                show_sys();
            } else if (!q->testAttribute(Qt::WA_DontShowOnScreen)) {
                q->setAttribute(Qt::WA_OutsideWSRange, false);

				if (data.window_state & Qt::WindowMaximized) {
                    //qt_wince_maximize(q);
                } else {
					gi_move_window( q->internalWinId(), fs.x(), fs.y());
					gi_resize_window( q->internalWinId(),  fs.width(), fs.height());
                    //MoveWindow(q->internalWinId(), fs.x(), fs.y(), fs.width(), fs.height(), true);
                }

                // If the window is hidden and in maximized state or minimized, instead of moving the
                // window, set the normal position of the window.
                /*SWP swp;
                WinQueryWindowPos(fId, &swp);
                if (((swp.fl & SWP_MAXIMIZE) && !WinIsWindowVisible(fId)) ||
                    (swp.fl & SWP_MINIMIZE)) {
                    WinSetWindowUShort(fId, QWS_XRESTORE, fs.x());
                    WinSetWindowUShort(fId, QWS_YRESTORE, // flip y coordinate
                                       sh - (fs.y() + fs.height()));
                    WinSetWindowUShort(fId, QWS_CXRESTORE, fs.width());
                    WinSetWindowUShort(fId, QWS_CYRESTORE, fs.height());
                } else {
                    WinSetWindowPos(fId, 0, fs.x(),
                                    // flip y coordinate
                                    sh - (fs.y() + fs.height()),
                                    fs.width(), fs.height(), SWP_MOVE | SWP_SIZE);
                }*/
                if (!q->isVisible()){
					gi_clear_window(q->internalWinId(),TRUE);
                    //WinInvalidateRect(q->internalWinId(), NULL, FALSE);
				}

				gi_window_info_t swp;
				gi_get_window_info(q->internalWinId(), &swp);

				QRect fs(frameStrut());

				//fixme
				data.crect.setRect(swp.x + fs.left(),
								   swp.y + fs.top(),
								   swp.width,
								   swp.height);
				isResize = data.crect.size() != oldSize;

                /*if (!(swp.fl & SWP_MINIMIZE)) {
                   
                    WinQueryWindowPos(fId, &swp);
                    // flip y coordinate
                    swp.y = sh - (swp.y + swp.cy);
                    QRect fs(frameStrut());
                    data.crect.setRect(swp.x + fs.left(),
                                       swp.y + fs.top(),
                                       swp.cx - fs.left() - fs.right(),
                                       swp.cy - fs.top() - fs.bottom());
                    isResize = data.crect.size() != oldSize;
                }*/
            } else {
                q->setAttribute(Qt::WA_OutsideWSRange, false);
                data.crect.setRect(x, y, w, h);
            }
        } else {
            QRect oldGeom(data.crect);
            data.crect.setRect(x, y, w, h);
            if (q->isVisible() && (!inTopLevelResize || q->internalWinId())) {
                // Top-level resize optimization does not work for native child widgets;
                // disable it for this particular widget.
                if (inTopLevelResize)
                    tlwExtra->inTopLevelResize = false;

                if (!isResize)
                    moveRect(QRect(oldPos, oldSize), x - oldPos.x(), y - oldPos.y());
                else
                    invalidateBuffer_resizeHelper(oldPos, oldSize);

                if (inTopLevelResize)
                    tlwExtra->inTopLevelResize = true;
            }
            if (q->testAttribute(Qt::WA_WState_Created))
                setWSGeometry();
        }
        q->setAttribute(Qt::WA_WState_ConfigPending, false);
    }

    if (q->isWindow() && q->isVisible() && isResize && !inTopLevelResize) {
        invalidateBuffer(q->rect()); //after the resize
    }

    // Process events immediately rather than in translateConfigEvent to
    // avoid windows message process delay.
    if (q->isVisible()) {
        if (isMove && q->pos() != oldPos) {
            // in QMoveEvent, pos() and oldPos() exclude the frame, adjust them
            QRect fs(frameStrut());
            QMoveEvent e(q->pos() + fs.topLeft(), oldPos + fs.topLeft());
            QApplication::sendEvent(q, &e);
        }
        if (isResize) {
            static bool slowResize = qgetenv("QT_SLOW_TOPLEVEL_RESIZE").toInt();
            // If we have a backing store with static contents, we have to disable the top-level
            // resize optimization in order to get invalidated regions for resized widgets.
            // The optimization discards all invalidateBuffer() calls since we're going to
            // repaint everything anyways, but that's not the case with static contents.
            const bool setTopLevelResize = !slowResize && q->isWindow() && extra && extra->topextra
                                           && !extra->topextra->inTopLevelResize
                                           && (!extra->topextra->backingStore
                                               || !extra->topextra->backingStore->hasStaticContents());
            if (setTopLevelResize)
                extra->topextra->inTopLevelResize = true;
            QResizeEvent e(q->size(), oldSize);
            QApplication::sendEvent(q, &e);
            if (setTopLevelResize)
                extra->topextra->inTopLevelResize = false;
        }
    } else {
        if (isMove && q->pos() != oldPos)
            q->setAttribute(Qt::WA_PendingMoveEvent, true);
        if (isResize)
            q->setAttribute(Qt::WA_PendingResizeEvent, true);
    }

	
}	

void QWidgetPrivate::registerDropSite(bool on)
{
    Q_UNUSED(on);
	//debugprint(1, "Unimplemented: QWidgetPrivate::registerDropSite\n");
}

void QWidgetPrivate::setWindowOpacity_sys(qreal opacity)
{
    Q_Q(QWidget);
    ulong value = ulong(opacity * 0xffffffff);

	gi_set_window_opacity(q->internalWinId(), value);
	//debugprint(1, "Unimplemented: QWidgetPrivate::setWindowOpacity_sys \n");
}

void QWidgetPrivate::deleteSysExtra()
{
//	debugprint(1, "Unimplemented: QWidgetPrivate::deleteSysExtra\n");
}

void QWidgetPrivate::deleteTLSysExtra()
{
	//debugprint(1, "Unimplemented: QWidgetPrivate::deleteTLSysExtra\n");
}

void QWidgetPrivate::createSysExtra()
{
    extra->compress_events = true;
    extra->xDndProxy = 0;
//	debugprint(1, "Unimplemented: QWidgetPrivate::createSysExtra\n");
}

void QWidgetPrivate::createTLSysExtra()
{
    //extra->topextra->spont_unmapped = 0;
    extra->topextra->dnd = 0;
    extra->topextra->winFlags = 0;
    //extra->topextra->validWMState = 0;
    extra->topextra->icon_big = 0;
    extra->topextra->icon_small = 0;
    //extra->topextra->waitingForMapNotify = 0;
    extra->topextra->parentWinId = 0;
//	debugprint(1, "Unimplemented: QWidgetPrivate::createTLSysExtra\n");
}

void QWidgetPrivate::setWindowIconText_sys(const QString& iconText)
{
	Q_Q(QWidget);
    if (!q->internalWinId())
        return;
    Q_ASSERT(q->testAttribute(Qt::WA_WState_Created));
	QByteArray icon_name = iconText.toUtf8();
	gi_set_window_icon_name(q->internalWinId(),(const char *) icon_name.constData());
//	debugprint(1, "Unimplemented: QWidgetPrivate::setWindowIconText_sys \n");
}

void QWidgetPrivate::scroll_sys(int,int, const QRect& r)
{
//	debugprint(1, "Reimplemented: QWidgetPrivate::scroll(int,int,const  QRect&\n");
	Q_Q(QWidget);
	q->update(r);
}

void QWidgetPrivate::scroll_sys(int, int)
{
//	debugprint(1, "Reimplemented: QWidgetPrivate::scroll(int,int)\n");
	Q_Q(QWidget);
	q->update();
}

void QWidgetPrivate::updateFrameStrut()
{
    Q_Q(QWidget);

    if (!q->testAttribute(Qt::WA_WState_Created))
        return;

#if 0
    if (!q->internalWinId()) {
        data.fstrut_dirty = false;
        return;
    }

    RECT rect = {0,0,0,0};

    QTLWExtra *top = topData();
    uint exstyle = GetWindowLong(q->internalWinId(), GWL_EXSTYLE);
    uint style = GetWindowLong(q->internalWinId(), GWL_STYLE);

    if (AdjustWindowRectEx(&rect, style, FALSE, exstyle)) {
        top->frameStrut.setCoords(-rect.left, -rect.top, rect.right, rect.bottom);
        data.fstrut_dirty = false;
    }
#endif
	
}

extern "C" int gi_wm_focus_window(gi_window_id_t window);

void QWidgetPrivate::setFocus_sys()
{
    Q_Q(QWidget);
    if (q->testAttribute(Qt::WA_WState_Created) 
		&& q->window()->windowType() != Qt::Popup){
		//gi_wm_activate_window( q->effectiveWinId());
        gi_wm_focus_window(q->effectiveWinId());
	}
}

QWindowSurface *QWidgetPrivate::createDefaultWindowSurface_sys()
{
	Q_Q(QWidget);
	return new QRasterWindowSurface(q);
}

void QWidgetPrivate::setMask_sys(const QRegion &region)
{
    Q_Q(QWidget);
    if (!q->internalWinId())
        return;

    if (region.isEmpty()) {
        //SetWindowRgn(q->internalWinId(), 0, true);
        return;
    }
//	debugprint(1, "Unimplemented: QWidgetPrivate::setMask_sys()\n");
}

int QWidget::metric(PaintDeviceMetric m) const
{
    Q_D(const QWidget);
    int val = 0;
	int mwidth, mheight;
	int swidth, sheight;

#define DPIX	75
#define DPIY	75
	    
    if (m == PdmWidth) {
        val = data->crect.width();
    } else if (m == PdmHeight) {
        val = data->crect.height();
    } else {
        
        switch (m) {
            case PdmDpiX:
            case PdmPhysicalDpiX:
                /*if (d->extra && d->extra->customDpiX)
                    val = d->extra->customDpiX;
                else if (d->parent)
                    val = static_cast<QWidget *>(d->parent)->metric(m);
                else*/
                    val = DPIX;
                break;
            case PdmDpiY:
            case PdmPhysicalDpiY:
                /*if (d->extra && d->extra->customDpiY)
                    val = d->extra->customDpiY;
                else if (d->parent)
                    val = static_cast<QWidget *>(d->parent)->metric(m);
                else*/
                    val = DPIY;
                break;
			case PdmWidthMM:
				swidth = gi_screen_width();
				mwidth      = (swidth * 254 + DPIX * 5) / (DPIX * 10);
                val = (mwidth*data->crect.width())/ swidth;
                break;

            case PdmHeightMM:
				sheight = gi_screen_height();
				mheight     = (sheight * 254  + DPIY * 5) / (DPIY * 10);
                val = (mheight*data->crect.height())/ sheight;
                break;

            case PdmNumColors:
				//int bpp = 32;//GetDeviceCaps(hd, BITSPIXEL);
                //if (bpp == 32)
                    val = INT_MAX; // ### this is bogus, it should be 2^24 colors for 32 bit as well
                //else if(bpp<=8)
                //    val = GetDeviceCaps(hd, NUMCOLORS);
                //else
                 //   val = 1 << (bpp * GetDeviceCaps(hd, PLANES));

                //val = d->xinfo.cells();
                break;
            case PdmDepth:
                val = 32;
                break;
            default:
                val = 0;
                qWarning("QWidget::metric: Invalid metric command: %d",m);
        }
    }


	return val;
}

QPaintEngine *QWidget::paintEngine() const
{
    //const_cast<QWidgetPrivate *>(d_func())->noPaintOnScreen = 1;
	return 0;
}

QPoint QWidget::mapToGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    if (!testAttribute(Qt::WA_WState_Created) || !internalWinId()) {
        QPoint p = pos + data->crect.topLeft();
        //cannot trust that !isWindow() implies parentWidget() before create
        return (isWindow() || !parentWidget()) ?  p : parentWidget()->mapToGlobal(p);
    }
    int           x, y;
    gi_window_id_t child;
    QPoint p = d->mapToWS(pos);
    gi_translate_coordinates( internalWinId(),
                          GI_DESKTOP_WINDOW_ID,
                          p.x(), p.y(), &x, &y, &child);
    return QPoint(x, y);
}

QPoint QWidget::mapFromGlobal(const QPoint &pos) const
{
    Q_D(const QWidget);
    if (!testAttribute(Qt::WA_WState_Created) || !internalWinId()) {
        //cannot trust that !isWindow() implies parentWidget() before create
        QPoint p = (isWindow() || !parentWidget()) ?  pos : parentWidget()->mapFromGlobal(pos);
        return p - data->crect.topLeft();
    }
    int           x, y;
    gi_window_id_t child;
    gi_translate_coordinates(
                          GI_DESKTOP_WINDOW_ID,
                          internalWinId(), pos.x(), pos.y(), &x, &y, &child);
    return d->mapFromWS(QPoint(x, y));

}
//extern "C" int gi_wm_activate_window(gi_window_id_t window);

void QWidget::activateWindow()
{
	QWidget *tlw = window();

    tlw->createWinId();
    //gi_set_focus_window(tlw->internalWinId());
	gi_wm_activate_window( tlw->effectiveWinId());

	if (tlw->isVisible() && !tlw->d_func()->topData()->embedded){
		//if (!qt_widget_private(tlw)->topData()->waitingForMapNotify)
          //gi_wm_activate_window( tlw->internalWinId());
		  //gi_set_focus_window(tlw->effectiveWinId());


	}
}

void QWidget::setWindowState(Qt::WindowStates newstate)
{
#if 1
    Q_D(QWidget);
    bool needShow = false;
    Qt::WindowStates oldstate = windowState();
    if (oldstate == newstate)
        return;
    if (isWindow()) {
        // Ensure the initial size is valid, since we store it as normalGeometry below.
        if (!testAttribute(Qt::WA_Resized) && !isVisible())
            adjustSize();

        QTLWExtra *top = d->topData();
		int wm_support_maximized_horz = 1;

        if ((oldstate & Qt::WindowMaximized) != (newstate & Qt::WindowMaximized)) {
            if (wm_support_maximized_horz) {
                if ((newstate & Qt::WindowMaximized) && !(oldstate & Qt::WindowFullScreen))
                    top->normalGeometry = geometry();
                //qt_change_net_wm_state(this, (newstate & Qt::WindowMaximized),
                //                       ATOM(_NET_WM_STATE_MAXIMIZED_HORZ),
                //                       ATOM(_NET_WM_STATE_MAXIMIZED_VERT));
            } else if (! (newstate & Qt::WindowFullScreen)) {
                if (newstate & Qt::WindowMaximized) {
                    // save original geometry
                    const QRect normalGeometry = geometry();

                    if (isVisible()) {
                        data->fstrut_dirty = true;
                        const QRect maxRect = QApplication::desktop()->availableGeometry(this);
                        const QRect r = top->normalGeometry;
                        const QRect fs = d->frameStrut();
                        setGeometry(maxRect.x() + fs.left(),
                                    maxRect.y() + fs.top(),
                                    maxRect.width() - fs.left() - fs.right(),
                                    maxRect.height() - fs.top() - fs.bottom());
                        top->normalGeometry = r;
                    }

                    if (top->normalGeometry.width() < 0)
                        top->normalGeometry = normalGeometry;
                } else {
                    // restore original geometry
                    setGeometry(top->normalGeometry);
                }
            }
        }

        if ((oldstate & Qt::WindowFullScreen) != (newstate & Qt::WindowFullScreen)) {
            const int support_fullscreen_state = 1;
			if (support_fullscreen_state) {
                if (newstate & Qt::WindowFullScreen) {
                    top->normalGeometry = geometry();
                    top->fullScreenOffset = d->frameStrut().topLeft();
                }

				gi_fullscreen_window(this->internalWinId(), (newstate & Qt::WindowFullScreen));
                //qt_change_net_wm_state(this, (newstate & Qt::WindowFullScreen),
                //                       ATOM(_NET_WM_STATE_FULLSCREEN));
            } else {
                needShow = isVisible();

                if (newstate & Qt::WindowFullScreen) {
                    data->fstrut_dirty = true;
                    const QRect normalGeometry = geometry();
                    const QPoint fullScreenOffset = d->frameStrut().topLeft();

                    top->savedFlags = windowFlags();
                    setParent(0, Qt::Window | Qt::FramelessWindowHint);
                    const QRect r = top->normalGeometry;
                    setGeometry(qApp->desktop()->screenGeometry(this));
                    top->normalGeometry = r;

                    if (top->normalGeometry.width() < 0) {
                        top->normalGeometry = normalGeometry;
                        top->fullScreenOffset = fullScreenOffset;
                    }
                } else {
                    setParent(0, top->savedFlags);

                    if (newstate & Qt::WindowMaximized) {
                        // from fullscreen to maximized
                        data->fstrut_dirty = true;
                        const QRect maxRect = QApplication::desktop()->availableGeometry(this);
                        const QRect r = top->normalGeometry;
                        const QRect fs = d->frameStrut();
                        setGeometry(maxRect.x() + fs.left(),
                                    maxRect.y() + fs.top(),
                                    maxRect.width() - fs.left() - fs.right(),
                                    maxRect.height() - fs.top() - fs.bottom());
                        top->normalGeometry = r;
                    } else {
                        // restore original geometry
                        setGeometry(top->normalGeometry.adjusted(-top->fullScreenOffset.x(),
                                                                 -top->fullScreenOffset.y(),
                                                                 -top->fullScreenOffset.x(),
                                                                 -top->fullScreenOffset.y()));
                    }
                }
            }
        }

        createWinId();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
        if ((oldstate & Qt::WindowMinimized) != (newstate & Qt::WindowMinimized)) {
            if (isVisible()) {
                if (newstate & Qt::WindowMinimized) {
					gi_iconify_window(data->winid);

                } else {
                    setAttribute(Qt::WA_Mapped);
                    gi_show_window( effectiveWinId());
                }
            }

            needShow = false;
        }
    }

    data->window_state = newstate;

    if (needShow)
        show();

    if (newstate & Qt::WindowActive)
        activateWindow();

    QWindowStateChangeEvent e(oldstate);
    QApplication::sendEvent(this, &e);
#endif
}

void QWidget::grabMouse()
{
    if (isVisible() && !qt_nograb()) {
        if (QWidgetPrivate::mouseGrabber && QWidgetPrivate::mouseGrabber != this)
            QWidgetPrivate::mouseGrabber->releaseMouse();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
#ifndef QT_NO_DEBUG
        int status =
#endif
            gi_grab_pointer( effectiveWinId(), TRUE,FALSE,
		GI_MASK_BUTTON_DOWN|GI_MASK_BUTTON_UP,0,GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M);
#ifndef QT_NO_DEBUG
        if (status) {
            const char *s =
                status == GrabNotViewable ? "\"GrabNotViewable\"" :
                status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
                status == GrabFrozen      ? "\"GrabFrozen\"" :
                status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
                "<?>";
            qWarning("QWidget::grabMouse: Failed with %s", s);
        }
#endif
        QWidgetPrivate::mouseGrabber = this;
    }

}

void QWidget::grabMouse(const QCursor &cursor)
{
    if (!qt_nograb()) {
        if (QWidgetPrivate::mouseGrabber && QWidgetPrivate::mouseGrabber != this)
            QWidgetPrivate::mouseGrabber->releaseMouse();
        Q_ASSERT(testAttribute(Qt::WA_WState_Created));
#ifndef QT_NO_DEBUG
        int status =
#endif
        gi_grab_pointer( effectiveWinId(), FALSE,TRUE,
                      GI_MASK_BUTTON_DOWN|GI_MASK_BUTTON_UP,
	0,GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M);
#ifndef QT_NO_DEBUG
        if (status) {
            const char *s =
                status == GrabNotViewable ? "\"GrabNotViewable\"" :
                status == AlreadyGrabbed  ? "\"AlreadyGrabbed\"" :
                status == GrabFrozen      ? "\"GrabFrozen\"" :
                status == GrabInvalidTime ? "\"GrabInvalidTime\"" :
                                            "<?>";
            qWarning("QWidget::grabMouse: Failed with %s", s);
        }
#endif
        QWidgetPrivate::mouseGrabber = this;
    }
}

void QWidget::releaseMouse()
{
    if (!qt_nograb() && QWidgetPrivate::mouseGrabber == this) {
        gi_ungrab_pointer();
        QWidgetPrivate::mouseGrabber = 0;
    }
}



void QWidget::grabKeyboard()
{
    if (!qt_nograb()) {
        if (QWidgetPrivate::keyboardGrabber && QWidgetPrivate::keyboardGrabber != this)
            QWidgetPrivate::keyboardGrabber->releaseKeyboard();
        gi_grab_keyboard(effectiveWinId(), FALSE, 0, 0);
        QWidgetPrivate::keyboardGrabber = this;
    }
}

void QWidget::releaseKeyboard()
{
    if (!qt_nograb() && QWidgetPrivate::keyboardGrabber == this) {
        gi_ungrab_keyboard();
        QWidgetPrivate::keyboardGrabber = 0;
    }
}


QWidget *QWidget::mouseGrabber()
{
    return QWidgetPrivate::mouseGrabber;
}


QWidget *QWidget::keyboardGrabber()
{
    return QWidgetPrivate::keyboardGrabber;
}


void QWidget::destroy(bool destroyWindow, bool destroySubWindows)
{
    Q_D(QWidget);
    d->aboutToDestroy();
    if (!isWindow() && parentWidget())
        parentWidget()->d_func()->invalidateBuffer(d->effectiveRectFor(geometry()));
    d->deactivateWidgetCleanup();
    if (testAttribute(Qt::WA_WState_Created)) {
        setAttribute(Qt::WA_WState_Created, false);
        QObjectList childList = children();
        for (int i = 0; i < childList.size(); ++i) { // destroy all widget children
            register QObject *obj = childList.at(i);
            if (obj->isWidgetType())
                static_cast<QWidget*>(obj)->destroy(destroySubWindows,
                                                    destroySubWindows);
        }
        if (QWidgetPrivate::mouseGrabber == this)
            releaseMouse();
        if (QWidgetPrivate::keyboardGrabber == this)
            releaseKeyboard();
        //if (isWindow())
         //   X11->deferred_map.removeAll(this);
        if (isModal()) {
            // just be sure we leave modal
            QApplicationPrivate::leaveModal(this);
        }
        else if ((windowType() == Qt::Popup))
            qApp->d_func()->closePopup(this);

        if ((windowType() == Qt::Desktop)) {
            //if (acceptDrops())
            dndEnable(this, false);
        } else {
            if (isWindow())
                dndEnable(this, false);
            if (destroyWindow)
                qt_XDestroyWindow(  data->winid);
        }
        QT_TRY {
            d->setWinId(0);
        } QT_CATCH (const std::bad_alloc &) {
            // swallow - destructors must not throw
        }       

        if(d->ic) {
            delete d->ic;
        } else {
            // release previous focus information participating with
            // preedit preservation of qic
            QInputContext *qic = QApplicationPrivate::inputContext;
            if (qic)
                qic->widgetDestroyed(this);
        }
    }
}

