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

#include "qdirectfbpixmap.h"

#ifndef QT_NO_QWS_DIRECTFB

#include "qdirectfbscreen.h"
#include "qdirectfbpaintengine.h"

#include <QtGui/qbitmap.h>
#include <QtCore/qfile.h>
#include <gi/gi.h>
#include <gi/property.h>

QT_BEGIN_NAMESPACE

static int global_ser_no = 0;

QGixPixmapData::QGixPixmapData(QGixScreen *screen, PixelType pixelType)
    : QPixmapData(pixelType, DirectFBClass), QGixPaintDevice(screen),
      alpha(false)
{
    setSerialNumber(0);
}

QGixPixmapData::~QGixPixmapData()
{
}

void QGixPixmapData::resize(int width, int height)
{
    if (width <= 0 || height <= 0) {
        invalidate();
        return;
    }

    imageFormat = screen->pixelFormat();
    dfbSurface = screen->createDFBSurface(QSize(width, height),
                                          imageFormat,
                                          QGixScreen::TrackSurface);
    d = QGixScreen::depth(imageFormat);
    alpha = false;
    if (!dfbSurface) {
        invalidate();
        qWarning("QGixPixmapData::resize(): Unable to allocate surface");
        return;
    }

    w = width;
    h = height;
    is_null = (w <= 0 || h <= 0);
    setSerialNumber(++global_ser_no);
}

#ifdef QT_DIRECTFB_OPAQUE_DETECTION
// mostly duplicated from qimage.cpp (QImageData::checkForAlphaPixels)
static bool checkForAlphaPixels(const QImage &img)
{
    const uchar *bits = img.bits();
    const int bytes_per_line = img.bytesPerLine();
    const uchar *end_bits = bits + bytes_per_line;
    const int width = img.width();
    const int height = img.height();
    switch (img.format()) {
    case QImage::Format_Indexed8:
        return img.hasAlphaChannel();
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied:
        for (int y=0; y<height; ++y) {
            for (int x=0; x<width; ++x) {
                if ((((uint *)bits)[x] & 0xff000000) != 0xff000000) {
                    return true;
                }
            }
            bits += bytes_per_line;
        }
        break;

    case QImage::Format_ARGB8555_Premultiplied:
    case QImage::Format_ARGB8565_Premultiplied:
        for (int y=0; y<height; ++y) {
            while (bits < end_bits) {
                if (bits[0] != 0) {
                    return true;
                }
                bits += 3;
            }
            bits = end_bits;
            end_bits += bytes_per_line;
        }
        break;

    case QImage::Format_ARGB6666_Premultiplied:
        for (int y=0; y<height; ++y) {
            while (bits < end_bits) {
                if ((bits[0] & 0xfc) != 0) {
                    return true;
                }
                bits += 3;
            }
            bits = end_bits;
            end_bits += bytes_per_line;
        }
        break;

    case QImage::Format_ARGB4444_Premultiplied:
        for (int y=0; y<height; ++y) {
            while (bits < end_bits) {
                if ((bits[0] & 0xf0) != 0) {
                    return true;
                }
                bits += 2;
            }
            bits = end_bits;
            end_bits += bytes_per_line;
        }
        break;

    default:
        break;
    }

    return false;
}
#endif // QT_DIRECTFB_OPAQUE_DETECTION

bool QGixPixmapData::hasAlphaChannel(const QImage &img, Qt::ImageConversionFlags flags)
{
    if (img.depth() == 1)
        return true;
#ifdef QT_DIRECTFB_OPAQUE_DETECTION
    return ((flags & Qt::NoOpaqueDetection) ? img.hasAlphaChannel() : checkForAlphaPixels(img));
#else
    Q_UNUSED(flags);
    return img.hasAlphaChannel();
#endif
}

