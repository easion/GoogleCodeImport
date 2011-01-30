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

#include "qdirectfbscreen.h"
#include "qdirectfbwindowsurface.h"
#include "qdirectfbpixmap.h"
#include "qdirectfbmouse.h"
#include "qdirectfbkeyboard.h"
#include <QtGui/qwsdisplay_qws.h>
#include <QtGui/qcolor.h>
#include <QtGui/qapplication.h>
#include <QtGui/qwindowsystem_qws.h>
#include <QtGui/private/qgraphicssystem_qws_p.h>
#include <QtGui/private/qwssignalhandler_p.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qvector.h>
#include <QtCore/qrect.h>

#ifndef QT_NO_QWS_DIRECTFB

QT_BEGIN_NAMESPACE


class QGixScreenPrivate : public QObject, public QWSGraphicsSystem
{
    Q_OBJECT
public:
    QGixScreenPrivate(QGixScreen *qptr);
    ~QGixScreenPrivate();

    void setFlipFlags(const QStringList &args);
    QPixmapData *createPixmapData(QPixmapData::PixelType type) const;
public slots:
#ifdef QT_DIRECTFB_WM
    void onWindowEvent(QWSWindow *window, QWSServer::WindowEvent event);
#endif
public:

    PLATFORM_CLIP_FLAGS flipFlags;
    QGixScreen::GixFlags gixFlags;
    QImage::Format alphaPixmapFormat;

    QSet<PLATFORM_SURFACE*> allocatedSurfaces;

#ifndef QT_NO_DIRECTFB_MOUSE
    QGixMouseHandler *mouse;
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    QGixKeyboardHandler *keyboard;
#endif

    QGixScreen *q;
    static QGixScreen *instance;
};

QGixScreen *QGixScreenPrivate::instance = 0;

QGixScreenPrivate::QGixScreenPrivate(QGixScreen *qptr)
    : QWSGraphicsSystem(qptr),  
      gixFlags(QGixScreen::NoFlags), alphaPixmapFormat(QImage::Format_Invalid),

#ifndef QT_NO_DIRECTFB_MOUSE
     mouse(0)
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    , keyboard(0)
#endif

    //, cursorSurface(0)
    //, cursorImageKey(0)
    , q(qptr)
{
#ifndef QT_NO_QWS_SIGNALHANDLER
    QWSSignalHandler::instance()->addObject(this);
#endif
#ifdef QT_DIRECTFB_WM
    connect(QWSServer::instance(), SIGNAL(windowEvent(QWSWindow*,QWSServer::WindowEvent)),
            this, SLOT(onWindowEvent(QWSWindow*,QWSServer::WindowEvent)));
#endif
}

QGixScreenPrivate::~QGixScreenPrivate()
{
#ifndef QT_NO_DIRECTFB_MOUSE
    delete mouse;
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    delete keyboard;
#endif

    for (QSet<PLATFORM_SURFACE*>::const_iterator it = allocatedSurfaces.begin(); it != allocatedSurfaces.end(); ++it) {
        GixReleaseSurface(*it);
    }

	gi_exit(); 
}

PLATFORM_SURFACE *QGixScreen::createDFBSurface(const QImage &image, QImage::Format format, PLATFORM_CREATE_OPTIONS options, int *resultPtr)
{
    if (image.isNull()) // assert?
        return 0;

    if (QGixScreen::getSurfacePixelFormat(format) == GI_RENDER_UNKNOWN) {
        format = QGixPixmapData::hasAlphaChannel(image) ? d_ptr->alphaPixmapFormat : pixelFormat();
    }
    if (image.format() != format) {
        return createDFBSurface(image.convertToFormat(format), format, options | NoPreallocated, resultPtr);
    }

    //DFBSurfaceDescription description;
    //memset(&description, 0, sizeof(DFBSurfaceDescription));
    int width = image.width();
    int height = image.height();
    //int flags = DSDESC_WIDTH|DSDESC_HEIGHT|DSDESC_PIXELFORMAT;
	gi_format_code_t pixelformat = QGixScreen::getSurfacePixelFormat(format);

    //initSurfaceDescriptionPixelFormat(&description, format);
    bool doMemCopy = true;

    int result;
    PLATFORM_SURFACE *surface = createDFBSurface(width,height,pixelformat, options, &result);
    if (resultPtr)
        *resultPtr = result;
    if (!surface) {
        PLATFORM_ERROR("Couldn't create surface createDFBSurface(QImage, QImage::Format, PLATFORM_CREATE_OPTIONS)", result);
        return 0;
    }
    if (doMemCopy) {
        int bplDFB;
        uchar *mem = QGixScreen::lockSurface(surface, 2, &bplDFB);
        if (mem) {
            const int height = image.height();
            const int bplQt = image.bytesPerLine();
            if (bplQt == bplDFB && bplQt == (image.width() * image.depth() / 8)) {
                memcpy(mem, image.bits(), image.byteCount());
            } else {
                for (int i=0; i<height; ++i) {
                    memcpy(mem, image.scanLine(i), bplQt);
                    mem += bplDFB;
                }
            }
            //surface->Unlock(surface);
        }
    }

    return surface;
}

