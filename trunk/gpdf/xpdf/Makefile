#========================================================================
#
# Xpdf Makefile
#
# Copyright 1996-2003 Glyph & Cog, LLC
#
#========================================================================

SHELL = /bin/sh

prefix = /usr/local
srcdir = .


GOOSRCDIR = $(srcdir)/../goo
GOOLIBDIR = ../goo
FOFISRCDIR = $(srcdir)/../fofi
FOFILIBDIR = ../fofi
SPLASHSRCDIR = $(srcdir)/../splash
SPLASHLIBDIR = ../splash

CXXFLAGS =     -g -O2 -DHAVE_CONFIG_H -I.. -I$(GOOSRCDIR) -I$(FOFISRCDIR) -I$(SPLASHSRCDIR) -I$(srcdir)          


ifeq ($(OS), linux)
CXXFLAGS +=  -I/usr/local/include/freetype2/ -I/opt/gi/include/freetype2/ -I/opt/gi/include/
else
CXXFLAGS +=  -nostdlib -nostdinc   -I$(SDKDIR)/uclibc/include  -I$(SDKDIR)/uclibc/include/freetype2 
LDFLAGS =  -nostdlib  -Bdynamic --nostdlib --script=$(SDKDIR)/bin/app.ld
endif



T1LIBS = 
FTLIBS = 

XLIBS =      -lXpm    

SPLASHLIBS = -L$(SPLASHLIBDIR) -lsplash

OTHERLIBS =   \
	-L$(FOFILIBDIR) -lfofi \
	-L$(GOOLIBDIR) -lGoo


AR = $(CROSSCOMPILER)ar rc
CC = $(CROSSCOMPILER)gcc
LD = $(CROSSCOMPILER)ld
STRIP = $(CROSSCOMPILER)strip
CXX =  $(CROSSCOMPILER)g++
RANLIB = $(CROSSCOMPILER)ranlib
LIBPREFIX = lib
EXE = 

#------------------------------------------------------------------------

.SUFFIXES: .cc

.cc.o:
	$(CXX) $(CXXFLAGS) -c $<

#------------------------------------------------------------------------

CXX_SRC = \
	$(srcdir)/Annot.cc \
	$(srcdir)/Array.cc \
	$(srcdir)/BuiltinFont.cc \
	$(srcdir)/BuiltinFontTables.cc \
	$(srcdir)/CMap.cc \
	$(srcdir)/Catalog.cc \
	$(srcdir)/CharCodeToUnicode.cc \
	$(srcdir)/CoreOutputDev.cc \
	$(srcdir)/Decrypt.cc \
	$(srcdir)/Dict.cc \
	$(srcdir)/Error.cc \
	$(srcdir)/FontEncodingTables.cc \
	$(srcdir)/Function.cc \
	$(srcdir)/Gfx.cc \
	$(srcdir)/GfxFont.cc \
	$(srcdir)/GfxState.cc \
	$(srcdir)/GlobalParams.cc \
	$(srcdir)/ImageOutputDev.cc \
	$(srcdir)/JArithmeticDecoder.cc \
	$(srcdir)/JBIG2Stream.cc \
	$(srcdir)/JPXStream.cc \
	$(srcdir)/Lexer.cc \
	$(srcdir)/Link.cc \
	$(srcdir)/NameToCharCode.cc \
	$(srcdir)/Object.cc \
	$(srcdir)/Outline.cc \
	$(srcdir)/OutputDev.cc \
	$(srcdir)/PDFCore.cc \
	$(srcdir)/PDFDoc.cc \
	$(srcdir)/PDFDocEncoding.cc \
	$(srcdir)/PSOutputDev.cc \
	$(srcdir)/PSTokenizer.cc \
	$(srcdir)/Page.cc \
	$(srcdir)/Parser.cc \
	$(srcdir)/PreScanOutputDev.cc \
	$(srcdir)/SecurityHandler.cc \
	$(srcdir)/SplashOutputDev.cc \
	$(srcdir)/Stream.cc \
	$(srcdir)/TextOutputDev.cc \
	$(srcdir)/UnicodeMap.cc \
	$(srcdir)/UnicodeTypeTable.cc \
	$(srcdir)/XPDFApp.cc \
	$(srcdir)/XPDFCore.cc \
	$(srcdir)/XPDFTree.cc \
	$(srcdir)/XPDFViewer.cc \
	$(srcdir)/XpdfPluginAPI.cc \
	$(srcdir)/XRef.cc \
	$(srcdir)/pdftops.cc \
	$(srcdir)/pdftotext.cc \
	$(srcdir)/pdfinfo.cc \
	$(srcdir)/pdffonts.cc \
	$(srcdir)/pdftoppm.cc \
	$(srcdir)/pdfimages.cc \
	$(srcdir)/xpdf.cc