#ifdef QT_DIRECTFB_IMAGEPROVIDER
bool QGixPixmapData::fromFile(const QString &filename, const char *format,
                                   Qt::ImageConversionFlags flags)
{
    if (!QFile::exists(filename))
        return false;
    if (flags == Qt::AutoColor) {
        if (filename.startsWith(QLatin1Char(':'))) { // resource
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly))
                return false;
            const QByteArray data = file.readAll();
            file.close();
            return fromData(reinterpret_cast<const uchar*>(data.constData()), data.size(), format, flags);
        } else {
            DFBDataBufferDescription description;
            description.is_file = 1;
            const QByteArray fileNameData = filename.toLocal8Bit();
            description.data = (void*)fileNameData.constData();
            if (fromDataBufferDescription(description)) {
                return true;
            }
            // fall back to Qt
        }
    }
    return QPixmapData::fromFile(filename, format, flags);
}

bool QGixPixmapData::fromData(const uchar *buffer, uint len, const char *format,
                                   Qt::ImageConversionFlags flags)
{
    if (flags == Qt::AutoColor) {
        DFBDataBufferDescription description;
        description.is_file = 0;
        description.data = (void*)buffer;
        description.length = len;
        if (fromDataBufferDescription(description))
            return true;
        // fall back to Qt
    }
    return QPixmapData::fromData(buffer, len, format, flags);
}

template <typename T> struct QGixInterfaceCleanupHandler
{
    static void cleanup(T *t) { if (t) t->Release(t); }
};

template <typename T>
class QGixPointer : public QScopedPointer<T, QGixInterfaceCleanupHandler<T> >
{
public:
    QGixPointer(T *t = 0)
        : QScopedPointer<T, QGixInterfaceCleanupHandler<T> >(t)
    {}
};

bool QGixPixmapData::fromDataBufferDescription(const DFBDataBufferDescription &dataBufferDescription)
{
    //IGix *dfb = screen->dfb();
    //Q_ASSERT(dfb);
    int result = 0;
    //IGixDataBuffer *dataBufferPtr;
    //if ((result = dfb->CreateDataBuffer(dfb, &dataBufferDescription, &dataBufferPtr)) != DFB_OK) {
    //    PLATFORM_ERROR("QGixPixmapData::fromDataBufferDescription()", result);
    //    return false;
    //}
    //QGixPointer<IGixDataBuffer> dataBuffer(dataBufferPtr);

    //IGixImageProvider *providerPtr;
    //if ((result = dataBuffer->CreateImageProvider(dataBuffer.data(), &providerPtr)) != DFB_OK) {
     //   PLATFORM_ERROR("QGixPixmapData::fromDataBufferDescription(): Can't create image provider", result);
      //  return false;
    //}
    //QGixPointer<IGixImageProvider> provider(providerPtr);

    //DFBSurfaceDescription surfaceDescription;
    //if ((result = provider->GetSurfaceDescription(provider.data(), &surfaceDescription)) != DFB_OK) {
     //   PLATFORM_ERROR("QGixPixmapData::fromDataBufferDescription(): Can't get surface description", result);
     //   return false;
    //}

    //DFBImageDescription imageDescription; //FIXME DPP
    //result = provider->GetImageDescription(provider.data(), &imageDescription);
    //if (result != DFB_OK) {
    //    PLATFORM_ERROR("QGixPixmapData::fromSurfaceDescription(): Can't get image description", result);
     //   return false;
    //}

    //alpha = imageDescription.caps & (DICAPS_ALPHACHANNEL|DICAPS_COLORKEY);//FIXME DPP
    /*imageFormat = alpha ? screen->alphaPixmapFormat() : screen->pixelFormat();

    dfbSurface = screen->createDFBSurface(QSize(surfaceDescription.width, surfaceDescription.height),
                                          imageFormat, QGixScreen::TrackSurface);

    result = provider->RenderTo(provider.data(), dfbSurface, 0);
    if (result != DFB_OK) {
        PLATFORM_ERROR("QGixPixmapData::fromSurfaceDescription(): Can't render to surface", result);
        return false;
    }

    w = surfaceDescription.width;
    h = surfaceDescription.height;
    is_null = (w <= 0 || h <= 0);
    d = QGixScreen::depth(imageFormat);
    setSerialNumber(++global_ser_no);


*/

    return false;
}