PLATFORM_SURFACE *QGixScreen::copyDFBSurface(PLATFORM_SURFACE *src,
                                                  QImage::Format format,
                                                  PLATFORM_CREATE_OPTIONS options,
                                                  int *result)
{
    Q_ASSERT(src);
    QSize size;
#if (0) //FIXME DPP
    src->GetSize(src, &size.rwidth(), &size.rheight());
    PLATFORM_SURFACE *surface = createDFBSurface(size, format, options, result);
    DFBSurfaceBlittingFlags flags = QGixScreen::hasAlphaChannel(surface)
                                    ? DSBLIT_BLEND_ALPHACHANNEL
                                    : DSBLIT_NOFX;
    if (flags & DSBLIT_BLEND_ALPHACHANNEL)
        surface->Clear(surface, 0, 0, 0, 0);

    surface->SetBlittingFlags(surface, flags);
    surface->Blit(surface, src, 0, 0, 0);
    surface->ReleaseSource(surface);
    return surface;
#endif
	return 0;
}

PLATFORM_SURFACE *QGixScreen::createDFBSurface(const QSize &size,
                                                    QImage::Format format,
                                                    PLATFORM_CREATE_OPTIONS options,
                                                    int *result)
{
    //DFBSurfaceDescription desc;
    //memset(&desc, 0, sizeof(DFBSurfaceDescription));
    //desc.flags |= DSDESC_WIDTH|DSDESC_HEIGHT;
	gi_format_code_t pixelformat = QGixScreen::getSurfacePixelFormat(format);
    //if (!QGixScreen::initSurfaceDescriptionPixelFormat(&desc, format))
    //    return 0;
    int width = size.width();
    int height = size.height();
    return createDFBSurface(width, height, pixelformat, options, result);
}


void GixReleaseSurface(PLATFORM_SURFACE *surface)
{
}

PLATFORM_SURFACE *GixCreateSurface(gi_window_id_t original_dc, gi_format_code_t gformat, int width,int height, void *bits)
{
  //void *bits = NULL;
  int i,pitch;
  PLATFORM_SURFACE *surface;
  gi_window_id_t pixmap = 0;
  gi_bool_t needfree = FALSE;

  pitch = gi_image_get_pitch(gformat, width);

  surface = (PLATFORM_SURFACE*)calloc(1,sizeof(PLATFORM_SURFACE));

  if (!surface)
	  return NULL;

#if 1

	if (!bits){
	  bits = malloc(height * pitch);
	  needfree = TRUE;
	}
	else{
		needfree = FALSE;
	}

  if (!bits)
  goto FAIL;
  memset(bits,0, height * pitch);

  pixmap = gi_create_pixmap_compatible(original_dc,width,height,gformat,pitch, bits);
#else
  pixmap = gi_create_pixmap_window(original_dc, width,height,gformat);
#endif

  surface->gc = gi_create_gc(pixmap,NULL);
  surface->mem = bits;
  surface->pixmap = pixmap;
  surface->PixelFormat = (gi_format_code_t)gformat;
  surface->pitch = pitch;
  surface->width = width;
  surface->height = height;
  surface->is_memdc = TRUE;
  return surface;

FAIL:
	if (surface)
	  free(surface);
	return NULL;
}

PLATFORM_SURFACE *QGixScreen::createDFBSurface(int width, int height,gi_format_code_t f,
	PLATFORM_CREATE_OPTIONS options, int *resultPtr)
{
    int tmp;
    int &result = (resultPtr ? *resultPtr : tmp);
    result = 0;
    PLATFORM_SURFACE *newSurface = 0;

   /* if (d_ptr->gixFlags & VideoOnly
        && !(desc.flags & DSDESC_PREALLOCATED)
        && (!(desc.flags & DSDESC_CAPS) || !(desc.caps & DSCAPS_SYSTEMONLY))) {
        // Add the video only capability. This means the surface will be created in video ram
        if (!(desc.flags & DSDESC_CAPS)) {
            desc.caps = DSCAPS_VIDEOONLY;
            desc.flags |= DSDESC_CAPS;
        } else {
            desc.caps |= DSCAPS_VIDEOONLY;
        }
        result = d_ptr->dfb->CreateSurface(d_ptr->dfb, &desc, &newSurface);
        if (result != DFB_OK
#ifdef QT_NO_DEBUG
            && (desc.flags & DSDESC_CAPS) && (desc.caps & DSCAPS_PRIMARY)
#endif
            ) {
            qWarning("QGixScreen::createDFBSurface() Failed to create surface in video memory!\n");
        }
        //desc.caps &= ~DSCAPS_VIDEOONLY;
    }*/

    //if (d_ptr->gixFlags & SystemOnly)
     //   desc.caps |= DSCAPS_SYSTEMONLY;

    if (!newSurface)
        newSurface = GixCreateSurface(0,f, width,height,NULL);

    if (!newSurface) {
        qWarning("QGixScreen::createDFBSurface() Failed!\n");
        return 0;
    }

    Q_ASSERT(newSurface);

    if (options & TrackSurface) {
        d_ptr->allocatedSurfaces.insert(newSurface);
    }

    return newSurface;
}


