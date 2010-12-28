//========================================================================
//
// GPDFApp.cc
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>
#include <stdio.h>
#include <string.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include "GString.h"
#include "GList.h"
#include "Error.h"
//#include "GPDFViewer.h"
#include "GPDFApp.h"
#include "config.h"


GPDFApp::GPDFApp(int *argc, char *argv[]) {
	gi_screen_info_t si;
	gi_color_t xcol;
	int err;
	char *spaperColor=NULL;
	char *smatteColor="wheat";
	char *sfullScreenMatteColor="whitesmoke";
  //appShell = XtAppInitialize(&appContext, xpdfAppName, xOpts, nXOpts,
	//		     argc, argv, fallbackResources, NULL, 0);
  //display = XtDisplay(appShell);
  //screenNum = XScreenNumberOfScreen(XtScreen(appShell));

  err = gi_init();
  if (err < 0)
  {
	  exit(1);
  }

  gi_get_screen_info(&si);


  fullScreen = gFalse;
  remoteAtom = 0;
  remoteViewer = NULL;
  remoteWin = 0;
#if 1
	geometry =  (GString *)NULL;
  title =  (GString *)NULL;
  //installCmap = (GBool)resources.installCmap;
  rgbCubeSize = 1;//resources.rgbCubeSize;
  reverseVideo = (GBool)FALSE;
  if (reverseVideo) {
    paperRGB[0] = paperRGB[1] = paperRGB[2] = 0;
    paperPixel = gi_color_to_pixel(&si,paperRGB[0],paperRGB[1],paperRGB[2] );
  } else {
    paperRGB[0] = paperRGB[1] = paperRGB[2] = 0xff;
    paperPixel = gi_color_to_pixel(&si,paperRGB[0],paperRGB[1],paperRGB[2] );
  }
 // XtVaGetValues(appShell, XmNcolormap, &colormap, NULL);
  if (spaperColor) {
    if (gi_parse_color(spaperColor,&xcol)) {
      paperRGB[0] = GI_RED(xcol) ;
      paperRGB[1] = GI_GREEN(xcol) ;
      paperRGB[2] = GI_BLUE(xcol) ;
      paperPixel = gi_color_to_pixel(&si,paperRGB[0],paperRGB[1],paperRGB[2] );
    } else {
      error(-1, "Couldn't allocate color '%s'", spaperColor);
    }
 }
  if (gi_parse_color(smatteColor,&xcol) ) {
    mattePixel = gi_color_to_pixel(&si,GI_RED(xcol),GI_GREEN(xcol),GI_BLUE(xcol) );
  } else {
    mattePixel = paperPixel;
  }

  if (gi_parse_color(sfullScreenMatteColor,&xcol) ) {
    fullScreenMattePixel = gi_color_to_pixel(&si,GI_RED(xcol),GI_GREEN(xcol),GI_BLUE(xcol) );
  } else {
    fullScreenMattePixel = paperPixel;
  }
  initialZoom =  (GString *)NULL;
#endif
  //getResources();

  viewers = new GList();

}

void GPDFApp::getResources() {

	mattePixel = 0xffffffff;
	fullScreenMattePixel = 0xff000000;
	//initialZoom = "";
	//title = "Xpdf";
  
}

GPDFApp::~GPDFApp() {
  deleteGList(viewers, GPDFViewer);
  if (geometry) {
    delete geometry;
  }
  if (title) {
    delete title;
  }
  if (initialZoom) {
    delete initialZoom;
  }
}

GPDFViewer *GPDFApp::open(GString *fileName, int page,
			  GString *ownerPassword, GString *userPassword) {
  GPDFViewer *viewer;


  viewer = new GPDFViewer(this, fileName, page, NULL, fullScreen,
			  ownerPassword, userPassword);
  if (!viewer->isOk()) {
    delete viewer;
    return NULL;
  }
  if (remoteAtom != 0) {
    remoteViewer = viewer;
    remoteWin = viewer->getWindow();
   // XtAddEventHandler(remoteWin, PropertyChangeMask, False,
	//	      &remoteMsgCbk, this);
    gi_set_selection_owner( remoteAtom, (remoteWin), 0L);
  }
  viewers->append(viewer);
  return viewer;
}

GPDFViewer *GPDFApp::openAtDest(GString *fileName, GString *dest,
				GString *ownerPassword,
				GString *userPassword) {
  GPDFViewer *viewer;


  viewer = new GPDFViewer(this, fileName, 1, dest, fullScreen,
			  ownerPassword, userPassword);
  if (!viewer->isOk()) {
    delete viewer;
    return NULL;
  }
  if (remoteAtom != 0) {
    remoteViewer = viewer;
    remoteWin = viewer->getWindow();
   // XtAddEventHandler(remoteWin, PropertyChangeMask, False,
	//	      &remoteMsgCbk, this);
    gi_set_selection_owner( remoteAtom, (remoteWin), 0);
  }
  viewers->append(viewer);
  return viewer;
}


GPDFViewer *GPDFApp::reopen(GPDFViewer *viewer, PDFDoc *doc, int page,
			    GBool fullScreenA) {
  int i;

  for (i = 0; i < viewers->getLength(); ++i) {
    if (((GPDFViewer *)viewers->get(i)) == viewer) {
      viewers->del(i);
      delete viewer;
    }
  }
  viewer = new GPDFViewer(this, doc, page, NULL, fullScreenA);
  if (!viewer->isOk()) {
    delete viewer;
    return NULL;
  }
  if (remoteAtom != 0) {
    remoteViewer = viewer;
    remoteWin = viewer->getWindow();
    //XtAddEventHandler(remoteWin, PropertyChangeMask, False,
	//	      &remoteMsgCbk, this);
    gi_set_selection_owner( remoteAtom, (remoteWin), 0);
  }
  viewers->append(viewer);
  return viewer;
}

void GPDFApp::close(GPDFViewer *viewer, GBool closeLast) {
  int i;

  if (viewers->getLength() == 1) {
    if (viewer != (GPDFViewer *)viewers->get(0)) {
      return;
    }
    if (closeLast) {
      quit();
    } else {
      viewer->clear();
    }
  } else {
    for (i = 0; i < viewers->getLength(); ++i) {
      if (((GPDFViewer *)viewers->get(i)) == viewer) {
	viewers->del(i);
	if (remoteAtom != 0 && remoteViewer == viewer) {
	  remoteViewer = (GPDFViewer *)viewers->get(viewers->getLength() - 1);
	  remoteWin = remoteViewer->getWindow();
	  gi_set_selection_owner( remoteAtom, (remoteWin),0);
	}
	delete viewer;
	return;
      }
    }
  }
}

void GPDFApp::quit() {
  if (remoteAtom != 0) {
    gi_set_selection_owner( remoteAtom, 0, 0);
  }
  while (viewers->getLength() > 0) {
    delete (GPDFViewer *)viewers->del(0);
  }

  exit(0);
}

void GPDFApp::run() {
	gi_msg_t msg;

	while (gi_get_new_message(&msg))
	{
		gi_dispatch_message(&msg);	
	}
}

