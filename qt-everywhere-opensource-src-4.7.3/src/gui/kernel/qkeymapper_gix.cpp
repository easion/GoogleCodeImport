/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui of the Qt Toolkit.
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
** In addition, as a special exception, Nokia gives you certain
** additional rights.  These rights are described in the Nokia Qt LGPL
** Exception version 1.1, included in the file LGPL_EXCEPTION.txt in this
** package.
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

#include "qkeymapper_p.h"

//#include <qt_windows.h>
#include <qdebug.h>
#include <private/qevent_p.h>
#include <private/qlocale_p.h>
#include <private/qapplication_p.h>
#include <qwidget.h>
#include <qapplication.h>
#include <ctype.h>
#include <stdio.h>

// QETWidget class is only for accessing the sendSpontaneousEvent function in QApplication
class QETWidget : public QWidget {
public:
    static bool sendSpontaneousEvent(QObject *r, QEvent *e)
    { return QApplication::sendSpontaneousEvent(r, e); }
};

static uint32_t scan_codes[] = {
        Qt::Key_Escape,		G_KEY_ESCAPE,
        Qt::Key_F1,			G_KEY_F1,
        Qt::Key_F2,			G_KEY_F2,
        Qt::Key_F3,			G_KEY_F3,
        Qt::Key_F4,			G_KEY_F4,
        Qt::Key_F5,			G_KEY_F5,
        Qt::Key_F6,			G_KEY_F6,
        Qt::Key_F7,			G_KEY_F7,
        Qt::Key_F8,			G_KEY_F8,
        Qt::Key_F9,			G_KEY_F9,
        Qt::Key_F10,		G_KEY_F10,
        Qt::Key_F11,		G_KEY_F11,
        Qt::Key_F12,		G_KEY_F12,
        Qt::Key_Print,		G_KEY_PRINT,
//      Qt::Key_ScrollLock = 0x0F,  //modificator
        Qt::Key_Pause,		G_KEY_PAUSE,

		//Qt::Key_AsciiTilde, G_KEY_0,
        Qt::Key_1,			G_KEY_1,
        Qt::Key_2,			G_KEY_2,
        Qt::Key_3,			G_KEY_3,
        Qt::Key_4,			G_KEY_4,
        Qt::Key_5,			G_KEY_5,
        Qt::Key_6,			G_KEY_6,
        Qt::Key_7,			G_KEY_7,
        Qt::Key_8,			G_KEY_8,
        Qt::Key_9,			G_KEY_9,
        Qt::Key_0,			G_KEY_0,
        Qt::Key_Minus,		G_KEY_MINUS,
        Qt::Key_Plus,		G_KEY_PLUS,
        Qt::Key_Backspace,	G_KEY_BACKSPACE,
        Qt::Key_Insert,		G_KEY_INSERT,
        Qt::Key_Home,		G_KEY_HOME,
        Qt::Key_PageUp,		G_KEY_PAGEUP,
//		Qt::Key_NumLock,	0x22,   //modificator    
		Qt::Key_Slash,		G_KEY_SLASH,
		Qt::Key_Asterisk,	G_KEY_ASTERISK,
		Qt::Key_Minus,		G_KEY_MINUS,		
		                
        Qt::Key_Tab,		G_KEY_TAB,        
        Qt::Key_Q,			G_KEY_q,
        Qt::Key_W,			G_KEY_w,
        Qt::Key_E,			G_KEY_e,
        Qt::Key_R,			G_KEY_r,
        Qt::Key_T,			G_KEY_t,
        Qt::Key_Y,			G_KEY_y,
        Qt::Key_U,			G_KEY_u,
        Qt::Key_I,			G_KEY_i,
        Qt::Key_O,			G_KEY_o,
        Qt::Key_P,			G_KEY_p,      
        Qt::Key_BracketLeft,G_KEY_LEFTBRACKET,
        Qt::Key_BracketRight,G_KEY_RIGHTBRACKET,
		Qt::Key_Backslash,	G_KEY_BACKSLASH,
        Qt::Key_Delete,		G_KEY_DELETE,
        Qt::Key_End,		G_KEY_END,
        Qt::Key_PageDown,	G_KEY_PAGEDOWN, 
		Qt::Key_Home,		G_KEY_HOME, //numpad		      
        Qt::Key_Up,			G_KEY_UP, //numpad
        Qt::Key_PageUp,		G_KEY_PAGEUP, //numpad
        Qt::Key_Plus,		G_KEY_PLUS, //numpad

//		Qt::Key_CapsLock,	0x3B, //modificator
        Qt::Key_A,			G_KEY_a,
        Qt::Key_S,			G_KEY_s,
        Qt::Key_D,			G_KEY_d,
        Qt::Key_F,			G_KEY_f,
        Qt::Key_G,			G_KEY_g,
        Qt::Key_H,			G_KEY_h,
        Qt::Key_J,			G_KEY_j,
        Qt::Key_K,			G_KEY_k,
        Qt::Key_L,			G_KEY_l,
        Qt::Key_Colon,		G_KEY_COLON,
        Qt::Key_QuoteDbl,	G_KEY_QUOTEDBL,
        Qt::Key_Return,		G_KEY_RETURN,              
        Qt::Key_Left,		G_KEY_LEFT, //numpad
		//Qt::Key_5,			0x49, //numpad ???
        Qt::Key_Right,		G_KEY_RIGHT, //numpad

        Qt::Key_Z,			G_KEY_z,
        Qt::Key_X,			G_KEY_x,
        Qt::Key_C,			G_KEY_c,
        Qt::Key_V,			G_KEY_v,
        Qt::Key_B,			G_KEY_b,
        Qt::Key_N,			G_KEY_n,
        Qt::Key_M,			G_KEY_m,
        Qt::Key_Less,		G_KEY_LESS,
        Qt::Key_Greater,	G_KEY_GREATER,
        Qt::Key_Question,	G_KEY_QUESTION,        
		Qt::Key_Enter,		G_KEY_ENTER,   //numpad

		Qt::Key_Space,		G_KEY_SPACE,
		Qt::Key_Down,		G_KEY_DOWN,   //cursor
		Qt::Key_Right,		G_KEY_RIGHT,   //cursor
		0,					0x00
	};


