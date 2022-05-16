QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Entities/collisionbox.cpp \
    Entities/entity.cpp \
    #Entities/monster.cpp \
    #Entities/movingentity.cpp \
    #Entities/movingmonster.cpp \
    #Entities/player.cpp \
    #Entities/staticmonster.cpp \
    Entities/living.cpp \
    Entities/projectile.cpp \
    Entities/samos.cpp \
    Entities/terrain.cpp \
    main.cpp \
    mainwindow.cpp \
    save.cpp

HEADERS += \
    Entities/collisionbox.h \
    Entities/entity.h \
    #Entities/monster.h \
    #Entities/movingentity.h \
    #Entities/movingmonster.h \
    #Entities/player.h \
    #Entities/staticmonster.h \
    Entities/living.h \
    Entities/projectile.h \
    Entities/samos.h \
    Entities/terrain.h \
    mainwindow.h \
    nlohmann/json.hpp \
    save.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    Maitrohide_fr_FR.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
