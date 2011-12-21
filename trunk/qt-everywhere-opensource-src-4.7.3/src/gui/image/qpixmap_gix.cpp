#include "qpixmap.h"

#include "qpixmap_raster_p.h"

#include "qbitmap.h"
#include "qimage.h"
#include "qwidget.h"
#include "qpainter.h"
#include "qdatastream.h"
#include "qbuffer.h"
#include "qapplication.h"
#include "qevent.h"
#include "qfile.h"
#include "qfileinfo.h"
#include "qdatetime.h"
#include "qpixmapcache.h"
#include "qimagereader.h"
#include "qimagewriter.h"
#include "qdebug.h"
#include "qicon.h"

#include <stdio.h>
#include <gi/gi.h>
#include <gi/property.h>

gi_image_t *
QPixmap::toGixBitmap(unsigned code) const
{
	gi_image_t *bitmap = NULL;
	gi_format_code_t format = (gi_format_code_t)code;
	
    if (isNull())
        return 0;
    
    if (data->classId() == QPixmapData::RasterClass) {
        QRasterPixmapData* d = static_cast<QRasterPixmapData*>(data.data());
        int w = d->image.width();
        int h = d->image.height();

        const QImage image = d->image.convertToFormat(QImage::Format_ARGB32);
        int bytes_per_line = w * 4;        
        
        //bitmap = new gi_image_t(gi_cliprect_t(0,0,w-1,h-1), B_RGBA32);
        bitmap = gi_create_image_depth(w,h,format);
        uchar *pixels = (uchar *)bitmap->rgba;

        for (int y=0; y<h; ++y)
            memcpy(pixels + y * bytes_per_line, image.scanLine(y), bytes_per_line);
	
	} else {
    
        QPixmapData *data = new QRasterPixmapData(depth() == 1 ?
                                                  QPixmapData::BitmapType : QPixmapData::PixmapType);
        data->fromImage(toImage(), Qt::AutoColor);
        return QPixmap(data).toGixBitmap(format);
    }        	
	return bitmap;
}

QPixmap
QPixmap::fromGixPixmap(gi_window_id_t pixmap)
{
	gi_image_t *bmp = NULL;
	gi_window_info_t wininfo;

	gi_get_window_info(pixmap,&wininfo); //
	//img = gi_get_window_image(src, 0, 0, wininfo.width, wininfo.height,winfo.format,0,NULL);
	img = gi_get_window_image(src, 0, 0, wininfo.width, wininfo.height,GI_RENDER_a8r8g8b8,0,NULL);
	if (!img)
	  return QPixmap();
	return fromGixBitmap(bmp);
}

QPixmap
QPixmap::fromGixBitmap(gi_image_t *bmp)
{
	int w, h;

	if(!bmp || (GI_RENDER_FORMAT_BPP(bmp->format) != 32 )) 
		return QPixmap();
		
	w = bmp->w;
	h = bmp->h;
	
	QImage image(w,h,QImage::Format_ARGB32);

	int bytes_per_line = w * 4;                        	
    uchar *pixels = (uchar *)bmp->rgba;

    for (int y=0; y<h; ++y)
       memcpy( image.scanLine(y), pixels + y * bytes_per_line, bytes_per_line);
    
	return QPixmap::fromImage(image);
}



