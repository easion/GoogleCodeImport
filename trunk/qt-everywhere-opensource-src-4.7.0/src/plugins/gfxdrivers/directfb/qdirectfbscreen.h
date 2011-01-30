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

#ifndef QDIRECTFBSCREEN_H
#define QDIRECTFBSCREEN_H

#include <qglobal.h>
#ifndef QT_NO_QWS_DIRECTFB
#include <QtGui/qscreen_qws.h>
#include <gi/gi.h>
#include <gi/property.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)



#if !defined QT_DIRECTFB_SUBSURFACE && !defined QT_NO_DIRECTFB_SUBSURFACE
#define QT_NO_DIRECTFB_SUBSURFACE
#endif
#if !defined QT_NO_DIRECTFB_LAYER && !defined QT_DIRECTFB_LAYER
#define QT_DIRECTFB_LAYER
#endif
#if !defined QT_NO_DIRECTFB_WM && !defined QT_DIRECTFB_WM
#define QT_DIRECTFB_WM
#endif
#if !defined QT_DIRECTFB_IMAGECACHE && !defined QT_NO_DIRECTFB_IMAGECACHE
#define QT_NO_DIRECTFB_IMAGECACHE
#endif
#if !defined QT_NO_DIRECTFB_IMAGEPROVIDER && !defined QT_DIRECTFB_IMAGEPROVIDER
#define QT_DIRECTFB_IMAGEPROVIDER
#endif
#if !defined QT_NO_DIRECTFB_STRETCHBLIT && !defined QT_DIRECTFB_STRETCHBLIT
#define QT_DIRECTFB_STRETCHBLIT
#endif
#if !defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE && !defined QT_NO_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
#define QT_NO_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
#endif
#if !defined QT_DIRECTFB_WINDOW_AS_CURSOR && !defined QT_NO_DIRECTFB_WINDOW_AS_CURSOR
#define QT_NO_DIRECTFB_WINDOW_AS_CURSOR
#endif
#if !defined QT_DIRECTFB_PALETTE && !defined QT_NO_DIRECTFB_PALETTE
#define QT_NO_DIRECTFB_PALETTE
#endif
#if !defined QT_NO_DIRECTFB_PREALLOCATED && !defined QT_DIRECTFB_PREALLOCATED
#define QT_DIRECTFB_PREALLOCATED
#endif
#if !defined QT_NO_DIRECTFB_MOUSE && !defined QT_DIRECTFB_MOUSE
#define QT_DIRECTFB_MOUSE
#endif
#if !defined QT_NO_DIRECTFB_KEYBOARD && !defined QT_DIRECTFB_KEYBOARD
#define QT_DIRECTFB_KEYBOARD
#endif
#if !defined QT_NO_DIRECTFB_OPAQUE_DETECTION && !defined QT_DIRECTFB_OPAQUE_DETECTION
#define QT_DIRECTFB_OPAQUE_DETECTION
#endif
#ifndef QT_NO_QWS_CURSOR
#if defined QT_DIRECTFB_WM && defined QT_DIRECTFB_WINDOW_AS_CURSOR
#define QT_DIRECTFB_CURSOR
#elif defined QT_DIRECTFB_LAYER
#define QT_DIRECTFB_CURSOR
#endif
#endif
#ifndef QT_DIRECTFB_CURSOR
#define QT_NO_DIRECTFB_CURSOR
#endif
#if defined QT_NO_DIRECTFB_LAYER && defined QT_DIRECTFB_WM
#error QT_NO_DIRECTFB_LAYER requires QT_NO_DIRECTFB_WM
#endif
#if defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE && defined QT_NO_DIRECTFB_IMAGEPROVIDER
#error QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE requires QT_DIRECTFB_IMAGEPROVIDER to be defined
#endif
#if defined QT_DIRECTFB_WINDOW_AS_CURSOR && defined QT_NO_DIRECTFB_WM
#error QT_DIRECTFB_WINDOW_AS_CURSOR requires QT_DIRECTFB_WM to be defined
#endif

#define Q_DIRECTFB_VERSION ((DIRECTFB_MAJOR_VERSION << 16) | (DIRECTFB_MINOR_VERSION << 8) | DIRECTFB_MICRO_VERSION)

#define DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(F)                         \
    static inline F operator~(F f) { return F(~int(f)); } \
    static inline F operator&(F left, F right) { return F(int(left) & int(right)); } \
    static inline F operator|(F left, F right) { return F(int(left) | int(right)); } \
    static inline F &operator|=(F &left, F right) { left = (left | right); return left; } \
    static inline F &operator&=(F &left, F right) { left = (left & right); return left; }

/*
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBInputDeviceCapabilities);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBWindowDescriptionFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBWindowCapabilities);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBWindowOptions);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceDescriptionFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceCapabilities);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(PLATFORM_LOCK_FLAGS);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceBlittingFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(DFBSurfaceDrawingFlags);
DIRECTFB_DECLARE_OPERATORS_FOR_FLAGS(PLATFORM_CLIP_FLAGS);
*/

struct PLATFORM_SURFACE{
	gi_gc_ptr_t gc;
	gi_format_code_t PixelFormat;
	gi_window_id_t pixmap;
	gi_bool_t is_memdc;
	void *mem;
	int pitch;
	int width,height;
};

void GixReleaseSurface(PLATFORM_SURFACE *surface);
PLATFORM_SURFACE *GixCreateSurface(gi_window_id_t original_dc, unsigned gformat, int width,int height, void *bits);

