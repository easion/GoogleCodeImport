/* GDK - The GIMP Drawing Kit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif /* HAVE_SYS_SELECT_H_ */

#include "gdk.h"
#include "gdkprivate.h"
#include "gdkinput.h"
#include "gdkx.h"
#include "gdki18n.h"
#include "gdkkeysyms.h"

#define X_GETTIMEOFDAY(tv)  gettimeofday (tv, NULL)



typedef struct _GdkPredicate  GdkPredicate;
typedef struct _GdkErrorTrap  GdkErrorTrap;

struct _GdkPredicate
{
  GdkEventFunc func;
  gpointer data;
};

struct _GdkErrorTrap
{
  gint error_warnings;
  gint error_code;
};

/* 
 * Private function declarations
 */

#ifndef HAVE_XCONVERTCASE
static void	 gdkx_XConvertCase	();
#define XConvertCase gdkx_XConvertCase
#endif

static void	    gdk_exit_func		 (void);
static int	    gdk_x_error			 ();
static int	    gdk_x_io_error		 ();

GdkFilterReturn gdk_wm_protocols_filter (GdkEvent  *event,
					 gpointer   data);

/* Private variable declarations
 */
static int gdk_initialized = 0;			    /* 1 if the library is initialized,
						     * 0 otherwise.
						     */

static gint autorepeat;

static GSList *gdk_error_traps = NULL;               /* List of error traps */
static GSList *gdk_error_trap_free_list = NULL;      /* Free list */

static struct timeval start;                        /* The time at which the library was
                                                     *  last initialized.
                                                     */
static guint32 timer_val;			 /* The timeout length as specified by
                                                  *  the user in milliseconds.
                                                  */
static struct timeval timer;                        /* Timeout interval to use in the call
                                                     *  to "select". This is used in
                                                     *  conjunction with "timerp" to create
                                                     *  a maximum time to wait for an event
                                                     *  to arrive.
                                                     */
static struct timeval *timerp;                      /* The actual timer passed to "select"
                                                     *  This may be NULL, in which case
                                                     *  "select" will block until an event
                                                     *  arrives.
                                                     */

extern gi_screen_info_t	gdk_screen_info;
#define PRINTFILE(s) g_message("gdk.c:%s()",s);

#ifdef G_ENABLE_DEBUG
static const GDebugKey gdk_debug_keys[] = {
  {"events",	    GDK_DEBUG_EVENTS},
  {"misc",	    GDK_DEBUG_MISC},
  {"dnd",	    GDK_DEBUG_DND},
  {"color-context", GDK_DEBUG_COLOR_CONTEXT},
  {"xim",	    GDK_DEBUG_XIM}
};

static const int gdk_ndebug_keys = sizeof(gdk_debug_keys)/sizeof(GDebugKey);

#endif /* G_ENABLE_DEBUG */