void GPDFApp::setRemoteName(char *remoteName) {
  remoteAtom = gi_intern_atom( remoteName, FALSE);
  remoteXWin = gi_get_selection_owner( remoteAtom);
}

GBool GPDFApp::remoteServerRunning() {
  return remoteXWin != 0;
}
#define remoteCmdSize 512

void GPDFApp::remoteExec(char *cmd) {
  char cmd2[remoteCmdSize];
  int n;

  n = strlen(cmd);
  if (n > remoteCmdSize - 2) {
    n = remoteCmdSize - 2;
  }
  memcpy(cmd2, cmd, n);
  cmd2[n] = '\n';
  cmd2[n+1] = '\0';
  gi_change_property( remoteXWin, remoteAtom, remoteAtom, 8,
		  G_PROP_MODE_Replace, (Guchar *)cmd2, n + 2);
}

void GPDFApp::remoteOpen(GString *fileName, int page, GBool raise) {
  char cmd[remoteCmdSize];

  sprintf(cmd, "openFileAtPage(%.200s,%d)\n",
	  fileName->getCString(), page);
  if (raise) {
    strcat(cmd, "raise\n");
  }
  gi_change_property( remoteXWin, remoteAtom, remoteAtom, 8,
		  G_PROP_MODE_Replace, (Guchar *)cmd, strlen(cmd) + 1);
}

void GPDFApp::remoteOpenAtDest(GString *fileName, GString *dest, GBool raise) {
  char cmd[remoteCmdSize];

  sprintf(cmd, "openFileAtDest(%.200s,%.256s)\n",
	  fileName->getCString(), dest->getCString());
  if (raise) {
    strcat(cmd, "raise\n");
  }
  gi_change_property( remoteXWin, remoteAtom, remoteAtom, 8,
		  G_PROP_MODE_Replace, (Guchar *)cmd, strlen(cmd) + 1);
}

void GPDFApp::remoteReload(GBool raise) {
  char cmd[remoteCmdSize];

  strcpy(cmd, "reload\n");
  if (raise) {
    strcat(cmd, "raise\n");
  }
  gi_change_property( remoteXWin, remoteAtom, remoteAtom, 8,
		  G_PROP_MODE_Replace, (Guchar *)cmd, strlen(cmd) + 1);
}

void GPDFApp::remoteRaise() {
  gi_change_property( remoteXWin, remoteAtom, remoteAtom, 8,
		  G_PROP_MODE_Replace, (Guchar *)"raise\n", 7);
}

void GPDFApp::remoteQuit() {
  gi_change_property( remoteXWin, remoteAtom, remoteAtom, 8,
		  G_PROP_MODE_Replace, (Guchar *)"quit\n", 6);
}

#if 1
void GPDFApp::remoteMsgCbk( void* ptr,
			   gi_msg_t *event, gi_bool_t *cont) {
  GPDFApp *app = (GPDFApp *)ptr;
  char *cmd, *p0, *p1;
  gi_atom_id_t type;
  int format;
  Gulong size, remain;
  GString *cmdStr;

  if (event->params[1] != app->remoteAtom) {
    *cont = TRUE;
    return;
  }
  *cont = FALSE;

  if (gi_get_window_property( (app->remoteWin),
			 app->remoteAtom, 0, remoteCmdSize/4,
			 TRUE, app->remoteAtom,
			 &type, &format, &size, &remain,
			 (Guchar **)&cmd) != 0) {
    return;
  }
  if (!cmd) {
    return;
  }
  p0 = cmd;
  while (*p0 && (p1 = strchr(p0, '\n'))) {
    cmdStr = new GString(p0, p1 - p0);
    app->remoteViewer->execCmd(cmdStr, NULL);
    delete cmdStr;
    p0 = p1 + 1;
  }
  free((void*)cmd);
}
#endif


/***************************************/

#include "gmem.h"
#include "GString.h"
#include "GList.h"
#include "Error.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "Link.h"
#include "ErrorCodes.h"
#include "GfxState.h"
#include "CoreOutputDev.h"
#include "PSOutputDev.h"
#include "TextOutputDev.h"
#include "SplashBitmap.h"
#include "SplashPattern.h"
#include "ErrorCodes.h"

GString *GPDFCore::currentSelection = NULL;
GPDFCore *GPDFCore::currentSelectionOwner = NULL;
gi_atom_id_t GPDFCore::targetsAtom;

class GPDFCoreTile: public PDFCoreTile {
public:
  GPDFCoreTile(int xDestA, int yDestA);
  virtual ~GPDFCoreTile();
  gi_image_t *image;
};

GPDFCoreTile::GPDFCoreTile(int xDestA, int yDestA):
  PDFCoreTile(xDestA, yDestA)
{
  image = NULL;
}

GPDFCoreTile::~GPDFCoreTile() {
  if (image) {
    //free(image->rgba);
    //image->rgba = NULL;
    gi_destroy_image(image);
	image=NULL;
  }
}

void GPDFCore::update(int topPageA, int scrollXA, int scrollYA,
		      double zoomA, int rotateA,
		      GBool force, GBool addToHist) {
  int oldPage;

  oldPage = topPage;
  PDFCore::update(topPageA, scrollXA, scrollYA, zoomA, rotateA,
		  force, addToHist);
  linkAction = NULL;
  if (doc && topPage != oldPage) {
    if (updateCbk) {
      (*updateCbk)(updateCbkData, NULL, topPage, -1, "");
    }
  }
}

void GPDFCore::takeFocus() {
  //XmProcessTraversal(drawArea, XmTRAVERSE_CURRENT);
}

void GPDFCore::copySelection() {
  int pg;
  double ulx, uly, lrx, lry;

  if (!doc->okToCopy()) {
    return;
  }
  if (getSelection(&pg, &ulx, &uly, &lrx, &lry)) {
    //~ for multithreading: need a mutex here
    if (currentSelection) {
      delete currentSelection;
    }
    currentSelection = extractText(pg, ulx, uly, lrx, lry);
    currentSelectionOwner = this;

	gi_clipboard_owner_set(drawArea,0,NULL);
	gi_clipbiard_set_target_data(GA_STRING,strdup(currentSelection->getCString()),currentSelection->getLength());

	//gi_clipbiard_request(drawArea,"STRING", 0,&convertSelectionCbk,NULL);
    //XtOwnSelection(drawArea, GA_PRIMARY, XtLastTimestampProcessed(display),
	//	   &convertSelectionCbk, NULL, NULL);
  }
}


gi_bool_t GPDFCore::convertSelectionCbk( gi_atom_id_t *selection,
				      gi_atom_id_t *target, gi_atom_id_t *type,
				      void* *value, unsigned long *length,
				      int *format) {
  gi_atom_id_t *array;

  // send back a list of supported conversion targets
  if (*target == targetsAtom) {
    if (!(array = (gi_atom_id_t *)malloc(sizeof(gi_atom_id_t)))) {
      return FALSE;
    }
    array[0] = GA_STRING;
    *value = (void*)array;
    *type = GA_ATOM;
    *format = 32;
    *length = 1;
    return TRUE;

  // send the selected text
  } else if (*target == GA_STRING) {
    //~ for multithreading: need a mutex here
    //*value = XtNewString(currentSelection->getCString());
    *value = strdup(currentSelection->getCString());
    *length = currentSelection->getLength();
    *type = GA_STRING;
    *format = 8; // 8-bit elements
    return TRUE;
  }

  return FALSE;
}

