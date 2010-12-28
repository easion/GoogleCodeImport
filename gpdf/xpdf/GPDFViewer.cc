
//========================================================================
//
// GPDFViewer.cc
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "gmem.h"
#include "gfile.h"
#include "GString.h"
#include "GList.h"
#include "Error.h"
#include "GlobalParams.h"
#include "PDFDoc.h"
#include "Link.h"
#include "ErrorCodes.h"
#include "Outline.h"
#include "UnicodeMap.h"

#include "GPDFApp.h"
#include "PSOutputDev.h"
#include "config.h"


#define IDC_OPENFILE 100
#define IDC_GO_NEXT 101
#define IDC_GO_PREV 102
#define IDC_ZOOM_IN 103
#define IDC_ZOOM_OUT 104
#define IDC_TEXT_PAGES 105
#define IDC_ZOOM_VALUE 106
#define IDC_FULL_SCR 108
#define IDC_GO_BACK 109
#define IDC_GO_FORWARD 110
#define IDC_DO_COPY 111
#define IDC_DO_PASTE 112
#define IDC_SEARCH_TEXT 113
#define IDC_SEARCH 114
#define IDC_TEXT_LINK 115
#define IDC_TEXTBOX_PAGE 116

static stk_popmenu_t *pdf_menu;


GPDFViewer::GPDFViewer(GPDFApp *appA, GString *fileName,
		       int pageA, GString *destName, GBool fullScreen,
		       GString *ownerPassword, GString *userPassword) {
  LinkDest *dest;
  int pg;
  double z;

  app = appA;
  //win = NULL;
  core = NULL;
  ok = gFalse;

  initWindow(fullScreen);


  // do Motif-specific initialization and create the window;
  // this also creates the core object
 
  //openDialog = NULL;
  //saveAsDialog = NULL;

  dest = NULL; // make gcc happy
  pg = pageA; // make gcc happy

  if (fileName) {
    if (loadFile(fileName, ownerPassword, userPassword)) {
      getPageAndDest(pageA, destName, &pg, &dest);

    } else {
      return;
    }
  }
  core->resizeToPage(pg);

  // map the window -- we do this after calling resizeToPage to avoid
  // an annoying on-screen resize
  mapWindow();

  // display the first page
  z = core->getZoom();
  if (dest) {
    displayDest(dest, z, core->getRotate(), gTrue);
    delete dest;
  } else {
    displayPage(pg, z, core->getRotate(), gTrue, gTrue);
  }

  ok = gTrue;
}

void GPDFViewer::closeWindow() {
  //XtPopdown(win);
 gi_destroy_window(win);
}

void GPDFViewer::mapWindow() {
	gi_show_window(win);
}

GPDFViewer::GPDFViewer(GPDFApp *appA, PDFDoc *doc, int pageA,
		       GString *destName, GBool fullScreen) {
  LinkDest *dest;
  int pg;
  double z;

  app = appA;
  //win = NULL;
  core = NULL;
  ok = gFalse;

  
  //openDialog = NULL;
  //saveAsDialog = NULL;

  dest = NULL; // make gcc happy
  pg = pageA; // make gcc happy

  if (doc) {
    core->loadDoc(doc);
    getPageAndDest(pageA, destName, &pg, &dest);

  }
  core->resizeToPage(pg);

  // map the window -- we do this after calling resizeToPage to avoid
  // an annoying on-screen resize
  mapWindow();

  // display the first page
  z = core->getZoom();
  if (dest) {
    displayDest(dest, z, core->getRotate(), gTrue);
    delete dest;
  } else {
    displayPage(pg, z, core->getRotate(), gTrue, gTrue);
  }

  ok = gTrue;
}

GPDFViewer::~GPDFViewer() {
  delete core;
 
  closeWindow();

}

void GPDFViewer::open(GString *fileName, int pageA, GString *destName) {
  LinkDest *dest;
  int pg;
  double z;

  if (!core->getDoc() || fileName->cmp(core->getDoc()->getFileName())) {
    if (!loadFile(fileName, NULL, NULL)) {
      return;
    }
  }
  getPageAndDest(pageA, destName, &pg, &dest);
  z = core->getZoom();
  if (dest) {
    displayDest(dest, z, core->getRotate(), gTrue);
    delete dest;
  } else {
    displayPage(pg, z, core->getRotate(), gTrue, gTrue);
  }
}

void GPDFViewer::clear() {
  char *title;
  //XmString s;

  core->clear();

  // set up title, number-of-pages display
  title = app->getTitle() ? app->getTitle()->getCString()
                          : (char *)xpdfAppName;
 

}

//------------------------------------------------------------------------
// load / display
//------------------------------------------------------------------------

GBool GPDFViewer::loadFile(GString *fileName, GString *ownerPassword,
			   GString *userPassword) {
  return core->loadFile(fileName, ownerPassword, userPassword) == errNone;
}

void GPDFViewer::reloadFile() {
  int pg;

  if (!core->getDoc()) {
    return;
  }
  pg = core->getPageNum();
  loadFile(core->getDoc()->getFileName());
  if (pg > core->getDoc()->getNumPages()) {
    pg = core->getDoc()->getNumPages();
  }
  displayPage(pg, core->getZoom(), core->getRotate(), gFalse, gFalse);
}

void GPDFViewer::displayPage(int pageA, double zoomA, int rotateA,
			     GBool scrollToTop, GBool addToHist) {
  core->displayPage(pageA, zoomA, rotateA, scrollToTop, addToHist);
}

void GPDFViewer::displayDest(LinkDest *dest, double zoomA, int rotateA,
			     GBool addToHist) {
  core->displayDest(dest, zoomA, rotateA, addToHist);
}


int GPDFViewer::getModifiers(Guint modifiers) {
  int mods;

  mods = 0;
  if (modifiers & (G_MODIFIERS_LSHIFT|G_MODIFIERS_RSHIFT)) {
    mods |= xpdfKeyModShift;
  }
  if (modifiers & (G_MODIFIERS_LCTRL|G_MODIFIERS_RCTRL)) {
    mods |= xpdfKeyModCtrl;
  }
  if (modifiers & (G_MODIFIERS_LALT|G_MODIFIERS_RALT)) {
    mods |= xpdfKeyModAlt;
  }
  return mods;
}

int GPDFViewer::getContext(Guint modifiers) {
  int context;

  context = (core->getFullScreen() ? xpdfKeyContextFullScreen
                                   : xpdfKeyContextWindow) |
            (core->getContinuousMode() ? xpdfKeyContextContinuous
                                       : xpdfKeyContextSinglePage) |
            (core->getLinkAction() ? xpdfKeyContextOverLink
                                   : xpdfKeyContextOffLink) |
            ((modifiers & G_MODIFIERS_SCRLOCK) ? xpdfKeyContextScrLockOn
	                            : xpdfKeyContextScrLockOff);
  return context;
}


void GPDFViewer::getPageAndDest(int pageA, GString *destName,
				int *pageOut, LinkDest **destOut) {
  Ref pageRef;

  // find the page number for a named destination
  *pageOut = pageA;
  *destOut = NULL;
  if (destName && (*destOut = core->getDoc()->findDest(destName))) {
    if ((*destOut)->isPageRef()) {
      pageRef = (*destOut)->getPageRef();
      *pageOut = core->getDoc()->findPage(pageRef.num, pageRef.gen);
    } else {
      *pageOut = (*destOut)->getPageNum();
    }
  }

  if (*pageOut <= 0) {
    *pageOut = 1;
  }
  if (*pageOut > core->getDoc()->getNumPages()) {
    *pageOut = core->getDoc()->getNumPages();
  }
}


void GPDFViewer::prevPageCbk( void* ptr,
			     void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->core->gotoPrevPage(1, gTrue, gFalse);
  viewer->core->takeFocus();
}

void GPDFViewer::prevTenPageCbk( void* ptr,
				void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->core->gotoPrevPage(10, gTrue, gFalse);
  viewer->core->takeFocus();
}

void GPDFViewer::nextPageCbk( void* ptr,
			     void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->core->gotoNextPage(1, gTrue);
  viewer->core->takeFocus();
}

void GPDFViewer::nextTenPageCbk( void* ptr,
				void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->core->gotoNextPage(10, gTrue);
  viewer->core->takeFocus();
}

void GPDFViewer::backCbk( void* ptr,
			 void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->core->goBackward();
  viewer->core->takeFocus();
}

void GPDFViewer::forwardCbk( void* ptr,
			    void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->core->goForward();
  viewer->core->takeFocus();
}

