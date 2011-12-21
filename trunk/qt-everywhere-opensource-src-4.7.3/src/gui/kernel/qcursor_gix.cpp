#include "qcursor.h"
#include "qcursor_p.h"
#include "qbitmap.h"
#include <qicon.h>
#include <gi/gi.h>

#include <stdio.h>


#include <qimage.h>
#include <private/qapplication_p.h>

extern QCursorData *qt_cursorTable[Qt::LastCursor + 1]; // qcursor.cpp
static int lastcursor_id = GI_CURSOR_USER0;

static int get_last_cursor_id(void)
{
	lastcursor_id ++;
	if (lastcursor_id>=GI_CURSOR_MAX || lastcursor_id < 0)
	{
		lastcursor_id = GI_CURSOR_USER0;
	}	
	return lastcursor_id;
}

QPoint QCursor::pos()
{
    gi_window_id_t child;
    int root_x, root_y, win_x, win_y;
    uint buttons;
   
	if (gi_query_pointer(  GI_DESKTOP_WINDOW_ID, &child, &root_x, &root_y,
					  &win_x, &win_y, &buttons))

		return QPoint(root_x, root_y);
    return QPoint();
}

void QCursor::setPos(int x, int y)
{
    uint buttons;
    gi_window_id_t child;
    int root_x, root_y, win_x, win_y;
    QPoint current, target(x, y);
  
	if (gi_query_pointer( GI_DESKTOP_WINDOW_ID, 
		 &child, &root_x, &root_y,
					  &win_x, &win_y, &buttons)) {
		current = QPoint(root_x, root_y);
	}
   
    if (current == target)
        return;

	gi_move_cursor(x,y);
}

#ifndef QT_NO_CURSOR
int QCursor::handle() const
{
	int i;
	
    if (!QCursorData::initialized)
        QCursorData::initialize();
	
    if (!d->hcurs)
        d->update();
    return d->hcurs;
}

enum { cursor_uparrow_x = 11, cursor_uparrow_y = 1 };
static char const * const cursor_uparrow_xpm[] = {
"24 24 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"                        ",
"           ++           ",
"          +..+          ",
"         +....+         ",
"        +......+        ",
"       +........+       ",
"        +++..+++        ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"          +..+          ",
"           ++           ",
"                        ",
"                        "};

enum { cursor_cross_x = 11, cursor_cross_y = 11 };
static char const * const cursor_cross_xpm[] = {
"24 24 3 1",
"       c None",
".      c #FFFFFF",
"+      c #000000",
"                        ",
"           ..           ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"  ........   ........   ",
" .++++++++   ++++++++.  ",
" .++++++++   ++++++++.  ",
"  ........   ........   ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"          .++.          ",
"           ..           ",
"                        "};

enum { cursor_vsplit_x = 11, cursor_vsplit_y = 11 };
static char const * const cursor_vsplit_xpm[] = {
"24 24 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"                        ",
"           ++           ",
"          +..+          ",
"         +....+         ",
"        +......+        ",
"       +........+       ",
"        +++..+++        ",
"          +..+          ",
"  +++++++++..+++++++++  ",
" +....................+ ",
" +....................+ ",
"  ++++++++++++++++++++  ",
"  ++++++++++++++++++++  ",
" +....................+ ",
" +....................+ ",
"  +++++++++..+++++++++  ",
"          +..+          ",
"        +++..+++        ",
"       +........+       ",
"        +......+        ",
"         +....+         ",
"          +..+          ",
"           ++           ",
"                        "};

enum { cursor_hsplit_x = 11, cursor_hsplit_y = 11 };
static char const * const cursor_hsplit_xpm[] = {
"24 24 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"                        ",
"         ++++++         ",
"        +..++..+        ",
"        +..++..+        ",
"        +..++..+        ",
"        +..++..+        ",
"        +..++..+        ",
"     +  +..++..+  +     ",
"    +.+ +..++..+ +.+    ",
"   +..+ +..++..+ +..+   ",
"  +...+++..++..+++...+  ",
" +.........++.........+ ",
" +.........++.........+ ",
"  +...+++..++..+++...+  ",
"   +..+ +..++..+ +..+   ",
"    +.+ +..++..+ +.+    ",
"     +  +..++..+  +     ",
"        +..++..+        ",
"        +..++..+        ",
"        +..++..+        ",
"        +..++..+        ",
"        +..++..+        ",
"         ++++++         ",
"                        "};