void
gdk_keyval_convert_case (guint symbol,
                         guint *lower,
                         guint *upper)
{
  guint xlower = symbol;
  guint xupper = symbol;

  switch (symbol >> 8)
    {
#if     defined (GDK_A) && defined (GDK_Ooblique)
    case 0: /* Latin 1 */
      if ((symbol >= GDK_A) && (symbol <= GDK_Z))
        xlower += (GDK_a - GDK_A);
      else if ((symbol >= GDK_a) && (symbol <= GDK_z))
        xupper -= (GDK_a - GDK_A);
      else if ((symbol >= GDK_Agrave) && (symbol <= GDK_Odiaeresis))
        xlower += (GDK_agrave - GDK_Agrave);
      else if ((symbol >= GDK_agrave) && (symbol <= GDK_odiaeresis))
        xupper -= (GDK_agrave - GDK_Agrave);
      else if ((symbol >= GDK_Ooblique) && (symbol <= GDK_Thorn))
        xlower += (GDK_oslash - GDK_Ooblique);
      else if ((symbol >= GDK_oslash) && (symbol <= GDK_thorn))
        xupper -= (GDK_oslash - GDK_Ooblique);
      break;
#endif  /* LATIN1 */
#if     defined (GDK_Aogonek) && defined (GDK_tcedilla)
    case 1: /* Latin 2 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol == GDK_Aogonek)
        xlower = GDK_aogonek;
      else if (symbol >= GDK_Lstroke && symbol <= GDK_Sacute)
        xlower += (GDK_lstroke - GDK_Lstroke);
      else if (symbol >= GDK_Scaron && symbol <= GDK_Zacute)
        xlower += (GDK_scaron - GDK_Scaron);
      else if (symbol >= GDK_Zcaron && symbol <= GDK_Zabovedot)
        xlower += (GDK_zcaron - GDK_Zcaron);
      else if (symbol == GDK_aogonek)
        xupper = GDK_Aogonek;
      else if (symbol >= GDK_lstroke && symbol <= GDK_sacute)
        xupper -= (GDK_lstroke - GDK_Lstroke);
      else if (symbol >= GDK_scaron && symbol <= GDK_zacute)
        xupper -= (GDK_scaron - GDK_Scaron);
      else if (symbol >= GDK_zcaron && symbol <= GDK_zabovedot)
        xupper -= (GDK_zcaron - GDK_Zcaron);
      else if (symbol >= GDK_Racute && symbol <= GDK_Tcedilla)
        xlower += (GDK_racute - GDK_Racute);
      else if (symbol >= GDK_racute && symbol <= GDK_tcedilla)
        xupper -= (GDK_racute - GDK_Racute);
      break;
#endif  /* LATIN2 */

#if     defined (GDK_Hstroke) && defined (GDK_Cabovedot)
    case 2: /* Latin 3 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= GDK_Hstroke && symbol <= GDK_Hcircumflex)
        xlower += (GDK_hstroke - GDK_Hstroke);
      else if (symbol >= GDK_Gbreve && symbol <= GDK_Jcircumflex)
        xlower += (GDK_gbreve - GDK_Gbreve);
      else if (symbol >= GDK_hstroke && symbol <= GDK_hcircumflex)
        xupper -= (GDK_hstroke - GDK_Hstroke);
      else if (symbol >= GDK_gbreve && symbol <= GDK_jcircumflex)
        xupper -= (GDK_gbreve - GDK_Gbreve);
      else if (symbol >= GDK_Cabovedot && symbol <= GDK_Scircumflex)
        xlower += (GDK_cabovedot - GDK_Cabovedot);
      else if (symbol >= GDK_cabovedot && symbol <= GDK_scircumflex)
        xupper -= (GDK_cabovedot - GDK_Cabovedot);
      break;
#endif  /* LATIN3 */

#if     defined (GDK_Rcedilla) && defined (GDK_Amacron)
    case 3: /* Latin 4 */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= GDK_Rcedilla && symbol <= GDK_Tslash)
        xlower += (GDK_rcedilla - GDK_Rcedilla);
      else if (symbol >= GDK_rcedilla && symbol <= GDK_tslash)
        xupper -= (GDK_rcedilla - GDK_Rcedilla);
      else if (symbol == GDK_ENG)
        xlower = GDK_eng;
      else if (symbol == GDK_eng)
        xupper = GDK_ENG;
      else if (symbol >= GDK_Amacron && symbol <= GDK_Umacron)
        xlower += (GDK_amacron - GDK_Amacron);
      else if (symbol >= GDK_amacron && symbol <= GDK_umacron)
        xupper -= (GDK_amacron - GDK_Amacron);
      break;
#endif  /* LATIN4 */
#if     defined (GDK_Serbian_DJE) && defined (GDK_Cyrillic_yu)
    case 6: /* Cyrillic */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= GDK_Serbian_DJE && symbol <= GDK_Serbian_DZE)
        xlower -= (GDK_Serbian_DJE - GDK_Serbian_dje);
      else if (symbol >= GDK_Serbian_dje && symbol <= GDK_Serbian_dze)
        xupper += (GDK_Serbian_DJE - GDK_Serbian_dje);
      else if (symbol >= GDK_Cyrillic_YU && symbol <= GDK_Cyrillic_HARDSIGN)
        xlower -= (GDK_Cyrillic_YU - GDK_Cyrillic_yu);
      else if (symbol >= GDK_Cyrillic_yu && symbol <= GDK_Cyrillic_hardsign)
        xupper += (GDK_Cyrillic_YU - GDK_Cyrillic_yu);
      break;