void GPDFViewer::mouseCbk(void *data, gi_msg_t *event) {
GPDFViewer *viewer = (GPDFViewer *)data;
  int keyCode;
  GList *cmds;
  int i;
  int btn,flbtn;
  long state=0;

  if (event->type == GI_MSG_BUTTON_DOWN) {

	  btn = event->params[2];
    
	if (btn & GI_BUTTON_L) {
		flbtn=1;
	}
  if (btn & GI_BUTTON_M) {
	  flbtn=2;
  }
  if (btn & GI_BUTTON_R)
	  {
	  flbtn=3;
	  stk_menu_popup(event->params[0],event->params[1],pdf_menu);
	  }
	  if (btn & GI_BUTTON_WHEEL_DOWN)
	  {
		  //printf("got GI_BUTTON_WHEEL_DOWN\n");
	  flbtn=5;
	  }
	  if (btn & GI_BUTTON_WHEEL_UP)
	  {
		  //printf("got GI_BUTTON_WHEEL_DOWN\n");
	  flbtn=4;
	  }


    if (flbtn >= 1 && flbtn <= 7) {
      keyCode = xpdfKeyCodeMousePress1 + flbtn - 1;
    } else {
      return;
    }
  } else if (event->type == GI_MSG_BUTTON_UP) {
	   btn = event->params[3];
	if (btn & GI_BUTTON_L) {
		flbtn=1;
	}
  if (btn & GI_BUTTON_M) {
	  flbtn=2;
  }
  if (btn & GI_BUTTON_R)
	  {
	  flbtn=3;
	  }	 

    if (flbtn >= 1 && flbtn <= 7) {
      keyCode = xpdfKeyCodeMouseRelease1 + flbtn - 1;
    } else {
      return;
    }
  } 
  /*else if (event->type == GI_MSG_WHEEL) {
	  if (event->params[4]==1)
	  {
		  keyCode = xpdfKeyCodeMousePress4;
	  }
	  else{
		keyCode = xpdfKeyCodeMousePress5;
	  }
  }*/
  else {
    return;
  }

   //printf("%s got line %d\n",__FUNCTION__,__LINE__);

  if ((cmds = globalParams->getKeyBinding(keyCode,
					  viewer->getModifiers(
						      state),
					  viewer->getContext(
						      state)))) {
    for (i = 0; i < cmds->getLength(); ++i) {
      viewer->execCmd((GString *)cmds->get(i), event);
    }
    deleteGList(cmds, GString);
  }
	//printf("%s got line %d\n",__FUNCTION__,__LINE__);
}

void GPDFViewer::actionCbk(void *data, char *action) {
  GPDFViewer *viewer = (GPDFViewer *)data;

  //printf("%s got line %d\n",__FUNCTION__,__LINE__);

  if (!strcmp(action, "Quit")) {
    viewer->app->quit();
  }
}


#define cmdMaxArgs 2


#define maxZoomIdx   0
#define defZoomIdx   3
#define minZoomIdx   7
#define zoomPageIdx  8
#define zoomWidthIdx 9


struct ZoomMenuInfo {
  char *label;
  double zoom;
};

static ZoomMenuInfo zoomMenuInfo[nZoomMenuItems] = {
  { "400%",      400 },
  { "200%",      200 },
  { "150%",      150 },
  { "125%",      125 },
  { "100%",      100 },
  { "fit width", zoomWidth },
  { "fit page",  zoomPage },
  { "50%",        50 },
  { "25%",        25 },
  { "12.5%",      12.5 }
};

static int zoom_idx;
XPDFViewerCmd GPDFViewer::cmdTab[] = {
#if 0
#endif
  { "about",                   0, gFalse, gFalse, &GPDFViewer::cmdAbout },
  { "closeOutline",            0, gFalse, gFalse, &GPDFViewer::cmdCloseOutline },
  { "closeWindow",             0, gFalse, gFalse, &GPDFViewer::cmdCloseWindow },
  { "continuousMode",          0, gFalse, gFalse, &GPDFViewer::cmdContinuousMode },
  { "endPan",                  0, gTrue,  gTrue,  &GPDFViewer::cmdEndPan },
  { "endSelection",            0, gTrue,  gTrue,  &GPDFViewer::cmdEndSelection },
  { "find",                    0, gTrue,  gFalse, &GPDFViewer::cmdFind },
  { "findNext",                0, gTrue,  gFalse, &GPDFViewer::cmdFindNext },
  { "focusToDocWin",           0, gFalse, gFalse, &GPDFViewer::cmdFocusToDocWin },
  { "focusToPageNum",          0, gFalse, gFalse, &GPDFViewer::cmdFocusToPageNum },
  { "followLink",              0, gTrue,  gTrue,  &GPDFViewer::cmdFollowLink },
  { "followLinkInNewWin",      0, gTrue,  gTrue,  &GPDFViewer::cmdFollowLinkInNewWin },
  { "followLinkInNewWinNoSel", 0, gTrue,  gTrue,  &GPDFViewer::cmdFollowLinkInNewWinNoSel },
  { "followLinkNoSel",         0, gTrue,  gTrue,  &GPDFViewer::cmdFollowLinkNoSel },
  { "fullScreenMode",          0, gFalse, gFalse, &GPDFViewer::cmdFullScreenMode },
  { "goBackward",              0, gFalse, gFalse, &GPDFViewer::cmdGoBackward },
  { "goForward",               0, gFalse, gFalse, &GPDFViewer::cmdGoForward },
  { "gotoDest",                1, gTrue,  gFalse, &GPDFViewer::cmdGotoDest },
  { "gotoLastPage",            0, gTrue,  gFalse, &GPDFViewer::cmdGotoLastPage },
  { "gotoLastPageNoScroll",    0, gTrue,  gFalse, &GPDFViewer::cmdGotoLastPageNoScroll },
  { "gotoPage",                1, gTrue,  gFalse, &GPDFViewer::cmdGotoPage },
  { "gotoPageNoScroll",        1, gTrue,  gFalse, &GPDFViewer::cmdGotoPageNoScroll },
  { "nextPage",                0, gTrue,  gFalse, &GPDFViewer::cmdNextPage },
  { "nextPageNoScroll",        0, gTrue,  gFalse, &GPDFViewer::cmdNextPageNoScroll },
  { "open",                    0, gFalse, gFalse, &GPDFViewer::cmdOpen },
  { "openFile",                1, gFalse, gFalse, &GPDFViewer::cmdOpenFile },
  { "openFileAtDest",          2, gFalse, gFalse, &GPDFViewer::cmdOpenFileAtDest },
  { "openFileAtDestInNewWin",  2, gFalse, gFalse, &GPDFViewer::cmdOpenFileAtDestInNewWin },
  { "openFileAtPage",          2, gFalse, gFalse, &GPDFViewer::cmdOpenFileAtPage },
  { "openFileAtPageInNewWin",  2, gFalse, gFalse, &GPDFViewer::cmdOpenFileAtPageInNewWin },
  { "openFileInNewWin",        1, gFalse, gFalse, &GPDFViewer::cmdOpenFileInNewWin },
  { "openInNewWin",            0, gFalse, gFalse, &GPDFViewer::cmdOpenInNewWin },
  { "openOutline",             0, gFalse, gFalse, &GPDFViewer::cmdOpenOutline },
  { "pageDown",                0, gTrue,  gFalse, &GPDFViewer::cmdPageDown },
  { "pageUp",                  0, gTrue,  gFalse, &GPDFViewer::cmdPageUp },
  { "postPopupMenu",           0, gFalse, gTrue,  &GPDFViewer::cmdPostPopupMenu },
  { "prevPage",                0, gTrue,  gFalse, &GPDFViewer::cmdPrevPage },
  { "prevPageNoScroll",        0, gTrue,  gFalse, &GPDFViewer::cmdPrevPageNoScroll },
  { "print",                   0, gTrue,  gFalse, &GPDFViewer::cmdPrint },
  { "quit",                    0, gFalse, gFalse, &GPDFViewer::cmdQuit },
  { "raise",                   0, gFalse, gFalse, &GPDFViewer::cmdRaise },
  { "redraw",                  0, gTrue,  gFalse, &GPDFViewer::cmdRedraw },
  { "reload",                  0, gTrue,  gFalse, &GPDFViewer::cmdReload },
  { "run",                     1, gFalse, gFalse, &GPDFViewer::cmdRun },
  { "scrollDown",              1, gTrue,  gFalse, &GPDFViewer::cmdScrollDown },
  { "scrollDownNextPage",      1, gTrue,  gFalse, &GPDFViewer::cmdScrollDownNextPage },
  { "scrollLeft",              1, gTrue,  gFalse, &GPDFViewer::cmdScrollLeft },
  { "scrollOutlineDown",       1, gTrue,  gFalse, &GPDFViewer::cmdScrollOutlineDown },
  { "scrollOutlineUp",         1, gTrue,  gFalse, &GPDFViewer::cmdScrollOutlineUp },
  { "scrollRight",             1, gTrue,  gFalse, &GPDFViewer::cmdScrollRight },
  { "scrollToBottomEdge",      0, gTrue,  gFalse, &GPDFViewer::cmdScrollToBottomEdge },
  { "scrollToBottomRight",     0, gTrue,  gFalse, &GPDFViewer::cmdScrollToBottomRight },
  { "scrollToLeftEdge",        0, gTrue,  gFalse, &GPDFViewer::cmdScrollToLeftEdge },
  { "scrollToRightEdge",       0, gTrue,  gFalse, &GPDFViewer::cmdScrollToRightEdge },
  { "scrollToTopEdge",         0, gTrue,  gFalse, &GPDFViewer::cmdScrollToTopEdge },
  { "scrollToTopLeft",         0, gTrue,  gFalse, &GPDFViewer::cmdScrollToTopLeft },
  { "scrollUp",                1, gTrue,  gFalse, &GPDFViewer::cmdScrollUp },
  { "scrollUpPrevPage",        1, gTrue,  gFalse, &GPDFViewer::cmdScrollUpPrevPage },
  { "singlePageMode",          0, gFalse, gFalse, &GPDFViewer::cmdSinglePageMode },
  { "startPan",                0, gTrue,  gTrue,  &GPDFViewer::cmdStartPan },
  { "startSelection",          0, gTrue,  gTrue,  &GPDFViewer::cmdStartSelection },
  { "toggleContinuousMode",    0, gFalse, gFalse, &GPDFViewer::cmdToggleContinuousMode },
  { "toggleFullScreenMode",    0, gFalse, gFalse, &GPDFViewer::cmdToggleFullScreenMode },
  { "toggleOutline",           0, gFalse, gFalse, &GPDFViewer::cmdToggleOutline },
  { "windowMode",              0, gFalse, gFalse, &GPDFViewer::cmdWindowMode },
  { "zoomFitPage",             0, gFalse, gFalse, &GPDFViewer::cmdZoomFitPage },
  { "zoomFitWidth",            0, gFalse, gFalse, &GPDFViewer::cmdZoomFitWidth },
  { "zoomIn",                  0, gFalse, gFalse, &GPDFViewer::cmdZoomIn },
  { "zoomOut",                 0, gFalse, gFalse, &GPDFViewer::cmdZoomOut },
  { "zoomPercent",             1, gFalse, gFalse, &GPDFViewer::cmdZoomPercent },
  { "zoomToSelection",         0, gTrue,  gFalse, &GPDFViewer::cmdZoomToSelection }
};

