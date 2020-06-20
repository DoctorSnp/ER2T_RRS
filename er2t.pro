# Шаблон проекта - динамическая библиотека (DLL)
TEMPLATE = lib
# Задаем те библиотеки QT, которые нам пригодятся
QT -= gui
QT += xml
QT += core
QT += network

RRS_ROOT="/home/den/RussianRailSimulator/"

HARD_FLAGS=" -Werror"
EXTERNAL_SHARE_FLAGS="-Wall -W -Wtype-limits -Wextra -Wempty-body \
                        -Wmaybe-uninitialized -Warray-bounds "
EXTERNAL_C_FLAGS=" -std=c11"
EXTERNAL_CXX_FLAGS=" -std=c++11  -Woverloaded-virtual -Wctor-dtor-privacy -Wenum-compare "
QMAKE_LFLAGS += -z defs


OBJECTS_DIR=temp/obj
MOC_DIR=temp/moc

lessThan(QT_MAJOR_VERSION, 5):error("This project reguire QT >= 5.0")
#CONFIG += c++11 console
#CONFIG -= app_bundle


# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

!isEmpty(HARD_CONTROL) {
    EXTERNAL_SHARE_FLAGS+=$${HARD_FLAGS}
    message("Hard warnings control")
}

QMAKE_CXXFLAGS +=   $${EXTERNAL_SHARE_FLAGS} $${EXTERNAL_CXX_FLAGS}
#QMAKE_CFLAGS +=  $${EXTERNAL_SHARE_FLAGS} $${EXTERNAL_C_FLAGS}


message( "ROOT ->" $$RRS_ROOT)
# Имя итогового файла библиотеки и путь, куда он должен быть помещен после
# сборки
TARGET = er2t
DESTDIR = $$RRS_ROOT/modules/$$join(TARGET,,,)

# Библиотеки симулятора, с которыми компонуется DLL локомотива
CONFIG(debug, debug|release){
LIBS += -L$$RRS_ROOT/lib -lCfgReader_d
LIBS += -L$$RRS_ROOT/lib -lphysics_d
LIBS += -L$$RRS_ROOT/lib -lvehicle_d
LIBS += -L$$RRS_ROOT/lib -ldevice_d
LIBS += -L$$RRS_ROOT/lib -lfilesystem_d

} else {

LIBS += -L$$RRS_ROOT/lib -lCfgReader
LIBS += -L$$RRS_ROOT/lib -lphysics
LIBS += -L$$RRS_ROOT/lib -lvehicle
LIBS += -L$$RRS_ROOT/lib -ldevice
LIBS += -L$$RRS_ROOT/lib -lfilesystem
}
# Путь к необходимым заголовочным файлам
SOURCES += \
    ../../RRS/simulator/device/src/airdistributor.cpp \
    ../../RRS/simulator/device/src/automatic-train-stop.cpp \
    ../../RRS/simulator/device/src/brake-crane.cpp \
    ../../RRS/simulator/device/src/brake-device.cpp \
    ../../RRS/simulator/device/src/brake-mech.cpp \
    ../../RRS/simulator/device/src/debug.cpp \
    ../../RRS/simulator/device/src/device.cpp \
    ../../RRS/simulator/device/src/loco-crane.cpp \
    ../../RRS/simulator/device/src/oscillator.cpp \
    ../../RRS/simulator/device/src/pantograph.cpp \
    ../../RRS/simulator/device/src/pneumo-relay.cpp \
    ../../RRS/simulator/device/src/pneumo-splitter.cpp \
    ../../RRS/simulator/device/src/protective-device.cpp \
    ../../RRS/simulator/device/src/reservoir.cpp \
    ../../RRS/simulator/device/src/switching-valve.cpp \
    ../../RRS/simulator/device/src/timer.cpp \
    ../../RRS/simulator/device/src/traction-controller.cpp \
    ../../RRS/simulator/device/src/train-horn.cpp \
    ../../RRS/simulator/device/src/trigger.cpp \
    ../../RRS/simulator/device/src/ubt367m.cpp \
    ../../RRS/simulator/device/src/virtual-interface-device.cpp \
    src/er2t.cpp \
    src/main_systems/electric/phase-splitter.cpp \
    src/main_systems/network.cpp \
    src/main_systems/pneumatic/motor-compressor.cpp \
    src/main_systems/pneumatic/motor-fan.cpp \
    src/main_systems/pneumatic/pressure-regulator.cpp \
    src/main_systems/pneumatic/trolley-brake-mech.cpp
INCLUDEPATH += $$(SOURCES)
INCLUDEPATH += src/inc
INCLUDEPATH += src/main_systems/

INCLUDEPATH += $$RRS_ROOT/RRS/CfgReader/include
INCLUDEPATH += $$RRS_ROOT/RRS/filesystem/include/
INCLUDEPATH += $$RRS_ROOT/RRS/simulator/device/include/ #sdk/include
INCLUDEPATH += $$RRS_ROOT/RRS/simulator/vehicle/include/
INCLUDEPATH += $$RRS_ROOT/RRS/simulator/solver/include/
INCLUDEPATH += $$RRS_ROOT/RRS/simulator/physics/include/

HEADERS += \
    src/er2t.h \
    src/main_systems/electric/phase-splitter.h \
    src/main_systems/network.h \
    src/main_systems/pneumatic/motor-compressor.h \
    src/main_systems/pneumatic/motor-fan.h \
    src/main_systems/pneumatic/pressure-regulator.h \
    src/main_systems/pneumatic/trolley-brake-mech.h

DISTFILES += \
    trains/er2t.xml \
    vehicle/brakes-init.xml \
    vehicle/er2t.xml \
    vehicle/main-switch.xml \
    vehicle/motor-brakes-mech.xml \
    vehicle/motor-compressor.xml