#endif  /* CYRILLIC */

#if     defined (GDK_Greek_ALPHAaccent) && defined (GDK_Greek_finalsmallsigma)
    case 7: /* Greek */
      /* Assume the KeySym is a legal value (ignore discontinuities) */
      if (symbol >= GDK_Greek_ALPHAaccent && symbol <= GDK_Greek_OMEGAaccent)
        xlower += (GDK_Greek_alphaaccent - GDK_Greek_ALPHAaccent);
      else if (symbol >= GDK_Greek_alphaaccent && symbol <= GDK_Greek_omegaaccent &&
               symbol != GDK_Greek_iotaaccentdieresis &&
               symbol != GDK_Greek_upsilonaccentdieresis)
        xupper -= (GDK_Greek_alphaaccent - GDK_Greek_ALPHAaccent);
      else if (symbol >= GDK_Greek_ALPHA && symbol <= GDK_Greek_OMEGA)
        xlower += (GDK_Greek_alpha - GDK_Greek_ALPHA);
      else if (symbol >= GDK_Greek_alpha && symbol <= GDK_Greek_omega &&
               symbol != GDK_Greek_finalsmallsigma)
        xupper -= (GDK_Greek_alpha - GDK_Greek_ALPHA);
      break;
#endif  /* GREEK */
    }

  if (lower)
    *lower = xlower;
  if (upper)
    *upper = xupper;

}
/*
 *--------------------------------------------------------------
 * gdk_init_check
 *
 *   Initialize the library for use.
 *
 * Arguments:
 *   "argc" is the number of arguments.
 *   "argv" is an array of strings.
 *
 * Results:
 *   "argc" and "argv" are modified to reflect any arguments
 *   which were not handled. (Such arguments should either
 *   be handled by the application or dismissed). If initialization
 *   fails, returns FALSE, otherwise TRUE.
 *
 * Side effects:
 *   The library is initialized.
 *
 *--------------------------------------------------------------
 */

