/* "$Id: Xutf8.h 8399 2011-02-07 22:22:16Z ianmacarthur $"
 *
 * Author: Jean-Marc Lienher ( http://oksid.ch )
 * Copyright 2000-2010 by O'ksi'D.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems on the following page:
 *
 *     http://www.fltk.org/str.php
 */

#ifndef _Xutf8_h
#define _Xutf8_h

#  ifdef __cplusplus
extern "C" {
#  endif
#include <gi/gi.h>
#include <gi/property.h>
#include <gi/region.h>
#include <gi/user_font.h>

typedef struct {
	int nb_font;
	char **font_name_list;
	int *encodings;
	gi_ufont_t **fonts;
	//Font fid;
	int ascent;
	int descent;
	int *ranges;
} XUtf8FontStruct;

typedef uint32_t KeySym;
XUtf8FontStruct *
XCreateUtf8FontStruct (

	const char      *base_font_name_list);

void
XUtf8DrawString(
        gi_window_id_t        	d,
        XUtf8FontStruct  *font_set,
        gi_gc_ptr_t              	gc,
        int             	x,
        int             	y,
        const char      	*string,
        int             	num_bytes);

void
XUtf8_measure_extents(
        gi_window_id_t        	d,
        XUtf8FontStruct  *font_set,
        gi_gc_ptr_t              	gc,
        int             	*xx,
        int             	*yy,
        int             	*ww,
        int             	*hh,
        const char      	*string,
        int             	num_bytes);

void
XUtf8DrawRtlString(
        gi_window_id_t        	d,
        XUtf8FontStruct  *font_set,
        gi_gc_ptr_t              	gc,
        int             	x,
        int             	y,
        const char      	*string,
        int             	num_bytes);

void
XUtf8DrawImageString(
        gi_window_id_t        d,
        XUtf8FontStruct         *font_set,
        gi_gc_ptr_t              gc,
        int             x,
        int             y,
        const char      *string,
        int             num_bytes);

int
XUtf8TextWidth(
        XUtf8FontStruct  *font_set,
        const char      	*string,
        int             	num_bytes);
int
XUtf8UcsWidth(
	XUtf8FontStruct  *font_set,
	unsigned int            ucs);

int
XGetUtf8FontAndGlyph(
        XUtf8FontStruct  *font_set,
        unsigned int            ucs,
        gi_ufont_t     **fnt,
        unsigned short  *id);

void
XFreeUtf8FontStruct(
        
        XUtf8FontStruct 	*font_set);


int
XConvertUtf8ToUcs(
	const unsigned char 	*buf,
	int 			len,
	unsigned int 		*ucs);

int
XConvertUcsToUtf8(
	unsigned int 		ucs,
	char 			*buf);

int
XUtf8CharByteLen(
	const unsigned char 	*buf,
	int 			len);

int
XCountUtf8Char(
	const unsigned char *buf,
	int len);

int
XFastConvertUtf8ToUcs(
	const unsigned char 	*buf,
	int 			len,
	unsigned int 		*ucs);

long
XKeysymToUcs(
	KeySym 	keysym);

int
XUtf8LookupString(
    gi_msg_t*   event,
    char*               buffer_return,
    int                 bytes_buffer,
    KeySym*             keysym,
    int*             status_return);

unsigned short
XUtf8IsNonSpacing(
	unsigned int ucs);

unsigned short
XUtf8IsRightToLeft(
        unsigned int ucs);


int
XUtf8Tolower(
        int ucs);

int
XUtf8Toupper(
        int ucs);


#  ifdef __cplusplus
}
#  endif

#endif

/*
 *  End of "$Id: Xutf8.h 8399 2011-02-07 22:22:16Z ianmacarthur $".
 */
