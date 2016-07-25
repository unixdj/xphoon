/* Copyright (C) 1988,1991 by Jef Poskanzer <jef@acme.com> and
** Craig Leres <leres@ee.lbl.gov>.  All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <sys/ioctl.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>
#include "vroot.h"
#include "astro.h"
#include "date_parse.h"
#include "moon.h"


/* Definitions. */

#define BitmapSize(w,h) ((((w)+7)/8)*(h))


/* Forward routines. */

static void xinit( char* display_name );
static void make_star_tiles( void );
static void hack_bits( time_t t, int bits_w, int bits_h, unsigned char* bits, int cx, int cy, int r, int blackflag, int demoflag );
static void set_root( int bits_w, int bits_h, unsigned char* bits, int cx, int cy, int r );
static void cleanup( void );


/* Routines. */

static char* argv0;
static int seed;

int
main( int argc, char** argv )
    {
    int moon_w, moon_h, cx, cy, r;
    unsigned char* moon;
    int delayminutes;
    int blackflag, demoflag;
    int printpid;
    char* display_name;
    int pid, tty;
    int size;
    unsigned char* mooncopy;

    argv0 = argv[0];
    getbits( &moon_w, &moon_h, &moon, &cx, &cy, &r );
    delayminutes = 0;
    blackflag = 0;
    demoflag = 0;
    printpid = 0;
    display_name = (char*) 0;

    for( ; ; )
	{
	if ( argc > 1 && strcmp( argv[1], "-b" ) == 0 )
	    {
	    ++argv; --argc;
	    blackflag = 1;
	    continue;
	    }
	if ( argc > 2 && strcmp( argv[1], "-t" ) == 0 )
	    {
	    ++argv; --argc;
	    if ( sscanf( argv[1], "%d", &delayminutes ) != 1 )
		goto usage;
	    ++argv; --argc;
	    continue;
	    }
	if ( argc > 1 && strcmp( argv[1], "-i" ) == 0 )
	    {
	    ++argv; --argc;
	    printpid = 1;
	    continue;
	    }
	if ( argc > 2 && (
	       strcmp( argv[1], "-display" ) == 0 ||
	       strcmp( argv[1], "-displa" ) == 0 ||
	       strcmp( argv[1], "-displ" ) == 0 ||
	       strcmp( argv[1], "-disp" ) == 0 ||
	       strcmp( argv[1], "-dis" ) == 0 ||
	       strcmp( argv[1], "-di" ) == 0 ||
	       strcmp( argv[1], "-d" ) == 0 ) )
	    {
	    ++argv; --argc;
	    display_name = argv[1];
	    ++argv; --argc;
	    continue;
	    }
	if ( argc > 1 && strcmp( argv[1], "-demo" ) == 0 )
	    {
	    ++argv; --argc;
	    demoflag = 1;
	    continue;
	    }
	if ( argc > 2 && strcmp( argv[1], "-x" ) == 0 )
	    {
	    ++argv; --argc;
	    if ( sscanf( argv[1], "%d", &cx ) != 1 )
		goto usage;
	    ++argv; --argc;
	    continue;
	    }
	if ( argc > 2 && strcmp( argv[1], "-y" ) == 0 )
	    {
	    ++argv; --argc;
	    if ( sscanf( argv[1], "%d", &cy ) != 1 )
		goto usage;
	    ++argv; --argc;
	    continue;
	    }
	break;
	}

    if ( argc > 1 )
	{
usage:
	(void) fprintf(
	    stderr,
	    "usage: %s [-b] [-t minutes [-i]] [-display name]\n",
	    argv0 );
	exit( 1 );
	}

    /* Initialize the random number generator. */
    srandom( (int) ( time( (time_t*) 0 ) ^ getpid() ) );

    /* Set up X stuff. */
    xinit( display_name );

    /* Set up the star tiles. */
    make_star_tiles();

    /* Save a random seed so that the stars always comes out the same. */
    seed = random();

    /* One-shot mode? */
    if ( delayminutes <= 0 && ! demoflag )
	{
	hack_bits(
	    time( (time_t*) 0 ), moon_w, moon_h, moon, cx, cy, r, blackflag,
	    demoflag );
	set_root( moon_w, moon_h, moon, cx, cy, r );
	cleanup();
	exit( 0 );
	}

    /* Stick-around mode. */
    if ( printpid )
	{
	pid = fork();
	if ( pid < 0 )
	    {
	    perror( "fork" );
	    exit( 1 );
	    }
	else if ( pid > 0 )
	    {
	    /* Parent just exits. */
	    exit( 0 );
	    }
	(void) printf( "%d\n", getpid() );
	(void) fflush( stdout );

	/* Go stealth (ditch our controlling tty). */
	tty = open( "/dev/tty", 0 );
	if ( tty < 0 )
	    {
	    (void) fprintf( stderr, "%s: ", argv0 );
	    perror( "/dev/tty open" );
	    exit( 1 );
	    }
	else
	    {
	    if ( ioctl( tty, TIOCNOTTY, 0 ) < 0 )
		{
		(void) fprintf( stderr, "%s: ", argv0 );
		perror( "TIOCNOTTY ioctl" );
		exit( 1 );
		}
	    (void) close( tty );
	    }
	}

    size = BitmapSize( moon_w, moon_h );
    mooncopy = (unsigned char*) malloc( (unsigned) size );
    if ( mooncopy == (unsigned char*) 0 )
	{
	(void) fprintf( stderr, "%s: couldn't copy moon bitmap", argv0 );
	exit( 1 );
	}

    for (;;)
	{
	bcopy( (void*) moon, (void*) mooncopy, size );
	hack_bits(
	    time( (time_t*) 0 ), moon_w, moon_h, mooncopy, cx, cy, r, blackflag,
	    demoflag );
	set_root( moon_w, moon_h, mooncopy, cx, cy, r );
	if ( demoflag )
	    sleep( 1 );		/* continuous mode */
	else
	    sleep( (unsigned) ( delayminutes * 60 ) );
	}

    /*NOTREACHED*/
    }