#------------------------------------------------------------------------

all: xpdf$(EXE) 
#pdftops$(EXE) pdftotext$(EXE) pdfinfo$(EXE) \
#	pdffonts$(EXE) pdftoppm$(EXE) pdfimages$(EXE)

all-no-x: pdftops$(EXE) pdftotext$(EXE) pdfinfo$(EXE) pdffonts$(EXE) \
	pdfimages$(EXE)

#------------------------------------------------------------------------

XPDF_OBJS = Annot.o Array.o BuiltinFont.o BuiltinFontTables.o Catalog.o \
	CharCodeToUnicode.o CMap.o CoreOutputDev.o Decrypt.o Dict.o \
	Error.o FontEncodingTables.o Function.o Gfx.o GfxFont.o \
	GfxState.o GlobalParams.o JArithmeticDecoder.o JBIG2Stream.o \
	JPXStream.o Lexer.o Link.o NameToCharCode.o Object.o Outline.o \
	OutputDev.o Page.o Parser.o PDFCore.o PDFDoc.o PDFDocEncoding.o \
	PreScanOutputDev.o PSOutputDev.o PSTokenizer.o SecurityHandler.x.o \
	SplashOutputDev.o Stream.o TextOutputDev.o UnicodeMap.o \
	UnicodeTypeTable.o  gpdf.o GPDFApp.o GPDFViewer.o\
	XpdfPluginAPI.o XRef.o

X11OBJ=XPDFCore.o XPDFViewer.o XPDFApp.o xpdf.o XPDFTree.o


XPDF_LIBS = -L$(GOOLIBDIR) -lGoo $(SPLASHLIBS) $(T1LIBS) $(FTLIBS) \
	$(XLIBS) $(OTHERLIBS) $(OTHERLIBS)   -L$(SDKDIR)/uclibc//lib/ -lgi -lfreetype -lpng -lz -lm -lnetbas -lcpp   -lc -lgcc

