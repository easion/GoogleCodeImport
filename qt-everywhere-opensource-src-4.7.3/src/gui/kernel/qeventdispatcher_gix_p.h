#ifndef FEASOIFESWR
#define FEASOIFESWR
#include <private/qabstracteventdispatcher_p.h>
#include <private/qeventdispatcher_unix_p.h>

class QEventDispatcherGixPrivate;

class QEventDispatcherGix : public QEventDispatcherUNIX
{
	//Q_OBJECT
	Q_DECLARE_PRIVATE(QEventDispatcherGix)

public:
	explicit QEventDispatcherGix(QObject *parent = 0);
	~QEventDispatcherGix();

	bool processEvents(QEventLoop::ProcessEventsFlags flags);
	bool hasPendingEvents();

	void flush();

	void startingUp();
	void closingDown();

protected:
    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
               timeval *timeout);
};

#endif
