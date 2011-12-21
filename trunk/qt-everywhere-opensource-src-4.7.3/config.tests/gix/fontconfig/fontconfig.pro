SOURCES = fontconfig.cpp
CONFIG += gix
CONFIG -= qt
LIBS += -lfreetype -lfontconfig
include(../../unix/freetype/freetype.pri)