xpdf$(EXE): $(XPDF_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(LD) $(LDFLAGS) -o xpdf$(EXE)  $(SDKDIR)/uclibc//lib/crt1.o $(XPDF_OBJS) $(XPDF_LIBS)
	cp -fr  $@  $(SDKDIR)/livecd/iso/system/bin/


XPDF_LINUX_LIBS = -L$(GOOLIBDIR) -lGoo $(SPLASHLIBS) $(T1LIBS) $(FTLIBS) \
	 $(OTHERLIBS) $(OTHERLIBS) -L/opt/gi/lib   -lgi -lfreetype -lpng -lz -lm  -lstdc++ 

xpdf_linux$(EXE): $(XPDF_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(CC) $(LDFLAGS) -o xpdf$(EXE)   $(XPDF_OBJS) $(XPDF_LINUX_LIBS)

cl:
	rm -f gpdf.o GPDFApp.o GPDFViewer.o
#------------------------------------------------------------------------

PDFTOPS_OBJS = Annot.o Array.o BuiltinFont.o BuiltinFontTables.o \
	Catalog.o CharCodeToUnicode.o CMap.o Decrypt.o Dict.o Error.o \
	FontEncodingTables.o Function.o Gfx.o GfxFont.o \
	GfxState.o GlobalParams.o JArithmeticDecoder.o JBIG2Stream.o \
	JPXStream.o Lexer.o Link.o NameToCharCode.o Outline.o Object.o \
	OutputDev.o Page.o Parser.o PDFDoc.o PDFDocEncoding.o \
	PreScanOutputDev.o PSOutputDev.o PSTokenizer.o SecurityHandler.o \
	SplashOutputDev.o Stream.o UnicodeMap.o XpdfPluginAPI.o XRef.o \
	pdftops.o
PDFTOPS_LIBS = -L$(GOOLIBDIR) -lGoo $(SPLASHLIBS) $(T1LIBS) $(FTLIBS) \
	$(OTHERLIBS) $(SDKDIR)/livecd/iso/system/lib/libX11.so  -L$(SDKDIR)/uclibc//lib/ -L../../exports/lib -lz -lm -lnetbas -lcpp   -lc -lgcc

pdftops$(EXE): $(PDFTOPS_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(LD)  $(LDFLAGS) -o pdftops$(EXE) $(SDKDIR)/uclibc//lib/crt0.o $(PDFTOPS_OBJS) \
		$(PDFTOPS_LIBS)

#------------------------------------------------------------------------

PDFTOTEXT_OBJS = Annot.o Array.o BuiltinFont.o BuiltinFontTables.o \
	Catalog.o CharCodeToUnicode.o CMap.o Decrypt.o Dict.o Error.o \
	FontEncodingTables.o Function.o Gfx.o GfxFont.o \
	GfxState.o GlobalParams.o JArithmeticDecoder.o JBIG2Stream.o \
	JPXStream.o Lexer.o Link.o NameToCharCode.o Object.o Outline.o \
	OutputDev.o Page.o Parser.o PDFDoc.o PDFDocEncoding.o PSTokenizer.o \
	SecurityHandler.o Stream.o TextOutputDev.o UnicodeMap.o \
	UnicodeTypeTable.o XpdfPluginAPI.o XRef.o pdftotext.o
PDFTOTEXT_LIBS = -L$(GOOLIBDIR) -lGoo $(OTHERLIBS) $(OTHERLIBS) $(SDKDIR)/livecd/iso/system/lib/libX11.so  -L$(SDKDIR)/uclibc//lib/ -L../../exports/lib -lz -lm -lnetbas -lcpp   -lc -lgcc

pdftotext$(EXE): $(PDFTOTEXT_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(LD) $(LDFLAGS) -o pdftotext$(EXE)  $(SDKDIR)/uclibc//lib/crt0.o $(PDFTOTEXT_OBJS) \
		$(PDFTOTEXT_LIBS)

#------------------------------------------------------------------------

PDFINFO_OBJS = Annot.o Array.o BuiltinFont.o BuiltinFontTables.o \
	Catalog.o CharCodeToUnicode.o CMap.o Decrypt.o Dict.o Error.o \
	FontEncodingTables.o Function.o Gfx.o GfxFont.o \
	GfxState.o GlobalParams.o JArithmeticDecoder.o JBIG2Stream.o \
	JPXStream.o Lexer.o Link.o NameToCharCode.o Object.o Outline.o \
	OutputDev.o Page.o Parser.o PDFDoc.o PDFDocEncoding.o PSTokenizer.o \
	SecurityHandler.o Stream.o UnicodeMap.o XpdfPluginAPI.o XRef.o \
	pdfinfo.o
PDFINFO_LIBS = -L$(GOOLIBDIR) -lGoo $(OTHERLIBS) $(OTHERLIBS) $(SDKDIR)/livecd/iso/system/lib/libX11.so  -L$(SDKDIR)/uclibc//lib/ -L../../exports/lib -lz -lm -lnetbas -lcpp   -lc -lgcc

pdfinfo$(EXE): $(PDFINFO_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(LD) $(LDFLAGS) -o pdfinfo$(EXE) $(SDKDIR)/uclibc//lib/crt0.o $(PDFINFO_OBJS) \
		$(PDFINFO_LIBS)

#------------------------------------------------------------------------

PDFFONTS_OBJS = Annot.o Array.o BuiltinFont.o BuiltinFontTables.o \
	Catalog.o CharCodeToUnicode.o CMap.o Decrypt.o Dict.o Error.o \
	FontEncodingTables.o Function.o Gfx.o GfxFont.o \
	GfxState.o GlobalParams.o JArithmeticDecoder.o JBIG2Stream.o \
	JPXStream.o Lexer.o Link.o NameToCharCode.o Object.o Outline.o \
	OutputDev.o Page.o Parser.o PDFDoc.o PDFDocEncoding.o PSTokenizer.o \
	SecurityHandler.o Stream.o UnicodeMap.o XpdfPluginAPI.o XRef.o \
	pdffonts.o
PDFFONTS_LIBS = -L$(GOOLIBDIR) -lGoo $(OTHERLIBS) $(OTHERLIBS) $(SDKDIR)/livecd/iso/system/lib/libX11.so  -L$(SDKDIR)/uclibc//lib/ -L../../exports/lib -lz -lm -lnetbas -lcpp   -lc -lgcc

pdffonts$(EXE): $(PDFFONTS_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(LD) $(LDFLAGS) -o pdffonts$(EXE) $(SDKDIR)/uclibc//lib/crt0.o $(PDFFONTS_OBJS) \
		$(PDFFONTS_LIBS)

#------------------------------------------------------------------------

PDFTOPPM_OBJS = Annot.o Array.o BuiltinFont.o BuiltinFontTables.o \
	Catalog.o CharCodeToUnicode.o CMap.o Decrypt.o Dict.o Error.o \
	FontEncodingTables.o Function.o Gfx.o GfxFont.o GfxState.o \
	GlobalParams.o JArithmeticDecoder.o JBIG2Stream.o JPXStream.o \
	Lexer.o Link.o NameToCharCode.o Object.o Outline.o OutputDev.o \
	Page.o Parser.o PDFDoc.o PDFDocEncoding.o PSTokenizer.o \
	SecurityHandler.o SplashOutputDev.o Stream.o TextOutputDev.o \
	UnicodeMap.o UnicodeTypeTable.o XpdfPluginAPI.o XRef.o pdftoppm.o
PDFTOPPM_LIBS = -L$(GOOLIBDIR) -lGoo $(SPLASHLIBS) $(T1LIBS) $(FTLIBS) \
	$(OTHERLIBS) $(OTHERLIBS) $(SDKDIR)/livecd/iso/system/lib/libX11.so  -L$(SDKDIR)/uclibc//lib/ -L../../exports/lib -lz -lm -lnetbas -lcpp   -lc -lgcc

pdftoppm$(EXE): $(PDFTOPPM_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(LD) $(LDFLAGS) -o pdftoppm$(EXE) $(SDKDIR)/uclibc//lib/crt0.o $(PDFTOPPM_OBJS) \
		$(PDFTOPPM_LIBS)

#------------------------------------------------------------------------

PDFIMAGES_OBJS = Annot.o Array.o BuiltinFont.o BuiltinFontTables.o \
	Catalog.o CharCodeToUnicode.o CMap.o Decrypt.o Dict.o Error.o \
	FontEncodingTables.o Function.o Gfx.o GfxFont.o GfxState.o \
	GlobalParams.o ImageOutputDev.o JArithmeticDecoder.o \
	JBIG2Stream.o JPXStream.o Lexer.o Link.o NameToCharCode.o Object.o \
	Outline.o OutputDev.o Page.o Parser.o PDFDoc.o PDFDocEncoding.o \
	PSTokenizer.o SecurityHandler.o Stream.o UnicodeMap.o \
	XpdfPluginAPI.o XRef.o pdfimages.o
PDFIMAGES_LIBS = -L$(GOOLIBDIR) -lGoo $(OTHERLIBS) $(OTHERLIBS) $(SDKDIR)/livecd/iso/system/lib/libX11.so  -L$(SDKDIR)/uclibc//lib/ -L../../exports/lib -lz -lm -lnetbas -lcpp   -lc -lgcc

pdfimages$(EXE): $(PDFIMAGES_OBJS) $(GOOLIBDIR)/$(LIBPREFIX)Goo.a
	$(LD) $(LDFLAGS) -o pdfimages$(EXE) $(SDKDIR)/uclibc//lib/crt0.o $(PDFIMAGES_OBJS) \
		$(PDFIMAGES_LIBS)

#------------------------------------------------------------------------

SecurityHandler.x.o: SecurityHandler.cc
	$(CXX) $(CXXFLAGS) -UHAVE_XPDFCORE -c -o SecurityHandler.x.o \
		SecurityHandler.cc

#------------------------------------------------------------------------

clean:
	rm -f $(XPDF_OBJS) xpdf$(EXE)
	rm -f $(PDFTOPS_OBJS) pdftops$(EXE)
	rm -f $(PDFTOTEXT_OBJS) pdftotext$(EXE)
	rm -f $(PDFINFO_OBJS) pdfinfo$(EXE)
	rm -f $(PDFFONTS_OBJS) pdffonts$(EXE)
	rm -f $(PDFTOPPM_OBJS) pdftoppm$(EXE)
	rm -f $(PDFIMAGES_OBJS) pdfimages$(EXE)

#------------------------------------------------------------------------

depend:
	$(CXX) $(CXXFLAGS) -MM $(CXX_SRC) >Makefile.dep

include Makefile.dep
