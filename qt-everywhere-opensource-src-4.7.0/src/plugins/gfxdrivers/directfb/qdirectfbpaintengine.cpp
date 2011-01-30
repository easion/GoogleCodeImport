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

#include "qdirectfbpaintengine.h"

#ifndef QT_NO_QWS_DIRECTFB

#include "qdirectfbwindowsurface.h"
#include "qdirectfbscreen.h"
#include "qdirectfbpixmap.h"
#include <gi/gi.h>
#include <gi/property.h>

#include <qtransform.h>
#include <qvarlengtharray.h>
#include <qcache.h>
#include <qmath.h>
#include <private/qpixmapdata_p.h>
#include <private/qpixmap_raster_p.h>
#include <private/qimagepixmapcleanuphooks_p.h>

QT_BEGIN_NAMESPACE

class SurfaceCache;
class QGixPaintEnginePrivate : public QRasterPaintEnginePrivate
{
public:
    enum TransformationTypeFlags {
        Matrix_NegativeScale = 0x100,
        Matrix_RectsUnsupported = (QTransform::TxRotate|QTransform::TxShear|QTransform::TxProject),
        Matrix_BlitsUnsupported = (Matrix_NegativeScale|Matrix_RectsUnsupported)
    };

    enum CompositionModeStatus {
        PorterDuff_None = 0x0,
        PorterDuff_Supported = 0x1,
        PorterDuff_PremultiplyColors = 0x2,
        PorterDuff_AlwaysBlend = 0x4
    };

    enum ClipType {
        ClipUnset,
        NoClip,
        RectClip,
        RegionClip,
        ComplexClip
    };

    QGixPaintEnginePrivate(QGixPaintEngine *p);
    ~QGixPaintEnginePrivate();

    inline void setTransform(const QTransform &transforma);
    inline void setPen(const QPen &pen);
    inline void setCompositionMode(QPainter::CompositionMode mode);
    inline void setRenderHints(QPainter::RenderHints hints);

    inline void setDFBColor(const QColor &color);

    inline void lock();
    inline void unlock();
    static inline void unlock(QGixPaintDevice *device);

    inline bool isSimpleBrush(const QBrush &brush) const;

    void drawTiledPixmap(const QRectF &dest, const QPixmap &pixmap, const QPointF &pos);
    void blit(const QRectF &dest, PLATFORM_SURFACE *surface, const QRectF &src);

    inline bool supportsStretchBlit() const;

    inline void updateClip();
    virtual void systemStateChanged();

    static PLATFORM_SURFACE *getSurface(const QImage &img, bool *release);


    void prepareForBlit(bool alpha);

    PLATFORM_SURFACE *surface;

    bool antialiased;
    bool simplePen;

    uint transformationType; // this is QTransform::type() + Matrix_NegativeScale if qMin(transform.m11(), transform.m22()) < 0

    SurfaceCache *surfaceCache;
    //IGix *fb;
    quint8 opacity;

    ClipType clipType;
    QGixPaintDevice *dfbDevice;
    uint compositionModeStatus;
    bool isPremultiplied;

    bool inClip;
    QRect currentClip;

    QGixPaintEngine *q;
};

class SurfaceCache
{
public:
    SurfaceCache() : surface(0), buffer(0), bufsize(0) {}
    ~SurfaceCache() { clear(); }
    PLATFORM_SURFACE *getSurface(const uint *buf, int size);
    void clear();
private:
    PLATFORM_SURFACE *surface;
    uint *buffer;
    int bufsize;
};



#if defined QT_DIRECTFB_WARN_ON_RASTERFALLBACKS || defined QT_DIRECTFB_DISABLE_RASTERFALLBACKS || defined QT_DEBUG
#define VOID_ARG() static_cast<bool>(false)
enum PaintOperation {
    DRAW_RECTS = 0x0001, DRAW_LINES = 0x0002, DRAW_IMAGE = 0x0004,
    DRAW_PIXMAP = 0x0008, DRAW_TILED_PIXMAP = 0x0010, STROKE_PATH = 0x0020,
    DRAW_PATH = 0x0040, DRAW_POINTS = 0x0080, DRAW_ELLIPSE = 0x0100,
    DRAW_POLYGON = 0x0200, DRAW_TEXT = 0x0400, FILL_PATH = 0x0800,
    FILL_RECT = 0x1000, DRAW_COLORSPANS = 0x2000, DRAW_ROUNDED_RECT = 0x4000,
    DRAW_STATICTEXT = 0x8000, ALL = 0xffff
};

#ifdef QT_DEBUG
static void initRasterFallbacksMasks(int *warningMask, int *disableMask)
{
    struct {
        const char *name;
        PaintOperation operation;
    } const operations[] = {
        { "DRAW_RECTS", DRAW_RECTS },
        { "DRAW_LINES", DRAW_LINES },
        { "DRAW_IMAGE", DRAW_IMAGE },
        { "DRAW_PIXMAP", DRAW_PIXMAP },
        { "DRAW_TILED_PIXMAP", DRAW_TILED_PIXMAP },
        { "STROKE_PATH", STROKE_PATH },
        { "DRAW_PATH", DRAW_PATH },
        { "DRAW_POINTS", DRAW_POINTS },
        { "DRAW_ELLIPSE", DRAW_ELLIPSE },
        { "DRAW_POLYGON", DRAW_POLYGON },
        { "DRAW_TEXT", DRAW_TEXT },
        { "FILL_PATH", FILL_PATH },
        { "FILL_RECT", FILL_RECT },
        { "DRAW_COLORSPANS", DRAW_COLORSPANS },
        { "DRAW_ROUNDED_RECT", DRAW_ROUNDED_RECT },
        { "ALL", ALL },
        { 0, ALL }
    };

    QStringList warning = QString::fromLatin1(qgetenv("QT_DIRECTFB_WARN_ON_RASTERFALLBACKS")).toUpper().split(QLatin1Char('|'),
                                                                                                              QString::SkipEmptyParts);
    QStringList disable = QString::fromLatin1(qgetenv("QT_DIRECTFB_DISABLE_RASTERFALLBACKS")).toUpper().split(QLatin1Char('|'),
                                                                                                              QString::SkipEmptyParts);
    *warningMask = 0;
    *disableMask = 0;
    if (!warning.isEmpty() || !disable.isEmpty()) {
        for (int i=0; operations[i].name; ++i) {
            const QString name = QString::fromLatin1(operations[i].name);
            int idx = warning.indexOf(name);
            if (idx != -1) {
                *warningMask |= operations[i].operation;
                warning.erase(warning.begin() + idx);
            }
            idx = disable.indexOf(name);
            if (idx != -1) {
                *disableMask |= operations[i].operation;
                disable.erase(disable.begin() + idx);
            }
        }
    }
    if (!warning.isEmpty()) {
        qWarning("QGixPaintEngine QT_DIRECTFB_WARN_ON_RASTERFALLBACKS Unknown operation(s): %s",
                 qPrintable(warning.join(QLatin1String("|"))));
    }
    if (!disable.isEmpty()) {
        qWarning("QGixPaintEngine QT_DIRECTFB_DISABLE_RASTERFALLBACKS Unknown operation(s): %s",
                 qPrintable(disable.join(QLatin1String("|"))));
    }

}
#endif

