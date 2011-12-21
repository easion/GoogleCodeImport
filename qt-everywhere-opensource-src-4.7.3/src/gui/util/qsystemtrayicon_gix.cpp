/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsystemtrayicon_p.h"

#ifndef QT_NO_SYSTEMTRAYICON

QT_BEGIN_NAMESPACE

#include "qapplication.h"
#include "qsystemtrayicon.h"
#include "qdebug.h"
#include "qcolor.h"
#include "qfileinfo.h"
#include <qicon.h>
#include <gi/gi.h>
#include <gi/property.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define NF_NONE		0
#define NF_QT		1
#define NF_NATIVE	2

static 	int32_t notifyMode = NF_NONE;
QList<QSystemTrayIconSys *> QSystemTrayIconSys::trayIcons;
QCoreApplication::EventFilter QSystemTrayIconSys::oldEventFilter = 0;
extern "C" gi_window_id_t locate_system_tray(void);

static int 
add_tray(gi_window_id_t twin,gi_image_t *img)
{
	if (img)
	  gi_set_window_icon(twin,(uint32_t*)img->rgba,img->w,img->h);

	gi_add_proxy_tray(twin);
	return 0;
}


static bool allowsMessages()
{
#if 1
	gi_window_id_t window;	

	window = locate_system_tray();	
    return (window != 0);
#else
	return false;
#endif
}

QSystemTrayIconSys::QSystemTrayIconSys(QSystemTrayIcon *object)
    : hIcon(0), proxyWindow(0),q(object), ignoreNextMouseRelease(false)
{	
    static bool eventFilterAdded = false;

	InstallIcon();
    maxTipLength = 128;

	proxyWindow = gi_create_window(GI_DESKTOP_WINDOW_ID,0,0,1,1,
		GI_RGB( 106, 106, 114 ), GI_FLAGS_NON_FRAME);

	//qDebug() << "QSystemTrayIconSys: proxy window " << event->ev_window ;

	if (!eventFilterAdded) {
        oldEventFilter = qApp->setEventFilter(sysTrayTracker);
	}

	eventFilterAdded = true;
	trayIcons.append(this);


}

QSystemTrayIconSys::~QSystemTrayIconSys()
{
	trayIcons.removeAt(trayIcons.indexOf(this));

	if (trayIcons.isEmpty()) {
	}

		if (hIcon)
			gi_destroy_image(hIcon);
		if (proxyWindow){
			gi_remove_proxy_tray(proxyWindow);
			gi_destroy_window(proxyWindow);
		}
}

void
QSystemTrayIconSys::InstallIcon(void)
{
	QString appName = QFileInfo(QApplication::applicationFilePath()).fileName();

	QSystemTrayIconSys *sys=this;	
}


void QSystemTrayIconSys::UpdateTooltip()
{   	
    QString tip = q->toolTip();

	if ((int)proxyWindow > 0 && hIcon)
	{
		add_tray(proxyWindow, hIcon);
	}
}

void QSystemTrayIconSys::UpdateIcon()
{    
    QIcon qicon = q->icon();
	gi_image_t *img = NULL;

    if (qicon.isNull())
        return;

	img = QPixmap::to_gix_icon(qicon);

	hIcon = img;	
	UpdateTooltip();	
}


#if 1
QSystemTrayIconSys* QSystemTrayIconSys::findwindow(gi_window_id_t window)
{
	for (int i = 0; i < trayIcons.count(); i++) {
            if(trayIcons[i]->proxyid() == window)
				return trayIcons[i];
        }
	return NULL;
}

bool QSystemTrayIconSys::process_event(gi_msg_t *m)
{
    bool retval = false;

	if(m->type ==  GI_MSG_CLIENT_MSG)
    {
      if(m->body.client.client_type == GA_WM_PROTOCOLS
          &&m->params[0] == GA_WM_DELETE_WINDOW){
		  //exit
	  }
	  if (m->params[0] == GA_WM_MANAGER){

		  switch (m->params[1])
		  {
		  case G_NIN_ACTIVE:
			{
			  int x,y,w,h;

			  x = (m->params[2] >> 16)& 0xffff;
			  y = m->params[2] & 0xffff;
			  w = (m->params[3] >> 16)& 0xffff;
			  h = m->params[3] & 0xffff;
			  shelfRect.setRect(x, y, w, h);
			  qDebug("pfWinEvent G_NIN_ACTIVE=%d,%d,%d,%d",x, y,w,h);
			}
			break;

		  case G_NIN_LBUTTONDBLCLK:
			  ignoreNextMouseRelease = true; // Since DBLCLICK Generates a second mouse 
                                               // release we must ignore it
              emit q->activated(QSystemTrayIcon::DoubleClick);
			  qDebug("Tray pfWinEvent DoubleClick\n");
			  break;		  

		  case G_NIN_MOUSE_UP:
			  qDebug("Tray pfWinEvent up\n");
			  if (m->body.client.client_param & GI_BUTTON_L)
			  {
			  if (ignoreNextMouseRelease)
                    ignoreNextMouseRelease = false;
                else 
                    emit q->activated(QSystemTrayIcon::Trigger);
			  }
			  else if (m->body.client.client_param & GI_BUTTON_M)
			  {
				  emit q->activated(QSystemTrayIcon::MiddleClick);
			  }
			  else if (m->body.client.client_param & GI_BUTTON_R)
			  {
				  QPoint gpos;

				  //gpos = QCursor::pos();
				  gpos = QPoint(shelfRect.x(),shelfRect.y());

				  if (q->contextMenu()) {
                    q->contextMenu()->popup(gpos);
                }
				  emit q->activated(QSystemTrayIcon::Context);
			  }
			  break;

		  case G_NIN_BALLOON_CLICK:
			  qDebug("Tray pfWinEvent messageClicked\n");
                emit q->messageClicked();
                break;

		   default:
			   break;		  
		  }		  
	  }
	  retval = true;
    }
	return retval;
}