GPDFCore::GPDFCore(gi_window_id_t win,gi_window_id_t parent,
		   SplashColorPtr paperColorA, Gulong paperPixelA,
		   Gulong mattePixelA, GBool fullScreenA, GBool reverseVideoA,
		   GBool installCmap, int rgbCubeSizeA):
  PDFCore(splashModeRGB8, 4, reverseVideoA, paperColorA, !fullScreenA)
{
  GString *initialZoom;

  //shell = shellA;
  //parentWidget = parentWidgetA;
  //display = XtDisplay(parentWidget);
  //screenNum = XScreenNumberOfScreen(XtScreen(parentWidget));
  targetsAtom = gi_intern_atom( "TARGETS", FALSE);

  paperPixel = paperPixelA;
  mattePixel = mattePixelA;
  fullScreen = fullScreenA;



   Gulong mask;
  int nVisuals;
  int r, g, b, n, m;
  GBool ok;
  gi_screen_info_t si;
  gi_get_screen_info(&si);

  //setupX(installCmap, rgbCubeSizeA);

  format = si.format;
 if (1) {
    trueColor = gTrue;
#if 0
    for (mask = si.redmask, rShift = 0;
         mask && !(mask & 1);
         mask >>= 1, ++rShift) ;
    for (rDiv = 8; mask; mask >>= 1, --rDiv) ;
    for (mask = si.greenmask, gShift = 0;
         mask && !(mask & 1);
         mask >>= 1, ++gShift) ;
    for (gDiv = 8; mask; mask >>= 1, --gDiv) ;
    for (mask = si.bluemask, bShift = 0;
         mask && !(mask & 1);
         mask >>= 1, ++bShift) ;
    for (bDiv = 8; mask; mask >>= 1, --bDiv) ;
#else
	rShift = si.redoffset;
	rDiv = 8-si.redlength;

	gShift = si.greenoffset;
	gDiv = 8-si.greenlength;

	bShift = si.blueoffset;
	bDiv = 8-si.bluelength;
#endif
  } else {
    trueColor = gFalse;
  }
#if 0
 // allocate a color cube
  if (!trueColor) {

    // set colors in private colormap
    if (installCmap) {
      for (rgbCubeSize = xMaxRGBCube; rgbCubeSize >= 2; --rgbCubeSize) {
        m = rgbCubeSize * rgbCubeSize * rgbCubeSize;
        if (XAllocColorCells(display, colormap, False, NULL, 0, colors, m)) {
          break;
        }
      }
      if (rgbCubeSize >= 2) {
        m = rgbCubeSize * rgbCubeSize * rgbCubeSize;
        xcolors = (XColor *)gmallocn(m, sizeof(XColor));
        n = 0;
        for (r = 0; r < rgbCubeSize; ++r) {
          for (g = 0; g < rgbCubeSize; ++g) {
            for (b = 0; b < rgbCubeSize; ++b) {
              xcolors[n].pixel = colors[n];
              xcolors[n].red = (r * 65535) / (rgbCubeSize - 1);
              xcolors[n].green = (g * 65535) / (rgbCubeSize - 1);
              xcolors[n].blue = (b * 65535) / (rgbCubeSize - 1);
              xcolors[n].flags = DoRed | DoGreen | DoBlue;
              ++n;
            }
          }
        }
        XStoreColors(display, colormap, xcolors, m);
        gfree(xcolors);
      } else {
        rgbCubeSize = 1;
        colors[0] = BlackPixel(display, screenNum);
        colors[1] = WhitePixel(display, screenNum);
      }

    // allocate colors in shared colormap
    } else {
      if (rgbCubeSize > xMaxRGBCube) {
        rgbCubeSize = xMaxRGBCube;
      }
      ok = gFalse;
      for (rgbCubeSize = rgbCubeSizeA; rgbCubeSize >= 2; --rgbCubeSize) {
        ok = gTrue;
        n = 0;
        for (r = 0; r < rgbCubeSize && ok; ++r) {
          for (g = 0; g < rgbCubeSize && ok; ++g) {
            for (b = 0; b < rgbCubeSize && ok; ++b) {
              if (n == 0) {
                colors[n] = BlackPixel(display, screenNum);
                ++n;
              } else {
                xcolor.red = (r * 65535) / (rgbCubeSize - 1);
                xcolor.green = (g * 65535) / (rgbCubeSize - 1);
                xcolor.blue = (b * 65535) / (rgbCubeSize - 1);
                if (XAllocColor(display, colormap, &xcolor)) {
                  colors[n++] = xcolor.pixel;
                } else {
                  ok = gFalse;
                }
              }
            }
          }
        }
        if (ok) {
          break;
        }
        XFreeColors(display, colormap, &colors[1], n-1, 0);
      }
      if (!ok) {
        rgbCubeSize = 1;
        colors[0] = BlackPixel(display, screenNum);
        colors[1] = WhitePixel(display, screenNum);
      }
    }
  }
#endif
  //scrolledWin = NULL;
  //hScrollBar = NULL;
  //vScrollBar = NULL;
  //drawAreaFrame = NULL;
  drawArea = win;

  // get the initial zoom value
  if (fullScreen) {
    zoom = zoomPage;
  } else {
    initialZoom = globalParams->getInitialZoom();
    if (!initialZoom->cmp("page")) {
      zoom = zoomPage;
    } else if (!initialZoom->cmp("width")) {
      zoom = zoomWidth;
    } else {
      zoom = atoi(initialZoom->getCString());
      if (zoom <= 0) {
	zoom = defZoom;
      }
    }
    delete initialZoom;
  }

  linkAction = NULL;

  panning = gFalse;

  updateCbk = NULL;
  actionCbk = NULL;
  keyPressCbk = NULL;
  mouseCbk = NULL;

  // optional features default to on
  hyperlinksEnabled = gTrue;
  selectEnabled = gTrue;

  // do X-specific initialization and create the widgets
  initWindow();
  //initPasswordDialog();
}