static inline int rasterFallbacksMask(bool warn)
{
#ifdef QT_DIRECTFB_WARN_ON_RASTERFALLBACKS
    if (warn)
        return QT_DIRECTFB_WARN_ON_RASTERFALLBACKS;
#endif
#ifdef QT_DIRECTFB_DISABLE_RASTERFALLBACKS
    if (!warn)
        return QT_DIRECTFB_DISABLE_RASTERFALLBACKS;
#endif
#ifndef QT_DEBUG
    return 0;
#else
    static int warnMask = -1;
    static int disableMask = -1;
    if (warnMask == -1)
        initRasterFallbacksMasks(&warnMask, &disableMask);
    return warn ? warnMask : disableMask;
#endif
}
#endif

#if defined QT_DIRECTFB_WARN_ON_RASTERFALLBACKS || defined QT_DEBUG
template <typename device, typename T1, typename T2, typename T3>
static void rasterFallbackWarn(const char *msg, const char *func, const device *dev,
                               uint transformationType, bool simplePen,
                               uint clipType, uint compositionModeStatus,
                               const char *nameOne, const T1 &one,
                               const char *nameTwo, const T2 &two,
                               const char *nameThree, const T3 &three);
#endif

#if defined QT_DEBUG || (defined QT_DIRECTFB_WARN_ON_RASTERFALLBACKS && defined QT_DIRECTFB_DISABLE_RASTERFALLBACKS)
#define RASTERFALLBACK(op, one, two, three)                             \
    {                                                                   \
        const bool disable = op & rasterFallbacksMask(false);           \
        if (op & rasterFallbacksMask(true))                             \
            rasterFallbackWarn(disable                                  \
                               ? "Disabled raster engine operation"     \
                               : "Falling back to raster engine for",   \
                               __FUNCTION__,                            \
                               state()->painter->device(),              \
                               d_func()->transformationType,            \
                               d_func()->simplePen,                     \
                               d_func()->clipType,                      \
                               d_func()->compositionModeStatus,         \
                               #one, one, #two, two, #three, three);    \
        if (disable)                                                    \
            return;                                                     \
    }
#elif defined QT_DIRECTFB_DISABLE_RASTERFALLBACKS
#define RASTERFALLBACK(op, one, two, three)                             \
    if (op & rasterFallbacksMask(false))                                \
        return;
#elif defined QT_DIRECTFB_WARN_ON_RASTERFALLBACKS
#define RASTERFALLBACK(op, one, two, three)                             \
    if (op & rasterFallbacksMask(true))                                 \
        rasterFallbackWarn("Falling back to raster engine for",         \
                           __FUNCTION__, state()->painter->device(),    \
                           d_func()->transformationType,                \
                           d_func()->simplePen,                         \
                           d_func()->clipType,                          \
                           d_func()->compositionModeStatus,             \
                           #one, one, #two, two, #three, three);
#else
#define RASTERFALLBACK(op, one, two, three)
#endif

template <class T>
static inline void drawLines(const T *lines, int n, const QTransform &transform, PLATFORM_SURFACE *surface);
template <class T>
static inline void fillRects(const T *rects, int n, const QTransform &transform, PLATFORM_SURFACE *surface);
template <class T>
static inline void drawRects(const T *rects, int n, const QTransform &transform, PLATFORM_SURFACE *surface);

#define CLIPPED_PAINT(operation) {                                      \
        d->unlock();                                                    \
        gi_boxrec_t clipRegion;                                           \
        switch (d->clipType) {                                          \
        case QGixPaintEnginePrivate::NoClip:                       \
        case QGixPaintEnginePrivate::RectClip:                     \
            operation;                                                  \
            break;                                                      \
        case QGixPaintEnginePrivate::RegionClip: {                 \
            Q_ASSERT(d->clip());                                        \
            const QVector<QRect> cr = d->clip()->clipRegion.rects();    \
            const int size = cr.size();                                 \
            for (int i=0; i<size; ++i) {                                \
                d->currentClip = cr.at(i);                              \
                clipRegion.x1 = d->currentClip.x();                     \
                clipRegion.y1 = d->currentClip.y();                     \
                clipRegion.x2 = d->currentClip.right();                 \
                clipRegion.y2 = d->currentClip.bottom();                \
                gi_set_gc_clip_rectangles(d->surface->gc, &clipRegion,1);           \
                operation;                                              \
            }                                                           \
            d->updateClip();                                            \
            break; }                                                    \
        case QGixPaintEnginePrivate::ComplexClip:                  \
        case QGixPaintEnginePrivate::ClipUnset:                    \
            qFatal("CLIPPED_PAINT internal error %d", d->clipType);     \
            break;                                                      \
        }                                                               \
    }


QGixPaintEngine::QGixPaintEngine(QPaintDevice *device)
    : QRasterPaintEngine(*(new QGixPaintEnginePrivate(this)), device)
{
}

QGixPaintEngine::~QGixPaintEngine()
{
}

bool QGixPaintEngine::begin(QPaintDevice *device)
{
    Q_D(QGixPaintEngine);
    if (device->devType() == QInternal::CustomRaster) {
        d->dfbDevice = static_cast<QGixPaintDevice*>(device);
    } else if (device->devType() == QInternal::Pixmap) {
        QPixmapData *data = static_cast<QPixmap*>(device)->pixmapData();
        Q_ASSERT(data->classId() == QPixmapData::DirectFBClass);
        QGixPixmapData *dfbPixmapData = static_cast<QGixPixmapData*>(data);
        QGixPaintEnginePrivate::unlock(dfbPixmapData);
        d->dfbDevice = static_cast<QGixPaintDevice*>(dfbPixmapData);
    }

    if (d->dfbDevice)
        d->surface = d->dfbDevice->gixSurface();

    if (!d->surface) {
        qFatal("QGixPaintEngine used on an invalid device: 0x%x",
               device->devType());
    }
    d->isPremultiplied = QGixScreen::isPremultiplied(d->dfbDevice->format());

    d->prepare(d->dfbDevice);
    gccaps = AllFeatures;
    d->setCompositionMode(state()->composition_mode);

    return QRasterPaintEngine::begin(device);
}

bool QGixPaintEngine::end()
{
    Q_D(QGixPaintEngine);
    d->unlock();
    d->dfbDevice = 0;
#if (Q_DIRECTFB_VERSION >= 0x010000)
    d->surface->ReleaseSource(d->surface);
#endif
    d->currentClip = QRect();
    //d->surface->SetClip(d->surface, NULL);  //FIXME DPP
    d->surface = 0;
    return QRasterPaintEngine::end();
}

void QGixPaintEngine::clipEnabledChanged()
{
    Q_D(QGixPaintEngine);
    QRasterPaintEngine::clipEnabledChanged();
    d->updateClip();
}

void QGixPaintEngine::penChanged()
{
    Q_D(QGixPaintEngine);
    d->setPen(state()->pen);

    QRasterPaintEngine::penChanged();
}

void QGixPaintEngine::opacityChanged()
{
    Q_D(QGixPaintEngine);
    d->opacity = quint8(state()->opacity * 255);
    QRasterPaintEngine::opacityChanged();
}

void QGixPaintEngine::compositionModeChanged()
{
    Q_D(QGixPaintEngine);
    d->setCompositionMode(state()->compositionMode());
    QRasterPaintEngine::compositionModeChanged();
}

void QGixPaintEngine::renderHintsChanged()
{
    Q_D(QGixPaintEngine);
    d->setRenderHints(state()->renderHints);
    QRasterPaintEngine::renderHintsChanged();
}

void QGixPaintEngine::transformChanged()
{
    Q_D(QGixPaintEngine);
    d->setTransform(state()->matrix);
    QRasterPaintEngine::transformChanged();
}

void QGixPaintEngine::setState(QPainterState *state)
{
    Q_D(QGixPaintEngine);
    QRasterPaintEngine::setState(state);
    d->setPen(state->pen);
    d->opacity = quint8(state->opacity * 255);
    d->setCompositionMode(state->compositionMode());
    d->setTransform(state->transform());
    d->setRenderHints(state->renderHints);
    if (d->surface)
        d->updateClip();
}

void QGixPaintEngine::clip(const QVectorPath &path, Qt::ClipOperation op)
{
    Q_D(QGixPaintEngine);
    const bool wasInClip = d->inClip;
    d->inClip = true;
    QRasterPaintEngine::clip(path, op);
    if (!wasInClip) {
        d->inClip = false;
        d->updateClip();
    }
}

void QGixPaintEngine::clip(const QRegion &region, Qt::ClipOperation op)
{
    Q_D(QGixPaintEngine);
    const bool wasInClip = d->inClip;
    d->inClip = true;
    QRasterPaintEngine::clip(region, op);
    if (!wasInClip) {
        d->inClip = false;
        d->updateClip();
    }
}

void QGixPaintEngine::clip(const QRect &rect, Qt::ClipOperation op)
{
    Q_D(QGixPaintEngine);
    const bool wasInClip = d->inClip;
    d->inClip = true;
    QRasterPaintEngine::clip(rect, op);
    if (!wasInClip) {
        d->inClip = false;
        d->updateClip();
    }
}

void QGixPaintEngine::drawRects(const QRect *rects, int rectCount)
{
    Q_D(QGixPaintEngine);
    const QPen &pen = state()->pen;
    const QBrush &brush = state()->brush;
    if (brush.style() == Qt::NoBrush && pen.style() == Qt::NoPen)
        return;

    if ((d->transformationType & QGixPaintEnginePrivate::Matrix_RectsUnsupported)
        || !d->simplePen
        || d->clipType == QGixPaintEnginePrivate::ComplexClip
        || !d->isSimpleBrush(brush)
        || !(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_RECTS, rectCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawRects(rects, rectCount);
        return;
    }

    if (brush.style() != Qt::NoBrush) {
        d->setDFBColor(brush.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(fillRects<QRect>)(rects, rectCount, state()->matrix, d->surface));
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawRects<QRect>)(rects, rectCount, state()->matrix, d->surface));
    }
}

void QGixPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
    Q_D(QGixPaintEngine);
    const QPen &pen = state()->pen;
    const QBrush &brush = state()->brush;
    if (brush.style() == Qt::NoBrush && pen.style() == Qt::NoPen)
        return;

    if ((d->transformationType & QGixPaintEnginePrivate::Matrix_RectsUnsupported)
        || !d->simplePen
        || d->clipType == QGixPaintEnginePrivate::ComplexClip
        || !d->isSimpleBrush(brush)
        || !(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_RECTS, rectCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawRects(rects, rectCount);
        return;
    }

    if (brush.style() != Qt::NoBrush) {
        d->setDFBColor(brush.color());
        CLIPPED_PAINT(fillRects<QRectF>(rects, rectCount, state()->matrix, d->surface));
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawRects<QRectF>)(rects, rectCount, state()->matrix, d->surface));
    }
}

void QGixPaintEngine::drawLines(const QLine *lines, int lineCount)
{
    Q_D(QGixPaintEngine);

    const QPen &pen = state()->pen;
    if (!d->simplePen
        || d->clipType == QGixPaintEnginePrivate::ComplexClip
        || !(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_LINES, lineCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawLines(lines, lineCount);
        return;
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawLines<QLine>)(lines, lineCount, state()->matrix, d->surface));
    }
}

void QGixPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
    Q_D(QGixPaintEngine);

    const QPen &pen = state()->pen;
    if (!d->simplePen
        || d->clipType == QGixPaintEnginePrivate::ComplexClip
        || !(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(DRAW_LINES, lineCount, VOID_ARG(), VOID_ARG());
        d->lock();
        QRasterPaintEngine::drawLines(lines, lineCount);
        return;
    }

    if (pen.style() != Qt::NoPen) {
        d->setDFBColor(pen.color());
        CLIPPED_PAINT(QT_PREPEND_NAMESPACE(drawLines<QLineF>)(lines, lineCount, state()->matrix, d->surface));
    }
}