bool QSystemTrayIconSys::sysTrayTracker(void *message, long *result)
{
    bool retval = false;
	QSystemTrayIconSys *mysys = NULL;
    if (QSystemTrayIconSys::oldEventFilter)
        retval = QSystemTrayIconSys::oldEventFilter(message, result);

    if (trayIcons.isEmpty())
        return retval;

    gi_msg_t *m = (gi_msg_t *)message;
	mysys = findwindow(m->ev_window);

	if (mysys == NULL)
		return retval;

	retval = mysys->process_event(m);
	return retval;
}

#else
bool QSystemTrayIconSys::pfWinEvent( gi_msg_t *m, long *result )
{
	bool retval = false;

	if (m->ev_window != proxyWindow)
		return QWidget::pfWinEvent(m, result);
	
    return retval;
}
#endif

void QSystemTrayIconPrivate::install_sys()
{
	fprintf(stderr, "Reimplemented: QSystemTrayIconPrivate::install_sys \n");
    Q_Q(QSystemTrayIcon);
    if (!sys) {
        sys = new QSystemTrayIconSys(q);		
        sys->UpdateIcon();
    }
    supportsMessages_sys();    
}


static int iconFlag( QSystemTrayIcon::MessageIcon icon )
{
    switch (icon) {
        case QSystemTrayIcon::Information:
            return G_ICON_FLAG_INFO;
        case QSystemTrayIcon::Warning:
            return G_ICON_FLAG_WARNING;
        case QSystemTrayIcon::Critical:
            return G_ICON_FLAG_ERROR;
        case QSystemTrayIcon::NoIcon:
            return G_ICON_FLAG_NONE;
        default:
            Q_ASSERT_X(false, "QSystemTrayIconSys::showMessage", "Invalid QSystemTrayIcon::MessageIcon value");
            return G_ICON_FLAG_NONE;
    }
}


void QSystemTrayIconPrivate::showMessage_sys(const QString &title, 
	const QString &message, QSystemTrayIcon::MessageIcon type, int timeOut)
{
		
	if(notifyMode == NF_QT) {
		QPoint point(sys->shelfRect.x(),sys->shelfRect.y());

	    QBalloonTip::showBalloon(type, title, message,
			sys->q, point, timeOut, false);
	}
	else if(notifyMode == NF_NATIVE) {
		QByteArray title_name = title.toUtf8();
		QByteArray message_name = message.toUtf8();

		gi_wm_set_balloon(sys->proxyid(),
			(const char *)title_name.data(),
			(const char *)message_name.data(),
			NULL, iconFlag(type));
		gi_wm_show_balloon(sys->proxyid(),timeOut);

	}
	else{
	fprintf(stderr, "Reimplemented:  QSystemTrayIconPrivate::showMessage_sys()\n");	
	}
}

QRect QSystemTrayIconPrivate::geometry_sys() const
{
	fprintf(stderr, "Reimplemented: QSystemTrayIconPrivate::geometry_sys \n");
	return sys->shelfRect;
}

void QSystemTrayIconPrivate::remove_sys()
{
	fprintf(stderr, "Reimplemented: QSystemTrayIconPrivate::remove_sys \n");
	if(sys) {    
    	delete sys;
    	sys = NULL;
	}
}

void QSystemTrayIconPrivate::updateIcon_sys()
{
	fprintf(stderr, "Reimplemented:  QSystemTrayIconPrivate::updateIcon_sys\n");	
    if (sys) {
	    sys->UpdateIcon();
    }
}

void QSystemTrayIconPrivate::updateMenu_sys()
{
	fprintf(stderr, "Unimplemented:  QSystemTrayIconPrivate::updateMenu_sys\n");
}

void QSystemTrayIconPrivate::updateToolTip_sys()
{
	fprintf(stderr, "Reimplemented:  QSystemTrayIconPrivate::updateToolTip_sys\n");
	if (sys) {
		sys->UpdateTooltip();
	}
}

bool QSystemTrayIconPrivate::isSystemTrayAvailable_sys()
{	
	supportsMessages_sys();	
	return true;
}

bool QSystemTrayIconPrivate::supportsMessages_sys()
{
	notifyMode = NF_QT;
	if (allowsMessages())
	{
		notifyMode = NF_NATIVE;
	}
	return true;
}

QT_END_NAMESPACE

#endif // QT_NO_SYSTEMTRAYICON
