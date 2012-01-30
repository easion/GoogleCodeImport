/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Alexander Sokoloff <sokoloff.a@gmail.ru>
 *   Maciej Płaza <plaza.maciej@gmail.com>
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


#include <QApplication>

#include <QtCore/QDebug>
#include <QToolButton>
#include <QSettings>

#include "razortaskbar.h"
#include <qtxdg/xdgicon.h>
#include <razorqt/xfitman.h>
#include <QtCore/QList>


#include <QDesktopWidget>

#include <gi/gi.h>
#include "razortaskbutton.h"
#include <gi/property.h>

#include <QX11Info>

EXPORT_RAZOR_PANEL_PLUGIN_CPP(RazorTaskBar)

/************************************************

************************************************/
RazorTaskBar::RazorTaskBar(const RazorPanelPluginStartInfo* startInfo, QWidget* parent) :
    RazorPanelPlugin(startInfo, parent),
    mButtonStyle(Qt::ToolButtonTextBesideIcon),
    mShowOnlyCurrentDesktopTasks(false)
{
    setObjectName("TaskBar");

    mLayout = qobject_cast<QBoxLayout*>(layout());
    if (!mLayout)
        return;

    QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    sp.setHorizontalStretch(1);
    sp.setVerticalStretch(1);
    setSizePolicy(sp);
    mLayout->addStretch();

    mRootWindow = GI_DESKTOP_WINDOW_ID;

    settigsChanged();
}


/************************************************

 ************************************************/
RazorTaskBar::~RazorTaskBar()
{
}


/************************************************

 ************************************************/
RazorTaskButton* RazorTaskBar::buttonByWindow(gi_window_id_t window) const
{
    if (mButtonsHash.contains(window))
        return mButtonsHash.value(window);
    return 0;
}


/************************************************

 ************************************************/
void RazorTaskBar::refreshTaskList()
{
    XfitMan xf = xfitMan();
    QList<gi_window_id_t> tmp = xf.getClientList();
	//printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);

    //qDebug() << "** Fill ********************************";
    //foreach (gi_window_id_t wnd, tmp)
    //    if (xf->acceptWindow(wnd)) qDebug() << XfitMan::debugWindow(wnd);
    //qDebug() << "****************************************";


    QMutableHashIterator<gi_window_id_t, RazorTaskButton*> i(mButtonsHash);
    while (i.hasNext())
    {
        i.next();
        int n = tmp.removeAll(i.key());

        if (!n)
        {
            delete i.value();
            i.remove();
        }
    }

    foreach (gi_window_id_t wnd, tmp)
    {
#if 1 //FIXME
        if (xf.acceptWindow(wnd))
        {
            RazorTaskButton* btn = new RazorTaskButton(wnd, this);
            btn->setToolButtonStyle(mButtonStyle);
            if (buttonMaxWidth == -1)
            {
                btn->setMaximumWidth(btn->height());
            }
            else
            {
                btn->setMaximumWidth(buttonMaxWidth);
            }
            mButtonsHash.insert(wnd, btn);
            // -1 is here due the last stretchable item
            mLayout->insertWidget(layout()->count()-1, btn);
            // now I want to set higher stretchable priority for buttons
            // to suppress stretchItem (last item) default value which
            // will remove that anoying aggresive space at the end -- petr
            mLayout->setStretch(layout()->count()-2, 1);
        }
#endif
    }

    refreshButtonVisibility();

    activeWindowChanged();

}

/************************************************

 ************************************************/
void RazorTaskBar::refreshButtonVisibility()
{
    int curretDesktop = xfitMan().getActiveDesktop();
    QHashIterator<gi_window_id_t, RazorTaskButton*> i(mButtonsHash);
    while (i.hasNext())
    {
        i.next();
        i.value()->setHidden(mShowOnlyCurrentDesktopTasks &&
                             i.value()->desktopNum() != curretDesktop
                            );
    }
}


/************************************************

 ************************************************/
void RazorTaskBar::activeWindowChanged()
{
    gi_window_id_t window = xfitMan().getActiveAppWindow();

    RazorTaskButton* btn = buttonByWindow(window);

    if (btn)
        btn->setChecked(true);
    else
        RazorTaskButton::unCheckAll();
}


/************************************************

 ************************************************/