void QGixScreen::releaseDFBSurface(PLATFORM_SURFACE *surface)
{
    Q_ASSERT(QGixScreen::instance());
    Q_ASSERT(surface);
    GixReleaseSurface(surface);
    if (!d_ptr->allocatedSurfaces.remove(surface))
        qWarning("QGixScreen::releaseDFBSurface() - %p not in list", surface);

    //qDebug("Released surface at %p. New count = %d", surface, d_ptr->allocatedSurfaces.count());
}

QGixScreen::GixFlags QGixScreen::gixFlags() const
{
    return d_ptr->gixFlags;
}


gi_format_code_t QGixScreen::getSurfacePixelFormat(QImage::Format format)
{
    switch (format) {

    //case QImage::Format_Indexed8:
    //    return GI_RENDER_r3g3b2;

    case QImage::Format_RGB888:
        return GI_RENDER_r8g8b8;
    case QImage::Format_ARGB4444_Premultiplied:
        return GI_RENDER_a4b4g4r4;

    case QImage::Format_RGB444:
        return GI_RENDER_x4b4g4r4;
    case QImage::Format_RGB555:
        return GI_RENDER_x1r5g5b5;

    case QImage::Format_RGB16:
        return GI_RENDER_r5g6b5;

    //case QImage::Format_ARGB6666_Premultiplied:
     //   return DSPF_ARGB6666;
    //case QImage::Format_RGB666:
    //    return DSPF_RGB18;

    case QImage::Format_RGB32:
        return GI_RENDER_x8r8g8b8;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB32:
        return GI_RENDER_a8r8g8b8;
    default:
        return (gi_format_code_t)GI_RENDER_UNKNOWN;
    };
}

QImage::Format QGixScreen::pixelFormatToQt(gi_format_code_t format)
{
    //gi_format_code_t format;
    //format = surface->PixelFormat;

    switch (format) {
    case GI_RENDER_r3g3b2:
        return QImage::Format_Indexed8;
    case GI_RENDER_r8g8b8:
        return QImage::Format_RGB888;
    case GI_RENDER_a4b4g4r4:
        return QImage::Format_ARGB4444_Premultiplied;

    case GI_RENDER_x4b4g4r4:
        return QImage::Format_RGB444;
    //case GI_RENDER_x1r5g5b5:

    case GI_RENDER_x1r5g5b5:
        return QImage::Format_RGB555;
    case GI_RENDER_r5g6b5:
        return QImage::Format_RGB16;

    //case DSPF_ARGB6666:
    //    return QImage::Format_ARGB6666_Premultiplied;
    //case DSPF_RGB18:
    //    return QImage::Format_RGB666;

    case GI_RENDER_x8r8g8b8:
        return QImage::Format_RGB32;
    case GI_RENDER_a8r8g8b8: {
        /*DFBSurfaceCapabilities caps;
        const int result = surface->GetCapabilities(surface, &caps);
        Q_ASSERT(result == DFB_OK);
        Q_UNUSED(result);
        return (caps & DSCAPS_PREMULTIPLIED
                ? QImage::Format_ARGB32_Premultiplied
                : QImage::Format_ARGB32); */

		//return QImage::Format_ARGB32_Premultiplied;
		return QImage::Format_ARGB32;
		}
    default:
        break;
    }
    return QImage::Format_Invalid;
}



#if defined QT_DIRECTFB_CURSOR
class Q_GUI_EXPORT QGixScreenCursor : public QScreenCursor
{
public:
    QGixScreenCursor();
    virtual void set(const QImage &image, int hotx, int hoty);
    virtual void move(int x, int y);
    virtual void show();
    virtual void hide();
private:

    //IGixDisplayLayer *layer;
};

QGixScreenCursor::QGixScreenCursor()
{
    //IGix *fb = QGixScreen::instance()->dfb();
    //if (!fb)
     //   qFatal("QGixScreenCursor: Gix not initialized");

    //layer = QGixScreen::instance()->dfbDisplayLayer();
    //Q_ASSERT(layer);

    enable = false;
    hwaccel = true;
    supportsAlpha = true;

}