gi_image_t* QPixmap::to_gix_icon(const QIcon &icon, bool isPointer,
                               int hotX, int hotY, bool embedRealAlpha,
                               bool isMini)
{
	uint32_t w = 22,h = 22;

    if (icon.isNull())
        return NULL;

	//gi_query_sys_metrics(0, &w);
	//gi_query_sys_metrics(1, &h);

	if (isPointer == false)	{
		w = 22;
		h = 22;
	}
	else{
		w = 32;
		h = 32;
	}
#if 0

	QVector<long> icon_data;

	QList<QSize> availableSizes = icon.availableSizes();
	if(availableSizes.isEmpty()) {
		//Èç¹ûÎª¿Õ
		// try to use default sizes since the icon can be a scalable image like svg.
		//availableSizes.push_back(QSize(16,16));
		availableSizes.push_back(QSize(22,22));            
	}
	for(int i = 0; i < availableSizes.size(); ++i) {
		QSize size = availableSizes.at(i);
		QPixmap pixmap = icon.pixmap(size);
		if (!pixmap.isNull()) {
			QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
			int pos = icon_data.size();
			icon_data.resize(pos  + image.width()*image.height());
			iw = image.width();
			ih = image.height();
			if (sizeof(long) == sizeof(quint32)) {
				memcpy(icon_data.data() + pos, image.scanLine(0), image.byteCount());
			} else {
				for (int y = 0; y < image.height(); ++y) {
					uint *scanLine = reinterpret_cast<uint *>(image.scanLine(y));
					for (int x = 0; x < image.width(); ++x)
						icon_data[pos + y*image.width() + x] = scanLine[x];
				}
			}
		}
	}
	if (!icon_data.isEmpty()) {         
	}
#endif
    // get the system icon size
   
    // obtain the closest (but never larger) icon size we have
    QSize size = icon.actualSize(QSize(w, h));

    QPixmap pm = icon.pixmap(size);
    if (pm.isNull())
        return NULL;

    // if we got a smaller pixmap then center it inside the box matching the
    // system size instead of letting WinCreatePointerIndirect() scale (this
    // covers a usual case when we get 32/16 px pixmaps on a 120 DPI system
    // where the icon size is 40/20 px respectively): scaling such small images
    // looks really ugly.
    if (!pm.isNull() && (pm.width() < w || pm.height() < h)) {
        Q_ASSERT(pm.width() <= w && pm.height() <= h);
        QPixmap pmNew(w, h);
        pmNew.fill(Qt::transparent);
        QPainter painter(&pmNew);
        int dx = (w - pm.width()) / 2;
        int dy = (h - pm.height()) / 2;
        painter.drawPixmap(dx, dy, pm);
        pm = pmNew;
        hotX += dx;
        hotY += dy;
    }
    
    gi_image_t* hIcon;
	hIcon = pm.toGixBitmap(GI_RENDER_a8r8g8b8);
    return hIcon;
}


//FIXME DPP
QPixmap 
QPixmap::grabWindow(WId winId, int x, int y, int w, int h )
{
    if (w == 0 || h == 0 )
        return QPixmap();
        	
	gi_image_t *bitmap=NULL;
	int screen_w = gi_screen_width();
	int screen_h = gi_screen_height();	

    if (w < 0)
        w = screen_w - x;
    if (h < 0)
        h = screen_h - y;

	QImage::Format format = QImage::Format_RGB32;

#if 0
	WId unused;

	if (!gi_translate_coordinates( winId, GI_DESKTOP_WINDOW_ID, x, y,
         &x, &y, &unused))
       return QPixmap();

	winId = GI_DESKTOP_WINDOW_ID;
#endif

#if 0
	switch(bitmap->ColorSpace()) {
		//case B_GRAY1:
		//	format = QImage::Format_Mono;
		//	break;
		//case B_GRAY8:
		//case B_CMAP8:
		//	format = QImage::Format_Indexed8;
		//	break;
		case B_RGB15:
		case B_RGBA15:
		case B_RGB16:
			format = QImage::Format_RGB16;
			break;
		case B_RGB32:
		default:
			format = QImage::Format_RGB32;
			break;
	}
#endif
	bitmap = gi_get_window_image(winId, x,y,w,h,GI_RENDER_x8r8g8b8, FALSE,NULL );
	QRect grabRect(x,y,w,h );
	QImage image((uchar*)bitmap->rgba, screen_w, screen_h, 
		bitmap->pitch, format);
    image = image.copy(grabRect);
    
	gi_destroy_image( bitmap);
	
	return QPixmap::fromImage(image);

}