gboolean
gdk_init_check (int	 *argc,
		char ***argv)
{
  gint synchronize;
  gint i, j, k;
  gchar **argv_orig = NULL;
  gint argc_orig = 0;
  gint encoding = 0;
  gint encoding_width = 0;
  
  if (gdk_initialized)
    return TRUE;
  
  if (g_thread_supported ())
    gdk_threads_mutex = g_mutex_new ();
  
  if (argc && argv)
    {
      argc_orig = *argc;
      
      argv_orig = g_malloc ((argc_orig + 1) * sizeof (char*));
      for (i = 0; i < argc_orig; i++)
	argv_orig[i] = g_strdup ((*argv)[i]);
      argv_orig[argc_orig] = NULL;
    }
  
  X_GETTIMEOFDAY (&start);
 
  gdk_display_name = NULL;
  
  //GrSetErrorHandler(errorcatcher); //XSetErrorHandler (gdk_x_error);
  				//XSetIOErrorHandler (gdk_x_io_error);
  
  synchronize = FALSE;
  
#ifdef G_ENABLE_DEBUG
  {
    gchar *debug_string = getenv("GDK_DEBUG");
    if (debug_string != NULL)
      gdk_debug_flags = g_parse_debug_string (debug_string,
					      (GDebugKey *) gdk_debug_keys,
					      gdk_ndebug_keys);
  }
gdk_debug_flags=GDK_DEBUG_EVENTS;
#endif	/* G_ENABLE_DEBUG */
  
  if (argc && argv)
    {
      if (*argc > 0)
	{
	  gchar *d;
	  
	  d = strrchr((*argv)[0],'/');
	  if (d != NULL)
	    g_set_prgname (d + 1);
	  else
	    g_set_prgname ((*argv)[0]);
	}
      
      for (i = 1; i < *argc;)
	{
#ifdef G_ENABLE_DEBUG	  
	  if ((strcmp ("--gdk-debug", (*argv)[i]) == 0) ||
	      (strncmp ("--gdk-debug=", (*argv)[i], 12) == 0))
	    {
	      gchar *equal_pos = strchr ((*argv)[i], '=');
	      
	      if (equal_pos != NULL)
		{
		  gdk_debug_flags |= g_parse_debug_string (equal_pos+1,
							   (GDebugKey *) gdk_debug_keys,
							   gdk_ndebug_keys);
		}
	      else if ((i + 1) < *argc && (*argv)[i + 1])
		{
		  gdk_debug_flags |= g_parse_debug_string ((*argv)[i+1],
							   (GDebugKey *) gdk_debug_keys,
							   gdk_ndebug_keys);
		  (*argv)[i] = NULL;
		  i += 1;
		}
	      (*argv)[i] = NULL;
	    }
	  else if ((strcmp ("--gdk-no-debug", (*argv)[i]) == 0) ||
		   (strncmp ("--gdk-no-debug=", (*argv)[i], 15) == 0))
	    {
	      gchar *equal_pos = strchr ((*argv)[i], '=');
	      
	      if (equal_pos != NULL)
		{
		  gdk_debug_flags &= ~g_parse_debug_string (equal_pos+1,
							    (GDebugKey *) gdk_debug_keys,
							    gdk_ndebug_keys);
		}
	      else if ((i + 1) < *argc && (*argv)[i + 1])
		{
		  gdk_debug_flags &= ~g_parse_debug_string ((*argv)[i+1],
							    (GDebugKey *) gdk_debug_keys,
							    gdk_ndebug_keys);
		  (*argv)[i] = NULL;
		  i += 1;
		}
	      (*argv)[i] = NULL;
	    }
	  else 
#endif /* G_ENABLE_DEBUG */
	    if (strcmp ("--display", (*argv)[i]) == 0)
	      {
		(*argv)[i] = NULL;
		
		if ((i + 1) < *argc && (*argv)[i + 1])
		  {
		    gdk_display_name = g_strdup ((*argv)[i + 1]);
		    (*argv)[i + 1] = NULL;
		    i += 1;
		  }
	      }
	    else if (strcmp ("--sync", (*argv)[i]) == 0)
	      {
		(*argv)[i] = NULL;
		synchronize = TRUE;
	      }
	    else if (strcmp ("--no-xshm", (*argv)[i]) == 0)
	      {
		(*argv)[i] = NULL;
		gdk_use_xshm = FALSE;
	      }
	    else if (strcmp ("--name", (*argv)[i]) == 0)
	      {
		if ((i + 1) < *argc && (*argv)[i + 1])
		  {
		    (*argv)[i++] = NULL;
		    g_set_prgname ((*argv)[i]);
		    (*argv)[i] = NULL;
		  }
	      }
	    else if (strcmp ("--class", (*argv)[i]) == 0)
	      {
		if ((i + 1) < *argc && (*argv)[i + 1])
		  {
		    (*argv)[i++] = NULL;
		    gdk_progclass = (*argv)[i];
		    (*argv)[i] = NULL;
		  }
	      }
	  
	  i += 1;
	}
      
      for (i = 1; i < *argc; i++)
	{
	  for (k = i; k < *argc; k++)
	    if ((*argv)[k] != NULL)
	      break;
	  
	  if (k > i)
	    {
	      k -= i;
	      for (j = i + k; j < *argc; j++)
		(*argv)[j-k] = (*argv)[j];
	      *argc -= k;
	    }
	}
    }
  else
    {
      g_set_prgname ("<unknown>");
    }
  
  GDK_NOTE (MISC, g_message ("progname: \"%s\"", g_get_prgname ()));
  
  if (gi_init()<0) {
	fprintf(stderr,"vannot open graphics\n");
	exit(1);
	}
    //    GrReqShmCmds(65536);

  gi_get_screen_info(&gdk_screen_info);
  //GrGetEncoding(&encoding, &encoding_width);
	if (encoding!=0)
	{
		//_gdk_set_encoding_l(encoding);
		//_gdk_set_encoding_width(encoding_width);
         //       printf("Received new lang %d width %d\n",_gdk_get_encoding(),_gdk_get_encoding_width());
	}//
 
  gdk_root_window = GI_DESKTOP_WINDOW_ID;
// ????????????????????
//  gdk_leader_window = XCreateSimpleWindow(gdk_display, gdk_root_window,
//					  10, 10, 10, 10, 0, 0 , 0);
// ????????????????????

//class_hint = XAllocClassHint();
  //class_hint->res_name = g_get_prgname ();
/*
  if (gdk_progclass == NULL)
    {
      gdk_progclass = g_strdup (g_get_prgname ());
      gdk_progclass[0] = toupper (gdk_progclass[0]);
    }
  class_hint->res_class = gdk_progclass;
  XmbSetWMProperties (gdk_display, gdk_leader_window,
                      NULL, NULL, argv_orig, argc_orig, 
                      NULL, NULL, class_hint);
  XFree (class_hint);
 */ 
  for (i = 0; i < argc_orig; i++)
  {
	printf("%s",argv_orig[i]);
    g_free(argv_orig[i]);
  }
  printf("Freeing argv\n");
  g_free(argv_orig);
  
/*
  gdk_wm_delete_window = XInternAtom (gdk_display, "WM_DELETE_WINDOW", False);
  gdk_wm_take_focus = XInternAtom (gdk_display, "WM_TAKE_FOCUS", False);
  gdk_wm_protocols = XInternAtom (gdk_display, "WM_PROTOCOLS", False);
  gdk_wm_window_protocols[0] = gdk_wm_delete_window;
  gdk_wm_window_protocols[1] = gdk_wm_take_focus;
 */
 
  gdk_selection_property = gi_intern_atom ("GDK_SELECTION", False);
//  XGetKeyboardControl (gdk_display, &keyboard_state);
  //autorepeat = keyboard_state.global_auto_repeat;
  
/*
  timer.tv_sec = 0;
  timer.tv_usec = 0;
  timerp = NULL;
 */ 
  g_atexit (gdk_exit_func);
  
  gdk_events_init ();
  gdk_visual_init ();
  gdk_window_init ();
  gdk_image_init ();
  gdk_input_init ();
  gdk_dnd_init ();

  
  gdk_initialized = 1;

  return TRUE;
}