#define nCmds (sizeof(cmdTab) / sizeof(XPDFViewerCmd))


void GPDFViewer::execCmd(GString *cmd, gi_msg_t *event) {
  GString *name;
  GString *args[cmdMaxArgs];
  char *p0, *p1;
  int nArgs, i;
  int a, b, m, cmp;

  //----- parse the command
  name = NULL;
  nArgs = 0;
  for (i = 0; i < cmdMaxArgs; ++i) {
    args[i] = NULL;
  }
  p0 = cmd->getCString();
  for (p1 = p0; *p1 && isalnum(*p1); ++p1) ;
  if (p1 == p0) {
    goto err1;
  }
  name = new GString(p0, p1 - p0);
  if (*p1 == '(') {
    while (nArgs < cmdMaxArgs) {
      p0 = p1 + 1;
      for (p1 = p0; *p1 && *p1 != ',' && *p1 != ')'; ++p1) ;
      args[nArgs++] = new GString(p0, p1 - p0);
      if (*p1 != ',') {
	break;
      }
    }
    if (*p1 != ')') {
      goto err1;
    }
    ++p1;
  }
  if (*p1) {
    goto err1;
  }
  
  //----- find the command
  a = -1;
  b = nCmds;
  // invariant: cmdTab[a].name < name < cmdTab[b].name
  while (b - a > 1) {
    m = (a + b) / 2;
    cmp = strcmp(cmdTab[m].name, name->getCString());
    if (cmp < 0) {
      a = m;
    } else if (cmp > 0) {
      b = m;
    } else {
      a = b = m;
    }
  }
  if (cmp != 0) {
    goto err1;
  }

  //----- execute the command
  if (nArgs != cmdTab[a].nArgs ||
      (cmdTab[a].requiresEvent && !event)) {
    goto err1;
  }
  if (cmdTab[a].requiresDoc && !core->getDoc()) {
    // don't issue an error message for this -- it happens, e.g., when
    // clicking in a window with no open PDF file
    goto err2;
  }
  (this->*cmdTab[a].func)(args, nArgs, event);

  //----- clean up
  delete name;
  for (i = 0; i < nArgs; ++i) {
    if (args[i]) {
      delete args[i];
    }
  }
  return;

 err1:
  error(-1, "Invalid command syntax: '%s'", cmd->getCString());
 err2:
  if (name) {
    delete name;
  }
  for (i = 0; i < nArgs; ++i) {
    if (args[i]) {
      delete args[i];
    }
  }
}


void GPDFViewer::updateCbk(void *data, GString *fileName,
			   int pageNum, int numPages, char *linkString) {
  GPDFViewer *viewer = (GPDFViewer *)data;
  GString *title;
  char buf[256];
  StkWindow *stkwindow ;
  gi_window_id_t win;


   if (fileName) {

    if (!(title = viewer->app->getTitle())) {
      title = (new GString(fileName))->append(" - ")->append("Gix PDF Reader");
    }

	gi_set_window_utf8_caption(viewer->win,title->getCString());
	//snprintf(buf,256,"%s - Gix PDF Reader",title->getCString());
	//gi_set_window_utf8_caption(viewer->win,buf);

	if (!viewer->app->getTitle()) {
      delete title;
    }

    //viewer->setupPrintDialog();
  }
	
  win = viewer->getWindow();

  stkwindow = ::stk_find_window(win);
    ////XtVaSetValues(viewer->win, XmNtitle, title->getCString(),
	//	  XmNiconName, title->getCString(), NULL);

	

	if (pageNum > 0)
	{
	snprintf(buf,256,"%d",pageNum);
	stk_set_widget_text(stk_get_widget_item(stkwindow,IDC_TEXTBOX_PAGE),buf);
	}

	if (numPages > 0)
	{
	snprintf(buf,256,"of %d",numPages);
	stk_set_widget_text(stk_get_widget_item(stkwindow,IDC_TEXT_PAGES),buf);
	}

	if (linkString) {
		stk_set_widget_text(stk_get_widget_item(stkwindow,IDC_TEXT_LINK),linkString);
      //s = XmStringCreateLocalized(linkString);
      //XtVaSetValues(viewer->linkLabel, XmNlabelString, s, NULL);
      //XmStringFree(s);
    }

    //if (!viewer->app->getTitle()) {
     // delete title;
    //}
#ifndef DISABLE_OUTLINE
    //viewer->setupOutline();
#endif
   // viewer->setupPrintDialog();


}

void GPDFViewer::keyPressCbk(void *data, int key, Guint modifiers,
			     gi_msg_t *event) {
GPDFViewer *viewer = (GPDFViewer *)data;
  int keyCode;
  GList *cmds;
  int i;

  if (key >= 0x20 && key <= 0xfe) {
    keyCode = (int)key;
  } else if (key == G_KEY_TAB ) {
    keyCode = xpdfKeyCodeTab;
  } else if (key == G_KEY_RETURN) {
    keyCode = xpdfKeyCodeReturn;
  } else if (key == G_KEY_RETURN) {
    keyCode = xpdfKeyCodeEnter; //fixme
  } else if (key == G_KEY_BACKSPACE) {
    keyCode = xpdfKeyCodeBackspace;
  } else if (key == G_KEY_INSERT ) {
    keyCode = xpdfKeyCodeInsert;
  } else if (key == G_KEY_DELETE ) {
    keyCode = xpdfKeyCodeDelete;
  } else if (key == G_KEY_HOME) {
    keyCode = xpdfKeyCodeHome;
  } else if (key == G_KEY_END) {
    keyCode = xpdfKeyCodeEnd;
  } else if (key == G_KEY_PAGEUP) {
    keyCode = xpdfKeyCodePgUp;
  } else if (key == G_KEY_PAGEDOWN) {
	  // printf("%s got line %d\n",__FUNCTION__,__LINE__);
    keyCode = xpdfKeyCodePgDn;
  } else if (key == G_KEY_LEFT) {
    keyCode = xpdfKeyCodeLeft;
  } else if (key == G_KEY_RIGHT) {
    keyCode = xpdfKeyCodeRight;
  } else if (key == G_KEY_UP) {
    keyCode = xpdfKeyCodeUp;
  } else if (key == G_KEY_DOWN) {
	  // printf("%s got line %d\n",__FUNCTION__,__LINE__);
    keyCode = xpdfKeyCodeDown;
  } else if (key >= G_KEY_F1 && key <= G_KEY_F15) {
    keyCode = xpdfKeyCodeF1 + (key - G_KEY_F1);
  } else {
    return;
  }

  if ((cmds = globalParams->getKeyBinding(keyCode,
					  viewer->getModifiers(modifiers),
					  viewer->getContext(modifiers)))) {
    for (i = 0; i < cmds->getLength(); ++i) {
      viewer->execCmd((GString *)cmds->get(i), event);
    }
    deleteGList(cmds, GString);
  }
					
}