void GPDFCore::initWindow() {
 // Arg args[20];
  int n;

  // create the cursors
  busyCursor =GI_CURSOR_WATCH;// XCreateFontCursor(display, XC_watch);
  linkCursor =GI_CURSOR_HAND;// XCreateFontCursor(display, XC_hand2);
  selectCursor = GI_CURSOR_CROSS;//XCreateFontCursor(display, XC_cross);
  currentCursor = 0;

  // create the scrolled window and scrollbars
 // n = 0;
  //XtSetArg(args[n], XmNscrollingPolicy, XmAPPLICATION_DEFINED); ++n;
  //XtSetArg(args[n], XmNvisualPolicy, XmVARIABLE); ++n;
  //scrolledWin = XmCreateScrolledWindow(parentWidget, "scroll", args, n);
  //XtManageChild(scrolledWin);
 // n = 0;
 // XtSetArg(args[n], XmNorientation, XmHORIZONTAL); ++n;
 // XtSetArg(args[n], XmNminimum, 0); ++n;
 // XtSetArg(args[n], XmNmaximum, 1); ++n;
 // XtSetArg(args[n], XmNsliderSize, 1); ++n;
 // XtSetArg(args[n], XmNvalue, 0); ++n;
  //XtSetArg(args[n], XmNincrement, 1); ++n;
  //XtSetArg(args[n], XmNpageIncrement, 1); ++n;
  //hScrollBar = XmCreateScrollBar(scrolledWin, "hScrollBar", args, n);
  //if (!fullScreen) {
    //XtManageChild(hScrollBar);
  //}
  //XtAddCallback(hScrollBar, XmNvalueChangedCallback,
	//	&hScrollChangeCbk, (void*)this);

  n = 0;
  //XtSetArg(args[n], XmNorientation, XmVERTICAL); ++n;
  //XtSetArg(args[n], XmNminimum, 0); ++n;
 // XtSetArg(args[n], XmNmaximum, 1); ++n;
 // XtSetArg(args[n], XmNsliderSize, 1); ++n;
 // XtSetArg(args[n], XmNvalue, 0); ++n;
 // XtSetArg(args[n], XmNincrement, 1); ++n;
 // XtSetArg(args[n], XmNpageIncrement, 1); ++n;
 // vScrollBar = XmCreateScrollBar(scrolledWin, "vScrollBar", args, n);
  if (!fullScreen) {
   // XtManageChild(vScrollBar);
  }
  //XtAddCallback(vScrollBar, XmNvalueChangedCallback,
	//	&vScrollChangeCbk, (void*)this);


  // create the drawing area
 // n = 0;
  //XtSetArg(args[n], XmNshadowType, XmSHADOW_IN); ++n;
  //XtSetArg(args[n], XmNmarginWidth, 0); ++n;
  //XtSetArg(args[n], XmNmarginHeight, 0); ++n;
  if (fullScreen) {
    //XtSetArg(args[n], XmNshadowThickness, 0); ++n;
  }
  //drawAreaFrame = XmCreateFrame(scrolledWin, "drawAreaFrame", args, n);
 // XtManageChild(drawAreaFrame);
  n = 0;
  //XtSetArg(args[n], XmNresizePolicy, XmRESIZE_ANY); ++n;
  //XtSetArg(args[n], XmNwidth, 700); ++n;
  //XtSetArg(args[n], XmNheight, 500); ++n;
  //drawArea = XmCreateDrawingArea(drawAreaFrame, "drawArea", args, n);
  //drawArea = gi_create_window(GI_DESKTOP_WINDOW_ID, 0,0,700, 500,GI_RGB( 192, 192, 192 ),GI_FLAGS_NORESIZE);
  //XtManageChild(drawArea);
  //XtAddCallback(drawArea, XmNresizeCallback, &resizeCbk, (void*)this);
  //XtAddCallback(drawArea, XmNexposeCallback, &redrawCbk, (void*)this);
  //XtAddCallback(drawArea, XmNinputCallback, &inputCbk, (void*)this);
 // resizeCbk(drawArea, this, NULL);
 int sx,sy;

	this->drawAreaWidth = (int)DRAWABLE_INIT_WIDTH;
  this->drawAreaHeight = (int)DRAWABLE_INIT_HEIGHT - DRAWABLE_NAVIGATION_HEIGHT;
  //if (core->zoom == zoomPage || core->zoom == zoomWidth) {
    sx = sy = -1;
 // } else {
   // sx = core->scrollX;
   // sy = core->scrollY;
 // }
  this->update(this->topPage, sx, sy, this->zoom, this->rotate, gTrue, gFalse);

  

  //gi_show_window(drawArea);

  // set up mouse motion translations
  //XtOverrideTranslations(drawArea, XtParseTranslationTable(
   //   "<Btn1Down>:DrawingAreaInput()\n"
   //   "<Btn1Up>:DrawingAreaInput()\n"
    //  "<Btn1Motion>:DrawingAreaInput()\n"
    //  "<Motion>:DrawingAreaInput()"));

  // can't create a GC until the window gets mapped
  drawAreaGC = NULL;
}


GPDFCore::~GPDFCore() {
  if (currentSelectionOwner == this && currentSelection) {
    delete currentSelection;
    currentSelection = NULL;
    currentSelectionOwner = NULL;
  }
  if (drawAreaGC) {
    gi_destroy_gc( drawAreaGC);
  }
  /*if (scrolledWin) {
    XtDestroyWidget(scrolledWin);
  }
  if (busyCursor) {
    XFreeCursor(display, busyCursor);
  }
  if (linkCursor) {
    XFreeCursor(display, linkCursor);
  }
  if (selectCursor) {
    XFreeCursor(display, selectCursor);
  }
  */
}

//------------------------------------------------------------------------
// loadFile / displayPage / displayDest
//------------------------------------------------------------------------

int GPDFCore::loadFile(GString *fileName, GString *ownerPassword,
		       GString *userPassword) {
  int err;

  err = PDFCore::loadFile(fileName, ownerPassword, userPassword);
  if (err == errNone) {
    // save the modification time
    modTime = getModTime(doc->getFileName()->getCString());

    // update the parent window
    if (updateCbk) {
      (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		   doc->getNumPages(), NULL);
    }
  }
  return err;
}

void GPDFCore::startPan(int wx, int wy) {
  panning = gTrue;
  panMX = wx;
  panMY = wy;
}

void GPDFCore::endPan(int wx, int wy) {
  panning = gFalse;
}

int GPDFCore::loadFile(BaseStream *stream, GString *ownerPassword,
		       GString *userPassword) {
  int err;

  err = PDFCore::loadFile(stream, ownerPassword, userPassword);
  if (err == errNone) {
    // no file
    modTime = 0;

    // update the parent window
    if (updateCbk) {
      (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		   doc->getNumPages(), NULL);
    }
  }
  return err;
}

void GPDFCore::loadDoc(PDFDoc *docA) {
  PDFCore::loadDoc(docA);

  // save the modification time
  if (doc->getFileName()) {
    modTime = getModTime(doc->getFileName()->getCString());
  }

  // update the parent window
  if (updateCbk) {
    (*updateCbk)(updateCbkData, doc->getFileName(), -1,
		 doc->getNumPages(), NULL);
  }
}


//------------------------------------------------------------------------
// selection
//------------------------------------------------------------------------
//#define NO_TEXT_SELECT 1
void GPDFCore::startSelection(int wx, int wy) {
  int pg, x, y;
#if 1
  takeFocus();
  if (doc && doc->getNumPages() > 0) {
    if (selectEnabled) {
      if (cvtWindowToDev(wx, wy, &pg, &x, &y)) {
	setSelection(pg, x, y, x, y);
	setCursor(selectCursor);
	dragging = gTrue;
      }
    }
  }
#endif
}