void QGixPaintEngine::drawImage(const QRectF &r, const QImage &image,
                                     const QRectF &sr,
                                     Qt::ImageConversionFlags flags)
{
    Q_D(QGixPaintEngine);
    Q_UNUSED(flags);

    /*  This is hard to read. The way it works is like this:

    - If you do not have support for preallocated surfaces and do not use an
    image cache we always fall back to raster engine.

    - If it's rotated/sheared/mirrored (negative scale) or we can't
    clip it we fall back to raster engine.

    - If we don't cache the image, but we do have support for
    preallocated surfaces we fall back to the raster engine if the
    image is in a format Gix can't handle.

    - If we do cache the image but don't have support for preallocated
    images and the cost of caching the image (bytes used) is higher
    than the max image cache size we fall back to raster engine.
    */

#if !defined QT_NO_DIRECTFB_PREALLOCATED 
    if (!(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)
        || (d->transformationType & QGixPaintEnginePrivate::Matrix_BlitsUnsupported)
        || (d->clipType == QGixPaintEnginePrivate::ComplexClip)
        || (!d->supportsStretchBlit() && state()->matrix.mapRect(r).size() != sr.size())
#ifndef QT_DIRECTFB_IMAGECACHE
        || (QGixScreen::getSurfacePixelFormat(image.format()) == GI_RENDER_UNKNOWN)
#elif defined QT_NO_DIRECTFB_PREALLOCATED
        || (QGixPaintEnginePrivate::cacheCost(image) > imageCache.maxCost())
#endif
        )
#endif
    {
        RASTERFALLBACK(DRAW_IMAGE, r, image.size(), sr);
        d->lock();
        QRasterPaintEngine::drawImage(r, image, sr, flags);
        return;
    }
#if !defined QT_NO_DIRECTFB_PREALLOCATED 
    bool release;
    PLATFORM_SURFACE *imgSurface = d->getSurface(image, &release);
    d->prepareForBlit(QGixScreen::hasAlphaChannel(imgSurface));
    CLIPPED_PAINT(d->blit(r, imgSurface, sr));
    if (release) {
#if (Q_DIRECTFB_VERSION >= 0x010000)
        d->surface->ReleaseSource(d->surface);
#endif
        //imgSurface->Release(imgSurface); //FIXME DPP
    }
#endif
}

void QGixPaintEngine::drawImage(const QPointF &p, const QImage &img)
{
    drawImage(QRectF(p, img.size()), img, img.rect());
}

void QGixPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pixmap,
                                      const QRectF &sr)
{
    Q_D(QGixPaintEngine);

    if (pixmap.pixmapData()->classId() != QPixmapData::DirectFBClass) {
        RASTERFALLBACK(DRAW_PIXMAP, r, pixmap.size(), sr);
        d->lock();
        QRasterPaintEngine::drawPixmap(r, pixmap, sr);
    } else {
        QPixmapData *data = pixmap.pixmapData();
        Q_ASSERT(data->classId() == QPixmapData::DirectFBClass);
        QGixPixmapData *dfbData = static_cast<QGixPixmapData*>(data);
        if (!(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)
            || (d->transformationType & QGixPaintEnginePrivate::Matrix_BlitsUnsupported)
            || (d->clipType == QGixPaintEnginePrivate::ComplexClip)
            || (!d->supportsStretchBlit() && state()->matrix.mapRect(r).size() != sr.size())) {
            RASTERFALLBACK(DRAW_PIXMAP, r, pixmap.size(), sr);
            const QImage *img = dfbData->buffer();
            d->lock();
            QRasterPaintEngine::drawImage(r, *img, sr);
        } else {
            QGixPaintEnginePrivate::unlock(dfbData);
            d->prepareForBlit(pixmap.hasAlphaChannel());
            PLATFORM_SURFACE *s = dfbData->gixSurface();
            CLIPPED_PAINT(d->blit(r, s, sr));
        }
    }
}

void QGixPaintEngine::drawPixmap(const QPointF &p, const QPixmap &pm)
{
    drawPixmap(QRectF(p, pm.size()), pm, pm.rect());
}

void QGixPaintEngine::drawTiledPixmap(const QRectF &r,
                                           const QPixmap &pixmap,
                                           const QPointF &offset)
{
    Q_D(QGixPaintEngine);
    if (pixmap.pixmapData()->classId() != QPixmapData::DirectFBClass) {
        RASTERFALLBACK(DRAW_TILED_PIXMAP, r, pixmap.size(), offset);
        d->lock();
        QRasterPaintEngine::drawTiledPixmap(r, pixmap, offset);
    } else if (!(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)
               || (d->transformationType & QGixPaintEnginePrivate::Matrix_BlitsUnsupported)
               || (d->clipType == QGixPaintEnginePrivate::ComplexClip)
               || (!d->supportsStretchBlit() && state()->matrix.isScaling())) {
        RASTERFALLBACK(DRAW_TILED_PIXMAP, r, pixmap.size(), offset);
        QPixmapData *pixmapData = pixmap.pixmapData();
        Q_ASSERT(pixmapData->classId() == QPixmapData::DirectFBClass);
        QGixPixmapData *dfbData = static_cast<QGixPixmapData*>(pixmapData);
        const QImage *img = dfbData->buffer();
        d->lock();
        QRasterPixmapData *data = new QRasterPixmapData(QPixmapData::PixmapType);
        data->fromImage(*img, Qt::AutoColor);
        const QPixmap pix(data);
        QRasterPaintEngine::drawTiledPixmap(r, pix, offset);
    } else {
        CLIPPED_PAINT(d->drawTiledPixmap(r, pixmap, offset));
    }
}