void GPDFViewer::initCore(gi_window_id_t parent, GBool fullScreen) {
  core = new GPDFCore(win, parent,
		      app->getPaperRGB(), app->getPaperPixel(),
		      app->getMattePixel(fullScreen),
		      fullScreen, app->getReverseVideo(),
		      app->getInstallCmap(), app->getRGBCubeSize());

  core->setUpdateCbk(&updateCbk, this);
  core->setActionCbk(&actionCbk, this);
  core->setKeyPressCbk(&keyPressCbk, this);
  core->setMouseCbk(&mouseCbk, this);

}

void GPDFViewer::setZoomVal(double z) {
}


static void menu_back_cb(struct stk_popmenu_t* menu, void *data)
{
	GPDFViewer*viewer = (GPDFViewer*)data;
	GPDFViewer::backCbk((void*)viewer, NULL);
}


static void menu_about_cb(struct stk_popmenu_t* menu, void *data)
{
	GPDFViewer*viewer = (GPDFViewer*)data;
	stk_msgbox(stk_find_window(viewer->getWindow()), "about gPdf Reader",MBOX_OK|MBOX_ICONINFORMATION,"基于xpdf引擎\n");
}


static void menu_help_cb(struct stk_popmenu_t* menu, void *data)
{
	GPDFViewer*viewer = (GPDFViewer*)data;
	stk_msgbox(stk_find_window(viewer->getWindow()), "Help",MBOX_OK|MBOX_ICONINFORMATION,"文档正在制作中\n");
}

static void menu_forward_cb(struct stk_popmenu_t* menu, void *data)
{
	GPDFViewer*viewer = (GPDFViewer*)data;
	GPDFViewer::forwardCbk((void*)viewer, NULL);
}



static stk_popmenu_t* menu_loader(void *data)
{
	stk_popmenu_t *np;
	stk_popmenu_t *P_start_menu;

	P_start_menu = stk_menu_new(NULL,"",G_KEY_F2, NULL,NULL);

	np = stk_menu_new(NULL,"Back",G_KEY_F1, menu_back_cb, data);
	if (!np)
		return NULL;
	stk_menu_add_item(P_start_menu,np);

	np = stk_menu_new(NULL,"Forward",G_KEY_F1, menu_forward_cb, data);
	if (!np)
		return NULL;
	stk_menu_add_item(P_start_menu,np);

	np = stk_menu_new(NULL,"Zoom In",G_KEY_F1, NULL, data);
	if (!np)
		return NULL;
	stk_menu_add_item(P_start_menu,np);

	np = stk_menu_new(NULL,"Zoom Out",G_KEY_F1, NULL, data);
	if (!np)
		return NULL;
	stk_menu_add_item(P_start_menu,np);

	//分隔符号

	np = stk_menu_new(NULL,NULL,G_KEY_F1, NULL, data);
	if (!np)
		return NULL;
	stk_menu_add_item(P_start_menu,np);

	np = stk_menu_new(NULL,"Help",G_KEY_F1, menu_help_cb, data);
	if (!np)
		return NULL;
	stk_menu_add_item(P_start_menu,np);

	np = stk_menu_new(NULL,"About gPDF Reader",G_KEY_F1, menu_about_cb, data);
	if (!np)
		return NULL;
	stk_menu_add_item(P_start_menu,np);

	return P_start_menu;	
}