void RazorTaskBar::pfEventFilter(gi_msg_t* event)
{
	//return;

    switch (event->type)
    {
        case GI_MSG_PROPERTYNOTIFY:
            handlePropertyNotify(event);
            break;

#if 0
        case MotionNotify:
            break;

        default:
        {
            qDebug() << "** gi_msg_t ************************";
            qDebug() << "Type:   " << xEventTypeToStr(event);
        }
#endif

    }
}


/************************************************

 ************************************************/
void RazorTaskBar::handlePropertyNotify(gi_msg_t* event)
{

    if (event->ev_window == mRootWindow)
    {
 	//printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);

	   // Windows list changed ...............................
        if (gi_message_property_notify_atom(event) == GA_NET_CLIENT_LIST)
			//XfitMan::atom("_NET_CLIENT_LIST"))
        {
			printf("GA_NET_CLIENT_LIST got ..\n");
            refreshTaskList();
            return;
        }

#if 0
        // Activate window ....................................
        if (gi_message_property_notify_atom(event) == XfitMan::atom("_NET_ACTIVE_WINDOW"))
        {
	printf("%s: dpp got line %d\n",__FUNCTION__,__LINE__);
            activeWindowChanged();
            return;
        }
#endif
#if 0
        // Desktop switch .....................................
        if (gi_message_property_notify_atom(event) == XfitMan::atom("_NET_CURRENT_DESKTOP"))
        {
            if (mShowOnlyCurrentDesktopTasks)
                refreshTaskList();
            return;
        }
#endif
    }
    else
    {
        RazorTaskButton* btn = buttonByWindow(event->ev_window);
        if (btn){
            btn->handlePropertyNotify(event);
		}
    }
//    char* aname = gi_intern_atom(QX11Info::display(), 
//	gi_message_property_notify_atom(event));
//    qDebug() << "** XPropertyEvent ********************";
//    qDebug() << "  atom:       0x" << hex << gi_message_property_notify_atom(event)
//            << " (" << (aname ? aname : "Unknown") << ')';
//    qDebug() << "  window:    " << XfitMan::debugWindow(event->window);
//    qDebug() << "  display:   " << event->display;
//    qDebug() << "  send_event:" << event->send_event;
//    qDebug() << "  serial:    " << event->serial;
//    qDebug() << "  state:     " << event->state;
//    qDebug() << "  time:      " << event->time;
//    qDebug();

}


/************************************************

 ************************************************/
void RazorTaskBar::setButtonStyle(Qt::ToolButtonStyle buttonStyle)
{
    mButtonStyle = buttonStyle;

    QHashIterator<gi_window_id_t, RazorTaskButton*> i(mButtonsHash);
    while (i.hasNext())
    {
        i.next();
        i.value()->setToolButtonStyle(mButtonStyle);
    }
}

void RazorTaskBar::setButtonMaxWidth(int maxWidth)
{
   QHash<gi_window_id_t, RazorTaskButton*>::const_iterator i = mButtonsHash.constBegin();
   while (i != mButtonsHash.constEnd())
   {
       if (maxWidth == -1)
       {
           i.value()->setMaximumWidth(i.value()->height());
       }
       else
       {
           i.value()->setMaximumWidth(maxWidth);
       }
       ++i;
   }
}

/************************************************

 ************************************************/
void RazorTaskBar::settigsChanged()
{
    buttonMaxWidth = settings().value("maxWidth", 400).toInt();
    QString s = settings().value("buttonStyle").toString().toUpper();

    if (s == "ICON")
    {
        setButtonStyle(Qt::ToolButtonIconOnly);
        buttonMaxWidth = -1;
        setButtonMaxWidth(buttonMaxWidth);
    }
    else if (s == "TEXT")
    {
        setButtonStyle(Qt::ToolButtonTextOnly);
        setButtonMaxWidth(buttonMaxWidth);
    }
    else
    {
        setButtonStyle(Qt::ToolButtonTextBesideIcon);
        setButtonMaxWidth(buttonMaxWidth);
    }

    mShowOnlyCurrentDesktopTasks = settings().value("showOnlyCurrentDesktopTasks", mShowOnlyCurrentDesktopTasks).toBool();
    refreshTaskList();
}

void RazorTaskBar::showConfigureDialog()
{
    RazorTaskbarConfiguration *confWindow = this->findChild<RazorTaskbarConfiguration*>("TaskbarConfigurationWindow");

    if (!confWindow)
    {
        confWindow = new RazorTaskbarConfiguration(settings(), this);
    }

    confWindow->show();
    confWindow->raise();
    confWindow->activateWindow();
}