void GPDFCore::endSelection(int wx, int wy) {
  int pg, x, y;
  GBool ok;
#if 1

  if (doc && doc->getNumPages() > 0) {
    ok = cvtWindowToDev(wx, wy, &pg, &x, &y);
    if (dragging) {
      dragging = gFalse;
      setCursor(0);
      if (ok) {
	moveSelection(pg, x, y);
      }
#ifndef NO_TEXT_SELECT
      if (selectULX != selectLRX &&
	  selectULY != selectLRY) {
	if (doc->okToCopy()) {
	  copySelection();
	} else {
	  error(-1, "Copying of text from this document is not allowed.");
	}
      }
#endif
    }
  }
#endif
}
void GPDFCore::resizeToPage(int pg) {
  int width, height;
  double width1, height1;
  int topW, topH, topBorder, daW, daH;
  int displayW, displayH;

  displayW = gi_screen_width( );
  displayH = gi_screen_height( );
  if (fullScreen) {
    width = displayW;
    height = displayH;
  } else {
    if (!doc || pg <= 0 || pg > doc->getNumPages()) {
      width1 = DRAWABLE_INIT_WIDTH; //612;
      height1 = DRAWABLE_INIT_HEIGHT; //792;
    } else if (doc->getPageRotate(pg) == 90 ||
	       doc->getPageRotate(pg) == 270) {
      width1 = doc->getPageCropHeight(pg);
      height1 = doc->getPageCropWidth(pg);
    } else {
      width1 = doc->getPageCropWidth(pg);
      height1 = doc->getPageCropHeight(pg);
    }
    if (zoom == zoomPage || zoom == zoomWidth) {
      width = (int)(width1 * 0.01 * defZoom + 0.5);
      height = (int)(height1 * 0.01 * defZoom + 0.5);
    } else {
      width = (int)(width1 * 0.01 * zoom + 0.5);
      height = (int)(height1 * 0.01 * zoom + 0.5);
    }
    if (continuousMode) {
      height += continuousModePageSpacing;
    }
    if (width > displayW - 100) {
      width = displayW - 100;
    }
    if (height > displayH - 100) {
      height = displayH - 100;
    }
  }

  /*if (XtIsRealized(shell)) {
    XtVaGetValues(shell, XmNwidth, &topW, XmNheight, &topH,
		  XmNborderWidth, &topBorder, NULL);
    XtVaGetValues(drawArea, XmNwidth, &daW, XmNheight, &daH, NULL);
    XtVaSetValues(shell, XmNwidth, width + (topW - daW),
		  XmNheight, height + (topH - daH), NULL);
  } else */{
    //XtVaSetValues(drawArea, XmNwidth, width, XmNheight, height, NULL);
	printf("resize to %d,%d\n",  width,  height);
	//gi_resize_window(drawArea,  width,  height);
  }
}



void GPDFCore::hScrollChangeCbk( void* ptr,
			     void* callData) {
  GPDFCore *core = (GPDFCore *)ptr;
  int value = (int)callData;

  core->scrollTo(value, core->scrollY);
}

void GPDFCore::hScrollDragCbk( void* ptr,
			      void* callData) {
  GPDFCore *core = (GPDFCore *)ptr;
  int value = (int)callData;

  core->scrollTo(value, core->scrollY);
}

void GPDFCore::vScrollChangeCbk( void* ptr,
			     void* callData) {
  GPDFCore *core = (GPDFCore *)ptr;
  int value = (int)callData;

  core->scrollTo(core->scrollX, value);
}

void GPDFCore::vScrollDragCbk( void* ptr,
			      void* callData) {
  GPDFCore *core = (GPDFCore *)ptr;
  int value = (int)callData;

  core->scrollTo(core->scrollX, value);
}

void GPDFCore::resizeCbk( void* ptr, void* callData) {
  GPDFCore *core = (GPDFCore *)ptr;
  gi_msg_t* event=(gi_msg_t*)callData;
  int x1, y1;
  Guint w1, h1, bw1, depth1;
  int n;
  int w, h;
  int sx, sy;
  StkWindow *stkwindow ;
  int yoffset;

  if (event->params[0] != GI_STRUCT_CHANGE_RESIZE)
	  return;

  n = 0;

  event->height -= DRAWABLE_NAVIGATION_HEIGHT;
  yoffset = event->height - core->drawAreaHeight;
 
  core->drawAreaWidth = (int)event->width;
  core->drawAreaHeight = (int)event->height; 

  stkwindow = ::stk_find_window(core->drawArea);
  if (stkwindow)
  {
  stk_move_widgets(stkwindow,0,yoffset);
  }

  if (core->zoom == zoomPage || core->zoom == zoomWidth) {
    sx = sy = -1;
  } else {
    sx = core->scrollX;
    sy = core->scrollY;
  }
  core->update(core->topPage, sx, sy, core->zoom, core->rotate, gTrue, gFalse);
}


void GPDFCore::redrawCbk( void* ptr, void* callData) {
  GPDFCore *core = (GPDFCore *)ptr;
  gi_msg_t* event=(gi_msg_t*)callData;
  int x, y, w, h;

    x = event->x;
    y = event->y;
    w = event->width;
    h = event->height;

	if (y + h > core->drawAreaHeight)
	{
		h =  (core->drawAreaHeight - y);
	}	

  core->redrawWindow(x, y, w, h, gFalse);
}

void GPDFCore::inputCbk( void* ptr, void* callData) {
  GPDFCore *core = (GPDFCore *)ptr;
  gi_msg_t* event=(gi_msg_t*)callData;
 // XmDrawingAreaCallbackStruct *data = (XmDrawingAreaCallbackStruct *)callData;
  LinkAction *action;
  int pg, x, y;
  double xu, yu;
  char *s;
  //KeySym key;
  GBool ok;

  switch (event->type) {

  case GI_MSG_BUTTON_DOWN:
    if (*core->mouseCbk) {
      (*core->mouseCbk)(core->mouseCbkData, event);
    }
    break;
  case GI_MSG_BUTTON_UP:
    if (*core->mouseCbk) {
      (*core->mouseCbk)(core->mouseCbkData, event);
    }
    break;
  case GI_MSG_MOUSE_MOVE:
    if (core->doc && core->doc->getNumPages() > 0) {
      ok = core->cvtWindowToDev(event->x, event->y,
				&pg, &x, &y);
      if (core->dragging) {
	if (ok) {
	  core->moveSelection(pg, x, y);
	}
      } else if (core->hyperlinksEnabled) {
	core->cvtDevToUser(pg, x, y, &xu, &yu);
	if (ok && (action = core->findLink(pg, xu, yu))) {
	  core->setCursor(core->linkCursor);
	  if (action != core->linkAction) {
	    core->linkAction = action;
	    if (core->updateCbk) {
	      s = "";
	      switch (action->getKind()) {
	      case actionGoTo:
		s = "[internal link]";
		break;
	      case actionGoToR:
		s = ((LinkGoToR *)action)->getFileName()->getCString();
		break;
	      case actionLaunch:
		s = ((LinkLaunch *)action)->getFileName()->getCString();
		break;
	      case actionURI:
		s = ((LinkURI *)action)->getURI()->getCString();
		break;
	      case actionNamed:
		s = ((LinkNamed *)action)->getName()->getCString();
		break;
	      case actionMovie:
		s = "[movie]";
		break;
	      case actionUnknown:
		s = "[unknown link]";
		break;
	      }
	      (*core->updateCbk)(core->updateCbkData, NULL, -1, -1, s);
	    }
	  }
	} else {
	  core->setCursor(0);
	  if (core->linkAction) {
	    core->linkAction = NULL;
	    if (core->updateCbk) {
	      (*core->updateCbk)(core->updateCbkData, NULL, -1, -1, "");
	    }
	  }
	}
      }
    }
    if (core->panning) {
      core->scrollTo(core->scrollX - (event->x - core->panMX),
		     core->scrollY - (event->y - core->panMY));
      core->panMX = event->x;
      core->panMY = event->y;
    }
    break;
  case GI_MSG_KEY_DOWN:
    if (core->keyPressCbk) {
      int keycode =event->params[4];
      (*core->keyPressCbk)(core->keyPressCbkData,
			   keycode, event->params[3], event);
    }
    break;
  }
}