/* xinit - initialize X stuff */

struct crtcinfo {
    int x, y, w, h;
    };

static Display* display;
static int screen;
static Window root;
static int root_w, root_h;
static GC onegc;
static GC zerogc;
static GC copygc;
static GC clipgc;
static int hasrandr, ncrtc;
struct crtcinfo *crtcs;

static void
xinit( char* display_name )
    {
    Pixmap temp_pixmap;
    int i, j;

    display = XOpenDisplay( display_name );
    if ( display == (Display*) 0 )
	{
	(void) fprintf(
	    stderr, "%s: can't open display \"%s\"\n", argv0,
	    XDisplayName( display_name ) );
	exit( 1 );
	}
    screen = DefaultScreen( display );
    root = DefaultRootWindow( display );
    hasrandr = XRRQueryExtension( display, &i, &j );
    if ( ! hasrandr )
	{
	root_w = DisplayWidth( display, screen );
	root_h = DisplayHeight( display, screen );
	ncrtc = 1;
	crtcs = malloc( sizeof( struct crtcinfo ) );
	if ( crtcs == NULL )
	    {
	    (void) fprintf( stderr, "%s: couldn't allocate crtc", argv0 );
	    exit( 1 );
	    }
	crtcs[0].x = 0;
	crtcs[0].y = 0;
	crtcs[0].w = root_w;
	crtcs[0].h = root_h;
	}
    temp_pixmap = XCreatePixmap( display, root, 1, 1, 1 );
    onegc = XCreateGC( display, temp_pixmap, 0, (XGCValues*) 0 );
    XSetForeground( display, onegc, 1L );
    XSetBackground( display, onegc, 0L );
    zerogc = XCreateGC( display, temp_pixmap, 0, (XGCValues*) 0 );
    XSetForeground( display, zerogc, 0L );
    XSetBackground( display, zerogc, 1L );
    XFreePixmap( display, temp_pixmap );
    copygc = XCreateGC( display, root, 0, (XGCValues*) 0 );
    XSetForeground( display, copygc, BlackPixel( display, screen ) );
    XSetBackground( display, copygc, WhitePixel( display, screen ) );
    clipgc = XCreateGC( display, root, 0, (XGCValues*) 0 );
    XSetForeground( display, clipgc, BlackPixel( display, screen ) );
    XSetBackground( display, clipgc, WhitePixel( display, screen ) );
    }