#endif

void QGixPixmapData::fromImage(const QImage &img, Qt::ImageConversionFlags flags)
{
    alpha = QGixPixmapData::hasAlphaChannel(img, flags);
    imageFormat = alpha ? screen->alphaPixmapFormat() : screen->pixelFormat();
    QImage image;
    if ((flags & ~Qt::NoOpaqueDetection) != Qt::AutoColor) {
        image = img.convertToFormat(imageFormat, flags);
        flags = Qt::AutoColor;
    } else if (img.format() == QImage::Format_RGB32 || img.depth() == 1) {
        image = img.convertToFormat(imageFormat, flags);
    } else {
        image = img;
    }

    PLATFORM_SURFACE *imageSurface = screen->createDFBSurface(image, image.format(), QGixScreen::DontTrackSurface);
    if (!imageSurface) {
        qWarning("QGixPixmapData::fromImage()");
        invalidate();
        return;
    }

    dfbSurface = screen->createDFBSurface(image.size(), imageFormat, QGixScreen::TrackSurface);
    if (!dfbSurface) {
        qWarning("QGixPixmapData::fromImage()");
        invalidate();
        return;
    }

    if (image.hasAlphaChannel()) {
        //dfbSurface->Clear(dfbSurface, 0, 0, 0, 0); //FIXME DPP
        //dfbSurface->SetBlittingFlags(dfbSurface, DSBLIT_BLEND_ALPHACHANNEL);
    } else {
        //dfbSurface->SetBlittingFlags(dfbSurface, DSBLIT_NOFX);
    }

    //dfbSurface->Blit(dfbSurface, imageSurface, 0, 0, 0);//FIXME DPP
    //imageSurface->Release(imageSurface);

    w = image.width();
    h = image.height();
    is_null = (w <= 0 || h <= 0);
    d = QGixScreen::depth(imageFormat);
    setSerialNumber(++global_ser_no);
#ifdef QT_NO_DIRECTFB_OPAQUE_DETECTION
    Q_UNUSED(flags);
#endif
}

void QGixPixmapData::copy(const QPixmapData *data, const QRect &rect)
{
    if (data->classId() != DirectFBClass) {
        QPixmapData::copy(data, rect);
        return;
    }

    const QGixPixmapData *otherData = static_cast<const QGixPixmapData*>(data);
#ifdef QT_NO_DIRECTFB_SUBSURFACE
    if (otherData->lockFlags()) {
        const_cast<QGixPixmapData*>(otherData)->unlockSurface();
    }
#endif
    PLATFORM_SURFACE *src = otherData->gixSurface();
    alpha = data->hasAlphaChannel();
    imageFormat = (alpha
                   ? QGixScreen::instance()->alphaPixmapFormat()
                   : QGixScreen::instance()->pixelFormat());


    dfbSurface = screen->createDFBSurface(rect.size(), imageFormat,
                                          QGixScreen::TrackSurface);
    if (!dfbSurface) {
        qWarning("QGixPixmapData::copy()");
        invalidate();
        return;
    }

    if (alpha) {
        //dfbSurface->Clear(dfbSurface, 0, 0, 0, 0); //FIXME DPP
        //dfbSurface->SetBlittingFlags(dfbSurface, DSBLIT_BLEND_ALPHACHANNEL);
    } else {
        //dfbSurface->SetBlittingFlags(dfbSurface, DSBLIT_NOFX);
    }
    const gi_cliprect_t blitRect = { rect.x(), rect.y(),
                                    rect.width(), rect.height() };
    w = rect.width();
    h = rect.height();
    d = otherData->d;
    is_null = (w <= 0 || h <= 0);
    unlockSurface();
    //int result = dfbSurface->Blit(dfbSurface, src, &blitRect, 0, 0); //FIXME DPP
#if (Q_DIRECTFB_VERSION >= 0x010000)
   // dfbSurface->ReleaseSource(dfbSurface);
#endif
    //if (result != 0) {
     //   PLATFORM_ERROR("QGixPixmapData::copy()", result);
     //   invalidate();
     //   return;
    //}

    setSerialNumber(++global_ser_no);
}