enum { cursor_blank_x = 0, cursor_blank_y = 0 };
static char const * const cursor_blank_xpm[] = {
"1 1 1 1",
"       c None",
" "};

enum { cursor_hand_x = 9, cursor_hand_y = 0 };
static char const * const cursor_hand_xpm[] = {
"24 24 3 1",
"       c None",
".      c #FFFFFF",
"+      c #000000",
"         ++             ",
"        +..+            ",
"        +..+            ",
"        +..+            ",
"        +..+            ",
"        +..+            ",
"        +..+++          ",
"        +..+..+++       ",
"        +..+..+..++     ",
"     ++ +..+..+..+.+    ",
"    +..++..+..+..+.+    ",
"    +...+..........+    ",
"     +.............+    ",
"      +............+    ",
"      +............+    ",
"       +..........+     ",
"       +..........+     ",
"        +........+      ",
"        +........+      ",
"        ++++++++++      ",
"        ++++++++++      ",
"        ++++++++++      ",
"                        ",
"                        "};

enum { cursor_whatsthis_x = 0, cursor_whatsthis_y = 0 };
static char const * const cursor_whatsthis_xpm[] = {
"24 24 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
".                       ",
"..          ++++++      ",
".+.        +......+     ",
".++.      +........+    ",
".+++.    +....++....+   ",
".++++.   +...+  +...+   ",
".+++++.  +...+  +...+   ",
".++++++.  +.+  +....+   ",
".+++++++.  +  +....+    ",
".++++++++.   +....+     ",
".+++++.....  +...+      ",
".++.++.      +...+      ",
".+. .++.     +...+      ",
"..  .++.      +++       ",
".    .++.    +...+      ",
"     .++.    +...+      ",
"      .++.   +...+      ",
"      .++.    +++       ",
"       ..               ",
"                        ",
"                        ",
"                        ",
"                        ",
"                        "};

enum { cursor_openhand_x = 7, cursor_openhand_y = 7 };
static char const * const cursor_openhand_xpm[] = {
"16 16 3 1",
"       g None",
".      g #000000",
"+      g #FFFFFF",
"       ..       ",
"   .. .++...    ",
"  .++..++.++.   ",
"  .++..++.++. . ",
"   .++.++.++..+.",
"   .++.++.++.++.",
" ..+.+++++++.++.",
".++..++++++++++.",
".+++.+++++++++. ",
" .++++++++++++. ",
"  .+++++++++++. ",
"  .++++++++++.  ",
"   .+++++++++.  ",
"    .+++++++.   ",
"     .++++++.   ",
"                "};

enum { cursor_closedhand_x = 7, cursor_closedhand_y = 7 };
static char const * const cursor_closedhand_xpm[] = {
"16 16 3 1",
"       g None",
".      g #000000",
"+      g #FFFFFF",
"                ",
"                ",
"                ",
"    .. .. ..    ",
"   .++.++.++..  ",
"   .++++++++.+. ",
"    .+++++++++. ",
"   ..+++++++++. ",
"  .+++++++++++. ",
"  .++++++++++.  ",
"   .+++++++++.  ",
"    .+++++++.   ",
"     .++++++.   ",
"     .++++++.   ",
"                ",
"                "};

#endif


QCursorData::QCursorData(Qt::CursorShape s)
    : cshape(s), bm(0), bmm(0), hcurs(0),hx(0), hy(0)
{
    ref = 1;
}

QCursorData::~QCursorData()
{ 
	if (hcurs){
        //gi_destroy_cursor(hcurs);
		hcurs=0;
	}
}