static int pdf_window_proc(gi_msg_t *msg,void *data)
{
	GPDFViewer*viewer = (GPDFViewer*)data;
	GPDFCore *core;

	switch (msg->type)
	{
		case GI_MSG_CLIENT_MSG:
			if(msg->client.client_type==GA_WM_PROTOCOLS
					&&msg->params[0] == GA_WM_DELETE_WINDOW){

			printf("loop exit ..\n");
			GPDFViewer::closeMsgCbk(data,0);
			gi_quit_dispatch();
		}
		break;

	case GI_WMSG_COMMAND:
		{

		StkWindow *stkwindow = (StkWindow *)msg->params[2];//stk_find_window(getWindow());
		//viewer = (GPDFViewer*)stkwindow->base.data;
		
		
		if (msg->params[0]== IDC_GO_NEXT){
			GPDFViewer::nextPageCbk((void*)viewer, NULL);
	    }
		if (msg->params[0]== IDC_GO_PREV){
			GPDFViewer::prevPageCbk((void*)viewer, NULL);
	    }
#if 1
		if (msg->params[0]== IDC_ZOOM_IN){
			zoom_idx--;
			zoom_idx = zoom_idx % (nZoomMenuItems);
			GPDFViewer::zoomMenuCbk((void*)viewer, (void*)zoom_idx);
			stk_set_widget_text(stk_get_widget_item(stkwindow,IDC_ZOOM_VALUE),zoomMenuInfo[zoom_idx].label);
	    }
		if (msg->params[0]== IDC_ZOOM_OUT){
			zoom_idx++;
			zoom_idx = zoom_idx % (nZoomMenuItems);
			GPDFViewer::zoomMenuCbk((void*)viewer, (void*)zoom_idx);
			stk_set_widget_text(stk_get_widget_item(stkwindow,IDC_ZOOM_VALUE),zoomMenuInfo[zoom_idx].label);
	    }
		if (msg->params[0]== IDC_DO_COPY){
			//GPDFApp::findCbk((void*)viewer->app, NULL);
			stk_msgbox(stkwindow, "复制",MBOX_OK|MBOX_EXIT|MBOX_ICONINFORMATION,"好像有点困难哦\n");
	    }
		if (msg->params[0]== IDC_DO_PASTE){
			//GPDFApp::findCbk((void*)viewer->app, NULL);
	    }
		if (msg->params[0]== IDC_SEARCH){
			char *text = "xxxxx";
			text = stk_textbox_get_text(stk_get_widget_item(stkwindow,IDC_SEARCH_TEXT));
			GPDFViewer::findCbk((void*)viewer, text);
	    }
		if (msg->params[0]== IDC_FULL_SCR){
			if (viewer->app->getFullScreen()==FALSE)
			{
			 gi_fullscreen_window(viewer->getWindow(),TRUE);
			 viewer->app->setFullScreen(TRUE);
			 gi_set_window_stack(viewer->getWindow(),G_WINDOW_LAYER_ABOVE_DOCK);
			}
			else{
				gi_set_window_stack(viewer->getWindow(),G_WINDOW_LAYER_NORMAL);
				gi_fullscreen_window(viewer->getWindow(),FALSE);
				viewer->app->setFullScreen(FALSE);
			}
			//GPDFApp::fullScreenToggleCbk((void*)viewer->app, NULL);
	    }
		if (msg->params[0]== IDC_GO_BACK){
			GPDFViewer::backCbk((void*)viewer, NULL);
	    }
		if (msg->params[0]== IDC_GO_FORWARD){
			GPDFViewer::forwardCbk((void*)viewer, NULL);
	    }

		if (msg->params[0]== IDC_OPENFILE){
		GPDFViewer::openCbk((void*)viewer, NULL);
	    }
#endif
		break;
		}

	case GI_MSG_PROPERTYNOTIFY:
		gi_bool_t cont;
	printf("%s got line %d\n",__FUNCTION__,__LINE__);
		GPDFApp::remoteMsgCbk((void*)viewer->app, msg,&cont);
		break;	

	case GI_MSG_EXPOSURE:
		
		GPDFCore::redrawCbk( (void*)viewer->core, msg);
		break;

	case GI_MSG_CONFIGURENOTIFY:
		{
		StkWindow *stkwindow;
        gi_window_id_t win;
		gi_msg_t expose;

        win = viewer->getWindow();
        stkwindow = ::stk_find_window(win);

		expose.type = GI_MSG_EXPOSURE;
		expose.ev_window = win;
		expose.x = 0;
		expose.y = 0;
		expose.width = msg->width;
		expose.height = msg->height;

		printf("GI_MSG_CONFIGURENOTIFY: got %d,%d\n",  msg->width,  msg->height);

		GPDFCore::resizeCbk( (void*)viewer->core, msg);
		stk_paint_widgets(stkwindow,&expose);
		}
	break;

	case GI_MSG_BUTTON_UP:
	case GI_MSG_BUTTON_DOWN:
	case GI_MSG_MOUSE_MOVE:
	case GI_MSG_KEY_DOWN:
		GPDFCore::inputCbk( (void*)viewer->core, msg);
	break;

	

	default:
		return gi_default_handler(msg,data);	
	}
	return TRUE;
}
void GPDFViewer::initWindow(GBool fullScreen) {
  //Colormap colormap;
  //XColor xcol;
  gi_atom_id_t state, val;
 // Arg args[20];
  int n;
  char *title;
  long event_mask =
      GI_MASK_SELECTIONNOTIFY | GI_MASK_EXPOSURE | GI_MASK_CONFIGURENOTIFY | GI_MASK_CLIENT_MSG|GI_MASK_PROPERTYNOTIFY|GI_MASK_SELECTIONNOTIFY
      | GI_MASK_KEY_DOWN | GI_MASK_KEY_UP |  GI_MASK_FOCUS_IN | GI_MASK_FOCUS_OUT
      | GI_MASK_BUTTON_DOWN | GI_MASK_BUTTON_UP
      | GI_MASK_MOUSE_ENTER | GI_MASK_MOUSE_EXIT
      | GI_MASK_MOUSE_MOVE;


  //display = XtDisplay(app->getAppShell());
  //screenNum = XScreenNumberOfScreen(XtScreen(app->getAppShell()));
#if 0

  toolBar = None;

  // private colormap
  if (app->getInstallCmap()) {
    XtVaGetValues(app->getAppShell(), XmNcolormap, &colormap, NULL);
    // ensure that BlackPixel and WhitePixel are reserved in the
    // new colormap
    xcol.red = xcol.green = xcol.blue = 0;
    XAllocColor(display, colormap, &xcol);
    xcol.red = xcol.green = xcol.blue = 65535;
    XAllocColor(display, colormap, &xcol);
    colormap = XCopyColormapAndFree(display, colormap);
  }
#endif

  // top-level window
  n = 0;
  title = app->getTitle() ? app->getTitle()->getCString()
                          : (char *)xpdfAppName;

  win = gi_create_window(GI_DESKTOP_WINDOW_ID, 0,0,
	  DRAWABLE_INIT_WIDTH, DRAWABLE_INIT_HEIGHT, GI_RGB( 230, 230, 230 ),0);


  gi_set_window_utf8_caption(win,"Gix PDF Reader");
  gi_set_dispatch_handler(win, &pdf_window_proc,this);
  gi_set_window_background(win,WINDOW_COLOR,GI_BG_USE_COLOR);

  zoom_idx = 5;

	gi_image_t *img=NULL;

	gi_image_create_from_png("/system/gi/icons/pdf16.png",&img);

	if(img){
	gi_set_window_icon(win,(uint32_t*)img->rgba,img->w,img->h);
	gi_destroy_image(img);
	}

  //gi_set_window_background(id,background,GI_BG_USE_COLOR);
	gi_set_events_mask(win,event_mask);

  //XtSetArg(args[n], XmNtitle, title); ++n;
  //XtSetArg(args[n], XmNiconName, title); ++n;
  //XtSetArg(args[n], XmNminWidth, 100); ++n;
  //XtSetArg(args[n], XmNminHeight, 100); ++n;
  //XtSetArg(args[n], XmNbaseWidth, 0); ++n;
  //XtSetArg(args[n], XmNbaseHeight, 0); ++n;
  //XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING); ++n;
  //win = XtCreatePopupShell("win", topLevelShellWidgetClass,
	//		   app->getAppShell(), args, n);
  //if (app->getInstallCmap()) {
   // XtVaSetValues(win, XmNcolormap, colormap, NULL);
  //}
  //XmAddWMProtocolCallback(win, XInternAtom(display, "WM_DELETE_WINDOW", False),
	//		  &closeMsgCbk, this);

  // create the full-screen window
  if (fullScreen) {
    initCore(GI_DESKTOP_WINDOW_ID, gTrue);

  // create the normal (non-full-screen) window
  } else {
    if (app->getGeometry()) {
      n = 0;
      //XtSetArg(args[n], XmNgeometry, app->getGeometry()->getCString()); ++n;
      //XtSetValues(win, args, n);
    }

    n = 0;
    //form = XmCreateForm(win, "form", args, n);
    //XtManageChild(form);

    //initToolbar(form);
    //n = 0;
    //XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
    //XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
    //XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); ++n;
    //XtSetValues(toolBar, args, n);

    initCore(GI_DESKTOP_WINDOW_ID, gFalse);
    //n = 0;
   // XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); ++n;
   // XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); ++n;
    //XtSetArg(args[n], XmNbottomWidget, toolBar); ++n;
    //XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); ++n;
    //XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); ++n;
    //XtSetValues(core->getWidget(), args, n);

  }

  // set the zoom menu to match the initial zoom setting
  setZoomVal(core->getZoom());

  int navposy = DRAWABLE_INIT_HEIGHT-DRAWABLE_NAVIGATION_HEIGHT;
  int navposx=5;
  StkWidget *widget;


    widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_OPENFILE,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/open.png",0,0) );
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "textbox","", navposx,navposy,36, 26,win,IDC_TEXTBOX_PAGE,  NULL);
	if(widget){
		navposx += 40;
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "label","of", navposx,navposy,46, 26,win,IDC_TEXT_PAGES,  NULL);
	if(widget){
		navposx += 50;
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_GO_PREV,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/up.png",0,0) );
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_GO_NEXT,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/down.png",0,0) );
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "label",zoomMenuInfo[zoom_idx].label, navposx,navposy,56, 26,win,IDC_ZOOM_VALUE,  NULL);
	if(widget){
		navposx += 60;
		stk_show_widget(widget);
	}

	

	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_ZOOM_IN,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/add.png",0,0) );
		stk_show_widget(widget);
	}


	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_ZOOM_OUT,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/remove.png",0,0) );
		stk_show_widget(widget);
	}

#if 0
	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_DO_COPY,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/copy.png",0,0) );
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_DO_PASTE,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/paste.png",0,0) );
		stk_show_widget(widget);
	}
#endif



	widget = stk_create_widget(0, "textbox","text", navposx,navposy,66, 26,win,IDC_SEARCH_TEXT,  NULL);
	if(widget){
		navposx += 70;
		stk_show_widget(widget);
	}


	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_SEARCH,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/search.png",0,0) );
		stk_show_widget(widget);
	}

#if 1
	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_GO_BACK,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/previous.png",0,0) );
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_GO_FORWARD,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/next.png",0,0) );
		stk_show_widget(widget);
	}
#endif	


	widget = stk_create_widget(0, "button","", navposx,navposy,26, 26,win,IDC_FULL_SCR,  NULL);
	if(widget){
		navposx += 30;
		stk_set_widget_image(widget,gi_load_image_file("/system/gi/gpdf/fullscreen.png",0,0) );
		stk_show_widget(widget);
	}

	widget = stk_create_widget(0, "label","link", navposx,navposy,136, 26,win,IDC_TEXT_LINK,  NULL);
	if(widget){
		navposx += 140;
		stk_show_widget(widget);
	}

	

  // set traversal order
  //XtVaSetValues(core->getDrawAreaWidget(),
	//	XmNnavigationType, XmEXCLUSIVE_TAB_GROUP, NULL);
  /*if (toolBar != None) {
    XtVaSetValues(backBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(prevTenPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(prevPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(nextPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(nextTenPageBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(forwardBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(pageNumText, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(zoomWidget, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(findBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(printBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(aboutBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
    XtVaSetValues(quitBtn, XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
		  NULL);
  }

  initPopupMenu();
  */

  pdf_menu = menu_loader((void*)this);

  if (fullScreen) {

	  gi_fullscreen_window(win,true);
#if 0
    // Set both the old-style Motif decorations hint and the new-style
    // _NET_WM_STATE property.  This is redundant, but might be useful
    // for older window managers.  We also set the geometry to +0+0 to
    // avoid interactive placement.  (Note: we need to realize the
    // shell, so it has a Window on which to set the _NET_WM_STATE
    // property, but we don't want to map it until later, so we set
    // mappedWhenManaged to false.)
    n = 0;
    XtSetArg(args[n], XmNmappedWhenManaged, False); ++n;
    XtSetArg(args[n], XmNmwmDecorations, 0); ++n;
    XtSetArg(args[n], XmNgeometry, "+0+0"); ++n;
    XtSetValues(win, args, n);
    XtRealizeWidget(win);
    state = XInternAtom(display, "_NET_WM_STATE", False);
    val = XInternAtom(display, "_NET_WM_STATE_FULLSCREEN", False);
    XChangeProperty(display, XtWindow(win), state, XA_ATOM, 32,
		    PropModeReplace, (Guchar *)&val, 1);
#endif
  }
}