static void
xrrinit( void )
    {
    XRRScreenResources *sr;
    XRRCrtcInfo *ci;
    int i;

    if ( ! hasrandr )
	return;
    root_w = DisplayWidth( display, screen );
    root_h = DisplayHeight( display, screen );
    sr = XRRGetScreenResources( display, root );
    ncrtc = sr->ncrtc;
    crtcs = malloc( sizeof( struct crtcinfo ) * ncrtc );
    if ( crtcs == NULL )
	{
	(void) fprintf( stderr, "%s: couldn't allocate crtc", argv0 );
	exit( 1 );
	}
    memset( crtcs, 0, sizeof( struct crtcinfo ) * ncrtc);
    for ( i = 0; i < ncrtc; i++ )
	{
	ci = XRRGetCrtcInfo( display, sr, sr->crtcs[i] );
	if ( ci == NULL )
	    continue;
	if ( ci->noutput != 0 )
	    {
	    crtcs[i].x = ci->x;
	    crtcs[i].y = ci->y;
	    crtcs[i].w = ci->width;
	    crtcs[i].h = ci->height;
	    }
	XRRFreeCrtcInfo( ci );
	}
    XRRFreeScreenResources( sr );
    }

static void
xrrcleanup( void )
    {
    if ( ! hasrandr )
	return;
    free( crtcs );
    crtcs = NULL;
    }


/* make_star_tiles - make random star tiles */

/* Define some stars. */
#define star1_width 1
#define star1_height 1
static unsigned char star1_bits[] = { 0x00 };
#define star2_width 2
#define star2_height 2
static unsigned char star2_bits[] = { 0x00, 0x00 };
#define star3a_width 3
#define star3a_height 3
static unsigned char star3a_bits[] = { 0x05, 0x00, 0x05 };
#define star3b_width 3
#define star3b_height 3
static unsigned char star3b_bits[] = { 0x05, 0x02, 0x05 };
#define star4a_width 4
#define star4a_height 4
static unsigned char star4a_bits[] = { 0x09, 0x00, 0x00, 0x09 };
#define star4b_width 4
#define star4b_height 4
static unsigned char star4b_bits[] = { 0x0b, 0x08, 0x01, 0x0d };
#define star4c_width 4
#define star4c_height 4
static unsigned char star4c_bits[] = { 0x0d, 0x01, 0x08, 0x0b };
#define star5a_width 5
#define star5a_height 5
static unsigned char star5a_bits[] = { 0x11, 0x00, 0x00, 0x00, 0x11 };
#define star5b_width 5
#define star5b_height 5
static unsigned char star5b_bits[] = { 0x1b, 0x11, 0x00, 0x11, 0x1b };

#define NUM_STARS 9

static int star_w[NUM_STARS] = {
    star1_width, star2_width, star3a_width, star3b_width, star4a_width,
    star4b_width, star4c_width, star5a_width, star5b_width };
static int star_h[NUM_STARS] = {
    star1_height, star2_height, star3a_height, star3b_height, star4a_height,
    star4b_height, star4c_height, star5a_height, star5b_height };
static unsigned char* star_bits[NUM_STARS] = {
    star1_bits, star2_bits, star3a_bits, star3b_bits, star4a_bits,
    star4b_bits, star4c_bits, star5a_bits, star5b_bits };
