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


#include <stdlib.h>
#include <string.h>

#include <qpushbutton.h>
#include <qapplication.h>
#include <qmenubar.h>
#include <qfiledialog.h>
#include <qkeycode.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qmessagebox.h>
#include <qcheckbox.h>

#include "lokiTV.h"



LokiTV::LokiTV( QWidget* parent ) : QWidget( parent )
{
    QVBoxLayout* lo = new QVBoxLayout( this, 4 );

    _createMenus();
    lo->setMenuBar( _menu );

    _createFileInfo( lo );

    _createButtons( lo );

    _createStatus( lo );

    lo->activate();


    setCaption( "LokiTV" );
    resize( 400, _menu->height() + 132 );
    
    sdl_screen = 0;
    _mpeg = 0;

#if 0 // This takes up too much CPU during movie play
    /* Set up a general timer */
    _timerId = startTimer( 1000 / 2 );
#endif
}


LokiTV::~LokiTV()
{
    _closeFile();
}


void LokiTV::_createMenus()
{
    QPopupMenu *file = new QPopupMenu();
    CHECK_PTR( file );
    file->insertItem( "&Open",  this, SLOT(openFile()), CTRL+Key_O );
    file->insertItem( "About File",  this, SLOT(aboutMovie()) );
    file->insertItem( "About LokiTV", this, SLOT(about()) );
    file->insertSeparator();
    file->insertItem( "E&xit",  qApp, SLOT(quit()), CTRL+Key_Q );

    _menu = new QMenuBar( this );
    CHECK_PTR( _menu );
    _menu->insertItem( "&File", file );
}


void LokiTV::_createFileInfo( QBoxLayout* parent )
{
    QHBoxLayout* lo;
    QLabel* l;

    /* Create a layout box for the option checkboxes */
    lo = new QHBoxLayout;
    parent->addLayout( lo );

    l = new QLabel( "File:", this );
    l->setAlignment( AlignLeft | AlignVCenter );
    lo->addWidget( l );
    _fileLoaded = new QLabel( this );
    _fileLoaded->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    _fileLoaded->setMinimumWidth(100);
    lo->addWidget( _fileLoaded );
}


void LokiTV::_createButtons( QBoxLayout* parent )
{
    QHBoxLayout* lo;
    QButton* b;

    /* Create a layout box for the buttons */
    lo = new QHBoxLayout;
    parent->addLayout( lo );

    b = button( "Play", lo );
    connect( b, SIGNAL(released()), SLOT(play()) );

    b = button( "Pause", lo );
    b->setAccel( ' ' );
    connect( b, SIGNAL(released()), SLOT(pause()) );

    b = button( "Stop", lo );
    connect( b, SIGNAL(released()), SLOT(stop()) );

    b = button( "Step", lo );
    connect( b, SIGNAL(released()), SLOT(step()) );

    b = button( "To End", lo );
    connect( b, SIGNAL(released()), SLOT(skipToEnd()) );
}


void LokiTV::_createStatus( QBoxLayout* parent )
{
    QHBoxLayout* lo;
    QLabel* l;

    /* Create a layout box for the option checkboxes */
    lo = new QHBoxLayout;
    parent->addLayout( lo );

    _doubleEnable = new QCheckBox( "Double", this );
    _doubleEnable->setChecked( FALSE );
    connect( _doubleEnable, SIGNAL(toggled(bool)), SLOT(enableDouble(bool)) );
    lo->addWidget( _doubleEnable );
    _loopEnable = new QCheckBox( "Loop", this );
    _loopEnable->setChecked( FALSE );
    connect( _loopEnable, SIGNAL(toggled(bool)), SLOT(enableLoop(bool)) );
    lo->addWidget( _loopEnable );
    _audioEnable = new QCheckBox( "Audio", this );
    _audioEnable->setChecked( TRUE );
    connect( _audioEnable, SIGNAL(toggled(bool)), SLOT(enableAudio(bool)) );
    lo->addWidget( _audioEnable );

    /* Create a layout box for the framerate display */
    lo = new QHBoxLayout;
    parent->addLayout( lo );

    l = new QLabel( "Frames:", this );
    l->setAlignment( AlignRight | AlignVCenter );
    lo->addWidget( l );

    _framesPlayed = new QLabel( this );
    _framesPlayed->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    _framesPlayed->setFixedSize( 50, 18 );
    lo->addWidget( _framesPlayed );

    l = new QLabel( "FPS:", this );
    l->setAlignment( AlignRight | AlignVCenter );
    lo->addWidget( l );

    _fps = new QLabel( this );
    _fps->setFrameStyle( QFrame::Panel | QFrame::Sunken );
    _fps->setFixedSize( 40, 18 );
    lo->addWidget( _fps );
}