static int mouseX(gi_msg_t *event) {
  switch (event->type) {
  case GI_MSG_BUTTON_DOWN:
  case GI_MSG_BUTTON_UP:
    return event->x;
  case GI_MSG_KEY_DOWN:
    return event->x;
  }
  return 0;
}

static int mouseY(gi_msg_t *event) {
  switch (event->type) {
  case GI_MSG_BUTTON_DOWN:
  case GI_MSG_BUTTON_UP:
    return event->y;
  case GI_MSG_KEY_DOWN:
    return event->y;
  }
  return 0;
}

void GPDFViewer::cmdAbout(GString *args[], int nArgs,
			  gi_msg_t *event) {
 // XtManageChild(aboutDialog);
}

void GPDFViewer::cmdCloseOutline(GString *args[], int nArgs,
				 gi_msg_t *event) {

}

void GPDFViewer::cmdCloseWindow(GString *args[], int nArgs,
				gi_msg_t *event) {
  app->close(this, gFalse);
}

void GPDFViewer::cmdContinuousMode(GString *args[], int nArgs,
				   gi_msg_t *event) {
  //Widget btn;

  if (core->getContinuousMode()) {
    return;
  }
  core->setContinuousMode(gTrue);

  //btn = XtNameToWidget(popupMenu, "continuousMode");
  //XtVaSetValues(btn, XmNset, XmSET, NULL);
}

void GPDFViewer::cmdEndPan(GString *args[], int nArgs,
			   gi_msg_t *event) {
  core->endPan(mouseX(event), mouseY(event));
}

void GPDFViewer::cmdEndSelection(GString *args[], int nArgs,
				 gi_msg_t *event) {
  core->endSelection(mouseX(event), mouseY(event));
}

void GPDFViewer::cmdFind(GString *args[], int nArgs,
			 gi_msg_t *event) {
  //mapFindDialog();
}

void GPDFViewer::doFind(GBool next) {
}

void GPDFViewer::doFind(char *findText, GBool findCaseSensitiveToggle, GBool findBackwardToggle, GBool next) {

	gi_load_cursor(getWindow(),GI_CURSOR_WATCH);

  //if (XtWindow(findDialog)) {
  //  XDefineCursor(display, XtWindow(findDialog), core->getBusyCursor());
  //}
  core->find(findText,
	     findCaseSensitiveToggle,
	     next,
	     findBackwardToggle,
	     gFalse);
  //if (XtWindow(findDialog)) {
   // XUndefineCursor(display, XtWindow(findDialog));
  //}

  gi_load_cursor(getWindow(),GI_CURSOR_ARROW);

}



void GPDFViewer::cmdFindNext(GString *args[], int nArgs,
			     gi_msg_t *event) {
  doFind(gTrue);
}

void GPDFViewer::cmdFocusToDocWin(GString *args[], int nArgs,
				  gi_msg_t *event) {
  core->takeFocus();
}

void GPDFViewer::cmdFocusToPageNum(GString *args[], int nArgs,
				   gi_msg_t *event) {
 // XmTextFieldSetSelection(pageNumText, 0,
//			  strlen(XmTextFieldGetString(pageNumText)),
	//		  XtLastTimestampProcessed(display));
  //XmProcessTraversal(pageNumText, XmTRAVERSE_CURRENT);
}

void GPDFViewer::cmdFollowLink(GString *args[], int nArgs,
			       gi_msg_t *event) {
  doLink(mouseX(event), mouseY(event), gFalse, gFalse);
}

void GPDFViewer::cmdFollowLinkInNewWin(GString *args[], int nArgs,
				       gi_msg_t *event) {
  doLink(mouseX(event), mouseY(event), gFalse, gTrue);
}

void GPDFViewer::cmdFollowLinkInNewWinNoSel(GString *args[], int nArgs,
					    gi_msg_t *event) {
  doLink(mouseX(event), mouseY(event), gTrue, gTrue);
}

void GPDFViewer::cmdFollowLinkNoSel(GString *args[], int nArgs,
				    gi_msg_t *event) {
  doLink(mouseX(event), mouseY(event), gTrue, gFalse);
}

void GPDFViewer::cmdFullScreenMode(GString *args[], int nArgs,
				   gi_msg_t *event) {
  PDFDoc *doc;
  GPDFViewer *viewer;
  int pg;
  //Widget btn;

  if (core->getFullScreen()) {
    return;
  }
  pg = core->getPageNum();
  //XtPopdown(win);
  doc = core->takeDoc(gFalse);
  viewer = app->reopen(this, doc, pg, gTrue);

  gi_fullscreen_window(win,TRUE);

  //btn = XtNameToWidget(viewer->popupMenu, "fullScreen");
  //XtVaSetValues(btn, XmNset, XmSET, NULL);
}

void GPDFViewer::cmdGoBackward(GString *args[], int nArgs,
			       gi_msg_t *event) {
  core->goBackward();
}

void GPDFViewer::cmdGoForward(GString *args[], int nArgs,
			      gi_msg_t *event) {
  core->goForward();
}

void GPDFViewer::cmdGotoDest(GString *args[], int nArgs,
			     gi_msg_t *event) {
  int pg;
  LinkDest *dest;

  getPageAndDest(1, args[0], &pg, &dest);
  if (dest) {
    displayDest(dest, core->getZoom(), core->getRotate(), gTrue);
    delete dest;
  }
}

void GPDFViewer::cmdGotoLastPage(GString *args[], int nArgs,
				 gi_msg_t *event) {
  displayPage(core->getDoc()->getNumPages(),
	      core->getZoom(), core->getRotate(),
	      gTrue, gTrue);
}

void GPDFViewer::cmdGotoLastPageNoScroll(GString *args[], int nArgs,
					 gi_msg_t *event) {
  displayPage(core->getDoc()->getNumPages(),
	      core->getZoom(), core->getRotate(),
	      gFalse, gTrue);
}

void GPDFViewer::cmdGotoPage(GString *args[], int nArgs,
			     gi_msg_t *event) {
  int pg;

  pg = atoi(args[0]->getCString());
  if (pg < 1 || pg > core->getDoc()->getNumPages()) {
    return;
  }
  displayPage(pg, core->getZoom(), core->getRotate(), gTrue, gTrue);
}

void GPDFViewer::cmdGotoPageNoScroll(GString *args[], int nArgs,
				     gi_msg_t *event) {
  int pg;

  pg = atoi(args[0]->getCString());
  if (pg < 1 || pg > core->getDoc()->getNumPages()) {
    return;
  }
  displayPage(pg, core->getZoom(), core->getRotate(), gFalse, gTrue);
}

void GPDFViewer::cmdNextPage(GString *args[], int nArgs,
			     gi_msg_t *event) {
  core->gotoNextPage(1, gTrue);
}

void GPDFViewer::cmdNextPageNoScroll(GString *args[], int nArgs,
				     gi_msg_t *event) {
  core->gotoNextPage(1, gFalse);
}

void GPDFViewer::cmdOpen(GString *args[], int nArgs,
			 gi_msg_t *event) {
  //mapOpenDialog(gFalse);
}

void GPDFViewer::cmdOpenFile(GString *args[], int nArgs,
			     gi_msg_t *event) {
  open(args[0], 1, NULL);
}

void GPDFViewer::cmdOpenFileAtDest(GString *args[], int nArgs,
				   gi_msg_t *event) {
  open(args[0], 1, args[1]);
}

void GPDFViewer::cmdOpenFileAtDestInNewWin(GString *args[], int nArgs,
					   gi_msg_t *event) {
  app->openAtDest(args[0], args[1]);
}

void GPDFViewer::cmdOpenFileAtPage(GString *args[], int nArgs,
				   gi_msg_t *event) {
  open(args[0], atoi(args[1]->getCString()), NULL);
}

void GPDFViewer::cmdOpenFileAtPageInNewWin(GString *args[], int nArgs,
					   gi_msg_t *event) {
  app->open(args[0], atoi(args[1]->getCString()));
}

void GPDFViewer::cmdOpenFileInNewWin(GString *args[], int nArgs,
				     gi_msg_t *event) {
  app->open(args[0]);
}

void GPDFViewer::cmdOpenInNewWin(GString *args[], int nArgs,
				 gi_msg_t *event) {
 // mapOpenDialog(gTrue);
}

void GPDFViewer::cmdOpenOutline(GString *args[], int nArgs,
				gi_msg_t *event) {

}

void GPDFViewer::cmdPageDown(GString *args[], int nArgs,
			     gi_msg_t *event) {
  core->scrollPageDown();
}

void GPDFViewer::cmdPageUp(GString *args[], int nArgs,
			   gi_msg_t *event) {
  core->scrollPageUp();
}