static int star_prob[NUM_STARS] = { 700, 60, 15, 15,  6,  6,  6,  2,  2 };
/*                                    1   2  3a  3b  4a  4b  4c  5a  5b */

#define NUM_TILES 20
#define TILE_SIZE 128
#define STARS_PER_TILE 40

static Pixmap star_tile[NUM_TILES];
static int star_tiles_made = 0;

static void
make_star_tiles( void )
    {
    int i, j, k, r, x, y;
    int total_prob;
    Pixmap star_pixmap[NUM_STARS];

    total_prob = 0;
    for ( i = 0; i < NUM_STARS; ++i )
	{
	star_pixmap[i] = XCreateBitmapFromData(
	    display, root, (char*) star_bits[i], star_w[i], star_h[i] );
	total_prob += star_prob[i];
	}
    
    for ( i = 0; i < NUM_TILES; ++i )
	{
	star_tile[i] = XCreatePixmap( display, root, TILE_SIZE, TILE_SIZE, 1 );
	XFillRectangle(
	    display, star_tile[i], onegc, 0, 0, TILE_SIZE, TILE_SIZE );
	for ( j = random() % STARS_PER_TILE + STARS_PER_TILE / 2; j > 0; --j )
	    {
	    r = random() % total_prob;
	    for ( k = 0; k < NUM_STARS; ++k )
		{
		r -= star_prob[k];
		if ( r < 0 )
		    {
		    x = random() % ( TILE_SIZE - star_w[k] );
		    y = random() % ( TILE_SIZE - star_h[k] );
		    XCopyArea(
			display, star_pixmap[k], star_tile[i], onegc, 0, 0,
			star_w[k], star_h[k], x, y );
		    break;
		    }
		}
	    }
	}

    for ( i = 0; i < NUM_STARS; ++i )
	XFreePixmap( display, star_pixmap[i] );
    
    star_tiles_made = 1;
    }


/* hack_bits - modify the moon bitmap for the current phase */

static unsigned char  leftmask[8] = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 };
static unsigned char rightmask[8] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f };