static inline bool isOpaqueFormat(QImage::Format format)
{
    switch (format) {
    case QImage::Format_RGB32:
    case QImage::Format_RGB16:
    case QImage::Format_RGB666:
    case QImage::Format_RGB555:
    case QImage::Format_RGB888:
    case QImage::Format_RGB444:
        return true;
    default:
        break;
    }
    return false;
}

void QGixPixmapData::fill(const QColor &color)
{
    if (!serialNumber())
        return;

    Q_ASSERT(dfbSurface);

    alpha |= (color.alpha() < 255);

    if (alpha && isOpaqueFormat(imageFormat)) {
        QSize size;
#if 0 //FIXME DPP
        dfbSurface->GetSize(dfbSurface, &size.rwidth(), &size.rheight());
        screen->releaseDFBSurface(dfbSurface);
#endif
        imageFormat = screen->alphaPixmapFormat();
        d = QGixScreen::depth(imageFormat);
        dfbSurface = screen->createDFBSurface(size, screen->alphaPixmapFormat(), QGixScreen::TrackSurface);
        setSerialNumber(++global_ser_no);
        if (!dfbSurface) {
            qWarning("QGixPixmapData::fill()");
            invalidate();
            return;
        }
    }

	//FIXME DPP
    //dfbSurface->Clear(dfbSurface, color.red(), color.green(), color.blue(), color.alpha());
}

QPixmap QGixPixmapData::transformed(const QTransform &transform,
                                         Qt::TransformationMode mode) const
{
    QGixPixmapData *that = const_cast<QGixPixmapData*>(this);
#ifdef QT_NO_DIRECTFB_SUBSURFACE
    if (lockFlags())
        that->unlockSurface();
#endif

    if (!dfbSurface || transform.type() != QTransform::TxScale
        || mode != Qt::FastTransformation)
    {
        const QImage *image = that->buffer();
        Q_ASSERT(image);
        const QImage transformed = image->transformed(transform, mode);
        QGixPixmapData *data = new QGixPixmapData(screen, QPixmapData::PixmapType);
        data->fromImage(transformed, Qt::AutoColor);
        return QPixmap(data);
    }

    const QSize size = transform.mapRect(QRect(0, 0, w, h)).size();
    if (size.isEmpty())
        return QPixmap();

    QGixPixmapData *data = new QGixPixmapData(screen, QPixmapData::PixmapType);
    data->setSerialNumber(++global_ser_no);
    //DFBSurfaceBlittingFlags flags = DSBLIT_NOFX;
    data->alpha = alpha;
    if (alpha) {
      //  flags = DSBLIT_BLEND_ALPHACHANNEL;
    }
    data->dfbSurface = screen->createDFBSurface(size,
                                                imageFormat,
                                                QGixScreen::TrackSurface);
#if 0 //FIXME DPP
    if (flags & DSBLIT_BLEND_ALPHACHANNEL) {
        data->dfbSurface->Clear(data->dfbSurface, 0, 0, 0, 0);
    }
    data->dfbSurface->SetBlittingFlags(data->dfbSurface, flags);

    const gi_cliprect_t destRect = { 0, 0, size.width(), size.height() };
    data->dfbSurface->StretchBlit(data->dfbSurface, dfbSurface, 0, &destRect);
    data->w = size.width();
    data->h = size.height();
    data->is_null = (data->w <= 0 || data->h <= 0);

#if (Q_DIRECTFB_VERSION >= 0x010000)
    data->dfbSurface->ReleaseSource(data->dfbSurface);
#endif

#endif

    return QPixmap(data);
}