void
gdk_init (int *argc, char ***argv)
{
    if (!gdk_init_check (argc, argv))
    {
      g_warning ("cannot open display:");
      exit(1);
    }
}

/*
 *--------------------------------------------------------------
 * gdk_exit
 *
 *   Restores the library to an un-itialized state and exits
 *   the program using the "exit" system call.
 *
 * Arguments:
 *   "errorcode" is the error value to pass to "exit".
 *
 * Results:
 *   Allocated structures are freed and the program exits
 *   cleanly.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
gdk_exit (int errorcode)
{
	exit (errorcode);
}

void
gdk_set_use_xshm (gboolean use_xshm)
{
	PRINTFILE("gdk_set_use_xshm");
	gdk_use_xshm = FALSE;
}

gboolean
gdk_get_use_xshm (void)
{
   return gdk_use_xshm;
}

/*
 *--------------------------------------------------------------
 * gdk_time_get
 *
 *   Get the number of milliseconds since the library was
 *   initialized.
 *
 * Arguments:
 *
 * Results:
 *   The time since the library was initialized is returned.
 *   This time value is accurate to milliseconds even though
 *   a more accurate time down to the microsecond could be
 *   returned.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

guint32
gdk_time_get (void)
{
  struct timeval end;
  struct timeval elapsed;
  guint32 milliseconds;

	PRINTFILE("gdk_time_get");
  X_GETTIMEOFDAY (&end);

  if (start.tv_usec > end.tv_usec)
    {
      end.tv_usec += 1000000;
      end.tv_sec--;
    }
  elapsed.tv_sec = end.tv_sec - start.tv_sec;
  elapsed.tv_usec = end.tv_usec - start.tv_usec;

  milliseconds = (elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000);

  return milliseconds;
}

/*
 *--------------------------------------------------------------
 * gdk_timer_get
 *
 *   Returns the current timer.
 *
 * Arguments:
 *
 * Results:
 *   Returns the current timer interval. This interval is
 *   in units of milliseconds.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

guint32
gdk_timer_get (void)
{
	return timer_val;
}

/*
 *--------------------------------------------------------------
 * gdk_timer_set
 *
 *   Sets the timer interval.
 *
 * Arguments:
 *   "milliseconds" is the new value for the timer.
 *
 * Results:
 *
 * Side effects:
 *   Calls to "gdk_event_get" will last for a maximum
 *   of time of "milliseconds". However, a value of 0
 *   milliseconds will cause "gdk_event_get" to block
 *   indefinately until an event is received.
 *
 *--------------------------------------------------------------
 */