void QGixScreenCursor::move(int x, int y)
{
    pos = QPoint(x, y);

    gi_move_cursor( x, y);
}

void QGixScreenCursor::hide()
{
    if (enable) {
        enable = false;
        int result;
		gi_window_id_t window = 0;

		window = 0;//fixme

		gi_load_cursor(window, GI_CURSOR_NONE);

        /*result = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::hide: "
                          "Unable to set cooperative level", result);
        }
        result = layer->SetCursorOpacity(layer, 0);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::hide: "
                          "Unable to set cursor opacity", result);
        }
        result = layer->SetCooperativeLevel(layer, DLSCL_SHARED);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::hide: "
                          "Unable to set cooperative level", result);
        }*/

    }
}

void QGixScreenCursor::show()
{
    if (!enable) {
        enable = true;

		gi_window_id_t window = 0;

		window = 0;//fixme

		gi_load_cursor(window, GI_CURSOR_USER0);
        /*int result;
        result = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }
        result = layer->SetCursorOpacity(layer,

                                         255
            );
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::show: "
                          "Unable to set cursor shape", result);
        }
        result = layer->SetCooperativeLevel(layer, DLSCL_SHARED);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }*/

    }
}

void QGixScreenCursor::set(const QImage &image, int hotx, int hoty)
{
    QGixScreen *screen = QGixScreen::instance();
    if (!screen)
        return;

    if (image.isNull()) {
        cursor = QImage();
        hide();
    } else {
        cursor = image.convertToFormat(screen->alphaPixmapFormat());
        size = cursor.size();
        hotspot = QPoint(hotx, hoty);
        int result = 0;

		//gi_setup_cursor(GI_CURSOR_USER0,hotx, hoty,img);

#if 0
        PLATFORM_SURFACE *surface = screen->createDFBSurface(cursor, screen->alphaPixmapFormat(),
                                                             QGixScreen::DontTrackSurface, &result);
        if (!surface) {
            PLATFORM_ERROR("QGixScreenCursor::set: Unable to create surface", result);
            return;
        }

        result = layer->SetCooperativeLevel(layer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }
        result = layer->SetCursorShape(layer, surface, hotx, hoty);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::show: "
                          "Unable to set cursor shape", result);
        }
        result = layer->SetCooperativeLevel(layer, DLSCL_SHARED);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::show: "
                          "Unable to set cooperative level", result);
        }
        
        GixReleaseSurface(surface);
#endif
        show();
    }

}
#endif // QT_DIRECTFB_CURSOR

QGixScreen::QGixScreen(int display_id)
    : QScreen(display_id, DirectFBClass), d_ptr(new QGixScreenPrivate(this))
{
    QGixScreenPrivate::instance = this;
}

QGixScreen::~QGixScreen()
{
    if (QGixScreenPrivate::instance == this)
        QGixScreenPrivate::instance = 0;
    delete d_ptr;
}

QGixScreen *QGixScreen::instance()
{
    return QGixScreenPrivate::instance;
}

int QGixScreen::depth(gi_format_code_t format)
{
    switch (format) {
    case GI_RENDER_a1:
        return 1;
    case GI_RENDER_a8:
    case GI_RENDER_r3g3b2:
    //case DSPF_ALUT44:
        return 8;
    //case GI_RENDER_yuy2:
    case GI_RENDER_yv12:
    case GI_RENDER_x4b4g4r4:
        return 12;
    case GI_RENDER_x1r5g5b5:
        return 15;
    case GI_RENDER_a1r5g5b5:
    case GI_RENDER_r5g6b5:
    case GI_RENDER_yuy2:
    case GI_RENDER_a4b4g4r4:
        return 16;
    case GI_RENDER_r8g8b8:
        return 24;
    case GI_RENDER_x8r8g8b8:
    case GI_RENDER_a8r8g8b8:
        return 32;
    case GI_RENDER_UNKNOWN:
    default:
        return 0;
    };
    return 0;
}

int QGixScreen::depth(QImage::Format format)
{
    int depth = 0;
    switch(format) {
    case QImage::Format_Invalid:
    case QImage::NImageFormats:
        Q_ASSERT(false);
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
        depth = 1;
        break;
    case QImage::Format_Indexed8:
        depth = 8;
        break;
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        depth = 32;
        break;
    case QImage::Format_RGB555:
    case QImage::Format_RGB16:
    case QImage::Format_RGB444:
    case QImage::Format_ARGB4444_Premultiplied:
        depth = 16;
        break;
    case QImage::Format_RGB666:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_RGB888:
        depth = 24;
        break;
    }
    return depth;
}

