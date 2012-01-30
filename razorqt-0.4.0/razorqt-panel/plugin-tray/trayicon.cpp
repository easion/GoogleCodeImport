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


// Warning: order of those include is important.
#include <QDebug>
#include <QApplication>
#include <QtGui/QResizeEvent>
#include <QtGui/QPainter>
#include <QtGui/QBitmap>
#include <QtGui/QStyle>

#include "../panel/razorpanel.h"
#include "trayicon.h"
#include "razorqt/xfitman.h"
#include <gi/gi.h>
#include <gi/region.h>
#include <gi/property.h>

#define XEMBED_EMBEDDED_NOTIFY 0


//static bool xError;


/************************************************

int windowErrorHandler(Display *d, XErrorEvent *e)
{
    d=d;e=e;
    //xError = true;
    if (e->error_code != BadWindow) {
        char str[1024];
        XGetErrorText(d, e->error_code,  str, 1024);
        qWarning() << "Error handler" << e->error_code
                   << str;
    }
    return 0;
}
************************************************/


/************************************************

 ************************************************/
TrayIcon::TrayIcon(gi_window_id_t iconId, QWidget* parent):
    QFrame(parent),
    mIconId(iconId),
    mWindowId(0),
    mIconSize(TRAY_ICON_SIZE_DEFAULT, TRAY_ICON_SIZE_DEFAULT)
    //mDamage(0)
{
    setObjectName("TrayIcon");

    mValid = init();
}



/************************************************

 ************************************************/
bool TrayIcon::init()
{
    //Display* dsp = QX11Info::display();

    gi_window_info_t attr;
    if (! gi_get_window_info( mIconId, &attr)) return false;

//    qDebug() << "New tray icon ***********************************";
//    qDebug() << "  * window id:  " << hex << mIconId;
//    qDebug() << "  * window name:" << xfitMan().getName(mIconId);
//    qDebug() << "  * size (WxH): " << attr.width << "x" << attr.height;
//    qDebug() << "  * color depth:" << attr.depth;

    //unsigned long mask = 0;
    //XSetWindowAttributes set_attr;

    //Visual* visual = attr.visual;
    //set_attr.colormap = attr.colormap;
    //set_attr.background_pixel = 0;
    //set_attr.border_pixel = 0;
    //mask = CWColormap|CWBackPixel|CWBorderPixel;

    mWindowId = gi_create_window( this->winId(), 0, 0, mIconSize.width(), mIconSize.height(),
                              GI_RGB( 192, 192, 192 ), GI_FLAGS_NON_FRAME);
	


    //xError = false;
    //XErrorHandler old;
    //old = XSetErrorHandler(windowErrorHandler);
    gi_reparent_window( mIconId, mWindowId, 0, 0);
    //XSync(dsp, false);
    //XSetErrorHandler(old);

    //if (xError) {
     //   qWarning() << "****************************************";
       // qWarning() << "* Not icon_swallow                     *";
      ///  qWarning() << "****************************************";
       // gi_destroy_window( mWindowId);
      //  return false;
    //}


    {
        gi_atom_id_t acttype;
        int actfmt;
        unsigned long nbitem, bytes;
        unsigned char *data = 0;
        int ret;

        ret = gi_get_window_property( mIconId, xfitMan().atom("_XEMBED_INFO"),
                                 0, 2, false, xfitMan().atom("_XEMBED_INFO"),
                                 &acttype, &actfmt, &nbitem, &bytes, &data);
        if (ret == 0) {
            if (data)
                free(data);
        }
        else {
            qWarning() << "TrayIcon: xembed error";
            gi_destroy_window( mWindowId);
            return false;
        }
    }

    {
        gi_msg_t e;
        e.type = GI_MSG_CLIENT_MSG;
        //e.xclient.serial = 0;
        //e.xclient.send_event = TRUE;
        e.body.client.client_type = xfitMan().atom("_XEMBED");
        e.body.client.client_format = 32;
        e.ev_window = mIconId;
        e.params[0] = 0;
        e.params[1] = XEMBED_EMBEDDED_NOTIFY;
        e.params[2] = 0;
        e.params[3] = mWindowId;
        //e.xclient.data.l[4] = 0;
        gi_send_event( mIconId, false, 0xFFFFFF, &e);
    }

    gi_set_events_mask( mIconId, GI_MASK_CLIENT_MSG);
    //mDamage = XDamageCreate(dsp, mIconId, XDamageReportRawRectangles);
    //XCompositeRedirectWindow(dsp, mWindowId, CompositeRedirectManual);

    gi_show_window( mIconId);
    gi_raise_window( mWindowId);

    xfitMan().resizeWindow(mWindowId, mIconSize.width(), mIconSize.height());
    xfitMan().resizeWindow(mIconId,   mIconSize.width(), mIconSize.height());

    return true;
}