void
gdk_timer_set (guint32 milliseconds)
{
  timer_val = milliseconds;
  timer.tv_sec = milliseconds / 1000;
  timer.tv_usec = (milliseconds % 1000) * 1000;
}

void
gdk_timer_enable (void)
{
  timerp = &timer;
}

void
gdk_timer_disable (void)
{
  timerp = NULL;
}

/*
 *--------------------------------------------------------------
 * gdk_pointer_grab
 *
 *   Grabs the pointer to a specific window
 *
 * Arguments:
 *   "window" is the window which will receive the grab
 *   "owner_events" specifies whether events will be reported as is,
 *     or relative to "window"
 *   "event_mask" masks only interesting events
 *   "confine_to" limits the cursor movement to the specified window
 *   "cursor" changes the cursor for the duration of the grab
 *   "time" specifies the time
 *
 * Results:
 *
 * Side effects:
 *   requires a corresponding call to gdk_pointer_ungrab
 *
 *--------------------------------------------------------------
 */

gint
gdk_pointer_grab (GdkWindow *	  window,
		  gint		  owner_events,
		  GdkEventMask	  event_mask,
		  GdkWindow *	  confine_to,
		  GdkCursor *	  cursor,
		  guint32	  time)
{
  gint return_val;
  GdkWindowPrivate *window_private;
  //GdkWindowPrivate *confine_to_private;
  //GdkCursorPrivate *cursor_private;
  guint xevent_mask;
  gi_window_id_t xwindow;
  gi_window_id_t xconfine_to;
  unsigned long event_mask_gix=0;    

  g_return_val_if_fail (window != NULL, 0);

  window_private = (GdkWindowPrivate*) window;
  //confine_to_private = (GdkWindowPrivate*) confine_to;

  xwindow = GDK_DRAWABLE_XID(window);

/*  if (!confine_to || GDK_DRAWABLE_DESTROYED(confine_to))
    xconfine_to = None;
  else
    xconfine_to = GDK_DRAWABLE_XID(confine_to); 
*/

  if (!GDK_DRAWABLE_DESTROYED(window)) {
          // Added By Yick_2002_03_07
	  // FIXME: some of the parameters pass do not take effect....
	  // owner_event will behave as TRUE even if you pass FALSE to it
	  // confine_to  will behave as NULL even if you pass a value to it
	  // cursor      will behave as NULL even if you pass a value to it
	  // time        is igonored...
          if (event_mask & GDK_BUTTON_PRESS_MASK)
              event_mask_gix|= GI_MASK_BUTTON_DOWN;

	  if (event_mask & GDK_BUTTON_RELEASE_MASK)
              event_mask_gix|= GI_MASK_BUTTON_UP;

	  //GrGrabPointer(xwindow, event_mask_gix); 
	  gi_grab_pointer(xwindow,TRUE,FALSE,event_mask_gix,0,GI_BUTTON_L|GI_BUTTON_R|GI_BUTTON_M);

	  // End of Yick_2002_03_07

        return_val =  GrabSuccess; /*XGrabPointer (window_private->xdisplay,
                                   xwindow,
                                   owner_events,
                                   xevent_mask,
                                   GrabModeAsync, GrabModeAsync,
                                   xconfine_to,
                                   xcursor,
                                   time);*/
      } else
        return_val = AlreadyGrabbed;

   if (return_val == GrabSuccess)
    gdk_xgrab_window = window_private;

   return return_val;
}

