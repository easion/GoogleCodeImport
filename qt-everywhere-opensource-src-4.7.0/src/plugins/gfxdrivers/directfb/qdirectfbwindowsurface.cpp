/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdirectfbwindowsurface.h"
#include "qdirectfbscreen.h"
#include "qdirectfbpaintengine.h"

#include <private/qwidget_p.h>
#include <qwidget.h>
#include <qwindowsystem_qws.h>
#include <qpaintdevice.h>
#include <qvarlengtharray.h>

#ifndef QT_NO_QWS_DIRECTFB

QT_BEGIN_NAMESPACE

QGixWindowSurface::QGixWindowSurface(PLATFORM_CLIP_FLAGS flip, QGixScreen *scr)
    : QGixPaintDevice(scr)
    , sibling(0)
    , dfbWindow(0)
    , flipFlags(flip)
    , boundingRectFlip(scr->gixFlags() & QGixScreen::BoundingRectFlip)
    , flushPending(false)
{
    setSurfaceFlags(Opaque | Buffered);
#ifdef QT_DIRECTFB_TIMING
    frames = 0;
    timer.start();
#endif
}

QGixWindowSurface::QGixWindowSurface(PLATFORM_CLIP_FLAGS flip, QGixScreen *scr, QWidget *widget)
    : QWSWindowSurface(widget), QGixPaintDevice(scr)
    , sibling(0)
    , dfbWindow(0)
    , flipFlags(flip)
    , boundingRectFlip(scr->gixFlags() & QGixScreen::BoundingRectFlip)
    , flushPending(false)
{
    SurfaceFlags flags = 0;
    if (!widget || widget->window()->windowOpacity() == 0xff)
        flags |= Opaque;

    setSurfaceFlags(flags);
#ifdef QT_DIRECTFB_TIMING
    frames = 0;
    timer.start();
#endif
}

QGixWindowSurface::~QGixWindowSurface()
{
    releaseSurface();
    // these are not tracked by QGixScreen so we don't want QGixPaintDevice to release it
}

bool QGixWindowSurface::isValid() const
{
    return true;
}

#ifdef QT_DIRECTFB_WM
void QGixWindowSurface::raise()
{
	gi_window_id_t window;

    if ( window = gixWindow()) {
		gi_raise_window(window);
        //window->RaiseToTop(window);
    }
}

gi_window_id_t QGixWindowSurface::gixWindow() const
{
    return (dfbWindow ? dfbWindow : (sibling ? sibling->dfbWindow : 0));
}

void QGixWindowSurface::createWindow(const QRect &rect)
{
    //IGixDisplayLayer *layer = screen->dfbDisplayLayer();
    //if (!layer)
     //   qFatal("QGixWindowSurface: Unable to get primary display layer!");

    updateIsOpaque();

    //DFBWindowDescription description;
    //memset(&description, 0, sizeof(DFBWindowDescription));

    //description.flags = DWDESC_CAPS|DWDESC_HEIGHT|DWDESC_WIDTH|DWDESC_POSX|DWDESC_POSY|DWDESC_SURFACE_CAPS|DWDESC_PIXELFORMAT;
    //description.caps = DWCAPS_NODECORATION;
    //description.surface_caps = DSCAPS_NONE;
    imageFormat = screen->pixelFormat();

    if (!(surfaceFlags() & Opaque)) {
        imageFormat = screen->alphaPixmapFormat();
        //description.caps |= DWCAPS_ALPHACHANNEL;

    }
    int pixelformat = QGixScreen::getSurfacePixelFormat(imageFormat);
    int posx = rect.x();
    int posy = rect.y();
    int width = rect.width();
    int height = rect.height();

    if (QGixScreen::isPremultiplied(imageFormat)){
     //   int surface_caps = DSCAPS_PREMULTIPLIED;
	}

    if (screen->gixFlags() & QGixScreen::VideoOnly){
     //   int surface_caps |= DSCAPS_VIDEOONLY;
	}

	qWarning("gi_create_window: %d,%d,%d,%d\n", posx,posy,width,height);

	dfbWindow = gi_create_window(GI_DESKTOP_WINDOW_ID,posx,posy,width,height, GI_RGB( 192, 192, 192 ),0 );
    //int result = layer->CreateWindow(layer, &description, &dfbWindow);

    if (dfbWindow == 0){
        //GixErrorFatal("QGixWindowSurface::createWindow");
	}

    if (window()) {
        if (window()->windowFlags() & Qt::WindowStaysOnTopHint) {
            //dfbWindow->SetStackingClass(dfbWindow, DWSC_UPPER);
        }
        /*DFBWindowID winid;
        result = dfbWindow->GetID(dfbWindow, &winid);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixWindowSurface::createWindow. Can't get ID", result);
        } else {
            window()->setProperty("_q_GixWindowID", winid);
        }
		*/
    }

    Q_ASSERT(!dfbSurface);
    //dfbWindow->GetSurface(dfbWindow, &dfbSurface); // FIXME  DPP
}

