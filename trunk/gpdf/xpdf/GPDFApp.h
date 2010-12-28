//========================================================================
//
// GPDFApp.h
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#ifndef XPDFAPP_H
#define XPDFAPP_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif
#include <gi/gi.h>
#include <gi/property.h>

#include "gtypes.h"
#include "SplashTypes.h"

class GString;
class GList;
class PDFDoc;


class GPDFViewer;


#include "gtypes.h"
#include "gfile.h" // for time_t
#include "SplashTypes.h"
#include "PDFCore.h"
#include <gi/gi.h>
#include <gi/stk.h>
class GString;
class BaseStream;
class PDFDoc;
class LinkAction;

#define DRAWABLE_INIT_WIDTH 700
#define DRAWABLE_INIT_HEIGHT	 500
#define DRAWABLE_NAVIGATION_HEIGHT	 30
//------------------------------------------------------------------------

#define xMaxRGBCube 6		// max size of RGB color cube

//------------------------------------------------------------------------
// callbacks
//------------------------------------------------------------------------

typedef void (*XPDFUpdateCbk)(void *data, GString *fileName,
			      int pageNum, int numPages, char *linkLabel);

typedef void (*XPDFActionCbk)(void *data, char *action);

typedef void (*XPDFKeyPressCbk)(void *data, int key, Guint modifiers,
				gi_msg_t *event);

typedef void (*XPDFMouseCbk)(void *data, gi_msg_t *event);


//------------------------------------------------------------------------

#define xpdfAppName "Xpdf"

//------------------------------------------------------------------------
// GPDFApp
//------------------------------------------------------------------------

class GPDFApp {
public:

  GPDFApp(int *argc, char *argv[]);
  ~GPDFApp();

  GPDFViewer *open(GString *fileName, int page = 1,
		   GString *ownerPassword = NULL,
		   GString *userPassword = NULL);
  GPDFViewer *openAtDest(GString *fileName, GString *dest,
			 GString *ownerPassword = NULL,
			 GString *userPassword = NULL);
  GPDFViewer *reopen(GPDFViewer *viewer, PDFDoc *doc, int page,
		     GBool fullScreenA);
  void close(GPDFViewer *viewer, GBool closeLast);
  void quit();

  void run();

  //----- remote server
  void setRemoteName(char *remoteName);
  GBool remoteServerRunning();
  void remoteExec(char *cmd);
  void remoteOpen(GString *fileName, int page, GBool raise);
  void remoteOpenAtDest(GString *fileName, GString *dest, GBool raise);
  void remoteReload(GBool raise);
  void remoteRaise();
  void remoteQuit();

  //----- resource/option values
  GString *getGeometry() { return geometry; }
  GString *getTitle() { return title; }
  GBool getInstallCmap() { return installCmap; }
  int getRGBCubeSize() { return rgbCubeSize; }
  GBool getReverseVideo() { return reverseVideo; }
  SplashColorPtr getPaperRGB() { return paperRGB; }
  Gulong getPaperPixel() { return paperPixel; }
  Gulong getMattePixel(GBool fullScreenA)
    { return fullScreenA ? fullScreenMattePixel : mattePixel; }
  GString *getInitialZoom() { return initialZoom; }

  
  void setFullScreen(GBool fullScreenA) { fullScreen = fullScreenA; }
  GBool getFullScreen() { return fullScreen; }
  static void remoteMsgCbk( void* ptr,
			   gi_msg_t *event, gi_bool_t *cont);

  //XtAppContext getAppContext() { return appContext; }
  //Widget getAppShell() { return appShell; }

private:

  void getResources();
  

  

  //Display *display;
  int screenNum;
  //XtAppContext appContext;
  //Widget appShell;
  GList *viewers;		// [GPDFViewer]

  gi_atom_id_t remoteAtom;
  gi_window_id_t remoteXWin;
  GPDFViewer *remoteViewer;
  gi_window_id_t remoteWin;

  //----- resource/option values
  GString *geometry;
  GString *title;
  GBool installCmap;
  int rgbCubeSize;
  GBool reverseVideo;
  SplashColor paperRGB;
  Gulong paperPixel;
  Gulong mattePixel;
  Gulong fullScreenMattePixel;
  GString *initialZoom;
  GBool fullScreen;
};


class GPDFCore: public PDFCore {
public:

  // Create viewer core inside <parentWidgetA>.
  GPDFCore(gi_window_id_t win,gi_window_id_t parent,
	   SplashColorPtr paperColorA, Gulong paperPixelA,
	   Gulong mattePixelA, GBool fullScreenA, GBool reverseVideoA,
	   GBool installCmap, int rgbCubeSizeA);