/*
 *--------------------------------------------------------------
 * gdk_pointer_ungrab
 *
 *   Releases any pointer grab
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
gdk_pointer_ungrab (guint32 time)
{
	 gi_ungrab_pointer();
	 gdk_xgrab_window = NULL;
}

/*
 *--------------------------------------------------------------
 * gdk_pointer_is_grabbed
 *
 *   Tell wether there is an active x pointer grab in effect
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gboolean
gdk_pointer_is_grabbed (void)
{
	return gdk_xgrab_window != NULL;
}

/*
 *--------------------------------------------------------------
 * gdk_keyboard_grab
 *
 *   Grabs the keyboard to a specific window
 *
 * Arguments:
 *   "window" is the window which will receive the grab
 *   "owner_events" specifies whether events will be reported as is,
 *     or relative to "window"
 *   "time" specifies the time
 *
 * Results:
 *
 * Side effects:
 *   requires a corresponding call to gdk_keyboard_ungrab
 *
 *--------------------------------------------------------------
 */

gint
gdk_keyboard_grab (GdkWindow *	   window,
		   gint		   owner_events,
		   guint32	   time)
{
	PRINTFILE("gdk_keyboard_grab");
	return 0;
}

/*
 *--------------------------------------------------------------
 * gdk_keyboard_ungrab
 *
 *   Releases any keyboard grab
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
gdk_keyboard_ungrab (guint32 time)
{
	PRINTFILE("gdk_keyboard_ungrab");
}

/*
 *--------------------------------------------------------------
 * gdk_screen_width
 *
 *   Return the width of the screen.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint
gdk_screen_width (void)
{
	return gdk_screen_info.scr_width;	
}

/*
 *--------------------------------------------------------------
 * gdk_screen_height
 *
 *   Return the height of the screen.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint
gdk_screen_height (void)
{
       return gdk_screen_info.scr_height;
}

/*
 *--------------------------------------------------------------
 * gdk_screen_width_mm
 *
 *   Return the width of the screen in millimeters.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint
gdk_screen_width_mm (void)
{
	return gdk_screen_info.scr_width/gdk_screen_info.xdpcm *10;
}

/*
 *--------------------------------------------------------------
 * gdk_screen_height
 *
 *   Return the height of the screen in millimeters.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

gint
gdk_screen_height_mm (void)
{
	return gdk_screen_info.scr_height/gdk_screen_info.xdpcm *10;
}

/*
 *--------------------------------------------------------------
 * gdk_set_sm_client_id
 *
 *   Set the SM_CLIENT_ID property on the WM_CLIENT_LEADER window
 *   so that the window manager can save our state using the
 *   X11R6 ICCCM session management protocol. A NULL value should 
 *   be set following disconnection from the session manager to
 *   remove the SM_CLIENT_ID property.
 *
 * Arguments:
 * 
 *   "sm_client_id" specifies the client id assigned to us by the
 *   session manager or NULL to remove the property.
 *
 * Results:
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

void
gdk_set_sm_client_id (const gchar* sm_client_id)
{
	PRINTFILE("gdk_set_sm_client_id");
}

void
gdk_key_repeat_disable (void)
{
}

void
gdk_key_repeat_restore (void)
{
}


void
gdk_beep (void)
{
	PRINTFILE("gdk_beep");
}

/*
 *--------------------------------------------------------------
 * gdk_exit_func
 *
 *   This is the "atexit" function that makes sure the
 *   library gets a chance to cleanup.
 *
 * Arguments:
 *
 * Results:
 *
 * Side effects:
 *   The library is un-initialized and the program exits.
 *
 *--------------------------------------------------------------
 */

static void
gdk_exit_func (void)
{
 	static gboolean in_gdk_exit_func = FALSE;

  /* This is to avoid an infinite loop if a program segfaults in
     an atexit() handler (and yes, it does happen, especially if a program
     has trounced over memory too badly for even g_message to work) */
  if (in_gdk_exit_func == TRUE)
    return;
  in_gdk_exit_func = TRUE;

  if (gdk_initialized)
    {
      gdk_image_exit ();
      gdk_input_exit ();
      gdk_key_repeat_restore ();

      gi_exit ();
      gdk_initialized = 0;
    }

}