QImage QGixPixmapData::toImage() const
{
    if (!dfbSurface)
        return QImage();

#if 0 //FIXME DPP
    // In later versions of Gix one can set a flag to tell
    // Gix not to move the surface to videomemory. When that
    // happens we can use this (hopefully faster) codepath
#ifndef QT_NO_DIRECTFB_PREALLOCATED
    QImage ret(w, h, QGixScreen::pixelFormatToQt(dfbSurface->PixelFormat));
    if (PLATFORM_SURFACE *imgSurface = screen->createDFBSurface(ret, QGixScreen::DontTrackSurface)) {
        if (hasAlphaChannel()) {
            imgSurface->SetBlittingFlags(imgSurface, DSBLIT_BLEND_ALPHACHANNEL);
            imgSurface->Clear(imgSurface, 0, 0, 0, 0);
        } else {
            imgSurface->SetBlittingFlags(imgSurface, DSBLIT_NOFX);
        }
        imgSurface->Blit(imgSurface, dfbSurface, 0, 0, 0);
#if (Q_DIRECTFB_VERSION >= 0x010000)
        imgSurface->ReleaseSource(imgSurface);
#endif
        imgSurface->Release(imgSurface);
        return ret;
    }
#endif
#endif

    QGixPixmapData *that = const_cast<QGixPixmapData*>(this);
    const QImage *img = that->buffer();
    return img->copy();
}

/* This is QPixmapData::paintEngine(), not QPaintDevice::paintEngine() */

QPaintEngine *QGixPixmapData::paintEngine() const
{
    if (!engine) {
        // QGixPixmapData is also a QCustomRasterPaintDevice, so pass
        // that to the paint engine:
        QGixPixmapData *that = const_cast<QGixPixmapData*>(this);
        that->engine = new QGixPaintEngine(that);
    }
    return engine;
}

QImage *QGixPixmapData::buffer()
{
    if (!lockFlgs) {
        lockSurface(3);
    }
    Q_ASSERT(lockFlgs);
    Q_ASSERT(!lockedImage.isNull());
    return &lockedImage;
}


bool QGixPixmapData::scroll(int dx, int dy, const QRect &rect)
{
    if (!dfbSurface) {
        return false;
    }
    unlockSurface();

#if 0 //FIXME DPP
    int result = dfbSurface->SetBlittingFlags(dfbSurface, DSBLIT_NOFX);
    if (result != DFB_OK) {
        PLATFORM_ERROR("QGixPixmapData::scroll", result);
        return false;
    }
    result = dfbSurface->SetPorterDuff(dfbSurface, DSPD_NONE);
    if (result != DFB_OK) {
        PLATFORM_ERROR("QGixPixmapData::scroll", result);
        return false;
    }

    const gi_cliprect_t source = { rect.x(), rect.y(), rect.width(), rect.height() };
    result = dfbSurface->Blit(dfbSurface, dfbSurface, &source, source.x + dx, source.y + dy);
    if (result != DFB_OK) {
        PLATFORM_ERROR("QGixPixmapData::scroll", result);
        return false;
    }
#endif
    return true;
}

void QGixPixmapData::invalidate()
{
    if (dfbSurface) {
        screen->releaseDFBSurface(dfbSurface);
        dfbSurface = 0;
    }
    setSerialNumber(0);
    alpha = false;
    d = w = h = 0;
    is_null = true;
    imageFormat = QImage::Format_Invalid;
}

#ifndef QT_DIRECTFB_PLUGIN
Q_GUI_EXPORT PLATFORM_SURFACE *qt_directfb_surface_for_pixmap(const QPixmap &pixmap)
{
    const QPixmapData *data = pixmap.pixmapData();
    if (!data || data->classId() != QPixmapData::DirectFBClass)
        return 0;
    const QGixPixmapData *dfbData = static_cast<const QGixPixmapData*>(data);
    return dfbData->gixSurface();
}
#endif

QT_END_NAMESPACE

#endif // QT_NO_QWS_DIRECTFB