void QCursorData::update()
{
	gi_image_t *img = NULL;

    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (hcurs)
        return;

    if (cshape == Qt::BitmapCursor && !pixmap.isNull()) {
		
		int err;
		QPixmap pm = pixmap;

		img = QPixmap::to_gix_icon(QIcon(pm), true, hx, hy);
		if (img)
		{
			hcurs = get_last_cursor_id(); //自动分配
			err = gi_setup_cursor(hcurs,0,0,img);
			if (err)
			{
				hcurs = 0;
			}
		}
        if (hcurs)
            return;
    }

    char const * const * xpm = 0;
    int xpm_hx = 0;
    int xpm_hy = 0;
    int sh = 0;

    switch (cshape) { // map to windows cursor
    case Qt::ArrowCursor:
        sh = GI_CURSOR_ARROW;
        break;
    case Qt::UpArrowCursor:
        sh = GI_CURSOR_UPARROW;
        break;
    case Qt::CrossCursor:
        sh = GI_CURSOR_CROSS;
        break;
    case Qt::WaitCursor:
        sh = GI_CURSOR_WAIT;
        break;
    case Qt::IBeamCursor:
        sh = GI_CURSOR_IBEAM;
        break;
    case Qt::SizeVerCursor:
        sh = GI_CURSOR_SIZENS;
        break;
    case Qt::SizeHorCursor:
        sh = GI_CURSOR_SIZEWE;
        break;
    case Qt::SizeBDiagCursor:
        sh = GI_CURSOR_SIZENESW;
        break;
    case Qt::SizeFDiagCursor:
        sh = GI_CURSOR_SIZENWSE;
        break;
    case Qt::SizeAllCursor:
        sh = GI_CURSOR_SIZEALL;
        break;
    case Qt::ForbiddenCursor:
        sh = GI_CURSOR_NO;
        break;
    case Qt::WhatsThisCursor:
        sh = GI_CURSOR_HELP;
        break;
    case Qt::BusyCursor:
        sh = GI_CURSOR_APPSTARTING;
        break;
    case Qt::PointingHandCursor:
        sh = GI_CURSOR_HAND;
        break;
     case Qt::BlankCursor:
        xpm = cursor_blank_xpm;
        xpm_hx = cursor_blank_x;
        xpm_hy = cursor_blank_y;
        break;
    case Qt::SplitVCursor:
        xpm = cursor_vsplit_xpm;
        xpm_hx = cursor_vsplit_x;
        xpm_hy = cursor_vsplit_y;
        break;
    case Qt::SplitHCursor:
        xpm = cursor_hsplit_xpm;
        xpm_hx = cursor_hsplit_x;
        xpm_hy = cursor_hsplit_y;
        break;   

    case Qt::OpenHandCursor:
        xpm = cursor_openhand_xpm;
        xpm_hx = cursor_openhand_x;
        xpm_hy = cursor_openhand_y;
        break;
    case Qt::ClosedHandCursor:
        xpm = cursor_closedhand_xpm;
        xpm_hx = cursor_closedhand_x;
        xpm_hy = cursor_closedhand_y;
        break;
    /*case Qt::DragCopyCursor:
    case Qt::DragMoveCursor:
    case Qt::DragLinkCursor: {
        QPixmap pixmap = QApplicationPrivate::instance()->getPixmapCursor(cshape);
        //hcurs = create32BitCursor(pixmap, hx, hy);
    }*/
    default:
        qWarning("QCursor::update: Invalid cursor shape %d", cshape);
        return;
    }


    if (xpm) {
#ifndef QT_NO_IMAGEFORMAT_XPM
        QPixmap pm(xpm);
		int err;
        img = QPixmap::to_gix_icon(QIcon(pm), true, xpm_hx, xpm_hy);
		if (img)
		{
			hcurs = get_last_cursor_id(); //自动分配
			err = gi_setup_cursor(hcurs,0,0,img);
			if(err) 
				hcurs = 0;
			gi_destroy_image(img);
		}

		if(hcurs)
          return;
#else
		printf("QT_NO_IMAGEFORMAT_XPM defined, bitmap cursor not loaded\n");
        sh = GI_CURSOR_ARROW;
#endif
    }

	hcurs = sh;

}

QCursorData *QCursorData::setBitmap(const QBitmap &bitmap, const QBitmap &mask, int hotX, int hotY)
{
    if (!QCursorData::initialized)
        QCursorData::initialize();
    if (bitmap.depth() != 1 || mask.depth() != 1 || bitmap.size() != mask.size()) {
        qWarning("QCursor: Cannot create bitmap cursor; invalid bitmap(s)");
        QCursorData *c = qt_cursorTable[0];
        c->ref.ref();
        return c;
    }
    QCursorData *d = new QCursorData;
    d->bm  = new QBitmap(bitmap);
    d->bmm = new QBitmap(mask);
    d->hcurs = 0;
    d->cshape = Qt::BitmapCursor;
    d->hx = hotX >= 0 ? hotX : bitmap.width()/2;
    d->hy = hotY >= 0 ? hotY : bitmap.height()/2;
    return d;
}