  ~GPDFCore();

  //----- loadFile / displayPage / displayDest

  // Load a new file.  Returns pdfOk or error code.
  virtual int loadFile(GString *fileName, GString *ownerPassword = NULL,
		       GString *userPassword = NULL);

  // Load a new file, via a Stream instead of a file name.  Returns
  // pdfOk or error code.
  virtual int loadFile(BaseStream *stream, GString *ownerPassword = NULL,
		       GString *userPassword = NULL);

  // Load an already-created PDFDoc object.
  virtual void loadDoc(PDFDoc *docA);

  // Resize the window to fit page <pg> of the current document.
  void resizeToPage(int pg);

  // Update the display, given the specified parameters.
  virtual void update(int topPageA, int scrollXA, int scrollYA,
		      double zoomA, int rotateA,
		      GBool force, GBool addToHist);
  void takeFocus();
  virtual void setBusyCursor(GBool busy);

  void setUpdateCbk(XPDFUpdateCbk cbk, void *data)
    { updateCbk = cbk; updateCbkData = data; }
  void setActionCbk(XPDFActionCbk cbk, void *data)
    { actionCbk = cbk; actionCbkData = data; }
  void setKeyPressCbk(XPDFKeyPressCbk cbk, void *data)
    { keyPressCbk = cbk; keyPressCbkData = data; }
  void setMouseCbk(XPDFMouseCbk cbk, void *data)
    { mouseCbk = cbk; mouseCbkData = data; }

  GBool getFullScreen() { return fullScreen; }

  static void resizeCbk( void* ptr, void* callData);
  static void redrawCbk( void* ptr, void* callData);
  static void inputCbk( void* ptr, void* callData);

  //----- selection

  void startSelection(int wx, int wy);
  void endSelection(int wx, int wy);
  void copySelection();
  void startPan(int wx, int wy);
  void endPan(int wx, int wy);

  //----- hyperlinks
  void runCommand(GString *cmdFmt, GString *arg);
  void doAction(LinkAction *action);
  LinkAction *getLinkAction() { return linkAction; }
  GString *mungeURL(GString *url);

  //----- find

  virtual GBool find(char *s, GBool caseSensitive,
		     GBool next, GBool backward, GBool onePageOnly);
  virtual GBool findU(Unicode *u, int len, GBool caseSensitive,
		      GBool next, GBool backward, GBool onePageOnly);

  //----- page/position changes

  virtual GBool gotoNextPage(int inc, GBool top);
  virtual GBool gotoPrevPage(int dec, GBool top, GBool bottom);
  virtual GBool goForward();
  virtual GBool goBackward();

  void enableHyperlinks(GBool on) { hyperlinksEnabled = on; }
  GBool getHyperlinksEnabled() { return hyperlinksEnabled; }


  gi_window_id_t drawArea;


 private:

	 virtual GBool checkForNewFile();

  //----- selection
  gi_bool_t convertSelectionCbk( gi_atom_id_t *selection,
				      gi_atom_id_t *target, gi_atom_id_t *type,
				      void* *value, unsigned long *length,
				      int *format);


  



 
 static void hScrollChangeCbk( void* ptr,
			       void* callData);
  static void hScrollDragCbk( void* ptr,
			     void* callData);
  static void vScrollChangeCbk( void* ptr,
			       void* callData);
  static void vScrollDragCbk( void* ptr,
			     void* callData);

 
  virtual PDFCoreTile *newTile(int xDestA, int yDestA);
  virtual void updateTileData(PDFCoreTile *tileA, int xSrc, int ySrc,
			      int width, int height, GBool composited);
  virtual void redrawRect(PDFCoreTile *tileA, int xSrc, int ySrc,
			  int xDest, int yDest, int width, int height,
			  GBool composited);
  virtual void updateScrollbars();
  void setCursor(int cursor);
  void initWindow();
  //int window_proc(gi_msg_t *msg,void *data);


 int busyCursor, linkCursor, selectCursor;
  int currentCursor;
  gi_gc_ptr_t drawAreaGC;		// GC for blitting into drawArea

  Gulong paperPixel;
  Gulong mattePixel;

   Guint format;                  // visual depth
   GBool trueColor;  
   int rDiv, gDiv, bDiv;         // RGB right shifts (for TrueColor)
  int rShift, gShift, bShift;   // RGB left shifts (for TrueColor)
  int rgbCubeSize;              // size of color cube (for non-TrueColor)
  Gulong                        // color cube (for non-TrueColor)
    colors[xMaxRGBCube * xMaxRGBCube * xMaxRGBCube];



