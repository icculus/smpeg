#ifndef _LOKITV_H
#define _LOKITV_H

//============================================================================
//
// $Id$
//
// LokiTV MPEG player
// Copyright (C) 1999 Loki Entertainment Software
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//============================================================================


#include <stdio.h>

#include <qwidget.h>
#include <qdatetime.h>

#include "smpeg.h"


class QPushButton;
class QBoxLayout;
class QLabel;
class QMenuBar;
class QCheckBox;


class LokiTV : public QWidget
{
    Q_OBJECT

public:

    LokiTV( QWidget* parent = 0 );

    ~LokiTV();

    void openFile( const char* );

protected:

    void timerEvent( QTimerEvent* );

private slots:

    void openFile();
    void clearscreen();
    void play();
    void pause();
    void stop();
    void step();
    void skipToEnd();
    void rewind();
    void enableDouble( bool );
    void enableLoop( bool );
    void enableAudio( bool );
    void about();
    void aboutMovie();

private:

    void _createMenus();
    void _closeFile();
    void _createFileInfo( QBoxLayout* );
    void _createButtons( QBoxLayout* );
    void _createStatus( QBoxLayout* );
    void _clearFPS( void );
    void _updateFPS( void );
    QPushButton* button( const char* label, QBoxLayout* lo );

    SDL_Surface* sdl_screen;
    SMPEG_Info _info;
    SMPEG* _mpeg;
    bool _paused;

    int _timerId;
    QMenuBar* _menu;
    QLabel* _fileLoaded;
    QLabel* _fps;
    QLabel* _framesPlayed;
    QCheckBox* _doubleEnable;
    QCheckBox* _loopEnable;
    QCheckBox* _audioEnable;
};


#endif // _LOKITV_H