void QGixPaintEngine::stroke(const QVectorPath &path, const QPen &pen)
{
    RASTERFALLBACK(STROKE_PATH, path, VOID_ARG(), VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::stroke(path, pen);
}

void QGixPaintEngine::drawPath(const QPainterPath &path)
{
    RASTERFALLBACK(DRAW_PATH, path, VOID_ARG(), VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPath(path);
}

void QGixPaintEngine::drawPoints(const QPointF *points, int pointCount)
{
    RASTERFALLBACK(DRAW_POINTS, pointCount, VOID_ARG(), VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPoints(points, pointCount);
}

void QGixPaintEngine::drawPoints(const QPoint *points, int pointCount)
{
    RASTERFALLBACK(DRAW_POINTS, pointCount, VOID_ARG(), VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPoints(points, pointCount);
}

void QGixPaintEngine::drawEllipse(const QRectF &rect)
{
    RASTERFALLBACK(DRAW_ELLIPSE, rect, VOID_ARG(), VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawEllipse(rect);
}

void QGixPaintEngine::drawPolygon(const QPointF *points, int pointCount,
                                       PolygonDrawMode mode)
{
    RASTERFALLBACK(DRAW_POLYGON, pointCount, mode, VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPolygon(points, pointCount, mode);
}

void QGixPaintEngine::drawPolygon(const QPoint *points, int pointCount,
                                       PolygonDrawMode mode)
{
    RASTERFALLBACK(DRAW_POLYGON, pointCount, mode, VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawPolygon(points, pointCount, mode);
}

void QGixPaintEngine::drawTextItem(const QPointF &p,
                                        const QTextItem &textItem)
{
    RASTERFALLBACK(DRAW_TEXT, p, textItem.text(), VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawTextItem(p, textItem);
}

void QGixPaintEngine::fill(const QVectorPath &path, const QBrush &brush)
{
    if (brush.style() == Qt::NoBrush)
        return;
    RASTERFALLBACK(FILL_PATH, path, brush, VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::fill(path, brush);
}

void QGixPaintEngine::drawRoundedRect(const QRectF &rect, qreal xrad, qreal yrad, Qt::SizeMode mode)
{
    RASTERFALLBACK(DRAW_ROUNDED_RECT, rect, xrad, yrad);
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawRoundedRect(rect, xrad, yrad, mode);
}

void QGixPaintEngine::drawStaticTextItem(QStaticTextItem *item)
{
    RASTERFALLBACK(DRAW_STATICTEXT, item, VOID_ARG(), VOID_ARG());
    Q_D(QGixPaintEngine);
    d->lock();
    QRasterPaintEngine::drawStaticTextItem(item);
}

void QGixPaintEngine::fillRect(const QRectF &rect, const QBrush &brush)
{
    Q_D(QGixPaintEngine);
    if (brush.style() == Qt::NoBrush)
        return;
    if (d->clipType != QGixPaintEnginePrivate::ComplexClip) {
        switch (brush.style()) {
        case Qt::SolidPattern: {
            const QColor color = brush.color();
            if (!color.isValid())
                return;

            if (d->transformationType & QGixPaintEnginePrivate::Matrix_RectsUnsupported
                || !(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)) {
                break;
            }
            d->setDFBColor(color);
            const QRect r = state()->matrix.mapRect(rect).toRect();

			//FIXME DPP
            //CLIPPED_PAINT(d->surface->FillRectangle(d->surface, r.x(), r.y(), r.width(), r.height()));
            return; }

        case Qt::TexturePattern: {
            if (!(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)
                || (d->transformationType & QGixPaintEnginePrivate::Matrix_BlitsUnsupported)
                || (!d->supportsStretchBlit() && state()->matrix.isScaling())) {
                break;
            }

            const QPixmap texture = brush.texture();
            if (texture.pixmapData()->classId() != QPixmapData::DirectFBClass)
                break;

			//FIXME DPP
            //CLIPPED_PAINT(d->drawTiledPixmap(rect, texture, rect.topLeft() - state()->brushOrigin));
            return; }
        default:
            break;
        }
    }
    RASTERFALLBACK(FILL_RECT, rect, brush, VOID_ARG());
    d->lock();
    QRasterPaintEngine::fillRect(rect, brush);
}

void QGixPaintEngine::fillRect(const QRectF &rect, const QColor &color)
{
    if (!color.isValid())
        return;
    Q_D(QGixPaintEngine);
    if ((d->transformationType & QGixPaintEnginePrivate::Matrix_RectsUnsupported)
        || (d->clipType == QGixPaintEnginePrivate::ComplexClip)
        || !(d->compositionModeStatus & QGixPaintEnginePrivate::PorterDuff_Supported)) {
        RASTERFALLBACK(FILL_RECT, rect, color, VOID_ARG());
        d->lock();
        QRasterPaintEngine::fillRect(rect, color);
    } else {
        d->setDFBColor(color);
        const QRect r = state()->matrix.mapRect(rect).toRect();

		//FIXME DPP
        //CLIPPED_PAINT(d->surface->FillRectangle(d->surface, r.x(), r.y(), r.width(), r.height()));
    }
}

void QGixPaintEngine::drawBufferSpan(const uint *buffer, int bufsize,
                                          int x, int y, int length,
                                          uint const_alpha)
{
    Q_D(QGixPaintEngine);
    PLATFORM_SURFACE *src = d->surfaceCache->getSurface(buffer, bufsize);
    // ### how does this play with setDFBColor
    //src->SetColor(src, 0, 0, 0, const_alpha);
 	gi_set_gc_foreground(src->gc, GI_ARGB(const_alpha,  0, 0, 0) );
   const gi_cliprect_t rect = { 0, 0, length, 1 };
    //d->surface->Blit(d->surface, src, &rect, x, y); //FIXME DPP
}





/***************************************************/


// ---- QGixPaintEnginePrivate ----
/***************************************************/






QGixPaintEnginePrivate::QGixPaintEnginePrivate(QGixPaintEngine *p)
    : surface(0), antialiased(false), simplePen(false),
      transformationType(0), opacity(255),
      clipType(ClipUnset), dfbDevice(0),
      compositionModeStatus(0), isPremultiplied(false), inClip(false), q(p)
{
    surfaceCache = new SurfaceCache;
}

QGixPaintEnginePrivate::~QGixPaintEnginePrivate()
{
    delete surfaceCache;
}

bool QGixPaintEnginePrivate::isSimpleBrush(const QBrush &brush) const
{
    return (brush.style() == Qt::NoBrush) || (brush.style() == Qt::SolidPattern && !antialiased);
}

void QGixPaintEnginePrivate::lock()
{
    // We will potentially get a new pointer to the buffer after a
    // lock so we need to call the base implementation of prepare so
    // it updates its rasterBuffer to point to the new buffer address.
    Q_ASSERT(dfbDevice);
    if (dfbDevice->lockSurface(3)) {
        prepare(dfbDevice);
    }
}

void QGixPaintEnginePrivate::unlock()
{
    Q_ASSERT(dfbDevice);

    QGixPaintEnginePrivate::unlock(dfbDevice);
}

void QGixPaintEnginePrivate::unlock(QGixPaintDevice *device)
{
#ifdef QT_NO_DIRECTFB_SUBSURFACE
    Q_ASSERT(device);
    device->unlockSurface();
#else
    Q_UNUSED(device);
#endif
}

void QGixPaintEnginePrivate::setTransform(const QTransform &transform)
{
    transformationType = transform.type();
    if (qMin(transform.m11(), transform.m22()) < 0) {
        transformationType |= QGixPaintEnginePrivate::Matrix_NegativeScale;
    }
    setPen(q->state()->pen);
}

void QGixPaintEnginePrivate::setPen(const QPen &pen)
{
    if (pen.style() == Qt::NoPen) {
        simplePen = true;
    } else if (pen.style() == Qt::SolidLine
               && !antialiased
               && pen.brush().style() == Qt::SolidPattern
               && pen.widthF() <= 1.0
               && (transformationType < QTransform::TxScale || pen.isCosmetic())) {
        simplePen = true;
    } else {
        simplePen = false;
    }
}

void QGixPaintEnginePrivate::setCompositionMode(QPainter::CompositionMode mode)
{
    if (!surface)
        return;

    static const bool forceRasterFallBack = qgetenv("QT_DIRECTFB_FORCE_RASTER").toInt() > 0;
    if (forceRasterFallBack) {
        compositionModeStatus = PorterDuff_None;
        return;
    }

    compositionModeStatus = PorterDuff_Supported|PorterDuff_PremultiplyColors|PorterDuff_AlwaysBlend;
#if 0
	switch (mode) {
    case QPainter::CompositionMode_Clear:
        surface->SetPorterDuff(surface, DSPD_CLEAR);
        break;
    case QPainter::CompositionMode_Source:
        surface->SetPorterDuff(surface, DSPD_SRC);
        compositionModeStatus &= ~PorterDuff_AlwaysBlend;
        if (!isPremultiplied)
            compositionModeStatus &= ~PorterDuff_PremultiplyColors;
        break;
    case QPainter::CompositionMode_SourceOver:
        compositionModeStatus &= ~PorterDuff_AlwaysBlend;
        surface->SetPorterDuff(surface, DSPD_SRC_OVER);
        break;
    case QPainter::CompositionMode_DestinationOver:
        surface->SetPorterDuff(surface, DSPD_DST_OVER);
        break;
    case QPainter::CompositionMode_SourceIn:
        surface->SetPorterDuff(surface, DSPD_SRC_IN);
        if (!isPremultiplied)
            compositionModeStatus &= ~PorterDuff_PremultiplyColors;
        break;
    case QPainter::CompositionMode_DestinationIn:
        surface->SetPorterDuff(surface, DSPD_DST_IN);
        break;
    case QPainter::CompositionMode_SourceOut:
        surface->SetPorterDuff(surface, DSPD_SRC_OUT);
        break;
    case QPainter::CompositionMode_DestinationOut:
        surface->SetPorterDuff(surface, DSPD_DST_OUT);
        break;

#if 0//(Q_DIRECTFB_VERSION >= 0x010000)
    case QPainter::CompositionMode_Destination:
        surface->SetPorterDuff(surface, DSPD_DST);
        break;

    case QPainter::CompositionMode_SourceAtop:
        surface->SetPorterDuff(surface, DSPD_SRC_ATOP);
        break;
    case QPainter::CompositionMode_DestinationAtop:
        surface->SetPorterDuff(surface, DSPD_DST_ATOP);
        break;
    case QPainter::CompositionMode_Plus:
        surface->SetPorterDuff(surface, DSPD_ADD);
        break;
    case QPainter::CompositionMode_Xor:
        surface->SetPorterDuff(surface, DSPD_XOR);
        break;
#endif
    default:
        compositionModeStatus = PorterDuff_None;
        break;
    }
#endif
}

void QGixPaintEnginePrivate::setRenderHints(QPainter::RenderHints hints)
{
    const bool old = antialiased;
    antialiased = bool(hints & QPainter::Antialiasing);
    if (old != antialiased) {
        setPen(q->state()->pen);
    }
}

void QGixPaintEnginePrivate::prepareForBlit(bool alpha)
{
    //DFBSurfaceBlittingFlags blittingFlags = alpha ? DSBLIT_BLEND_ALPHACHANNEL : DSBLIT_NOFX;
    if (opacity != 255) {
        //blittingFlags |= DSBLIT_BLEND_COLORALPHA; //FIXME DPP
    }
	gi_set_gc_foreground(surface->gc, GI_ARGB(opacity,  0xff, 0xff, 0xff) );
    //surface->SetColor(surface, 0xff, 0xff, 0xff, opacity);
    //surface->SetBlittingFlags(surface, blittingFlags);
}

static inline uint ALPHA_MUL(uint x, uint a)
{
    uint t = x * a;
    t = ((t + (t >> 8) + 0x80) >> 8) & 0xff;
    return t;
}

void QGixPaintEnginePrivate::setDFBColor(const QColor &color)
{
    Q_ASSERT(surface);
    Q_ASSERT(compositionModeStatus & PorterDuff_Supported);
    const quint8 alpha = (opacity == 255 ?
                          color.alpha() : ALPHA_MUL(color.alpha(), opacity));
    QColor col;
    if (compositionModeStatus & PorterDuff_PremultiplyColors) {
        col = QColor(ALPHA_MUL(color.red(), alpha),
                     ALPHA_MUL(color.green(), alpha),
                     ALPHA_MUL(color.blue(), alpha),
                     alpha);
    } else {
        col = QColor(color.red(), color.green(), color.blue(), alpha);
    }

	gi_set_gc_foreground(surface->gc, GI_ARGB(col.alpha(), col.red(), col.green(), col.blue()));
    //surface->SetColor(surface, col.red(), col.green(), col.blue(), col.alpha());
    //surface->SetDrawingFlags(surface, alpha == 255 && !(compositionModeStatus & PorterDuff_AlwaysBlend) ? DSDRAW_NOFX : DSDRAW_BLEND);
}

PLATFORM_SURFACE *QGixPaintEnginePrivate::getSurface(const QImage &img, bool *release)
{
#ifdef QT_NO_DIRECTFB_IMAGECACHE
    *release = true;
    return QGixScreen::instance()->createDFBSurface(img, img.format(), QGixScreen::DontTrackSurface);
#else
    const qint64 key = img.cacheKey();
    *release = false;
    if (imageCache.contains(key)) {
        return imageCache[key]->surface;
    }

    const int cost = cacheCost(img);
    const bool cache = cost <= imageCache.maxCost();
    QGixScreen *screen = QGixScreen::instance();
    const QImage::Format format = (img.format() == screen->alphaPixmapFormat() || QGixPixmapData::hasAlphaChannel(img)
                                   ? screen->alphaPixmapFormat() : screen->pixelFormat());

    PLATFORM_SURFACE *surface = screen->createDFBSurface(img, format,
                                                         cache
                                                         ? QGixScreen::TrackSurface
                                                         : QGixScreen::DontTrackSurface);
    if (cache) {
        CachedImage *cachedImage = new CachedImage;
        const_cast<QImage&>(img).data_ptr()->is_cached = true;
        cachedImage->surface = surface;
        imageCache.insert(key, cachedImage, cost);
    } else {
        *release = true;
    }
    return surface;
#endif
}


void QGixPaintEnginePrivate::blit(const QRectF &dest, PLATFORM_SURFACE *s, const QRectF &src)
{
    const QRect sr = src.toRect();
    const QRect dr = q->state()->matrix.mapRect(dest).toRect();
    if (dr.isEmpty())
        return;
    const gi_cliprect_t sRect = { sr.x(), sr.y(), sr.width(), sr.height() };
    int result;

    if (dr.size() == sr.size()) {
        //result = surface->Blit(surface, s, &sRect, dr.x(), dr.y()); //FIXME DPP
    } else {
        Q_ASSERT(supportsStretchBlit());
        const gi_cliprect_t dRect = { dr.x(), dr.y(), dr.width(), dr.height() };
        //result = surface->StretchBlit(surface, s, &sRect, &dRect); //FIXME DPP
    }
    //if (result != DFB_OK)
    //    PLATFORM_ERROR("QGixPaintEngine::drawPixmap()", result);
}

static inline qreal fixCoord(qreal rect_pos, qreal pixmapSize, qreal offset)
{
    qreal pos = rect_pos - offset;
    while (pos > rect_pos)
        pos -= pixmapSize;
    while (pos + pixmapSize < rect_pos)
        pos += pixmapSize;
    return pos;
}

void QGixPaintEnginePrivate::drawTiledPixmap(const QRectF &dest, const QPixmap &pixmap, const QPointF &off)
{
    Q_ASSERT(!(transformationType & Matrix_BlitsUnsupported));
    const QTransform &transform = q->state()->matrix;
    const QRect destinationRect = transform.mapRect(dest).toRect().normalized();
    QRect newClip = destinationRect;
    if (!currentClip.isEmpty())
        newClip &= currentClip;

    if (newClip.isNull())
        return;

    const gi_boxrec_t clip = {
        newClip.x(),
        newClip.y(),
        newClip.right(),
        newClip.bottom()
    };
    //surface->SetClip(surface, &clip);  //FIXME DPP

    QPointF offset = off;
    Q_ASSERT(transform.type() <= QTransform::TxScale);
    prepareForBlit(pixmap.hasAlphaChannel());
    QPixmapData *data = pixmap.pixmapData();
    Q_ASSERT(data->classId() == QPixmapData::DirectFBClass);
    QGixPixmapData *dfbData = static_cast<QGixPixmapData*>(data);
    QGixPaintEnginePrivate::unlock(dfbData);
    const QSize pixmapSize = dfbData->size();
    PLATFORM_SURFACE *sourceSurface = dfbData->gixSurface();
    if (transform.isScaling()) {
        Q_ASSERT(supportsStretchBlit());
        Q_ASSERT(qMin(transform.m11(), transform.m22()) >= 0);
        offset.rx() *= transform.m11();
        offset.ry() *= transform.m22();

        const QSizeF mappedSize(pixmapSize.width() * transform.m11(), pixmapSize.height() * transform.m22());
        qreal y = fixCoord(destinationRect.y(), mappedSize.height(), offset.y());
        const qreal startX = fixCoord(destinationRect.x(), mappedSize.width(), offset.x());
        while (y <= destinationRect.bottom()) {
            qreal x = startX;
            while (x <= destinationRect.right()) {
                const gi_cliprect_t destination = { qRound(x), qRound(y), mappedSize.width(), mappedSize.height() };
                //FIXME DPP
				//surface->StretchBlit(surface, sourceSurface, 0, &destination);
                x += mappedSize.width();
            }
            y += mappedSize.height();
        }
    } else {
        qreal y = fixCoord(destinationRect.y(), pixmapSize.height(), offset.y());
        const qreal startX = fixCoord(destinationRect.x(), pixmapSize.width(), offset.x());
        int horizontal = qMax(1, destinationRect.width() / pixmapSize.width()) + 1;
        if (startX != destinationRect.x())
            ++horizontal;
        int vertical = qMax(1, destinationRect.height() / pixmapSize.height()) + 1;
        if (y != destinationRect.y())
            ++vertical;

        const int maxCount = (vertical * horizontal);
        QVarLengthArray<gi_cliprect_t, 16> sourceRects(maxCount);
        QVarLengthArray<gi_point_t, 16> points(maxCount);

        int i = 0;
        while (y <= destinationRect.bottom()) {
            Q_ASSERT(i < maxCount);
            qreal x = startX;
            while (x <= destinationRect.right()) {
                points[i].x = qRound(x);
                points[i].y = qRound(y);
                sourceRects[i].x = 0;
                sourceRects[i].y = 0;
                sourceRects[i].w = int(pixmapSize.width());
                sourceRects[i].h = int(pixmapSize.height());
                x += pixmapSize.width();
                ++i;
            }
            y += pixmapSize.height();
        }
		//FIXME DPP
        //surface->BatchBlit(surface, sourceSurface, sourceRects.constData(), points.constData(), i);
    }

    if (currentClip.isEmpty()) {
        gi_set_gc_clip_rectangles(surface->gc, NULL, 0);  //FIXME DPP
    } else {
        const gi_boxrec_t clip = {
            currentClip.x(),
            currentClip.y(),
            currentClip.right(),
            currentClip.bottom()
        };
        //surface->SetClip(surface, &clip);  //FIXME DPP
    }
}

void QGixPaintEnginePrivate::updateClip()
{
    Q_ASSERT(surface);
    currentClip = QRect();
    const QClipData *clipData = clip();
    if (!clipData || !clipData->enabled) {
        gi_set_gc_clip_rectangles(surface->gc, NULL, 0);  //FIXME DPP
        clipType = NoClip;
    } else if (clipData->hasRectClip) {
        const gi_boxrec_t r = {
            clipData->clipRect.x(),
            clipData->clipRect.y(),
            clipData->clipRect.right(),
            clipData->clipRect.bottom()
        };
        //surface->SetClip(surface, &r); //FIXME DPP
        currentClip = clipData->clipRect.normalized();
        // ### is this guaranteed to always be normalized?
        clipType = RectClip;
    } else if (clipData->hasRegionClip) {
        clipType = RegionClip;
    } else {
        clipType = ComplexClip;
    }
}

bool QGixPaintEnginePrivate::supportsStretchBlit() const
{
#ifdef QT_DIRECTFB_STRETCHBLIT
    return !(q->state()->renderHints & QPainter::SmoothPixmapTransform);
#else
    return false;
#endif
}


void QGixPaintEnginePrivate::systemStateChanged()
{
    QRasterPaintEnginePrivate::systemStateChanged();
    updateClip();
}

PLATFORM_SURFACE *SurfaceCache::getSurface(const uint *buf, int size)
{
    if (buffer == buf && bufsize == size)
        return surface;

    clear();

    //const DFBSurfaceDescription description = QGixScreen::getSurfaceDescription(buf, size);
    surface =NULL; //FIXME DPP
	//surface = QGixScreen::instance()->createDFBSurface(description, QGixScreen::TrackSurface, 0);
    if (!surface)
        qWarning("QGixPaintEngine: SurfaceCache: Unable to create surface");

    buffer = const_cast<uint*>(buf);
    bufsize = size;

    return surface;
}

void SurfaceCache::clear()
{
    if (surface && QGixScreen::instance())
        QGixScreen::instance()->releaseDFBSurface(surface);
    surface = 0;
    buffer = 0;
    bufsize = 0;
}


static inline QRect mapRect(const QTransform &transform, const QRect &rect) { return transform.mapRect(rect); }
static inline QRect mapRect(const QTransform &transform, const QRectF &rect) { return transform.mapRect(rect).toRect(); }
static inline QLine map(const QTransform &transform, const QLine &line) { return transform.map(line); }
static inline QLine map(const QTransform &transform, const QLineF &line) { return transform.map(line).toLine(); }
template <class T>
static inline void drawLines(const T *lines, int n, const QTransform &transform, PLATFORM_SURFACE *surface)
{
    if (n == 1) {
        const QLine l = map(transform, lines[0]);
        //surface->DrawLine(surface, l.x1(), l.y1(), l.x2(), l.y2());
		gi_draw_line(surface->gc, l.x1(),l.y1(),l.x2(),l.y2());
    } else {
        //QVarLengthArray<gi_boxrec_t, 32> lineArray(n);
        QVarLengthArray<gi_point_t, 32> lineArray(n);
        for (int i=0; i<n; ++i) {
            const QLine l = map(transform, lines[i]);
            //lineArray[i].x1 = l.x1();
            //lineArray[i].y1 = l.y1();
            //lineArray[i].x2 = l.x2();
            //lineArray[i].y2 = l.y2();

			gi_draw_line(surface->gc, l.x1(),l.y1(),l.x2(),l.y2());
        }
        //surface->DrawLines(surface, lineArray.constData(), n);
		//gi_draw_lines(surface->gc,lineArray.constData(), n);
    }
}

template <class T>
static inline void fillRects(const T *rects, int n, const QTransform &transform, PLATFORM_SURFACE *surface)
{
    if (n == 1) {
        const QRect r = mapRect(transform, rects[0]);
        //surface->FillRectangle(surface, r.x(), r.y(), r.width(), r.height());
		gi_fill_rect(surface->gc,r.x(), r.y(), r.width(), r.height());
    } else {
        QVarLengthArray<gi_cliprect_t, 32> rectArray(n);
        for (int i=0; i<n; ++i) {
            const QRect r = mapRect(transform, rects[i]);
           // rectArray[i].x = r.x();
            //rectArray[i].y = r.y();
            //rectArray[i].w = r.width();
            //rectArray[i].h = r.height();

			gi_fill_rect(surface->gc,r.x(), r.y(), r.width(), r.height());
        }
        //surface->FillRectangles(surface, rectArray.constData(), n);
    }
}

template <class T>
static inline void drawRects(const T *rects, int n, const QTransform &transform, PLATFORM_SURFACE *surface)
{
    for (int i=0; i<n; ++i) {
        const QRect r = mapRect(transform, rects[i]);
		gi_draw_rect(surface->gc, r.x(), r.y(), r.width(), r.height()); //FIXME DPP
        //surface->DrawRectangle(surface, r.x(), r.y(), r.width(), r.height());
    }
}

#if defined QT_DIRECTFB_WARN_ON_RASTERFALLBACKS || defined QT_DEBUG
template <typename T> inline const T *ptr(const T &t) { return &t; }
template <> inline const bool* ptr<bool>(const bool &) { return 0; }
template <typename device, typename T1, typename T2, typename T3>
static void rasterFallbackWarn(const char *msg, const char *func, const device *dev,
                               uint transformationType, bool simplePen,
                               uint clipType, uint compositionModeStatus,
                               const char *nameOne, const T1 &one,
                               const char *nameTwo, const T2 &two,
                               const char *nameThree, const T3 &three)
{
    QString out;
    QDebug dbg(&out);
    dbg << msg << (QByteArray(func) + "()")  << "painting on";
    if (dev->devType() == QInternal::Widget) {
        dbg << static_cast<const QWidget*>(dev);
    } else {
        dbg << dev << "of type" << dev->devType();
    }

    dbg << QString::fromLatin1("transformationType 0x%1").arg(transformationType, 3, 16, QLatin1Char('0'))
        << "simplePen" << simplePen
        << "clipType" << clipType
        << "compositionModeStatus" << compositionModeStatus;

    const T1 *t1 = ptr(one);
    const T2 *t2 = ptr(two);
    const T3 *t3 = ptr(three);

    if (t1) {
        dbg << nameOne << *t1;
        if (t2) {
            dbg << nameTwo << *t2;
            if (t3) {
                dbg << nameThree << *t3;
            }
        }
    }
    qWarning("%s", qPrintable(out));
}

#endif // QT_DIRECTFB_WARN_ON_RASTERFALLBACKS

QT_END_NAMESPACE

#endif // QT_NO_QWS_DIRECTFB