PDFCoreTile *GPDFCore::newTile(int xDestA, int yDestA) {
  return new GPDFCoreTile(xDestA, yDestA);
}


// Divide a 16-bit value (in [0, 255*255]) by 255, returning an 8-bit result.
static inline Guchar div255(int x) {
  return (Guchar)((x + (x >> 8) + 0x80) >> 8);
}

void GPDFCore::setBusyCursor(GBool busy) {
  //setCursor(busy ? busyCursor : None);
}

void GPDFCore::updateTileData(PDFCoreTile *tileA, int xSrc, int ySrc,
			      int width, int height, GBool composited) {
  GPDFCoreTile *tile = (GPDFCoreTile *)tileA;
  gi_image_t *image;
  SplashColorPtr dataPtr, p;
  Gulong pixel;
  Guchar *ap;
  Guchar alpha, alpha1;
  int w, h, bw, x, y, r, g, b, gray;
  int *errDownR, *errDownG, *errDownB;
  int errRightR, errRightG, errRightB;
  int errDownRightR, errDownRightG, errDownRightB;
  int r0, g0, b0, re, ge, be;

  if (!tile->image) {
    w = tile->xMax - tile->xMin;
    h = tile->yMax - tile->yMin;
    image = gi_create_image_depth(  w, h, (gi_format_code_t)format);
    tile->image = image;
  } else {
    image = (gi_image_t *)tile->image;
  }

  //~ optimize for known gi_image_t formats
  bw = tile->bitmap->getRowSize();
  dataPtr = tile->bitmap->getDataPtr();

  if (trueColor) {
    for (y = 0; y < height; ++y) {
      p = dataPtr + (ySrc + y) * bw + xSrc * 3;
      if (!composited && tile->bitmap->getAlphaPtr()) {
	ap = tile->bitmap->getAlphaPtr() +
	       (ySrc + y) * tile->bitmap->getWidth() + xSrc;
      } else {
	ap = NULL;
      }
      for (x = 0; x < width; ++x) {
	r = splashRGB8R(p);
	g = splashRGB8G(p);
	b = splashRGB8B(p);
	if (ap) {
	  alpha = *ap++;
	  alpha1 = 255 - alpha;
	  r = div255(alpha1 * paperColor[0] + alpha * r);
	  g = div255(alpha1 * paperColor[1] + alpha * g);
	  b = div255(alpha1 * paperColor[2] + alpha * b);
	}
	r >>= rDiv;
	g >>= gDiv;
	b >>= bDiv;
	pixel = ((Gulong)r << rShift) +
	        ((Gulong)g << gShift) +
	        ((Gulong)b << bShift);
	gi_image_putpixel(image, xSrc + x, ySrc + y, pixel);
	p += 3;
      }
    }
  } else if (rgbCubeSize == 1) {
    //~ this should really use splashModeMono, with non-clustered dithering
    for (y = 0; y < height; ++y) {
      p = dataPtr + (ySrc + y) * bw + xSrc * 3;
      if (!composited && tile->bitmap->getAlphaPtr()) {
	ap = tile->bitmap->getAlphaPtr() +
	       (ySrc + y) * tile->bitmap->getWidth() + xSrc;
      } else {
	ap = NULL;
      }
      for (x = 0; x < width; ++x) {
	r = splashRGB8R(p);
	g = splashRGB8G(p);
	b = splashRGB8B(p);
	if (ap) {
	  alpha = *ap++;
	  alpha1 = 255 - alpha;
	  r = div255(alpha1 * paperColor[0] + alpha * r);
	  g = div255(alpha1 * paperColor[1] + alpha * g);
	  b = div255(alpha1 * paperColor[2] + alpha * b);
	}
	gray = (int)(0.299 * r + 0.587 * g + 0.114 * b + 0.5);
	if (gray < 128) {
	  pixel = colors[0];
	} else {
	  pixel = colors[1];
	}
	gi_image_putpixel(image, xSrc + x, ySrc + y, pixel);
	p += 3;
      }
    }
  } else {
    // do Floyd-Steinberg dithering on the whole bitmap
    errDownR = (int *)gmallocn(width + 2, sizeof(int));
    errDownG = (int *)gmallocn(width + 2, sizeof(int));
    errDownB = (int *)gmallocn(width + 2, sizeof(int));
    errRightR = errRightG = errRightB = 0;
    errDownRightR = errDownRightG = errDownRightB = 0;
    memset(errDownR, 0, (width + 2) * sizeof(int));
    memset(errDownG, 0, (width + 2) * sizeof(int));
    memset(errDownB, 0, (width + 2) * sizeof(int));
    for (y = 0; y < height; ++y) {
      p = dataPtr + (ySrc + y) * bw + xSrc * 3;
      if (!composited && tile->bitmap->getAlphaPtr()) {
	ap = tile->bitmap->getAlphaPtr() +
	       (ySrc + y) * tile->bitmap->getWidth() + xSrc;
      } else {
	ap = NULL;
      }
      for (x = 0; x < width; ++x) {
	r = splashRGB8R(p);
	g = splashRGB8G(p);
	b = splashRGB8B(p);
	if (ap) {
	  alpha = *ap++;
	  alpha1 = 255 - alpha;
	  r = div255(alpha1 * paperColor[0] + alpha * r);
	  g = div255(alpha1 * paperColor[1] + alpha * g);
	  b = div255(alpha1 * paperColor[2] + alpha * b);
	}
	r0 = r + errRightR + errDownR[x+1];
	g0 = g + errRightG + errDownG[x+1];
	b0 = b + errRightB + errDownB[x+1];
	if (r0 < 0) {
	  r = 0;
	} else if (r0 >= 255) {
	  r = rgbCubeSize - 1;
	} else {
	  r = div255(r0 * (rgbCubeSize - 1));
	}
	if (g0 < 0) {
	  g = 0;
	} else if (g0 >= 255) {
	  g = rgbCubeSize - 1;
	} else {
	  g = div255(g0 * (rgbCubeSize - 1));
	}
	if (b0 < 0) {
	  b = 0;
	} else if (b0 >= 255) {
	  b = rgbCubeSize - 1;
	} else {
	  b = div255(b0 * (rgbCubeSize - 1));
	}
	re = r0 - ((r << 8) - r) / (rgbCubeSize - 1);
	ge = g0 - ((g << 8) - g) / (rgbCubeSize - 1);
	be = b0 - ((b << 8) - b) / (rgbCubeSize - 1);
	errRightR = (re * 7) >> 4;
	errRightG = (ge * 7) >> 4;
	errRightB = (be * 7) >> 4;
	errDownR[x] += (re * 3) >> 4;
	errDownG[x] += (ge * 3) >> 4;
	errDownB[x] += (be * 3) >> 4;
	errDownR[x+1] = ((re * 5) >> 4) + errDownRightR;
	errDownG[x+1] = ((ge * 5) >> 4) + errDownRightG;
	errDownB[x+1] = ((be * 5) >> 4) + errDownRightB;
	errDownRightR = re >> 4;
	errDownRightG = ge >> 4;
	errDownRightB = be >> 4;
	pixel = colors[(r * rgbCubeSize + g) * rgbCubeSize + b];
	gi_image_putpixel(image, xSrc + x, ySrc + y, pixel);
	p += 3;
      }
    }
    gfree(errDownR);
    gfree(errDownG);
    gfree(errDownB);
  }
}