void
LokiTV:: _clearFPS(void)
{
    _fps->setText( "" );
    _framesPlayed->setNum( 0 );
}


void
LokiTV:: _updateFPS(void)
{
    char ave[ 20 ];

    SMPEG_getinfo(_mpeg, &_info);
    _framesPlayed->setNum( _info.current_frame );
    sprintf( ave, "%2.2f", _info.current_fps );
    _fps->setText( ave );
}


QPushButton* LokiTV::button( const char* label, QBoxLayout* lo )
{
    QPushButton* b;

    b = new QPushButton( this );
    b->setText( label );
    b->setFixedSize( b->sizeHint() );

    lo->addWidget( b );
    
    return b;
}


void LokiTV::openFile( const char* file )
{
    /* Close any current file */
    _closeFile();

    /* Create the MPEG stream */
    _mpeg = SMPEG_new(file, &_info, 1);
    if ( SMPEG_error(_mpeg) ) {
        QMessageBox::warning( this, file, SMPEG_error(_mpeg) );
        SMPEG_delete(_mpeg);
        _mpeg = NULL;
        return;
    }

    /* Set up video display if needed */
    if ( _info.has_video ) {
        sdl_screen=SDL_SetVideoMode(_info.width*2,_info.height*2,16,0);
        SMPEG_setdisplay(_mpeg, sdl_screen, NULL, NULL);
        enableDouble(_doubleEnable->isChecked());
    }
    enableLoop(_loopEnable->isChecked());

    /* Update the status bar */
    { const char *basefile;
        basefile = strrchr(file, '/');
        if ( basefile ) {
            ++basefile;
        } else {
            basefile = file;
        }
        _fileLoaded->setText(basefile);
    }
}


void LokiTV::_closeFile()
{
    stop();
    if ( _mpeg ) {
        SMPEG_delete( _mpeg );
    }
    _fileLoaded->setText("");
    _clearFPS();
}


void LokiTV::timerEvent( QTimerEvent* e )
{
    /* Handle SDL quit events */
    if ( SDL_QuitRequested() )
    {
        qApp->quit();
    }

    /* Print MPEG playback status */
    if ( _mpeg )
    {
        _updateFPS();
    }
}


void LokiTV::openFile()
{
    QString file;

    file = QFileDialog::getOpenFileName( 0, 0, this );
    if( ! file.isEmpty() )
    {
        openFile( file );
    }
}

void LokiTV::clearscreen()
{
    if ( sdl_screen ) {
        SDL_FillRect(sdl_screen,NULL,SDL_MapRGB(sdl_screen->format,0,0,0));
        SDL_UpdateRect(sdl_screen, 0, 0, 0, 0);
    }
}

void LokiTV::play()
{
    if( _mpeg )
    {
        /* Set audio playback */
        SMPEG_enableaudio(_mpeg, _audioEnable->isChecked());

        /* Play the movie */
        rewind();
        SMPEG_play( _mpeg );
    }
}


void LokiTV::pause()
{
    if( _mpeg )
    {
        SMPEG_pause( _mpeg );
    }
}