QKeyMapperPrivate::QKeyMapperPrivate()
{
 

	
	//memcpy(ScanCodes,scan_codes,sizeof(scan_codes));
	//memcpy(ScanCodes_Numlock,scan_codes_numlock,sizeof(scan_codes_numlock));
}

QKeyMapperPrivate::~QKeyMapperPrivate()
{ }

void QKeyMapperPrivate::clearMappings()
{ }

QList<int> QKeyMapperPrivate::possibleKeys(QKeyEvent *)
{
	return QList<int>();
}

uint32_t QKeyMapperPrivate::translateKeyCode(int32_t modifiers,int32_t key)
{
	uint32_t code = 0;
	int i = 0;
#if 1
	code = key;
	
    while (scan_codes[i]) {
      		if ( key == scan_codes[i+1]) {
            code = scan_codes[i];
      		    break;
        	}
       	i += 2;
  	}	
#endif

	return code;   
}




bool QKeyMapperPrivate::translateKeyEventInternal(QWidget *keyWidget,
                                                  const gi_msg_t *event,
                                                  int &keysym,
                                                  int& count,
                                                  QString& text,
                                                  Qt::KeyboardModifiers &modifiers,
                                                  int& code,
                                                  QEvent::Type &type,
                                                  bool statefulTranslation)
{
    

    return true;
}

