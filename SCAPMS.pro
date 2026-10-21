# =============================================================================
#  SCAPMS - Smart Clinic Appointment & Patient Management System
#  File   : SCAPMS.pro  (qmake project file)
#
#  Build with the Qt 6 MinGW kit:
#      qmake SCAPMS.pro
#      mingw32-make -j4
# =============================================================================
QT += core gui widgets

CONFIG += c++17 warn_on release
CONFIG -= debug_and_release debug

TEMPLATE = app
TARGET = SCAPMS
DESTDIR = $$OUT_PWD

INCLUDEPATH += $$PWD/src

SOURCES += $$files($$PWD/src/*.cpp, true)
SOURCES += $$files($$PWD/tests/*.cpp, true)

HEADERS += $$files($$PWD/src/*.h, true)
HEADERS += $$files($$PWD/tests/*.h, true)

RESOURCES += $$PWD/resources/resources.qrc

# Keep the console attached so that "--selftest" can print its report.
CONFIG += console

MOC_DIR = $$OUT_PWD/.moc
OBJECTS_DIR = $$OUT_PWD/.obj
RCC_DIR = $$OUT_PWD/.rcc