//#define QT_NO_DIRECTFB_LAYER 1
#define PLATFORM_ERROR printf
#define PLATFORM_GOT_LINE qWarning("%s got line %d\n",__FUNCTION__,__LINE__)

typedef long PLATFORM_CREATE_OPTIONS;
typedef long PLATFORM_LOCK_FLAGS;
typedef long PLATFORM_CLIP_FLAGS;
//typedef long DFBSurfaceDescription;


class QGixScreenPrivate;
class Q_GUI_EXPORT QGixScreen : public QScreen
{
public:
    QGixScreen(int display_id);
    ~QGixScreen();

    enum GixFlag {
        NoFlags = 0x00,
        VideoOnly = 0x01,
        SystemOnly = 0x02,
        BoundingRectFlip = 0x04,
        NoPartialFlip = 0x08
    };

    Q_DECLARE_FLAGS(GixFlags, GixFlag);

    GixFlags gixFlags() const;

    bool connect(const QString &displaySpec);
    void disconnect();
    bool initDevice();
    void shutdownDevice();

    void exposeRegion(QRegion r, int changing);
    void solidFill(const QColor &color, const QRegion &region);

    void setMode(int width, int height, int depth);
    void blank(bool on);

    QWSWindowSurface *createSurface(QWidget *widget) const;
    QWSWindowSurface *createSurface(const QString &key) const;

    static QGixScreen *instance();
    void waitIdle();
    PLATFORM_SURFACE *surfaceForWidget(const QWidget *widget, QRect *rect) const;

#ifdef QT_DIRECTFB_WM
    gi_window_id_t windowForWidget(const QWidget *widget) const;
#endif

    // Track surface creation/release so we can release all on exit
    enum SurfaceCreationOption {
        DontTrackSurface = 0x1,
        TrackSurface = 0x2,
        NoPreallocated = 0x4
    };
    Q_DECLARE_FLAGS(PLATFORM_CREATE_OPTIONS, SurfaceCreationOption);
    PLATFORM_SURFACE *createDFBSurface(const QImage &image,
                                       QImage::Format format,
                                       PLATFORM_CREATE_OPTIONS options,
                                       int *result = 0);
    PLATFORM_SURFACE *createDFBSurface(const QSize &size,
                                       QImage::Format format,
                                       PLATFORM_CREATE_OPTIONS options,
                                       int *result = 0);
    PLATFORM_SURFACE *copyDFBSurface(PLATFORM_SURFACE *src,
                                     QImage::Format format,
                                     PLATFORM_CREATE_OPTIONS options,
                                     int *result = 0);
    PLATFORM_SURFACE *createDFBSurface(int width, int height,gi_format_code_t f,
                                       PLATFORM_CREATE_OPTIONS options,
                                       int *result);


    void flipSurface(PLATFORM_SURFACE *surface, PLATFORM_CLIP_FLAGS flipFlags,
                     const QRegion &region, const QPoint &offset);
    void releaseDFBSurface(PLATFORM_SURFACE *surface);

    using QScreen::depth;
    static int depth(gi_format_code_t format);
    static int depth(QImage::Format format);

    static gi_format_code_t getSurfacePixelFormat(QImage::Format format);
    //static DFBSurfaceDescription getSurfaceDescription(const uint *buffer,
     //                                                  int length);
    static QImage::Format pixelFormatToQt(gi_format_code_t format);
    //static bool initSurfaceDescriptionPixelFormat(DFBSurfaceDescription *description, QImage::Format format);
    static inline bool isPremultiplied(QImage::Format format);
    static inline bool hasAlphaChannel(gi_format_code_t format);
    static inline bool hasAlphaChannel(PLATFORM_SURFACE *surface);
    QImage::Format alphaPixmapFormat() const;



    static uchar *lockSurface(PLATFORM_SURFACE *surface, PLATFORM_LOCK_FLAGS flags, int *bpl = 0);
#if defined QT_DIRECTFB_IMAGEPROVIDER && defined QT_DIRECTFB_IMAGEPROVIDER_KEEPALIVE
    void setGixImageProvider(IGixImageProvider *provider);
#endif
private:
    QGixScreenPrivate *d_ptr;
};

//Q_DECLARE_OPERATORS_FOR_FLAGS(QGixScreen::PLATFORM_CREATE_OPTIONS);
//Q_DECLARE_OPERATORS_FOR_FLAGS(QGixScreen::GixFlags);

inline bool QGixScreen::isPremultiplied(QImage::Format format)
{
    switch (format) {
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
    case QImage::Format_ARGB6666_Premultiplied:
    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB4444_Premultiplied:
        return true;
    default:
        break;
    }
    return false;
}

inline bool QGixScreen::hasAlphaChannel(gi_format_code_t format)
{
    switch (format) {
    case GI_RENDER_a1r5g5b5:
    case GI_RENDER_a8r8g8b8:
    case GI_RENDER_r3g3b2:
    //case DSPF_AiRGB:
    case GI_RENDER_a1:
    //case DSPF_ARGB2554:
    case GI_RENDER_a4b4g4r4:

        return true;
    default:
        return false;
    }
}

inline bool QGixScreen::hasAlphaChannel(PLATFORM_SURFACE *surface)
{
    Q_ASSERT(surface);
    gi_format_code_t format;
    format = surface->PixelFormat;
    return QGixScreen::hasAlphaChannel(format);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_QWS_DIRECTFB
#endif // QDIRECTFBSCREEN_H