static int setWindowGeometry(gi_window_id_t dfbWindow, const QRect &old, const QRect &rect)
{
    int result = 0;
    const bool isMove = old.isEmpty() || rect.topLeft() != old.topLeft();
    const bool isResize = rect.size() != old.size();

    /*if (isResize && isMove) {
        result = dfbWindow->SetBounds(dfbWindow, rect.x(), rect.y(),
                                      rect.width(), rect.height());
		return;
    }*/  
		
	if (isResize) {
        result = gi_resize_window(dfbWindow,
                                   rect.width(), rect.height());
    } else if (isMove) {
        result = gi_move_window(dfbWindow, rect.x(), rect.y());
    }

    return result;
}
#endif // QT_DIRECTFB_WM

void QGixWindowSurface::setGeometry(const QRect &rect)
{
    const QRect oldRect = geometry();
    if (oldRect == rect)
        return;

    PLATFORM_SURFACE *oldSurface = dfbSurface;
    const bool sizeChanged = oldRect.size() != rect.size();
    if (sizeChanged) {
        delete engine;
        engine = 0;
        releaseSurface();
        Q_ASSERT(!dfbSurface);
    }

    if (rect.isNull()) {
#if 0
        if (dfbWindow) {
            if (window())
                window()->setProperty("_q_GixWindowID", QVariant());

            dfbWindow->Release(dfbWindow);
            dfbWindow = 0;
        }
#endif
        Q_ASSERT(!dfbSurface);

    } else {
#ifdef QT_DIRECTFB_WM
        if (!dfbWindow) {
            createWindow(rect);
        } else {
            setWindowGeometry(dfbWindow, oldRect, rect);
            Q_ASSERT(!sizeChanged || !dfbSurface);
            if (sizeChanged){
                //dfbWindow->GetSurface(dfbWindow, &dfbSurface); //FIXME DPP
			}
        }
      
#endif
    }
    if (oldSurface != dfbSurface) {
        imageFormat = dfbSurface ? QGixScreen::pixelFormatToQt(dfbSurface->PixelFormat) : QImage::Format_Invalid;
    }

    if (oldRect.size() != rect.size()) {
        QWSWindowSurface::setGeometry(rect);
    } else {
        QWindowSurface::setGeometry(rect);
    }
}

QByteArray QGixWindowSurface::permanentState() const
{
    QByteArray state(sizeof(this), 0);
    *reinterpret_cast<const QGixWindowSurface**>(state.data()) = this;
    return state;
}

void QGixWindowSurface::setPermanentState(const QByteArray &state)
{
    if (state.size() == sizeof(this)) {
        sibling = *reinterpret_cast<QGixWindowSurface *const*>(state.constData());
        Q_ASSERT(sibling);
        setSurfaceFlags(sibling->surfaceFlags());
    }
}

bool QGixWindowSurface::scroll(const QRegion &region, int dx, int dy)
{
	int DSFLIP_BLIT = 2;
    if (!dfbSurface || !(flipFlags & DSFLIP_BLIT) || region.rectCount() != 1)
        return false;
    if (flushPending) {
        //dfbSurface->Flip(dfbSurface, 0, DSFLIP_BLIT);
    } else {
        flushPending = true;
    }
    //dfbSurface->SetBlittingFlags(dfbSurface, DSBLIT_NOFX); //FIXME DPP
    const QRect r = region.boundingRect();
    const gi_cliprect_t rect = { r.x(), r.y(), r.width(), r.height() };
    //dfbSurface->Blit(dfbSurface, dfbSurface, &rect, r.x() + dx, r.y() + dy);
    return true;
}

bool QGixWindowSurface::move(const QPoint &moveBy)
{
    setGeometry(geometry().translated(moveBy));
    return true;
}