void GPDFCore::redrawRect(PDFCoreTile *tileA, int xSrc, int ySrc,
			  int xDest, int yDest, int width, int height,
			  GBool composited) {
  GPDFCoreTile *tile = (GPDFCoreTile *)tileA;
  //Window drawAreaWin;
 // XGCValues gcValues;

  // create a GC for the drawing area
  //drawAreaWin = XtWindow(drawArea);
  if (!drawAreaGC) {
    //gcValues.foreground = mattePixel;
    drawAreaGC = gi_create_gc( drawArea, NULL);
	gi_set_gc_foreground_pixel( drawAreaGC, mattePixel);
  }

  // draw the document
  if (tile) {
	 // printf("%s got line %d\n",__FUNCTION__,__LINE__);
    gi_bitblt_bitmap(  drawAreaGC,xSrc, ySrc,width, height, tile->image, xDest, yDest );

  // draw the background
  } else {
	 // printf("%s got line %d\n",__FUNCTION__,__LINE__);
    gi_fill_rect(  drawAreaGC,	   xDest, yDest, width, height);
  }
}

void GPDFCore::updateScrollbars() {
  int n;
  int maxPos;

  if (pages->getLength() > 0) {
    if (continuousMode) {
      maxPos = maxPageW;
    } else {
      maxPos = ((PDFCorePage *)pages->get(0))->w;
    }
  } else {
    maxPos = 1;
  }
  if (maxPos < drawAreaWidth) {
    maxPos = drawAreaWidth;
  }
  n = 0;
  //XtSetArg(args[n], XmNvalue, scrollX); ++n;
  //XtSetArg(args[n], XmNmaximum, maxPos); ++n;
  //XtSetArg(args[n], XmNsliderSize, drawAreaWidth); ++n;
  //XtSetArg(args[n], XmNincrement, 16); ++n;
  //XtSetArg(args[n], XmNpageIncrement, drawAreaWidth); ++n;
  //XtSetValues(hScrollBar, args, n);

  if (pages->getLength() > 0) {
    if (continuousMode) {
      maxPos = totalDocH;
    } else {
      maxPos = ((PDFCorePage *)pages->get(0))->h;
    }
  } else {
    maxPos = 1;
  }
  if (maxPos < drawAreaHeight) {
    maxPos = drawAreaHeight;
  }
  n = 0;
  //XtSetArg(args[n], XmNvalue, scrollY); ++n;
  //XtSetArg(args[n], XmNmaximum, maxPos); ++n;
  //XtSetArg(args[n], XmNsliderSize, drawAreaHeight); ++n;
  //XtSetArg(args[n], XmNincrement, 16); ++n;
  //XtSetArg(args[n], XmNpageIncrement, drawAreaHeight); ++n;
  //XtSetValues(vScrollBar, args, n);
}

void GPDFCore::setCursor(int cursor) {
  //Window topWin;

  if (cursor == currentCursor) {
    return;
  }
  //if (!(topWin = XtWindow(shell))) {
  //  return;
  //}
  if (cursor == 0) {
	  gi_load_cursor(drawArea,GI_CURSOR_ARROW);
    //XUndefineCursor(display, topWin);
  } else {
	  gi_load_cursor(drawArea,cursor);
    //XDefineCursor(display, topWin, cursor);
  }
 // XFlush(display);
  currentCursor = cursor;
}