void GPDFViewer::cmdPostPopupMenu(GString *args[], int nArgs,
				  gi_msg_t *event) {
  //XmMenuPosition(popupMenu, event->type == ButtonPress ? &event->xbutton
	//	                                       : (XButtonEvent *)NULL);
  //XtManageChild(popupMenu);

  // this is magic (taken from DDD) - weird things happen if this
  // call isn't made (this is done in two different places, in hopes
  // of squashing this stupid bug)
  //XtUngrabButton(core->getDrawAreaWidget(), AnyButton, AnyModifier);
}

void GPDFViewer::cmdPrevPage(GString *args[], int nArgs,
			     gi_msg_t *event) {
  core->gotoPrevPage(1, gTrue, gFalse);
}

void GPDFViewer::cmdPrevPageNoScroll(GString *args[], int nArgs,
				     gi_msg_t *event) {
  core->gotoPrevPage(1, gFalse, gFalse);
}

void GPDFViewer::cmdPrint(GString *args[], int nArgs,
			  gi_msg_t *event) {
  //XtManageChild(printDialog);
}

void GPDFViewer::cmdQuit(GString *args[], int nArgs,
			 gi_msg_t *event) {
  app->quit();
}

void GPDFViewer::cmdRaise(GString *args[], int nArgs,
			  gi_msg_t *event) {
  gi_show_window( win);
  gi_raise_window( win);
}

void GPDFViewer::cmdRedraw(GString *args[], int nArgs,
			   gi_msg_t *event) {
  displayPage(core->getPageNum(), core->getZoom(), core->getRotate(),
	      gFalse, gFalse);
}

void GPDFViewer::cmdReload(GString *args[], int nArgs,
			   gi_msg_t *event) {
  reloadFile();
}

