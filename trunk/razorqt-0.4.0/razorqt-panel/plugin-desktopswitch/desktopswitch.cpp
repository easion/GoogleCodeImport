/* BEGIN_COMMON_COPYRIGHT_HEADER
 *
 * Razor - a lightweight, Qt based, desktop toolset
 * http://razor-qt.org
 *
 * Copyright: 2011 Razor team
 * Authors:
 *   Petr Vanek <petr@scribus.info>
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


#include <QButtonGroup>
#include <QToolButton>
#include <QWheelEvent>
#include <QtDebug>
#include <QSignalMapper>
#include <razorqt/xfitman.h>
#include <razorqxt/qxtglobalshortcut.h>

#include "desktopswitch.h"
#include "desktopswitchbutton.h"


EXPORT_RAZOR_PANEL_PLUGIN_CPP(DesktopSwitch)


DesktopSwitch::DesktopSwitch(const RazorPanelPluginStartInfo* startInfo, QWidget* parent)
    : RazorPanelPlugin(startInfo, parent),
      m_pSignalMapper(new QSignalMapper(this)),
      m_desktopCount(1)
{
    setObjectName("DesktopSwitch");
    m_buttons = new QButtonGroup(this);
    
    connect ( m_pSignalMapper, SIGNAL(mapped(int)), this, SLOT(setDesktop(int)));

    setup();
}

void DesktopSwitch::setup()
{
    // clear current state
    foreach (QAbstractButton * b, m_buttons->buttons())
    {
        // TODO/FIXME: maybe it has to be removed from layout too?
        m_pSignalMapper->removeMappings(b);
        m_buttons->removeButton(b);
        delete b;
    }

    // create new desktop layout
    int firstKey = Qt::Key_F1;
    int maxKey = Qt::Key_F35; // max defined in Qt

    for (int i = 0; i < m_desktopCount; ++i)
    {
        QKeySequence sequence;
        if (firstKey < maxKey)
        {
            sequence = QKeySequence(Qt::CTRL + firstKey++);
        }

        DesktopSwitchButton * m = new DesktopSwitchButton(this, i, sequence, xfitMan().getDesktopName(i, tr("Desktop %1").arg(i+1)));
        m_pSignalMapper->setMapping(m, i);
        connect(m, SIGNAL(activated()), m_pSignalMapper, SLOT(map())) ; 
        addWidget(m);
        m_buttons->addButton(m, i);
    }

    int activeDesk = qMax(xfitMan().getActiveDesktop(), 0);
    m_buttons->button(activeDesk)->setChecked(true);

    connect(m_buttons, SIGNAL(buttonClicked(int)),
            this, SLOT(setDesktop(int)));
}

DesktopSwitch::~DesktopSwitch()
{
}

void DesktopSwitch::pfEventFilter(gi_msg_t* _event)
{
	return; //FIXME

    if (_event->type == GI_MSG_PROPERTYNOTIFY)
    {
        int count = qMax(xfitMan().getNumDesktop(), 1);
        if (m_desktopCount != count)
        {
            qDebug() << "Desktop count changed from" << m_desktopCount << "to" << count;
            m_desktopCount = count;
            m_desktopNames = xfitMan().getDesktopNames();
            setup();
        }
        
        if (m_desktopNames != xfitMan().getDesktopNames())
        {
            m_desktopNames = xfitMan().getDesktopNames();
            setup();
        }

        int activeDesk = qMax(xfitMan().getActiveDesktop(), 0);
        m_buttons->button(activeDesk)->setChecked(true);
    }
}
void DesktopSwitch::setDesktop(int desktop)
{
    xfitMan().setActiveDesktop(desktop);
}

void DesktopSwitch::wheelEvent(QWheelEvent* e)
{
    int max = xfitMan().getNumDesktop() - 1;
    int delta = e->delta() > 0 ? 1 : -1;
    int current = xfitMan().getActiveDesktop() + delta;
    
    if (current > max)
        current = 0;
    else if (current < 0)
        current = max;

    xfitMan().setActiveDesktop(current);
}