void QGixScreenPrivate::setFlipFlags(const QStringList &args)
{
    QRegExp flipRegexp(QLatin1String("^flip=([\\w,]*)$"));
    int index = args.indexOf(flipRegexp);
    if (index >= 0) {
        const QStringList flips = flipRegexp.cap(1).split(QLatin1Char(','),
                                                          QString::SkipEmptyParts);
        /*flipFlags = DSFLIP_NONE;
        foreach(const QString &flip, flips) {
            if (flip == QLatin1String("wait"))
                flipFlags |= DSFLIP_WAIT;
            else if (flip == QLatin1String("blit"))
                flipFlags |= DSFLIP_BLIT;
            else if (flip == QLatin1String("onsync"))
                flipFlags |= DSFLIP_ONSYNC;
            else if (flip == QLatin1String("pipeline"))
                flipFlags |= DSFLIP_PIPELINE;
            else
                qWarning("QGixScreen: Unknown flip argument: %s",
                         qPrintable(flip));
						 
        }*/
    } else {
        //flipFlags = DSFLIP_BLIT|DSFLIP_ONSYNC;
    }
}

#ifdef QT_DIRECTFB_WM
void QGixScreenPrivate::onWindowEvent(QWSWindow *window, QWSServer::WindowEvent event)
{
    if (event == QWSServer::Raise) {
        QWSWindowSurface *windowSurface = window->windowSurface();
        if (windowSurface && windowSurface->key() == QLatin1String("gix")) {
            static_cast<QGixWindowSurface*>(windowSurface)->raise();
        }
    }
}
#endif

QPixmapData *QGixScreenPrivate::createPixmapData(QPixmapData::PixelType type) const
{
    if (type == QPixmapData::BitmapType)
        return QWSGraphicsSystem::createPixmapData(type);

    return new QGixPixmapData(q, type);
}



static inline bool setIntOption(const QStringList &arguments, const QString &variable, int *value)
{
    Q_ASSERT(value);
    QRegExp rx(QString::fromLatin1("%1=?(\\d+)").arg(variable));
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    if (arguments.indexOf(rx) != -1) {
        *value = rx.cap(1).toInt();
        return true;
    }
    return false;
}

static inline QColor colorFromName(const QString &name)
{
    QRegExp rx(QLatin1String("#([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])([0-9a-f][0-9a-f])"));
    rx.setCaseSensitivity(Qt::CaseInsensitive);
    if (rx.exactMatch(name)) {
        Q_ASSERT(rx.captureCount() == 4);
        int ints[4];
        int i;
        for (i=0; i<4; ++i) {
            bool ok;
            ints[i] = rx.cap(i + 1).toUInt(&ok, 16);
            if (!ok || ints[i] > 255)
                break;
        }
        if (i == 4)
            return QColor(ints[0], ints[1], ints[2], ints[3]);
    }
    return QColor(name);
}