static unsigned char  shade_0_bits[] = { 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
static unsigned char  shade_1_bits[] = { 0xfe,0xff,0xff,0xff,0xff,0xff,0xff,0xff };
static unsigned char  shade_2_bits[] = { 0xfe,0xff,0xfb,0xff,0xff,0xff,0xff,0xff };
static unsigned char  shade_3_bits[] = { 0xfe,0xff,0xfb,0xff,0x7f,0xff,0xff,0xff };
static unsigned char  shade_4_bits[] = { 0xfe,0xff,0xfb,0xff,0x7f,0xff,0xff,0xef };
static unsigned char  shade_5_bits[] = { 0xfe,0xbf,0xfb,0xff,0x7f,0xff,0xff,0xef };
static unsigned char  shade_6_bits[] = { 0xfe,0xbf,0xfb,0xdf,0x7f,0xff,0xff,0xef };
static unsigned char  shade_7_bits[] = { 0xfe,0xbf,0xfb,0xdf,0x7f,0xff,0xfe,0xef };
static unsigned char  shade_8_bits[] = { 0xfe,0xbf,0xfb,0xdf,0x7f,0xdf,0xfe,0xef };
static unsigned char  shade_9_bits[] = { 0xfe,0xbf,0xfb,0xdf,0x7d,0xdf,0xfe,0xef };
static unsigned char shade_10_bits[] = { 0xfe,0xbf,0xfb,0xdf,0x7d,0xdf,0xfa,0xef };
static unsigned char shade_11_bits[] = { 0xfe,0xbf,0xfb,0xdf,0x7d,0xdf,0xfa,0xaf };
static unsigned char shade_12_bits[] = { 0xfe,0xbf,0xfa,0xdf,0x7d,0xdf,0xfa,0xaf };
static unsigned char shade_13_bits[] = { 0xfe,0xaf,0xfa,0xdf,0x7d,0xdf,0xfa,0xaf };
static unsigned char shade_14_bits[] = { 0xfe,0xaf,0xfa,0xdf,0x75,0xdf,0xfa,0xaf };
static unsigned char shade_15_bits[] = { 0xfa,0xaf,0xfa,0xdf,0x75,0xdf,0xfa,0xaf };

unsigned char *shades[16] = {
    shade_0_bits,  shade_1_bits,  shade_2_bits,  shade_3_bits,
    shade_4_bits,  shade_5_bits,  shade_6_bits,  shade_7_bits,
    shade_8_bits,  shade_9_bits,  shade_10_bits, shade_11_bits,
    shade_12_bits, shade_13_bits, shade_14_bits, shade_15_bits };


static void
hack_bits( time_t t, int bits_w, int bits_h, unsigned char* bits, int cx, int cy, int r, int blackflag, int demoflag )
    {
    double jd, angphase, cphase, aom, cdist, cangdia, csund, csuang;
    int i;
    int x, y;
    int xleft, xright;
    double fxleft, fxright;
    double fy;
    int bytexleft, bitxleft, bytexright, bitxright;
    int off;
    double cap, ratio;
    int shadeindex;
    unsigned char shade;
    static double demoinc = 0.0;

    jd = unix_to_julian( t );
    if ( demoflag )
	{
	/* Jump ahead a day each time through. */
	jd += demoinc;
	demoinc += 1.0;
	}

    angphase =
      phase( jd, &cphase, &aom, &cdist, &cangdia, &csund, &csuang ) *
      2.0 * M_PI;
    cap = cos( angphase );

    /* Hack to figure approximate earthlighting. */
    if ( cphase < 0.1 ) cphase = 0.1;
    if ( cphase > 0.9 ) cphase = 0.9;
    ratio = (1.0 - cphase) / cphase;	/* ratio varies from 9.0 to 0.111 */
    shadeindex = (int) ( ratio / 9.0 * 15.9999 );

    for ( i = 0; i < 2 * r; ++i )
	{
	y = cy - r + i;
	if ( y < 0 || y >= bits_h )
	    continue;
	fy = i - r;
	fxright = r * sqrt( 1.0 - ( fy * fy ) / ( r * r ) );
	fxleft = - fxright;
	if ( angphase >= 0.0 && angphase < M_PI )
	    fxright *= cap;
	else
	    fxleft *= cap;

	xleft = fxleft + cx + 0.5;
	xright = fxright + cx + 0.5;

	bytexleft = xleft / 8;
	bitxleft = xleft % 8;

	bytexright = xright / 8;
	bitxright = xright % 8;

	off = y * ( ( bits_w + 7 ) / 8 );

	if ( blackflag )
	    shade = 0xff;
	else
	    shade = shades[shadeindex][y % 8];
	if ( bytexleft == bytexright )
	    bits[bytexleft + off] |=
		leftmask[bitxleft] & shade & rightmask[bitxright];
	else
	    {
	    bits[bytexleft + off] |= leftmask[bitxleft] & shade;
	    for ( x = bytexleft + 1; x < bytexright; ++x )
		bits[x + off] |= shade;
	    bits[bytexright + off] |= rightmask[bitxright] & shade;
	    }
	}
    }


/* set_root - set the root bitmap */

static void
set_root( int bits_w, int bits_h, unsigned char* bits, int cx, int cy, int r )
    {
    Pixmap moon_bitmap;
    Pixmap mask_bitmap;
    Pixmap root_pixmap;
    int x, y, i;
    unsigned long length, after;
    int format;
    Atom prop, type;
    unsigned char* data;

    /* Send the moon bitmap to the server. */
    moon_bitmap = XCreateBitmapFromData( display, root, (char*) bits, bits_w, bits_h );
    if ( moon_bitmap == (Pixmap) 0 )
	{
	(void) fprintf( stderr, "%s: unable to store moon bitmap", argv0 );
	exit( 1 );
	}

    /* Make the mask bitmap. */
    mask_bitmap = XCreatePixmap( display, root, bits_w, bits_h, 1 );
    if ( mask_bitmap == (Pixmap) 0 )
	{
	(void) fprintf( stderr, "%s: unable to create mask bitmap", argv0 );
	exit( 1 );
	}
    XFillRectangle( display, mask_bitmap, zerogc, 0, 0, bits_w, bits_h );
    /* Draw the mask smaller than r, to allow for slop. */
    XFillArc(
	display, mask_bitmap, onegc, cx - r + 2, cy - r + 2,
	r * 2 - 3, r * 2 - 3, 0, 360 * 64 );

    xrrinit();

    /* Make the root pixmap. */
    root_pixmap = XCreatePixmap(
	display, root, root_w, root_h, DefaultDepth( display, screen ) );
    if ( root_pixmap == (Pixmap) 0 )
	{
	(void) fprintf( stderr, "%s: unable to create root pixmap", argv0 );
	exit( 1 );
	}

    /* Fill in root pixmap with randomly-chosen star tiles. */
    srandom( seed );
    for ( y = 0; y < root_h; y += TILE_SIZE )
	for ( x = 0; x < root_w; x += TILE_SIZE )
	    XCopyPlane(
		display, star_tile[random() % NUM_TILES], root_pixmap, copygc,
		0, 0, TILE_SIZE, TILE_SIZE, x, y, 1 );
    /* And just for good measure, add some with random x and y. */
    for ( i = ( root_w * root_h ) / ( TILE_SIZE * TILE_SIZE ) / 5; i > 0; --i )
	{
	x = random() % ( root_w - TILE_SIZE );
	y = random() % ( root_h - TILE_SIZE );
	XCopyPlane(
	    display, star_tile[random() % NUM_TILES], root_pixmap, copygc,
	    0, 0, TILE_SIZE, TILE_SIZE, x, y, 1 );
	}

    /* Put the moon into the stars. */
    for ( i = 0; i < ncrtc; i++ )
	{
	if ( crtcs[i].w == 0 )
	    continue;
	x = crtcs[i].x + ( ( crtcs[i].w - bits_w ) / 2 );
	y = crtcs[i].y + ( ( crtcs[i].h - bits_h ) / 2 );
	XSetClipMask( display, clipgc, mask_bitmap );
	XSetClipOrigin( display, clipgc, x, y );
	XCopyPlane(
	    display, moon_bitmap, root_pixmap, clipgc, 0, 0, bits_w, bits_h,
	    x, y, 1 );
	}

    xrrcleanup();

    /* And set the root. */
    XSetWindowBackgroundPixmap( display, root, root_pixmap );

    /* Clean up. */
    XFreePixmap( display, moon_bitmap );
    XFreePixmap( display, mask_bitmap );
    XFreePixmap( display, root_pixmap );
    XClearWindow( display, root );
    XFlush( display );

    /* Not sure what this garbage does, but xsetroot has it, so... */
    prop = XInternAtom( display, "_XSETROOT_ID", False );
    (void) XGetWindowProperty(
	display, root, prop, 0L, 1L, True, AnyPropertyType, &type, &format,
	&length, &after, &data );
    if ( type == XA_PIXMAP && format == 32 && length == 1 && after == 0 )
	XKillClient( display, *( (Pixmap*) data ) );
    else if ( type != None )
	(void) fprintf(
	    stderr, "%s: warning: _XSETROOT_ID property is garbage\n", argv0 );
    XFlush( display );
    }


static void
cleanup( void )
    {
    int i;

    if ( star_tiles_made )
	for ( i = 0; i < NUM_TILES; ++i )
	    XFreePixmap( display, star_tile[i] );
    if ( ! hasrandr )
	free( crtcs );
    XFreeGC( display, onegc );
    XFreeGC( display, zerogc );
    XFreeGC( display, copygc );
    XFreeGC( display, clipgc );
    XCloseDisplay( display );
    }