void GPDFCore::doAction(LinkAction *action) {
  LinkActionKind kind;
  LinkDest *dest;
  GString *namedDest;
  char *s;
  GString *fileName, *fileName2;
  GString *cmd;
  GString *actionName;
  Object movieAnnot, obj1, obj2;
  GString *msg;
  int i;

  switch (kind = action->getKind()) {

  // GoTo / GoToR action
  case actionGoTo:
  case actionGoToR:
    if (kind == actionGoTo) {
      dest = NULL;
      namedDest = NULL;
      if ((dest = ((LinkGoTo *)action)->getDest())) {
	dest = dest->copy();
      } else if ((namedDest = ((LinkGoTo *)action)->getNamedDest())) {
	namedDest = namedDest->copy();
      }
    } else {
      dest = NULL;
      namedDest = NULL;
      if ((dest = ((LinkGoToR *)action)->getDest())) {
	dest = dest->copy();
      } else if ((namedDest = ((LinkGoToR *)action)->getNamedDest())) {
	namedDest = namedDest->copy();
      }
      s = ((LinkGoToR *)action)->getFileName()->getCString();
      //~ translate path name for VMS (deal with '/')
      if (isAbsolutePath(s)) {
	fileName = new GString(s);
      } else {
	fileName = appendToPath(grabPath(doc->getFileName()->getCString()), s);
      }
      if (loadFile(fileName) != errNone) {
	if (dest) {
	  delete dest;
	}
	if (namedDest) {
	  delete namedDest;
	}
	delete fileName;
	return;
      }
      delete fileName;
    }
    if (namedDest) {
      dest = doc->findDest(namedDest);
      delete namedDest;
    }
    if (dest) {
      displayDest(dest, zoom, rotate, gTrue);
      delete dest;
    } else {
      if (kind == actionGoToR) {
	displayPage(1, zoom, 0, gFalse, gTrue);
      }
    }
    break;

  // Launch action
  case actionLaunch:
    fileName = ((LinkLaunch *)action)->getFileName();
    s = fileName->getCString();
    if (!strcmp(s + fileName->getLength() - 4, ".pdf") ||
	!strcmp(s + fileName->getLength() - 4, ".PDF")) {
      //~ translate path name for VMS (deal with '/')
      if (isAbsolutePath(s)) {
	fileName = fileName->copy();
      } else {
	fileName = appendToPath(grabPath(doc->getFileName()->getCString()), s);
      }
      if (loadFile(fileName) != errNone) {
	delete fileName;
	return;
      }
      delete fileName;
      displayPage(1, zoom, rotate, gFalse, gTrue);
    } else {
      fileName = fileName->copy();
      if (((LinkLaunch *)action)->getParams()) {
	fileName->append(' ');
	fileName->append(((LinkLaunch *)action)->getParams());
      }
#ifdef VMS
      fileName->insert(0, "spawn/nowait ");
#elif defined(__EMX__)
      fileName->insert(0, "start /min /n ");
#else
      fileName->append(" &");
#endif
      msg = new GString("About to execute the command:\n");
      msg->append(fileName);
      //if (doQuestionDialog("Launching external application", msg)) {
	//system(fileName->getCString());
     // }
      delete fileName;
      delete msg;
    }
    break;

  // URI action
  case actionURI:
    if (!(cmd = globalParams->getURLCommand())) {
      error(-1, "No urlCommand defined in config file");
      break;
    }
    runCommand(cmd, ((LinkURI *)action)->getURI());
    break;

  // Named action
  case actionNamed:
    actionName = ((LinkNamed *)action)->getName();
    if (!actionName->cmp("NextPage")) {
      gotoNextPage(1, gTrue);
    } else if (!actionName->cmp("PrevPage")) {
      gotoPrevPage(1, gTrue, gFalse);
    } else if (!actionName->cmp("FirstPage")) {
      if (topPage != 1) {
	displayPage(1, zoom, rotate, gTrue, gTrue);
      }
    } else if (!actionName->cmp("LastPage")) {
      if (topPage != doc->getNumPages()) {
	displayPage(doc->getNumPages(), zoom, rotate, gTrue, gTrue);
      }
    } else if (!actionName->cmp("GoBack")) {
      goBackward();
    } else if (!actionName->cmp("GoForward")) {
      goForward();
    } else if (!actionName->cmp("Quit")) {
      if (actionCbk) {
	(*actionCbk)(actionCbkData, actionName->getCString());
      }
    } else {
      error(-1, "Unknown named action: '%s'", actionName->getCString());
    }
    break;

  // Movie action
  case actionMovie:
    if (!(cmd = globalParams->getMovieCommand())) {
      error(-1, "No movieCommand defined in config file");
      break;
    }
    if (((LinkMovie *)action)->hasAnnotRef()) {
      doc->getXRef()->fetch(((LinkMovie *)action)->getAnnotRef()->num,
			    ((LinkMovie *)action)->getAnnotRef()->gen,
			    &movieAnnot);
    } else {
      //~ need to use the correct page num here
      doc->getCatalog()->getPage(topPage)->getAnnots(&obj1);
      if (obj1.isArray()) {
	for (i = 0; i < obj1.arrayGetLength(); ++i) {
	  if (obj1.arrayGet(i, &movieAnnot)->isDict()) {
	    if (movieAnnot.dictLookup("Subtype", &obj2)->isName("Movie")) {
	      obj2.free();
	      break;
	    }
	    obj2.free();
	  }
	  movieAnnot.free();
	}
	obj1.free();
      }
    }
    if (movieAnnot.isDict()) {
      if (movieAnnot.dictLookup("Movie", &obj1)->isDict()) {
	if (obj1.dictLookup("F", &obj2)) {
	  if ((fileName = LinkAction::getFileSpecName(&obj2))) {
	    if (!isAbsolutePath(fileName->getCString())) {
	      fileName2 = appendToPath(
			      grabPath(doc->getFileName()->getCString()),
			      fileName->getCString());
	      delete fileName;
	      fileName = fileName2;
	    }
	    runCommand(cmd, fileName);
	    delete fileName;
	  }
	  obj2.free();
	}
	obj1.free();
      }
    }
    movieAnnot.free();
    break;

  // unknown action type
  case actionUnknown:
    error(-1, "Unknown link action type: '%s'",
	  ((LinkUnknown *)action)->getAction()->getCString());
    break;
  }
}

// Run a command, given a <cmdFmt> string with one '%s' in it, and an
// <arg> string to insert in place of the '%s'.
void GPDFCore::runCommand(GString *cmdFmt, GString *arg) {
  GString *cmd;
  char *s;

  if ((s = strstr(cmdFmt->getCString(), "%s"))) {
    cmd = mungeURL(arg);
    cmd->insert(0, cmdFmt->getCString(),
		s - cmdFmt->getCString());
    cmd->append(s + 2);
  } else {
    cmd = cmdFmt->copy();
  }
#ifdef VMS
  cmd->insert(0, "spawn/nowait ");
#elif defined(__EMX__)
  cmd->insert(0, "start /min /n ");
#else
  cmd->append(" &");
#endif
  system(cmd->getCString());
  delete cmd;
}


// Escape any characters in a URL which might cause problems when
// calling system().
GString *GPDFCore::mungeURL(GString *url) {
  static char *allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "abcdefghijklmnopqrstuvwxyz"
                         "0123456789"
                         "-_.~/?:@&=+,#%";
  GString *newURL;
  char c;
  char buf[4];
  int i;

  newURL = new GString();
  for (i = 0; i < url->getLength(); ++i) {
    c = url->getChar(i);
    if (strchr(allowed, c)) {
      newURL->append(c);
    } else {
      sprintf(buf, "%%%02x", c & 0xff);
      newURL->append(buf);
    }
  }
  return newURL;
}

//------------------------------------------------------------------------
// find
//------------------------------------------------------------------------

GBool GPDFCore::find(char *s, GBool caseSensitive,
		     GBool next, GBool backward, GBool onePageOnly) {
  if (!PDFCore::find(s, caseSensitive, next, backward, onePageOnly)) {
    gi_beep( 0,50);
    return gFalse;
  }
#ifndef NO_TEXT_SELECT
  copySelection();
#endif
  return gTrue;
}

GBool GPDFCore::findU(Unicode *u, int len, GBool caseSensitive,
		      GBool next, GBool backward, GBool onePageOnly) {
  if (!PDFCore::findU(u, len, caseSensitive, next, backward, onePageOnly)) {
    gi_beep( 0,50);
    return gFalse;
  }
#ifndef NO_TEXT_SELECT
  copySelection();
#endif
  return gTrue;
}
GBool GPDFCore::checkForNewFile() {
  time_t newModTime;

  if (doc->getFileName()) {
    newModTime = getModTime(doc->getFileName()->getCString());
    if (newModTime != modTime) {
      modTime = newModTime;
      return gTrue;
    }
  }
  return gFalse;
}

//------------------------------------------------------------------------
// page/position changes
//------------------------------------------------------------------------

GBool GPDFCore::gotoNextPage(int inc, GBool top) {
  if (!PDFCore::gotoNextPage(inc, top)) {
    gi_beep( 0,50);
    return gFalse;
  }
  return gTrue;
}

GBool GPDFCore::gotoPrevPage(int dec, GBool top, GBool bottom) {
  if (!PDFCore::gotoPrevPage(dec, top, bottom)) {
    gi_beep( 0,50);
    return gFalse;
  }
  return gTrue;
}

GBool GPDFCore::goForward() {
  if (!PDFCore::goForward()) {
    gi_beep( 0,50);
    return gFalse;
  }
  return gTrue;
}

GBool GPDFCore::goBackward() {
  if (!PDFCore::goBackward()) {
    gi_beep( 0,50);
    return gFalse;
  }
  return gTrue;
}