bool QGixScreen::connect(const QString &displaySpec)
{
    int result = 0;

    {   // pass command line arguments to Gix
        const QStringList args = QCoreApplication::arguments();
        int argc = args.size();
        char **argv = new char*[argc];

        for (int i = 0; i < argc; ++i)
            argv[i] = qstrdup(args.at(i).toLocal8Bit().constData());

		result = gi_init();

        if (result < 0) {
            PLATFORM_ERROR("QGixScreen: error initializing Gix");
        }
        delete[] argv;
    }

    const QStringList displayArgs = displaySpec.split(QLatin1Char(':'),
                                                      QString::SkipEmptyParts);

    //d_ptr->setFlipFlags(displayArgs);

    //result = GixCreate(&d_ptr->dfb);
   // if (result != DFB_OK) {
    //    PLATFORM_ERROR("QGixScreen: error creating Gix interface",
    //                  result);
     //   return false;
    //}

    if (displayArgs.contains(QLatin1String("videoonly"), Qt::CaseInsensitive))
        d_ptr->gixFlags |= VideoOnly;

    if (displayArgs.contains(QLatin1String("systemonly"), Qt::CaseInsensitive)) {
        if (d_ptr->gixFlags & VideoOnly) {
            qWarning("QGixScreen: error. videoonly and systemonly are mutually exclusive");
        } else {
            d_ptr->gixFlags |= SystemOnly;
        }
    }

    if (displayArgs.contains(QLatin1String("boundingrectflip"), Qt::CaseInsensitive)) {
        d_ptr->gixFlags |= BoundingRectFlip;
    } else if (displayArgs.contains(QLatin1String("nopartialflip"), Qt::CaseInsensitive)) {
        d_ptr->gixFlags |= NoPartialFlip;
    }


    if (displayArgs.contains(QLatin1String("fullscreen"))){
        //d_ptr->dfb->SetCooperativeLevel(d_ptr->dfb, DFSCL_FULLSCREEN);
	}

    const bool forcePremultiplied = displayArgs.contains(QLatin1String("forcepremultiplied"), Qt::CaseInsensitive);

    //DFBSurfaceDescription description;
    //memset(&description, 0, sizeof(DFBSurfaceDescription));
    //PLATFORM_SURFACE *surface;
	gi_format_code_t pixelformat = (gi_format_code_t)gi_screen_format(); // QGixScreen::getSurfacePixelFormat(format);


    //int flags = DSDESC_WIDTH|DSDESC_HEIGHT;
    //int width,height;
	
	//width =  height = 1;
    //surface = createDFBSurface(width,height,pixelformat, DontTrackSurface, &result);
    //if (!surface) {
    //    PLATFORM_ERROR("QGixScreen: error creating surface", result);
    //    return false;
    //}

    // Work out what format we're going to use for surfaces with an alpha channel
    QImage::Format pixelFormat = QGixScreen::pixelFormatToQt(pixelformat);
    d_ptr->alphaPixmapFormat = pixelFormat;

    switch (pixelFormat) {
    case QImage::Format_RGB666:
        d_ptr->alphaPixmapFormat = QImage::Format_ARGB6666_Premultiplied;
        break;
    case QImage::Format_RGB444:
        d_ptr->alphaPixmapFormat = QImage::Format_ARGB4444_Premultiplied;
        break;
    case QImage::Format_RGB32:
        pixelFormat = d_ptr->alphaPixmapFormat = QImage::Format_ARGB32_Premultiplied;
        // ### Format_RGB32 doesn't work so well with Qt. Force ARGB32 for windows/pixmaps
        break;
    case QImage::Format_Indexed8:
        qWarning("QGixScreen::connect(). Qt/Gix does not work with the LUT8  pixelformat.");
        return false;
    case QImage::NImageFormats:
    case QImage::Format_Invalid:
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_RGB888:
    case QImage::Format_RGB16:
    case QImage::Format_RGB555:
        d_ptr->alphaPixmapFormat = QImage::Format_ARGB32_Premultiplied;
        break;
    case QImage::Format_ARGB32:
        if (forcePremultiplied)
            d_ptr->alphaPixmapFormat = pixelFormat = QImage::Format_ARGB32_Premultiplied;
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
        // works already
        break;
    }
    setPixelFormat(pixelFormat);
    QScreen::d = QGixScreen::depth(pixelFormat);
    data = 0;
    lstep = 0;
    size = 0;

    //if (result != DFB_OK) {
    //    PLATFORM_ERROR("QGixScreen::connect: "
    //                  "Unable to get screen!", result);
     //   return false;
    //}
    const QString qws_size = QString::fromLatin1(qgetenv("QWS_SIZE"));
    if (!qws_size.isEmpty()) {
        QRegExp rx(QLatin1String("(\\d+)x(\\d+)"));
        if (!rx.exactMatch(qws_size)) {
            qWarning("QGixScreen::connect: Can't parse QWS_SIZE=\"%s\"", qPrintable(qws_size));
        } else {
            int *ints[2] = { &w, &h };
            for (int i=0; i<2; ++i) {
                *ints[i] = rx.cap(i + 1).toInt();
                if (*ints[i] <= 0) {
                    qWarning("QGixScreen::connect: %s is not a positive integer",
                             qPrintable(rx.cap(i + 1)));
                    w = h = 0;
                    break;
                }
            }
        }
    }

    setIntOption(displayArgs, QLatin1String("width"), &w);
    setIntOption(displayArgs, QLatin1String("height"), &h);
    //result = d_ptr->dfb->GetScreen(d_ptr->dfb, 0, &d_ptr->dfbScreen);

    if (w <= 0 || h <= 0) {

        /*PLATFORM_SURFACE *layerSurface;
        if (d_ptr->dfbLayer->GetSurface(d_ptr->dfbLayer, &layerSurface) != DFB_OK) {
            result = layerSurface->GetSize(layerSurface, &w, &h);
            layerSurface->Release(layerSurface);
        }
		*/

        if (w <= 0 || h <= 0) {
			w = gi_screen_width();
			h = gi_screen_height();

            //result = d_ptr->dfbScreen->GetSize(d_ptr->dfbScreen, &w, &h);
        }

        //if (result != DFB_OK) {
        //    PLATFORM_ERROR("QGixScreen::connect: "
         //                 "Unable to get screen size!", result);
          //  return false;
        //}
    }


    dw = w;
    dh = h;

    Q_ASSERT(dw != 0 && dh != 0);

    physWidth = physHeight = -1;
    setIntOption(displayArgs, QLatin1String("mmWidth"), &physWidth);
    setIntOption(displayArgs, QLatin1String("mmHeight"), &physHeight);
    const int dpi = 72;
    if (physWidth < 0)
        physWidth = qRound(dw * 25.4 / dpi);
    if (physHeight < 0)
        physHeight = qRound(dh * 25.4 / dpi);

    setGraphicsSystem(d_ptr);


#ifdef QT_DIRECTFB_WM
    //GixReleaseSurface(surface);
    QColor backgroundColor;
#endif

    QRegExp backgroundColorRegExp(QLatin1String("bgcolor=(.+)"));
    backgroundColorRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    if (displayArgs.indexOf(backgroundColorRegExp) != -1) {
        backgroundColor = colorFromName(backgroundColorRegExp.cap(1));
    }

    if (backgroundColor.isValid()) {

		qWarning("backgroundColor.isValid()\n");

#if 0 //FIXME DPP
		gi_set_window_background(window,
			 GI_ARGB(backgroundColor.alpha(),backgroundColor.red(), 
			backgroundColor.green(), backgroundColor.blue()),
			GI_BG_USE_COLOR	);
#endif

#if 0 //FIXME DPP
        int result = d_ptr->dfbLayer->SetCooperativeLevel(d_ptr->dfbLayer, DLSCL_ADMINISTRATIVE);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreen::connect "
                          "Unable to set cooperative level", result);
        }
        result = d_ptr->dfbLayer->SetBackgroundColor(d_ptr->dfbLayer, backgroundColor.red(), backgroundColor.green(),
                                                     backgroundColor.blue(), backgroundColor.alpha());
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::connect: "
                          "Unable to set background color", result);
        }

        result = d_ptr->dfbLayer->SetBackgroundMode(d_ptr->dfbLayer, DLBM_COLOR);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreenCursor::connect: "
                          "Unable to set background mode", result);
        }

        result = d_ptr->dfbLayer->SetCooperativeLevel(d_ptr->dfbLayer, DLSCL_SHARED);
        if (result != DFB_OK) {
            PLATFORM_ERROR("QGixScreen::connect "
                          "Unable to set cooperative level", result);
        }
