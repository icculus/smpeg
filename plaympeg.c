/*
   plaympeg - Sample MPEG player using the SMPEG library
   Copyright (C) 1999 Loki Entertainment Software
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef unix
#define NET_SUPPORT
#endif

#include <stdlib.h>
#include <string.h>
#ifdef NET_SUPPORT
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#endif
#include "smpeg.h"


void usage(char *argv0)
{
    printf(
"Usage: %s [options] file ...\n"
"Where the options are one of:\n"
"	--noaudio	     Don't play audio stream\n"
"	--novideo	     Don't play video stream\n"
"	--fullscreen	     Play MPEG in fullscreen mode\n"
"	--double or -2	     Play MPEG at double size\n"
"	--loop or -l	     Play MPEG over and over\n"
"	--volume N or -v N   Set audio volume to N (0-100)\n"
"	--scale wxh or -s wxh  Play MPEG at given resolution\n"
"       --seek N or -S N     Skip N bytes\n"
#ifdef USE_SYSTEM_TIMESTAMP
"       --skip N or -k N     Skip N seconds\n"
#endif
"	--help or -h\n"
"	--version or -V\n"
"Specifying - as filename will use stdin for input\n", argv0);
}

#ifdef NET_SUPPORT
int is_address_multicast(unsigned long address)
{
  if((address & 255) >= 224 && (address & 255) <= 239) return(1);
  return(0);
}
#endif

void update(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h)
{
    if ( screen->flags & SDL_DOUBLEBUF ) {
        SDL_Flip(screen);
    }
}

int main(int argc, char *argv[])
{
    int video_inited = 0, audio_inited = 0;
    int use_audio, use_video;
    int fullscreen;
    int scalesize;
    int scale_width, scale_height;
    int loop_play;
    int i, done, pause;
    int volume;
    Uint32 seek;
    float skip;
    SDL_Surface *screen;
    SMPEG *mpeg;
    SMPEG_Info info;
    char *basefile;
    SDL_version sdlver;
    SMPEG_version smpegver;

    /* Get the command line options */
    use_audio = 1;
    use_video = 1;
    fullscreen = 0;
    scalesize = 1;
    scale_width = 0;
    scale_height = 0;
    loop_play = 0;
    volume = 100;
    seek = 0;
    skip = 0;
    for ( i=1; argv[i] && (argv[i][0] == '-') && (argv[i][1] != 0); ++i ) {
        if ( (strcmp(argv[i], "--noaudio") == 0) ||
             (strcmp(argv[i], "--nosound") == 0) ) {
            use_audio = 0;
        } else
        if ( strcmp(argv[i], "--novideo") == 0 ) {
            use_video = 0;
        } else
        if ( strcmp(argv[i], "--fullscreen") == 0 ) {
            fullscreen = 1;
        } else
        if ((strcmp(argv[i], "--double") == 0)||(strcmp(argv[i], "-2") == 0)) {
            scalesize = 2;
        } else
        if ((strcmp(argv[i], "--loop") == 0) || (strcmp(argv[i], "-l") == 0)) {
            loop_play = 1;
        } else
        if ((strcmp(argv[i], "--seek") == 0)||(strcmp(argv[i], "-S") == 0)) {
            ++i;
            if ( argv[i] ) {
                seek = atol(argv[i]);
            }
        } else
        if ((strcmp(argv[i], "--skip") == 0)||(strcmp(argv[i], "-k") == 0)) {
            ++i;
            if ( argv[i] ) {
                skip = (float)atof(argv[i]);
            }
        } else
        if ((strcmp(argv[i], "--volume") == 0)||(strcmp(argv[i], "-v") == 0)) {
            ++i;
	    if (i >= argc)
	      {
		fprintf(stderr, "Please specify volume when using --volume or -v\n");
		exit(1);
	      }
            if ( argv[i] ) {
                volume = atoi(argv[i]);
            }
	    if ( ( volume < 0 ) || ( volume > 100 ) ) {
	      fprintf(stderr, "Volume must be between 0 and 100\n");
	      volume = 100;
	    }
	} else
        if ((strcmp(argv[i], "--version") == 0) ||
	    (strcmp(argv[i], "-V") == 0)) {
            SDL_VERSION(&sdlver);
            SMPEG_VERSION(&smpegver);
	    printf("SDL version: %d.%d.%d\n"
                   "SMPEG version: %d.%d.%d\n",
		   sdlver.major, sdlver.minor, sdlver.patch,
		   smpegver.major, smpegver.minor, smpegver.patch);
            exit(0);
        } else
        if ((strcmp(argv[i], "--scale") == 0)||(strcmp(argv[i], "-s") == 0)) {
            ++i;
            if ( argv[i] ) {
                sscanf(argv[i], "%dx%d", &scale_width, &scale_height);
            }
        } else
        if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0)) {
            usage(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "Warning: Unknown option: %s\n", argv[i]);
        }
    }
    /* If there were no arguments just print the usage */
    if (argc == 1) {
        usage(argv[0]);
        exit(0);
    }

    /* Play the mpeg files! */
    for ( ; argv[i]; ++i ) {
	/* Initialize SDL */
	if ( !video_inited && use_video ) {
	  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
	    fprintf(stderr, "Warning: Couldn't init SDL video: %s\n",
		    SDL_GetError());
	    fprintf(stderr, "Will ignore video stream\n");
	    use_video = 0;
	  }
	  else
	    video_inited = 1;
	}
	
	if ( !audio_inited && use_audio ) {
	  if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) {
	    fprintf(stderr, "Warning: Couldn't init SDL audio: %s\n",
		    SDL_GetError());
	    fprintf(stderr, "Will ignore audio stream\n");
	    use_audio = 0;
	  }
	  else
	    audio_inited = 1;
	}
	
        /* Create the MPEG stream */