/*
 *--------------------------------------------------------------
 * gdk_x_error
 *
 *   The X error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *   "error" is the XErrorEvent that we are handling.
 *
 * Results:
 *   Either we were expecting some sort of error to occur,
 *   in which case we set the "gdk_error_code" flag, or this
 *   error was unexpected, in which case we will print an
 *   error message and exit. (Since trying to continue will
 *   most likely simply lead to more errors).
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
gdk_x_error ()
{
	PRINTFILE("gdk_x_error");
	return 0;
}

/*
 *--------------------------------------------------------------
 * gdk_x_io_error
 *
 *   The X I/O error handling routine.
 *
 * Arguments:
 *   "display" is the X display the error orignated from.
 *
 * Results:
 *   An X I/O error basically means we lost our connection
 *   to the X server. There is not much we can do to
 *   continue, so simply print an error message and exit.
 *
 * Side effects:
 *
 *--------------------------------------------------------------
 */

static int
gdk_x_io_error ()
{
	PRINTFILE("gdk_x_io_error");
	return 0;
}

gchar *
gdk_get_display (void)
{
	return 0;
}

/*************************************************************
 * gdk_error_trap_push:
 *     Push an error trap. X errors will be trapped until
 *     the corresponding gdk_error_pop(), which will return
 *     the error code, if any.
 *   arguments:
 *     
 *   results:
 *************************************************************/

void
gdk_error_trap_push (void)
{
  GSList *node;
  GdkErrorTrap *trap;

  if (gdk_error_trap_free_list)
    {
      node = gdk_error_trap_free_list;
      gdk_error_trap_free_list = gdk_error_trap_free_list->next;
    }
  else
    {
      node = g_slist_alloc ();
      node->data = g_new (GdkErrorTrap, 1);
    }

  node->next = gdk_error_traps;
  gdk_error_traps = node;

  trap = node->data;
  trap->error_code = gdk_error_code;
  trap->error_warnings = gdk_error_warnings;

  gdk_error_code = 0;
  gdk_error_warnings = 0;
}

/*************************************************************
 * gdk_error_trap_pop:
 *     Pop an error trap added with gdk_error_push()
 *   arguments:
 *     
 *   results:
 *     0, if no error occured, otherwise the error code.
 *************************************************************/

gint
gdk_error_trap_pop (void)
{
  GSList *node;
  GdkErrorTrap *trap;
  gint result;

  g_return_val_if_fail (gdk_error_traps != NULL, 0);

  node = gdk_error_traps;
  gdk_error_traps = gdk_error_traps->next;

  node->next = gdk_error_trap_free_list;
  gdk_error_trap_free_list = node;

  result = gdk_error_code;

  trap = node->data;
  gdk_error_code = trap->error_code;
  gdk_error_warnings = trap->error_warnings;

  return result;
}

gint 
gdk_send_xevent (gboolean propagate, glong event_mask)
{
	PRINTFILE("gdk_send_xevent");
	return FALSE;
}

#ifndef HAVE_XCONVERTCASE
/* compatibility function from X11R6.3, since XConvertCase is not
 * supplied by X11R5.
 */
static void
gdkx_XConvertCase ()
{
	PRINTFILE("gdkx_XConvertCase");
}
#endif

gchar*
gdk_keyval_name (guint	      keyval)
{
	PRINTFILE("gdk_keyval_name");
	return 0;
}

guint
gdk_keyval_from_name (const gchar *keyval_name)
{
	PRINTFILE("gdk_keyval_from_name");
	return 0;
}

guint
gdk_keyval_to_upper (guint	  keyval)
{
	PRINTFILE("gdk_keyval_to_upper");
	return 0;
}

guint
gdk_keyval_to_lower (guint	  keyval)
{
  guint result;
  gdk_keyval_convert_case (keyval, &result, NULL);
  return result;
}

gboolean
gdk_keyval_is_upper (guint	  keyval)
{
	PRINTFILE("gdk_keyval_is_upper");
	return 0;
}

gboolean
gdk_keyval_is_lower (guint	  keyval)
{
	PRINTFILE("gdk_keyval_is_lower");
	return 0;
}

void
gdk_threads_enter ()
{
  GDK_THREADS_ENTER ();
}

void
gdk_threads_leave ()
{
  GDK_THREADS_LEAVE ();
}

