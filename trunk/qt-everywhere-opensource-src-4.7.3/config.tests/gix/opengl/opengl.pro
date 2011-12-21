SOURCES = opengl.cpp
CONFIG += gix
INCLUDEPATH += $$QMAKE_INCDIR_OPENGL

for(p, QMAKE_LIBDIR_OPENGL) {
    exists($$p):LIBS += -L$$p
}

CONFIG -= qt
win32-g++*:LIBS += -lopengl32
else:LIBS += -lGL -lGLU