	gi_bool_t fullScreen;
 static GString *currentSelection;  // selected text
  static GPDFCore *currentSelectionOwner;
  static gi_atom_id_t targetsAtom;
  time_t modTime;		// last modification time of PDF file

  GBool panning;
  int panMX, panMY;
  XPDFUpdateCbk updateCbk;
  void *updateCbkData;
  XPDFActionCbk actionCbk;
  void *actionCbkData;
  XPDFKeyPressCbk keyPressCbk;
  void *keyPressCbkData;
  XPDFMouseCbk mouseCbk;
  void *mouseCbkData;
  LinkAction *linkAction;	// mouse cursor is over this link

  GBool hyperlinksEnabled;
  GBool selectEnabled;


};



// NB: this must match the defn of zoomMenuBtnInfo in GPDFViewer.cc
#define nZoomMenuItems 10

//------------------------------------------------------------------------

struct XPDFViewerCmd {
  char *name;
  int nArgs;
  GBool requiresDoc;
  GBool requiresEvent;
  void (GPDFViewer::*func)(GString *args[], int nArgs, gi_msg_t *event);
};


class GPDFViewer {
public:

  GPDFViewer(GPDFApp *appA, GString *fileName,
	     int pageA, GString *destName, GBool fullScreen,
	     GString *ownerPassword, GString *userPassword);
  GPDFViewer(GPDFApp *appA, PDFDoc *doc, int pageA,
	     GString *destName, GBool fullScreen);
  GBool isOk() { return ok; }
  ~GPDFViewer();

  void open(GString *fileName, int pageA, GString *destName);
  void clear();
  void reloadFile();

  void execCmd(GString *cmd, gi_msg_t *event);
  static void closeCbk( void* ptr,
		       void* callData);
  static void closeMsgCbk( void* ptr,
			  void* callData);

  static void zoomMenuCbk(void* ptr,
			  void* callData);

   static void findCbk( void* ptr,
		      void* callData);
  static void printCbk( void* ptr,
		       void* callData);
  static void aboutCbk( void* ptr,
		       void* callData);
  static void quitCbk( void* ptr,
		      void* callData);
  static void openCbk( void* ptr,
		      void* callData);
  static void openInNewWindowCbk( void* ptr,
				 void* callData);
  static void reloadCbk( void* ptr,
			void* callData);
  static void saveAsCbk( void* ptr,
			void* callData);
  static void continuousModeToggleCbk( void* ptr,
				      void* callData);
  static void fullScreenToggleCbk( void* ptr,
				  void* callData);
  static void rotateCCWCbk( void* ptr,
			   void* callData);
  static void rotateCWCbk( void* ptr,
			  void* callData);
  static void zoomToSelectionCbk( void* ptr,
				 void* callData);
  
  static void pageNumCbk( void* ptr,
			 void* callData);
  static void updateCbk(void *data, GString *fileName,
			int pageNum, int numPages, char *linkString);

  static void prevPageCbk( void* ptr,
			  void* callData);
  static void prevTenPageCbk( void* ptr,
			     void* callData);
  static void nextPageCbk( void* ptr,
			  void* callData);
  static void nextTenPageCbk( void* ptr,
			     void* callData);
  static void backCbk( void* ptr,
		      void* callData);
  static void forwardCbk( void* ptr,
			 void* callData);



	GPDFCore *core;
	GPDFApp *app;

  gi_window_id_t getWindow() { return win; }
private:

	//----- command functions
  void cmdAbout(GString *args[], int nArgs, gi_msg_t *event);
  void cmdCloseOutline(GString *args[], int nArgs, gi_msg_t *event);
  void cmdCloseWindow(GString *args[], int nArgs, gi_msg_t *event);
  void cmdContinuousMode(GString *args[], int nArgs, gi_msg_t *event);
  void cmdEndPan(GString *args[], int nArgs, gi_msg_t *event);
  void cmdEndSelection(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFind(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFindNext(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFocusToDocWin(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFocusToPageNum(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFollowLink(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFollowLinkInNewWin(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFollowLinkInNewWinNoSel(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFollowLinkNoSel(GString *args[], int nArgs, gi_msg_t *event);
  void cmdFullScreenMode(GString *args[], int nArgs, gi_msg_t *event);
  void cmdGoBackward(GString *args[], int nArgs, gi_msg_t *event);
  void cmdGoForward(GString *args[], int nArgs, gi_msg_t *event);
  void cmdGotoDest(GString *args[], int nArgs, gi_msg_t *event);
  void cmdGotoLastPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdGotoLastPageNoScroll(GString *args[], int nArgs, gi_msg_t *event);
  void cmdGotoPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdGotoPageNoScroll(GString *args[], int nArgs, gi_msg_t *event);
  void cmdNextPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdNextPageNoScroll(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpen(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenFile(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenFileAtDest(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenFileAtDestInNewWin(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenFileAtPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenFileAtPageInNewWin(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenFileInNewWin(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenInNewWin(GString *args[], int nArgs, gi_msg_t *event);
  void cmdOpenOutline(GString *args[], int nArgs, gi_msg_t *event);
  void cmdPageDown(GString *args[], int nArgs, gi_msg_t *event);
  void cmdPageUp(GString *args[], int nArgs, gi_msg_t *event);
  void cmdPostPopupMenu(GString *args[], int nArgs, gi_msg_t *event);
  void cmdPrevPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdPrevPageNoScroll(GString *args[], int nArgs, gi_msg_t *event);
  void cmdPrint(GString *args[], int nArgs, gi_msg_t *event);
  void cmdQuit(GString *args[], int nArgs, gi_msg_t *event);
  void cmdRaise(GString *args[], int nArgs, gi_msg_t *event);
  void cmdRedraw(GString *args[], int nArgs, gi_msg_t *event);
  void cmdReload(GString *args[], int nArgs, gi_msg_t *event);
  void cmdRun(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollDown(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollDownNextPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollLeft(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollOutlineDown(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollOutlineUp(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollRight(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollToBottomEdge(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollToBottomRight(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollToLeftEdge(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollToRightEdge(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollToTopEdge(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollToTopLeft(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollUp(GString *args[], int nArgs, gi_msg_t *event);
  void cmdScrollUpPrevPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdSinglePageMode(GString *args[], int nArgs, gi_msg_t *event);
  void cmdStartPan(GString *args[], int nArgs, gi_msg_t *event);
  void cmdStartSelection(GString *args[], int nArgs, gi_msg_t *event);
  void cmdToggleContinuousMode(GString *args[], int nArgs, gi_msg_t *event);
  void cmdToggleFullScreenMode(GString *args[], int nArgs, gi_msg_t *event);
  void cmdToggleOutline(GString *args[], int nArgs, gi_msg_t *event);
  void cmdWindowMode(GString *args[], int nArgs, gi_msg_t *event);
  void cmdZoomFitPage(GString *args[], int nArgs, gi_msg_t *event);
  void cmdZoomFitWidth(GString *args[], int nArgs, gi_msg_t *event);
  void cmdZoomIn(GString *args[], int nArgs, gi_msg_t *event);
  void cmdZoomOut(GString *args[], int nArgs, gi_msg_t *event);
  void cmdZoomPercent(GString *args[], int nArgs, gi_msg_t *event);
  void cmdZoomToSelection(GString *args[], int nArgs, gi_msg_t *event);

	 int getModifiers(Guint modifiers);
  int getContext(Guint modifiers);

  void doFind(GBool next);
  void doFind(char *findText, GBool findCaseSensitiveToggle, GBool findBackwardToggle, GBool next);

  


  //----- keyboard/mouse input
  static void keyPressCbk(void *data, int key, Guint modifiers,
			  gi_msg_t *event);
  static void mouseCbk(void *data, gi_msg_t *event);
  //static void updateCbk(void *data, GString *fileName,
	//		int pageNum, int numPages, char *linkString);

	//----- load / display
  GBool loadFile(GString *fileName, GString *ownerPassword = NULL,
		 GString *userPassword = NULL);
  void displayPage(int pageA, double zoomA, int rotateA,
                   GBool scrollToTop, GBool addToHist);
  void displayDest(LinkDest *dest, double zoomA, int rotateA,
		   GBool addToHist);
  void getPageAndDest(int pageA, GString *destName,
		      int *pageOut, LinkDest **destOut);

  //----- hyperlinks / actions
  void doLink(int wx, int wy, GBool onlyIfNoSelection, GBool newWin);
  static void actionCbk(void *data, char *action);

  void mapWindow();
  void closeWindow();
  int getZoomIdx();
  void setZoomIdx(int idx);
  void setZoomVal(double z);
   void initWindow(GBool fullScreen); 
  void initCore(gi_window_id_t parent, GBool fullScreen);


	GBool ok;
	gi_window_id_t win;
	static XPDFViewerCmd cmdTab[];

};

#endif