#ifdef NET_SUPPORT
        /* Check if source is a file or an ip address */
        if((access(argv[i], F_OK) < 0) && (strchr(argv[i], ':') != NULL))
	{
	  char * address;
	  int port;
	  int enable = 1L;
	  struct sockaddr_in stAddr;
	  struct sockaddr_in stLclAddr;
	  struct ip_mreq stMreq;
	  int sock;

	  /* Source is an ip adress */
	  port = atoi(strchr(argv[i], ':') + sizeof(char));
	  *strchr(argv[i], ':') = '\0';
	  address = argv[i];
	  
	  printf("Connecting to %s port %d...\n", address, port);
	  
          stAddr.sin_family = AF_INET; 
	  stAddr.sin_addr.s_addr = inet_addr(address); 
	  stAddr.sin_port = htons(port);

	  /* Open socket */
	  sock = socket(AF_INET, SOCK_DGRAM, 0);
	  if (sock <= 0)
	    fprintf(stderr, "socket() failed\n");
	    
	  /* Allow multiple instance of the client to share */
	  /* the same address and port */
 	  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		        (char *) &enable, sizeof(unsigned long int)) < 0)
	    fprintf(stderr, "setsockopt() SO_REUSEADDR failed, %s\n",
		    strerror(errno));

	  /* Do protocol specific initialization */
	  /* If the address is multicast, register to the multicast group */
	  if(is_address_multicast(stAddr.sin_addr.s_addr))
	  {
	    /* Bind the socket to port */
	    stLclAddr.sin_family      = AF_INET;
	    stLclAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    stLclAddr.sin_port        = stAddr.sin_port;
      
	    if(bind(sock, (struct sockaddr*) & stLclAddr,
		    sizeof(stLclAddr)) < 0)
	    fprintf(stderr, "bind() failed, %s\n", strerror(errno));
      
	    /* Register to a multicast address */
	    stMreq.imr_multiaddr.s_addr = stAddr.sin_addr.s_addr;
	    stMreq.imr_interface.s_addr = INADDR_ANY;
      
	    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		 	  (char *) & stMreq, sizeof(stMreq)) < 0)
	      fprintf(stderr, "setsockopt() IP_ADD_MEMBERSHIP failed\n");
	  }
	  else
	  {
	    /* Bind the socket to port */
	    if(bind(sock, (struct sockaddr *) & stAddr, sizeof(stAddr)) < 0)
	      fprintf(stderr, "bind() failed, %s\n", strerror(errno));
	  }

	  /* Connected, now the socket is just like a regular file */
	  mpeg = SMPEG_new_descr(sock, &info, use_audio);
	}
	else