#endif

    }


    return true;
}

void QGixScreen::disconnect()
{
	PLATFORM_GOT_LINE;

    foreach (PLATFORM_SURFACE *surf, d_ptr->allocatedSurfaces){
        GixReleaseSurface(surf);
	}

    d_ptr->allocatedSurfaces.clear();
	gi_exit();
}

bool QGixScreen::initDevice()
{
#ifndef QT_NO_DIRECTFB_MOUSE
    if (qgetenv("QWS_MOUSE_PROTO").isEmpty()) {
        QWSServer::instance()->setDefaultMouse("None");
        d_ptr->mouse = new QGixMouseHandler;
    }
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    if (qgetenv("QWS_KEYBOARD").isEmpty()) {
        QWSServer::instance()->setDefaultKeyboard("None");
        d_ptr->keyboard = new QGixKeyboardHandler(QString());
    }
#endif

#ifdef QT_DIRECTFB_CURSOR
    qt_screencursor = new QGixScreenCursor;
#elif !defined QT_NO_QWS_CURSOR
    QScreenCursor::initSoftwareCursor();
#endif
    return true;
}

void QGixScreen::shutdownDevice()
{
#ifndef QT_NO_DIRECTFB_MOUSE
    delete d_ptr->mouse;
    d_ptr->mouse = 0;
#endif
#ifndef QT_NO_DIRECTFB_KEYBOARD
    delete d_ptr->keyboard;
    d_ptr->keyboard = 0;
#endif

#ifndef QT_NO_QWS_CURSOR
    delete qt_screencursor;
    qt_screencursor = 0;
#endif
}

void QGixScreen::setMode(int width, int height, int depth)
{
	PLATFORM_GOT_LINE;
    //d_ptr->dfb->SetVideoMode(d_ptr->dfb, width, height, depth);
}

void QGixScreen::blank(bool on)
{
	PLATFORM_GOT_LINE;
    //d_ptr->dfbScreen->SetPowerMode(d_ptr->dfbScreen,
    //                               (on ? DSPM_ON : DSPM_SUSPEND));
}

QWSWindowSurface *QGixScreen::createSurface(QWidget *widget) const
{
	PLATFORM_GOT_LINE;
    return new QGixWindowSurface(d_ptr->flipFlags, const_cast<QGixScreen*>(this), widget);
}

QWSWindowSurface *QGixScreen::createSurface(const QString &key) const
{
	PLATFORM_GOT_LINE;
    if (key == QLatin1String("gix")) {
        return new QGixWindowSurface(d_ptr->flipFlags, const_cast<QGixScreen*>(this));
    }
    return QScreen::createSurface(key);
}



void QGixScreen::exposeRegion(QRegion r, int)
{
    Q_UNUSED(r);
	PLATFORM_GOT_LINE;

}