void LokiTV::stop()
{
    if( _mpeg )
    {
        /* Stop playback */
        SMPEG_stop( _mpeg );
        rewind();
    }
}

void LokiTV::step()
{
    if( _mpeg && _info.has_video && (SMPEG_status(_mpeg) != SMPEG_PLAYING) )
    {
        int next_frame;

        /* Play the next frame and update the status */
        SMPEG_getinfo( _mpeg, &_info );
        next_frame = _info.current_frame+1;
        if ( _doubleEnable->isChecked() )
            SMPEG_renderFrame( _mpeg, next_frame, sdl_screen, 0, 0 );
        else
            SMPEG_renderFrame( _mpeg, next_frame, sdl_screen,
                               (sdl_screen->w-_info.width)/2,
                               (sdl_screen->h-_info.height)/2);

        /* See if we need to rewind (have we reached end of film?) */
        SMPEG_getinfo( _mpeg, &_info );
        if ( _info.current_frame != next_frame )
        {
            rewind();
            step();
        }
        _framesPlayed->setNum( _info.current_frame );
    }
}

void LokiTV::skipToEnd()
{
    if( _mpeg )
    {
        /* Stop playback */
        SMPEG_stop( _mpeg );

        /* Display the last frame */
        if ( _doubleEnable->isChecked() )
            SMPEG_renderFinal( _mpeg, sdl_screen, 0, 0 );
        else
            SMPEG_renderFinal( _mpeg, sdl_screen,
                               (sdl_screen->w-_info.width)/2,
                               (sdl_screen->h-_info.height)/2);

        /* Show the frame number of the last frame */
        SMPEG_getinfo( _mpeg, &_info );
        _framesPlayed->setNum( _info.current_frame );
    }
}


void LokiTV::rewind()
{
    if( _mpeg )
    {
        /* Rewind the movie */
        SMPEG_rewind( _mpeg );
        clearscreen();

        /* Update the frame display */
        _framesPlayed->setNum( _info.current_frame );
    }
}

void LokiTV::enableDouble( bool on )
{
    if ( _mpeg && sdl_screen )
    {
        if ( on )
            SMPEG_move( _mpeg, 0, 0 );
        else
            SMPEG_move( _mpeg, (sdl_screen->w-_info.width)/2,
                               (sdl_screen->h-_info.height)/2);
        SMPEG_double(_mpeg, on);
        clearscreen();
    }
}

void LokiTV::enableLoop( bool on )
{
    if ( _mpeg )
    {
        SMPEG_loop( _mpeg, on );
    }
}

void LokiTV::enableAudio( bool on )
{
    return;
}

void LokiTV::about()
{
    QMessageBox::about( this, "About LokiTV", "MPEG 1 audio/video player\n" );
}


void LokiTV::aboutMovie()
{
    QString infoStr;

    if ( _mpeg ) {
        infoStr.sprintf( "File: %s (%s stream)\n\nSize: %dx%d\n",
                _fileLoaded->text(),
                (_info.has_audio && _info.has_video) ? "system" :
                (_info.has_video ? "video" :
                (_info.has_audio ? "audio" : "not an MPEG")),
                _info.width, _info.height);

        QMessageBox::information( this, "About Movie", infoStr );
    }
}


int main( int argc, char** argv )
{
    QApplication app( argc, argv );
    app.setFont( QFont( "helvetica" ) );

    if( SDL_Init( SDL_INIT_AUDIO|SDL_INIT_VIDEO ) < 0 )
    {
        fprintf( stderr, "Couldn't initialize SDL: %s\n", SDL_GetError() );
        exit( 1 );
    }
    atexit( SDL_Quit );

    LokiTV* gui = new LokiTV;
    app.setMainWidget( gui );
    gui->show();

    if( app.argc() > 1 )
    {
        gui->openFile( app.argv()[ 1 ] );
    }

    return app.exec();
}
 

// EOF