void QGixWindowSurface::setOpaque(bool opaque)
{
    SurfaceFlags flags = surfaceFlags();
    if (opaque != (flags & Opaque)) {
        if (opaque) {
            flags |= Opaque;
        } else {
            flags &= ~Opaque;
        }
        setSurfaceFlags(flags);
    }
}


void QGixWindowSurface::flush(QWidget *widget, const QRegion &region,
                                   const QPoint &offset)
{
    QWidget *win = window();
    if (!win)
        return;

    QWExtra *extra = qt_widget_private(widget)->extraData();
    if (extra && extra->proxyWidget)
        return;

    const quint8 windowOpacity = quint8(win->windowOpacity() * 0xff);
    const QRect windowGeometry = geometry();
#ifdef QT_DIRECTFB_WM
    //quint8 currentOpacity;
    Q_ASSERT(dfbWindow);
    //dfbWindow->GetOpacity(dfbWindow, &currentOpacity);
    if (currentOpacity != windowOpacity) {
		if (windowOpacity == 0)		{
			gi_hide_window(dfbWindow);
		}
		else{
			gi_show_window(dfbWindow);
		}
        //dfbWindow->SetOpacity(dfbWindow, windowOpacity);
		currentOpacity = windowOpacity;
    }

    screen->flipSurface(dfbSurface, flipFlags, region, offset);

#endif

#ifdef QT_DIRECTFB_TIMING
    enum { Secs = 3 };
    ++frames;
    if (timer.elapsed() >= Secs * 1000) {
        qDebug("%d fps", int(double(frames) / double(Secs)));
        frames = 0;
        timer.restart();
    }
#endif
    flushPending = false;
}

void QGixWindowSurface::beginPaint(const QRegion &)
{
    if (!engine) {
        engine = new QGixPaintEngine(this);
    }
    flushPending = true;
}

void QGixWindowSurface::endPaint(const QRegion &)
{
#ifdef QT_NO_DIRECTFB_SUBSURFACE
    unlockSurface();
#endif
}

PLATFORM_SURFACE *QGixWindowSurface::gixSurface() const
{
    if (!dfbSurface && sibling && sibling->dfbSurface)
        return sibling->dfbSurface;
    return dfbSurface;
}


PLATFORM_SURFACE *QGixWindowSurface::surfaceForWidget(const QWidget *widget, QRect *rect) const
{
    Q_ASSERT(widget);
    if (!dfbSurface) {
        if (sibling && (!sibling->sibling || sibling->dfbSurface)){
            //return sibling->surfaceForWidget(widget, rect); //FIXME DPP
		}
        return 0;
    }
    QWidget *win = window();
    Q_ASSERT(win);
    if (rect) {
        if (win == widget) {
            *rect = widget->rect();
        } else {
            *rect = QRect(widget->mapTo(win, QPoint(0, 0)), widget->size());
        }
    }

    Q_ASSERT(win == widget || win->isAncestorOf(widget));
    return dfbSurface;
}

void QGixWindowSurface::releaseSurface()
{
    if (dfbSurface) {
        unlockSurface();
        GixReleaseSurface(dfbSurface);
        dfbSurface = 0;
    }
}

void QGixWindowSurface::updateIsOpaque()
{
    const QWidget *win = window();
    Q_ASSERT(win);
    if (win->testAttribute(Qt::WA_OpaquePaintEvent) || win->testAttribute(Qt::WA_PaintOnScreen)) {
        setOpaque(true);
        return;
    }

    if (qFuzzyCompare(static_cast<float>(win->windowOpacity()), 1.0f)) {
        const QPalette &pal = win->palette();

        if (win->autoFillBackground()) {
            const QBrush &autoFillBrush = pal.brush(win->backgroundRole());
            if (autoFillBrush.style() != Qt::NoBrush && autoFillBrush.isOpaque()) {
                setOpaque(true);
                return;
            }
        }

        if (win->isWindow() && !win->testAttribute(Qt::WA_NoSystemBackground)) {
            const QBrush &windowBrush = win->palette().brush(QPalette::Window);
            if (windowBrush.style() != Qt::NoBrush && windowBrush.isOpaque()) {
                setOpaque(true);
                return;
            }
        }
    }
    setOpaque(false);
}

QT_END_NAMESPACE

#endif // QT_NO_QWS_DIRECTFB