void QGixScreen::solidFill(const QColor &color, const QRegion &region)
{
#ifdef QT_DIRECTFB_WM
    Q_UNUSED(color);
    Q_UNUSED(region);

#endif
	PLATFORM_GOT_LINE;
}

QImage::Format QGixScreen::alphaPixmapFormat() const
{
    return d_ptr->alphaPixmapFormat;
}

uchar *QGixScreen::lockSurface(PLATFORM_SURFACE *surface, PLATFORM_LOCK_FLAGS flags, int *bpl)
{
    void *mem = 0;
    //const int result = surface->Lock(surface, flags, &mem, bpl);
    //if (result != DFB_OK) {
     //   PLATFORM_ERROR("QGixScreen::lockSurface()", result);
    //}

	if (surface-> is_memdc)
	{
	*bpl = surface->pitch;
	mem = surface->mem;
	}
	else{
	PLATFORM_GOT_LINE;
	}



    return reinterpret_cast<uchar*>(mem);
}

static inline bool isFullUpdate(PLATFORM_SURFACE *surface, const QRegion &region, const QPoint &offset)
{
#if 0 //DPP FIXME
    if (offset == QPoint(0, 0) && region.rectCount() == 1) {
	QSize size;
	surface->GetSize(surface, &size.rwidth(), &size.rheight());
	if (region.boundingRect().size() == size)
	    return true;
    }
#endif
    return false;
}

void QGixScreen::flipSurface(PLATFORM_SURFACE *surface, PLATFORM_CLIP_FLAGS flipFlags,
                                  const QRegion &region, const QPoint &offset)
{
#if 0 //DPP FIXME
    if (d_ptr->gixFlags & NoPartialFlip
        || (!(flipFlags & DSFLIP_BLIT) && QT_PREPEND_NAMESPACE(isFullUpdate(surface, region, offset)))) {
        surface->Flip(surface, 0, flipFlags);
    } else {
        if (!(d_ptr->gixFlags & BoundingRectFlip) && region.rectCount() > 1) {
            const QVector<QRect> rects = region.rects();
            const PLATFORM_CLIP_FLAGS nonWaitFlags = flipFlags & ~DSFLIP_WAIT;
            for (int i=0; i<rects.size(); ++i) {
                const QRect &r = rects.at(i);
                const DFBRegion dfbReg = { r.x() + offset.x(), r.y() + offset.y(),
                                           r.right() + offset.x(),
                                           r.bottom() + offset.y() };
                surface->Flip(surface, &dfbReg, i + 1 < rects.size() ? nonWaitFlags : flipFlags);
            }
        } else {
            const QRect r = region.boundingRect();
            const DFBRegion dfbReg = { r.x() + offset.x(), r.y() + offset.y(),
                                       r.right() + offset.x(),
                                       r.bottom() + offset.y() };
            surface->Flip(surface, &dfbReg, flipFlags);
        }
    }
#endif
}



void QGixScreen::waitIdle()
{
    //d_ptr->dfb->WaitIdle(d_ptr->dfb);
}

#ifdef QT_DIRECTFB_WM
gi_window_id_t QGixScreen::windowForWidget(const QWidget *widget) const
{
    if (widget) {
        const QWSWindowSurface *surface = static_cast<const QWSWindowSurface*>(widget->windowSurface());
        if (surface && surface->key() == QLatin1String("gix")) {
            return static_cast<const QGixWindowSurface*>(surface)->gixWindow();
        }
    }
    return 0;
}
#endif

PLATFORM_SURFACE * QGixScreen::surfaceForWidget(const QWidget *widget, QRect *rect) const
{
    Q_ASSERT(widget);
    if (!widget->isVisible() || widget->size().isNull())
        return 0;

    const QWSWindowSurface *surface = static_cast<const QWSWindowSurface*>(widget->windowSurface());
    if (surface && surface->key() == QLatin1String("gix")) {
        return static_cast<const QGixWindowSurface*>(surface)->surfaceForWidget(widget, rect);
    }
    return 0;
}


#ifndef QT_DIRECTFB_PLUGIN
Q_GUI_EXPORT PLATFORM_SURFACE *qt_directfb_surface_for_widget(const QWidget *widget, QRect *rect)
{
	PLATFORM_GOT_LINE;
    return QGixScreen::instance() ? QGixScreen::instance()->surfaceForWidget(widget, rect) : 0;
}

#ifdef QT_DIRECTFB_WM
Q_GUI_EXPORT gi_window_id_t qt_directfb_window_for_widget(const QWidget *widget)
{
	PLATFORM_GOT_LINE;
    return QGixScreen::instance() ? QGixScreen::instance()->windowForWidget(widget) : 0;
}

#endif
#endif

QT_END_NAMESPACE

#include "qdirectfbscreen.moc"
#endif // QT_NO_QWS_DIRECTFB

