#include "qdesktopwidget.h"



#include <stdio.h>
#include <gi/gi.h>

QDesktopWidget::QDesktopWidget()
    : QWidget(0, Qt::Desktop)
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::QDesktopWidget\n");
}

QDesktopWidget::~QDesktopWidget()
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::~QDesktopWidget\n");
}

void
QDesktopWidget::resizeEvent(QResizeEvent*)
{
	fprintf(stderr, "Unimplemented: QDesktopWidget::resizeEvent\n");
}

const QRect QDesktopWidget::availableGeometry(int screen) const
{
	int workarea[4];
	int rv;
	QRect workArea;

	rv = gi_wm_get_workarea(workarea);
	if (!rv)
	{
		workArea = QRect(workarea[0], workarea[1], workarea[2], workarea[3]);
	}
	else{
		workArea = screenGeometry(0);
	}
	
	return workArea;//QRect(0,0,info.scr_width,info.scr_height);
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{	
	gi_screen_info_t info;

	gi_get_screen_info(&info);	
	return QRect(0,0,info.scr_width,info.scr_height);
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
	Q_UNUSED(widget);
	//fprintf(stderr, "Reimplemented: QDesktopWidget::screenNumber(widget) \n");
	return 0;
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
	Q_UNUSED(point);
	//fprintf(stderr, "Reimplemented: QDesktopWidget::screenNumber\n");
	return 0;
}

bool QDesktopWidget::isVirtualDesktop() const
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::isVirtualDesktop\n");
	return true;
}

int QDesktopWidget::primaryScreen() const
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::primaryScreen\n");
	return 0;
}

int QDesktopWidget::numScreens() const
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::numScreens\n");
	return 1;
}

QWidget *QDesktopWidget::screen(int /* screen */)
{
	//fprintf(stderr, "Unimplemented: QDesktopWidget::screen\n");
	// It seems that a Qt::WType_Desktop cannot be moved?
	return this;
}