/************************************************

 ************************************************/
TrayIcon::~TrayIcon()
{
    //Display* dsp = QX11Info::display();
    gi_set_events_mask( mIconId, GI_MSG_NONE);

    //if (mDamage)
     //   XDamageDestroy(dsp, mDamage);

    // reparent to root
    //xError = false;
    //XErrorHandler old = XSetErrorHandler(windowErrorHandler);

    gi_hide_window( mIconId);
    gi_reparent_window( mIconId, GI_DESKTOP_WINDOW_ID, 0, 0);

    gi_destroy_window( mWindowId);
    //XSync(dsp, FALSE);
    //XSetErrorHandler(old);
}


/************************************************

 ************************************************/
QSize TrayIcon::sizeHint() const
{
    QMargins margins = contentsMargins();
    return QSize(margins.left() + mIconSize.width() + margins.right(),
                 margins.top() + mIconSize.height() + margins.bottom()
                );
}


/************************************************

 ************************************************/
void TrayIcon::setIconSize(QSize iconSize)
{
    mIconSize = iconSize;

    if (mWindowId)
        xfitMan().resizeWindow(mWindowId, mIconSize.width(), mIconSize.height());

    if (mIconId)
        xfitMan().resizeWindow(mIconId,   mIconSize.width(), mIconSize.height());
}


/************************************************

 ************************************************/
bool TrayIcon::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::Paint:
            draw(static_cast<QPaintEvent*>(event));
            break;

        case QEvent::Resize:
            {
                QRect rect = iconGeometry();
                xfitMan().moveWindow(mWindowId, rect.left(), rect.top());
            }
            break;

            case QEvent::MouseButtonPress:
                event->accept();
                break;
         default:
            break;
    }

    return QFrame::event(event);
}


/************************************************

 ************************************************/
QRect TrayIcon::iconGeometry()
{
    QRect res = QRect(QPoint(0, 0), mIconSize);

    res.moveCenter(QRect(0, 0, width(), height()).center());
    return res;
}


/************************************************

 ************************************************/
void TrayIcon::draw(QPaintEvent* /*event*/)
{
    //Display* dsp = QX11Info::display();

    gi_window_info_t attr;
    if (!gi_get_window_info( mIconId, &attr))
    {
        qWarning() << "Paint error";
        return;
    }

    gi_image_t* ximage = gi_get_window_image( mIconId, 0, 0, 
		attr.width, attr.height, GI_RENDER_a8r8g8b8, FALSE,NULL);
    if (!ximage)
    {
        qWarning() << "    * Error image is NULL";
        return;
    }


//    qDebug() << "Paint icon **************************************";
//    qDebug() << "  * XComposite: " << isXCompositeAvailable();
//    qDebug() << "  * Icon geometry:" << iconGeometry();
//    qDebug() << "  Icon";
//    qDebug() << "    * window id:  " << hex << mIconId;
//    qDebug() << "    * window name:" << xfitMan().getName(mIconId);
//    qDebug() << "    * size (WxH): " << attr.width << "x" << attr.height;
//    qDebug() << "    * pos (XxY):  " << attr.x << attr.y;
//    qDebug() << "    * color depth:" << attr.depth;
//    qDebug() << "  XImage";
//    qDebug() << "    * size (WxH):  " << ximage->width << "x" << ximage->height;
//    switch (ximage->format)
//    {
//        case XYBitmap: qDebug() << "    * format:   XYBitmap"; break;
//        case XYPixmap: qDebug() << "    * format:   XYPixmap"; break;
//        case ZPixmap:  qDebug() << "    * format:   ZPixmap"; break;
//    }
//    qDebug() << "    * color depth:  " << ximage->depth;
//    qDebug() << "    * bits per pixel:" << ximage->bits_per_pixel;


    //const uchar* d =(uchar*) ximage->data;
    QImage image = QImage((const uchar*) ximage->rgba, ximage->w, ximage->h, 
		ximage->pitch,  QImage::Format_ARGB32_Premultiplied);


    // Draw QImage ...........................
    QPainter painter(this);
    QRect iconRect = iconGeometry();
    if (image.size() != iconRect.size())
    {
        image = image.scaled(iconRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QRect r = image.rect();
        r.moveCenter(iconRect.center());
        iconRect = r;
    }
//    qDebug() << " Draw rect:" << iconRect;

    painter.drawImage(iconRect, image);

    gi_destroy_image(ximage);
//    debug << "End paint icon **********************************";
}


/************************************************

 ************************************************/
bool TrayIcon::isXCompositeAvailable()
{
    //int eventBase, errorBase;
    return false;//XCompositeQueryExtension(QX11Info::display(), &eventBase, &errorBase );
}