void GPDFViewer::cmdRun(GString *args[], int nArgs,
			gi_msg_t *event) {
  GString *fmt, *cmd, *s;
  LinkAction *action;
  double selLRX, selLRY, selURX, selURY;
  int selPage;
  GBool gotSel;
  char buf[64];
  char *p;
  char c0, c1;
  int i;

  cmd = new GString();
  fmt = args[0];
  i = 0;
  gotSel = gFalse;
  while (i < fmt->getLength()) {
    c0 = fmt->getChar(i);
    if (c0 == '%' && i+1 < fmt->getLength()) {
      c1 = fmt->getChar(i+1);
      switch (c1) {
      case 'f':
	if (core->getDoc() && (s = core->getDoc()->getFileName())) {
	  cmd->append(s);
	}
	break;
      case 'b':
	if (core->getDoc() && (s = core->getDoc()->getFileName())) {
	  if ((p = strrchr(s->getCString(), '.'))) {
	    cmd->append(s->getCString(), p - s->getCString());
	  } else {
	    cmd->append(s);
	  }
	}
	break;
      case 'u':
	if ((action = core->getLinkAction()) &&
	    action->getKind() == actionURI) {
	  s = core->mungeURL(((LinkURI *)action)->getURI());
	  cmd->append(s);
	  delete s;
	}
	break;
      case 'x':
      case 'y':
      case 'X':
      case 'Y':
	if (!gotSel) {
	  if (!core->getSelection(&selPage, &selURX, &selURY,
				  &selLRX, &selLRY)) {
	    selPage = 0;
	    selURX = selURY = selLRX = selLRY = 0;
	  }
	  gotSel = gTrue;
	}
	sprintf(buf, "%g",
		(c1 == 'x') ? selURX :
		(c1 == 'y') ? selURY :
		(c1 == 'X') ? selLRX : selLRY);
	cmd->append(buf);
	break;
      default:
	cmd->append(c1);
	break;
      }
      i += 2;
    } else {
      cmd->append(c0);
      ++i;
    }
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

void GPDFViewer::cmdScrollDown(GString *args[], int nArgs,
			       gi_msg_t *event) {
  core->scrollDown(atoi(args[0]->getCString()));
}

void GPDFViewer::cmdScrollDownNextPage(GString *args[], int nArgs,
				       gi_msg_t *event) {
  core->scrollDownNextPage(atoi(args[0]->getCString()));
}

void GPDFViewer::cmdScrollLeft(GString *args[], int nArgs,
			       gi_msg_t *event) {
  core->scrollLeft(atoi(args[0]->getCString()));
}

void GPDFViewer::cmdScrollOutlineDown(GString *args[], int nArgs,
				      gi_msg_t *event) {

}

void GPDFViewer::cmdScrollOutlineUp(GString *args[], int nArgs,
				    gi_msg_t *event) {

}

void GPDFViewer::cmdScrollRight(GString *args[], int nArgs,
				gi_msg_t *event) {
  core->scrollRight(atoi(args[0]->getCString()));
}

void GPDFViewer::cmdScrollToBottomEdge(GString *args[], int nArgs,
				       gi_msg_t *event) {
  core->scrollToBottomEdge();
}

void GPDFViewer::cmdScrollToBottomRight(GString *args[], int nArgs,
					gi_msg_t *event) {
  core->scrollToBottomRight();
}

void GPDFViewer::cmdScrollToLeftEdge(GString *args[], int nArgs,
				     gi_msg_t *event) {
  core->scrollToLeftEdge();
}

void GPDFViewer::cmdScrollToRightEdge(GString *args[], int nArgs,
				      gi_msg_t *event) {
  core->scrollToRightEdge();
}

void GPDFViewer::cmdScrollToTopEdge(GString *args[], int nArgs,
				    gi_msg_t *event) {
  core->scrollToTopEdge();
}

void GPDFViewer::cmdScrollToTopLeft(GString *args[], int nArgs,
				    gi_msg_t *event) {
  core->scrollToTopLeft();
}

void GPDFViewer::cmdScrollUp(GString *args[], int nArgs,
			     gi_msg_t *event) {
  core->scrollUp(atoi(args[0]->getCString()));
}

void GPDFViewer::cmdScrollUpPrevPage(GString *args[], int nArgs,
				     gi_msg_t *event) {
  core->scrollUpPrevPage(atoi(args[0]->getCString()));
}

void GPDFViewer::cmdSinglePageMode(GString *args[], int nArgs,
				   gi_msg_t *event) {
  //Widget btn;

  if (!core->getContinuousMode()) {
    return;
  }
  core->setContinuousMode(gFalse);

  //btn = XtNameToWidget(popupMenu, "continuousMode");
  //XtVaSetValues(btn, XmNset, XmUNSET, NULL);
}

void GPDFViewer::cmdStartPan(GString *args[], int nArgs,
			     gi_msg_t *event) {
  core->startPan(mouseX(event), mouseY(event));
}

void GPDFViewer::cmdStartSelection(GString *args[], int nArgs,
				   gi_msg_t *event) {
  core->startSelection(mouseX(event), mouseY(event));
}

void GPDFViewer::cmdToggleContinuousMode(GString *args[], int nArgs,
					 gi_msg_t *event) {
  if (core->getContinuousMode()) {
    cmdSinglePageMode(NULL, 0, event);
  } else {
    cmdContinuousMode(NULL, 0, event);
  }
}

void GPDFViewer::cmdToggleFullScreenMode(GString *args[], int nArgs,
					 gi_msg_t *event) {
  if (core->getFullScreen()) {
    cmdWindowMode(NULL, 0, event);
  } else {
    cmdFullScreenMode(NULL, 0, event);
  }
}

void GPDFViewer::cmdToggleOutline(GString *args[], int nArgs,
				  gi_msg_t *event) {

}

void GPDFViewer::cmdWindowMode(GString *args[], int nArgs,
			       gi_msg_t *event) {
  PDFDoc *doc;
  GPDFViewer *viewer;
  int pg;
  //Widget btn;

  if (!core->getFullScreen()) {
    return;
  }
  pg = core->getPageNum();
  //XtPopdown(win);
  doc = core->takeDoc(gFalse);
  viewer = app->reopen(this, doc, pg, gFalse);

  //btn = XtNameToWidget(viewer->popupMenu, "fullScreen");
  //XtVaSetValues(btn, XmNset, XmUNSET, NULL);
}

void GPDFViewer::cmdZoomFitPage(GString *args[], int nArgs,
				gi_msg_t *event) {
  if (core->getZoom() != zoomPage) {
    setZoomIdx(zoomPageIdx);
    displayPage(core->getPageNum(), zoomPage,
		core->getRotate(), gTrue, gFalse);
  }
}

void GPDFViewer::cmdZoomFitWidth(GString *args[], int nArgs,
				 gi_msg_t *event) {
  if (core->getZoom() != zoomWidth) {
    setZoomIdx(zoomWidthIdx);
    displayPage(core->getPageNum(), zoomWidth,
		core->getRotate(), gTrue, gFalse);
  }
}

void GPDFViewer::cmdZoomIn(GString *args[], int nArgs,
			   gi_msg_t *event) {
  int z;

  z = getZoomIdx();
  if (z <= minZoomIdx && z > maxZoomIdx) {
    --z;
    setZoomIdx(z);
    displayPage(core->getPageNum(), zoomMenuInfo[z].zoom,
		core->getRotate(), gTrue, gFalse);
  }
}

void GPDFViewer::cmdZoomOut(GString *args[], int nArgs,
			    gi_msg_t *event) {
  int z;

  z = getZoomIdx();
  if (z < minZoomIdx && z >= maxZoomIdx) {
    ++z;
    setZoomIdx(z);
    displayPage(core->getPageNum(), zoomMenuInfo[z].zoom,
		core->getRotate(), gTrue, gFalse);
  }
}

void GPDFViewer::cmdZoomPercent(GString *args[], int nArgs,
				gi_msg_t *event) {
  double z;

  z = atof(args[0]->getCString());
  setZoomVal(z);
  displayPage(core->getPageNum(), z, core->getRotate(), gTrue, gFalse);
}

void GPDFViewer::cmdZoomToSelection(GString *args[], int nArgs,
				    gi_msg_t *event) {
  int pg;
  double ulx, uly, lrx, lry;

  if (core->getSelection(&pg, &ulx, &uly, &lrx, &lry)) {
    core->zoomToRect(pg, ulx, uly, lrx, lry);
  }
}


void GPDFViewer::doLink(int wx, int wy, GBool onlyIfNoSelection,
			GBool newWin) {
  GPDFViewer *newViewer;
  LinkAction *action;
  int pg, selPg;
  double xu, yu, selULX, selULY, selLRX, selLRY;

  if (core->getHyperlinksEnabled() &&
      core->cvtWindowToUser(wx, wy, &pg, &xu, &yu) &&
      !(onlyIfNoSelection &&
	core->getSelection(&selPg, &selULX, &selULY, &selLRX, &selLRY))) {
    if ((action = core->findLink(pg, xu, yu))) {
      if (newWin &&
	  core->getDoc()->getFileName() &&
	  (action->getKind() == actionGoTo ||
	   action->getKind() == actionGoToR ||
	   (action->getKind() == actionNamed &&
	    ((LinkNamed *)action)->getName()->cmp("Quit")))) {
	newViewer = app->open(core->getDoc()->getFileName());
	newViewer->core->doAction(action);
      } else {
	core->doAction(action);
      }
    }
  }
}


int GPDFViewer::getZoomIdx() {
  int i;

  for (i = 0; i < nZoomMenuItems; ++i) {
    if (core->getZoom() == zoomMenuInfo[i].zoom) {
      return i;
    }
  }
  return -1;
}

void GPDFViewer::setZoomIdx(int idx) {
	gi_window_id_t win;
	StkWindow* swindow;

	win = getWindow();
	swindow = stk_find_window(win);

	printf("setZoomIdx = %d\n",idx);
	stk_set_widget_text(stk_get_widget_item(swindow,IDC_ZOOM_VALUE),zoomMenuInfo[idx%nZoomMenuItems].label);
}




#if 1

//void GPDFViewer::addToolTip( char *text) {

//}





void GPDFViewer::findCbk( void* ptr,
			 void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  if (!viewer->core->getDoc()) {
    return;
  }

  viewer->doFind((char*)callData, gFalse, gFalse, gFalse);
  //viewer->mapFindDialog();
}

void GPDFViewer::printCbk( void* ptr,
			  void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  if (!viewer->core->getDoc()) {
    return;
  }
  //XtManageChild(viewer->printDialog);
}

void GPDFViewer::aboutCbk( void* ptr,
			  void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  //XtManageChild(viewer->aboutDialog);
}

void GPDFViewer::quitCbk( void* ptr,
			 void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->app->quit();
}

void GPDFViewer::openCbk( void* ptr,
			 void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;  
  int err;
  char tmp[512];
  GString *fileNameStr;
  StkWindow *stkwindow ;
  gi_window_id_t win;

  win = viewer->getWindow();

  stkwindow = ::stk_find_window(win);
  err = ::stk_get_open_file(stkwindow,"选择文件",tmp,512);

  if (err != STK_RET_OK)
  {
	  return ;
  }

  fileNameStr = new GString(tmp);

  if (1) {
      viewer->app->open(fileNameStr);
    } else {
      if (viewer->loadFile(fileNameStr)) {
	viewer->displayPage(1, viewer->core->getZoom(),
			    viewer->core->getRotate(), gTrue, gTrue);
      }
	}

	delete fileNameStr;

  //viewer->mapOpenDialog(gFalse);
}

void GPDFViewer::openInNewWindowCbk( void* ptr,
				    void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  char buf[512];
  //stk_get_open_file(stk_find_window(mainWin),"选择文件",buf,512);

  //viewer->mapOpenDialog(gTrue);
}

void GPDFViewer::reloadCbk( void* ptr,
			 void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->reloadFile();
}

void GPDFViewer::saveAsCbk( void* ptr,
			   void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  if (!viewer->core->getDoc()) {
    return;
  }

  //char buf[512];
  //stk_get_open_file(stk_find_window(mainWin),"选择文件",buf,512);
  //viewer->mapSaveAsDialog();
}

void GPDFViewer::continuousModeToggleCbk( void* ptr,
					 void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;
  //XmToggleButtonCallbackStruct *data =
  //    (XmToggleButtonCallbackStruct *)callData;

  viewer->core->setContinuousMode(0);
}

void GPDFViewer::fullScreenToggleCbk( void* ptr,
				     void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;
  //XmToggleButtonCallbackStruct *data =
   //   (XmToggleButtonCallbackStruct *)callData;

  if (callData) {
    viewer->cmdFullScreenMode(NULL, 0, NULL);
  } else {
    viewer->cmdWindowMode(NULL, 0, NULL);
  }
}

void GPDFViewer::rotateCCWCbk( void* ptr,
			      void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;
  int r;

  r = viewer->core->getRotate();
  r = (r == 0) ? 270 : r - 90;
  viewer->displayPage(viewer->core->getPageNum(), viewer->core->getZoom(),
		      r, gTrue, gFalse);
}

void GPDFViewer::rotateCWCbk( void* ptr,
			     void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;
  int r;

  r = viewer->core->getRotate();
  r = (r == 270) ? 0 : r + 90;
  viewer->displayPage(viewer->core->getPageNum(), viewer->core->getZoom(),
		      r, gTrue, gFalse);
}

void GPDFViewer::zoomMenuCbk(void* ptr,
			     void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;
  long userData = (long)callData;
  //XmPushButtonCallbackStruct *data = (XmPushButtonCallbackStruct *)callData;
  //XtPointer userData;
  double z;

  //XtVaGetValues(widget, XmNuserData, &userData, NULL);
  z = zoomMenuInfo[(long)userData].zoom;
  // only redraw if this was triggered by an event; otherwise
  // the caller is responsible for doing the redraw
  if (z != viewer->core->getZoom() ) {//&& data->event
    viewer->displayPage(viewer->core->getPageNum(), z,
			viewer->core->getRotate(), gTrue, gFalse);
  }
  viewer->core->takeFocus();
}

void GPDFViewer::zoomToSelectionCbk( void* ptr,
				    void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;
  int pg;
  double ulx, uly, lrx, lry;

  if (viewer->core->getSelection(&pg, &ulx, &uly, &lrx, &lry)) {
    viewer->core->zoomToRect(pg, ulx, uly, lrx, lry);
  }
}

void GPDFViewer::closeCbk( void* ptr,
			  void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->app->close(viewer, gFalse);
}


void GPDFViewer::closeMsgCbk( void* ptr,
			     void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;

  viewer->app->close(viewer, gTrue);
}

void GPDFViewer::pageNumCbk( void* ptr,
			    void* callData) {
  GPDFViewer *viewer = (GPDFViewer *)ptr;
  char *s, *p;
  int pg;
  char buf[20];

  if (!viewer->core->getDoc()) {
    goto err;
  }
  s = "0";//fixme XmTextFieldGetString(viewer->pageNumText);
  for (p = s; *p; ++p) {
    if (!isdigit(*p)) {
      goto err;
    }
  }
  pg = atoi(s);
  if (pg < 1 || pg > viewer->core->getDoc()->getNumPages()) {
    goto err;
  }
  viewer->displayPage(pg, viewer->core->getZoom(),
		      viewer->core->getRotate(), gFalse, gTrue);
  viewer->core->takeFocus();
  return;

 err:
  sprintf(buf, "%d", viewer->core->getPageNum());
	 return;
  //XBell(viewer->display, 0);
  //XmTextFieldSetString(viewer->pageNumText, buf);
}
#endif