#endif
	{
	  if(strcmp(argv[i], "-") == 0) /* Use stdin for input */
	    mpeg = SMPEG_new_descr(0, &info, use_audio);
	  else
	    mpeg = SMPEG_new(argv[i], &info, use_audio);
	}

        if ( SMPEG_error(mpeg) ) {
            fprintf(stderr, "%s: %s\n", argv[i], SMPEG_error(mpeg));
            SMPEG_delete(mpeg);
            continue;
        }
        SMPEG_enableaudio(mpeg, use_audio);
        SMPEG_enablevideo(mpeg, use_video);
        SMPEG_setvolume(mpeg, volume);

        /* Print information about the video */
        basefile = strrchr(argv[i], '/');
        if ( basefile ) {
            ++basefile;
        } else {
            basefile = argv[i];
        }
        if ( info.has_audio && info.has_video ) {
            printf("%s: MPEG system stream (audio/video)\n", basefile);
        } else if ( info.has_audio ) {
            printf("%s: MPEG audio stream\n", basefile);
        } else if ( info.has_video ) {
            printf("%s: MPEG video stream\n", basefile);
        }
        if ( info.has_video ) {
            printf("\tVideo %dx%d resolution\n", info.width, info.height);
        }
        if ( info.has_audio ) {
	    printf("\tAudio %s\n", info.audio_string);
        }
        if ( info.total_size ) {
	    printf("\tSize: %d\n", info.total_size);
        }

        /* Set up video display if needed */
        if ( info.has_video && use_video ) {
            const SDL_VideoInfo *video_info;
            Uint32 video_flags;
            int video_bpp;
            int width, height;

            /* Get the "native" video mode */
            video_info = SDL_GetVideoInfo();
            switch (video_info->vfmt->BitsPerPixel) {
                case 16:
                case 32:
                    video_bpp = video_info->vfmt->BitsPerPixel;
                    break;
                default:
                    video_bpp = 16;
                    break;
            }
            if ( scale_width ) {
                width = scale_width;
            } else {
                width = info.width;
            }
            width *= scalesize;
            if ( scale_height ) {
                height = scale_height;
            } else {
                height = info.height;
            }
            height *= scalesize;
            video_flags = SDL_SWSURFACE;
            if ( fullscreen ) {
                video_flags = SDL_FULLSCREEN|SDL_DOUBLEBUF|SDL_HWSURFACE;
            }
            video_flags |= SDL_ASYNCBLIT;
            video_flags |= SDL_RESIZABLE;
            screen = SDL_SetVideoMode(width, height, video_bpp, video_flags);
            if ( screen == NULL ) {
                fprintf(stderr, "Unable to set %dx%d video mode: %s\n",
                                	width, height, SDL_GetError());
                continue;
            }
            if ( screen->flags & SDL_FULLSCREEN ) {
                SDL_ShowCursor(0);
            }
            SMPEG_setdisplay(mpeg, screen, NULL, update);
            SMPEG_scaleXY(mpeg, screen->w, screen->h);
        }

        /* Set any special playback parameters */
        if ( loop_play ) {
            SMPEG_loop(mpeg, 1);
        }

	/* Seek starting position */
	if(seek) SMPEG_seek(mpeg, seek);

	/* Skip seconds to starting position */
	if(skip) SMPEG_skip(mpeg, skip);
	
        /* Play it, and wait for playback to complete */
        SMPEG_play(mpeg);
        done = 0;
	pause = 0;
        while ( ! done && ( pause || (SMPEG_status(mpeg) == SMPEG_PLAYING) ) ) {
            SDL_Event event;

            while ( use_video && SDL_PollEvent(&event) ) {
                switch (event.type) {
                    case SDL_VIDEORESIZE: {
                        SDL_Surface *old_screen = screen;
                        SMPEG_pause(mpeg);
                        screen = SDL_SetVideoMode(event.resize.w, event.resize.h, screen->format->BitsPerPixel, screen->flags);
			if ( old_screen != screen ) {
                            SMPEG_setdisplay(mpeg, screen, NULL, update);
                        }
                        SMPEG_scaleXY(mpeg, screen->w, screen->h);
                        SMPEG_pause(mpeg);
                    } break;
                    case SDL_KEYDOWN:
                        if ( (event.key.keysym.sym == SDLK_ESCAPE) || (event.key.keysym.sym == SDLK_q) ) {
			  // Quit
			  done = 1;
                        } else if ( event.key.keysym.sym == SDLK_RETURN ) {
			  // toggle fullscreen
			  if ( event.key.keysym.mod & KMOD_ALT ) {
                            SDL_WM_ToggleFullScreen(screen);
                          }
                        } else if ( event.key.keysym.sym == SDLK_UP ) {
			  // Volume up
			  if ( volume < 100 ) {
			    if ( event.key.keysym.mod & KMOD_SHIFT ) {   // 10+
			      volume += 10;
			    } else if ( event.key.keysym.mod & KMOD_CTRL ) { // 100+
			      volume = 100;
			    } else {                                     // 1+
			      volume++;
			    }
			    if ( volume > 100 ) 
			      volume = 100;
			    SMPEG_setvolume(mpeg, volume);
			  }
                        } else if ( event.key.keysym.sym == SDLK_DOWN ) {
			  // Volume down
			  if ( volume > 0 ) {
			    if ( event.key.keysym.mod & KMOD_SHIFT ) {
			      volume -= 10;
			    } else if ( event.key.keysym.mod & KMOD_CTRL ) {
			      volume = 0;
			    } else {
			      volume--;
			    }
			    if ( volume < 0 ) 
			      volume = 0;
			    SMPEG_setvolume(mpeg, volume);
			  }
                        } else if ( event.key.keysym.sym == SDLK_PAGEUP ) {
			  // Full volume
			  volume = 100;
			  SMPEG_setvolume(mpeg, volume);
                        } else if ( event.key.keysym.sym == SDLK_PAGEDOWN ) {
			  // Volume off
			  volume = 0;
			  SMPEG_setvolume(mpeg, volume);
                        } else if ( event.key.keysym.sym == SDLK_SPACE ) {
			  // Toggle play / pause
			  if ( SMPEG_status(mpeg) == SMPEG_PLAYING ) {
			    SMPEG_pause(mpeg);
			    pause = 1;
			  } else {
			    SMPEG_play(mpeg);
			    pause = 0;
			  }
			} else if ( event.key.keysym.sym == SDLK_RIGHT ) {
			  // Forward
			  if ( event.key.keysym.mod & KMOD_SHIFT ) {
			    SMPEG_skip(mpeg, 100);
			  } else if ( event.key.keysym.mod & KMOD_CTRL ) {
			    SMPEG_skip(mpeg, 50);
			  } else {
			    SMPEG_skip(mpeg, 5);
			  }
                        } else if ( event.key.keysym.sym == SDLK_LEFT ) {
			  // Reverse
			  if ( event.key.keysym.mod & KMOD_SHIFT ) {

			  } else if ( event.key.keysym.mod & KMOD_CTRL ) {

			  } else {

			  }
                        } else if ( event.key.keysym.sym == SDLK_KP_MINUS ) {
			  // Scale minus
			  if ( scalesize > 1 ) {
			    scalesize--;
			  }
                        } else if ( event.key.keysym.sym == SDLK_KP_PLUS ) {
			  // Scale plus
			  scalesize++;
			}
                        break;
                    case SDL_QUIT:
                        done = 1;
                        break;
                    default:
                        break;
                }
            }
            SDL_Delay(1000/2);
        }
        SMPEG_delete(mpeg);
    }
    SDL_Quit();

    exit(0);
}