bool QKeyMapperPrivate::translateKeyEvent(QWidget *widget, const gi_msg_t *event, bool grab)
{
    Q_Q(QKeyMapper);
    Q_UNUSED(q); // Strange, but the compiler complains on q not being referenced, even if it is..

	int keycode = 0;
	int nModifiers = 0;
	int qtcode = 0;
	int state = 0;
	bool k0 = false;
    bool k1 = false;

	keycode = event->params[3];
	nModifiers = event->body.message[3];

	qtcode = translateKeyCode(nModifiers, keycode);
    state |= (nModifiers & G_MODIFIERS_SHIFT ? Qt::ShiftModifier : 0);
    state |= (nModifiers & G_MODIFIERS_CTRL ? Qt::ControlModifier : 0);
    state |= (nModifiers & G_MODIFIERS_ALT ? Qt::AltModifier : 0);
    state |= (nModifiers & G_MODIFIERS_META ? Qt::MetaModifier : 0);

	if (event->type == GI_MSG_KEY_DOWN)
	{
		QString s;
		QChar ch = QChar((uint)keycode);
		if (!ch.isNull())
			s += ch;

		if (event->attribute_1) {

			k0 = q->sendKeyEvent(widget, grab, QEvent::KeyPress, 0, Qt::KeyboardModifier(state),
				s, false, 0, keycode, qtcode, nModifiers);
			k1 = q->sendKeyEvent(widget, grab, QEvent::KeyRelease, 0, Qt::KeyboardModifier(state),
				s, false, 0, keycode, qtcode, nModifiers);
			//usleep(450);
		}
		else{
			k0 = q->sendKeyEvent(widget, grab, QEvent::KeyPress, 
				qtcode, Qt::KeyboardModifier(state), s, false, 1, keycode, qtcode, nModifiers);
		}
	}
	else{	
	 // Input method characters not found by our look-ahead

        QString s;
        QChar ch = QChar((ushort)keycode);
        if (!ch.isNull())
            s += ch;
        
        k1 = q->sendKeyEvent(widget, grab, QEvent::KeyRelease, qtcode ,
			Qt::KeyboardModifier(state), s, false, 1, keycode, qtcode, nModifiers);
    }


	//return false;
	return k0 || k1;
}

static inline bool isModifierKey(int code)
{
    return (code >= Qt::Key_Shift) && (code <= Qt::Key_ScrollLock);
}

// QKeyMapper (Windows) implementation -------------------------------------------------[ start ]---

bool QKeyMapper::sendKeyEvent(QWidget *widget, bool grab,
                              QEvent::Type type, int code, Qt::KeyboardModifiers modifiers,
                              const QString &text, bool autorepeat, int count,
                              quint32 nativeScanCode, quint32 nativeVirtualKey, 
							  quint32 nativeModifiers,
                              bool *)
{

    Q_UNUSED(count);
#if defined QT3_SUPPORT && !defined(QT_NO_SHORTCUT)
    if (type == QEvent::KeyPress
        && !grab
        && QApplicationPrivate::instance()->use_compat()) {
        // send accel events if the keyboard is not grabbed
        QKeyEventEx a(type, code, modifiers,
                      text, autorepeat, qMax(1, int(text.length())),
                      nativeScanCode, nativeVirtualKey, nativeModifiers);
        if (QApplicationPrivate::instance()->qt_tryAccelEvent(widget, &a))
            return true;
    }
#else
    Q_UNUSED(grab);
#endif
    if (!widget->isEnabled())
        return false;

    QKeyEventEx e(type, code, modifiers,
                  text, autorepeat, qMax(1, int(text.length())),
                  nativeScanCode, nativeVirtualKey, nativeModifiers);
    QETWidget::sendSpontaneousEvent(widget, &e);

    if (!isModifierKey(code)
        && modifiers == Qt::AltModifier
        && ((code >= Qt::Key_A && code <= Qt::Key_Z) || (code >= Qt::Key_0 && code <= Qt::Key_9))
        && type == QEvent::KeyPress
        && !e.isAccepted())
        QApplication::beep();           // Emulate windows behavior

    return e.isAccepted();
}

