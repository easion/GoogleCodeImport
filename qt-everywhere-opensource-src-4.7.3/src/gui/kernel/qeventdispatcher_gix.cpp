#include <private/qabstracteventdispatcher_p.h>
#include "qeventdispatcher_gix_p.h"
#include <private/qcoreapplication_p.h>
#include <private/qthread_p.h>
#include <private/qmutexpool_p.h>
#include "qapplication.h"

#include <gi/gi.h>
#include <stdio.h>

// Debugging part



/*****************************************************************************
  Safe configuration (move,resize,setGeometry) mechanism to avoid
  recursion when processing messages.
 *****************************************************************************/

struct QWinConfigRequest {
    WId         id;                                        // widget to be configured
    int         req;                                        // 0=move, 1=resize, 2=setGeo
    int         x, y, w, h;                                // request parameters
};

static QList<QWinConfigRequest*> *configRequests = 0;

void qWinRequestConfig(WId id, int req, int x, int y, int w, int h)
{
    if (!configRequests)                        // create queue
        configRequests = new QList<QWinConfigRequest*>;
    QWinConfigRequest *r = new QWinConfigRequest;
    r->id = id;                                        // create new request
    r->req = req;
    r->x = x;
    r->y = y;
    r->w = w;
    r->h = h;
    configRequests->append(r);                // store request in queue
}

static void qWinProcessConfigRequests()                // perform requests in queue
{
    if (!configRequests)
        return;
    QWinConfigRequest *r;
    for (;;) {
        if (configRequests->isEmpty())
            break;
        r = configRequests->takeLast();
        QWidget *w = QWidget::find(r->id);
        QRect rect(r->x, r->y, r->w, r->h);
        int req = r->req;
        delete r;

        if ( w ) {                              // widget exists
            if (w->testAttribute(Qt::WA_WState_ConfigPending))
                return;                         // biting our tail
            if (req == 0)
                w->move(rect.topLeft());
            else if (req == 1)
                w->resize(rect.size());
            else
                w->setGeometry(rect);
        }
    }
    delete configRequests;
    configRequests = 0;
}



class QEventDispatcherGixPrivate : public QEventDispatcherUNIXPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherGix)
public:
    inline QEventDispatcherGixPrivate()
        : xfd(-1)
    { }
    int xfd;
    QList<gi_msg_t> queuedUserInputEvents;
};

QEventDispatcherGix::QEventDispatcherGix(QObject *parent)
    : QEventDispatcherUNIX(*new QEventDispatcherGixPrivate, parent)
{ }

QEventDispatcherGix::~QEventDispatcherGix()
{ }

bool QEventDispatcherGix::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherGix);
	int rv = 0;

    d->interrupt = false;
    QApplication::sendPostedEvents();
    int nevents = 0;

    do {
        while (!d->interrupt) {
            gi_msg_t event;
            if (!(flags & QEventLoop::ExcludeUserInputEvents)
                && !d->queuedUserInputEvents.isEmpty()) {
                // process a pending user input event
                event = d->queuedUserInputEvents.takeFirst();
            } else if (gi_peek_message(&event) > 0) {
                // process events from the X server
                gi_next_message( &event);

                if (flags & QEventLoop::ExcludeUserInputEvents) {
                    // queue user input events
                    switch (event.type) {
                    case GI_MSG_BUTTON_DOWN:
                    case GI_MSG_BUTTON_UP:
                    case GI_MSG_MOUSE_MOVE:
                    case GI_MSG_KEY_DOWN:
                    case GI_MSG_KEY_UP:
                    case GI_MSG_MOUSE_ENTER:
                    case GI_MSG_MOUSE_EXIT:
                        d->queuedUserInputEvents.append(event);
                        continue;

                    case GI_MSG_CLIENT_MSG:                        
                        d->queuedUserInputEvents.append(event);
                        continue;

                    default:
                        break;
                    }
                }
            } else {
                // no event to process
                break;
            }

            // send through event filter
            if (filterEvent(&event))
                continue;

            nevents++;
			rv = qApp->pfProcessEvent(&event);
            //if (rv == 1)
			{
              //  return true;
			}
        }
    } while (!d->interrupt && gi_get_event_cached(FALSE) > 0);	


 out:
    if (!d->interrupt) {

	 //QEventLoop::GixExcludeTimers 
        const uint exclude_all =
            QEventLoop::ExcludeSocketNotifiers |  QEventLoop::WaitForMoreEvents;
        if (nevents > 0 && ((uint)flags & exclude_all) == exclude_all) {
            QApplication::sendPostedEvents();
            return nevents > 0;
        }
        // return true if we handled events, false otherwise
        return QEventDispatcherUNIX::processEvents(flags) ||  (nevents > 0);
    }

	if (configRequests )                        // any pending configs?
        qWinProcessConfigRequests();

    return nevents > 0;
}

bool QEventDispatcherGix::hasPendingEvents()
{
	bool res = false;
    extern uint qGlobalPostedEventsCount(); // from qapplication.cpp
	res = (qGlobalPostedEventsCount() || gi_get_event_cached(FALSE) > 0);	
    return res;
}

void QEventDispatcherGix::flush()
{
}

void QEventDispatcherGix::startingUp()
{
    Q_D(QEventDispatcherGix);
    d->xfd = gi_get_connection_fd();
}

void QEventDispatcherGix::closingDown()
{
    Q_D(QEventDispatcherGix);
    d->xfd = -1;
}

int QEventDispatcherGix::select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                                timeval *timeout)
{
	int rv;
    Q_D(QEventDispatcherGix);
    if (d->xfd > 0) {
        nfds = qMax(nfds - 1, d->xfd) + 1;
        FD_SET(d->xfd, readfds);
    }

    rv = QEventDispatcherUNIX::select(nfds, readfds, writefds, exceptfds, timeout);
	return rv;
}

