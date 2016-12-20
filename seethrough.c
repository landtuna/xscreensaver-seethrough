/* seethrough xscreensaver hack

MIT License

Copyright (c) 2016 Basic Commerce & Industries, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include "screenhack.h"
#include <time.h>
#include <limits.h>

struct state {
  Display *dpy;
  Screen *screen;
  Window window;
  Pixmap saved;

  int sizex, sizey;
  int updatesecs;
  GC gc;
  time_t start_time;
};

#define min(a,b) (((a) < (b)) ? (a) : (b))

static void
load_image_async_cb (Screen *screen, Window window, Drawable drawable,
                     const char *name, XRectangle *geom, void *closure)
{
  XWindowAttributes xgwa;
  struct state *st = (struct state *) closure;

  st->start_time = time ((time_t *) 0);

  XGetWindowAttributes (st->dpy, st->window, &xgwa);

  if (xgwa.width != geom->width || xgwa.height != geom->height) {
    st->sizex = min(xgwa.width, geom->width);
    st->sizey = min(xgwa.height, geom->height);
  }

  XCopyArea (st->dpy, st->saved, st->window, st->gc, 0, 0,
             st->sizex, st->sizey, 0, 0);
}

static void
seethrough_load_image (struct state *st)
{
  load_image_async (st->screen, st->window,
                    st->saved, load_image_async_cb, st);
}

static void *
seethrough_init (Display *dpy, Window window)
{
  struct state *st = (struct state *) calloc (1, sizeof(*st));
  XGCValues gcv;
  long gcflags;
  unsigned long bg;
  XWindowAttributes xgwa;

  st->dpy = dpy;
  st->window = window;

  st->updatesecs = get_integer_resource(st->dpy, "updatesecs", "Integer");
  if (st->updatesecs < 1) st->updatesecs = 1;

  XGetWindowAttributes (st->dpy, st->window, &xgwa);

  st->screen = xgwa.screen;
  st->sizex = xgwa.width;
  st->sizey = xgwa.height;

  gcv.function = GXcopy;
  gcv.subwindow_mode = IncludeInferiors;
  bg = get_pixel_resource (st->dpy, xgwa.colormap, "background", "Background");
  gcv.foreground = bg;

  gcflags = GCForeground | GCFunction;
  if (use_subwindow_mode_p(xgwa.screen, st->window)) /* see grabscreen.c */
    gcflags |= GCSubwindowMode;
  st->gc = XCreateGC (st->dpy, st->window, gcflags, &gcv);

  if (!st->saved) {
    st->saved = XCreatePixmap (st->dpy, st->window,
                               xgwa.width, xgwa.height,
                               xgwa.depth);
  }

  st->start_time = time ((time_t *) 0);
  seethrough_load_image (st);

  return st;
}


/*
 * perform one iteration
 */
static unsigned long
seethrough_draw (Display *dpy, Window window, void *closure)
{
  struct state *st = (struct state *) closure;

  if (st->start_time + st->updatesecs < time ((time_t *) 0)) {
    st->start_time = INT_MAX - st->updatesecs;

    seethrough_load_image (st);
  }

  return 100;
}

static void
seethrough_reshape (Display *dpy, Window window, void *closure,
                 unsigned int w, unsigned int h)
{
}

static Bool
seethrough_event (Display *dpy, Window window, void *closure, XEvent *event)
{
  return False;
}

static void
seethrough_free (Display *dpy, Window window, void *closure)
{
  struct state *st = (struct state *) closure;

  XFreePixmap(st->dpy, st->saved);

  free(st);
}

static const char *seethrough_defaults [] = {
  ".background:			Black",
  ".foreground:			Yellow",
  "*dontClearRoot:		True",
  "*fpsSolid:			True",

#ifdef __sgi	/* really, HAVE_READ_DISPLAY_EXTENSION */
  "*visualID:			Best",
#endif

  "*updatesecs:			1",
#ifdef HAVE_MOBILE
  "*ignoreRotation:             True",
  "*rotateImages:               True",
#endif
  0
};

static XrmOptionDescRec seethrough_options [] = {
  { "-updatesecs",	".updatesecs",		XrmoptionSepArg, 0 },
  { 0, 0, 0, 0 }
};


XSCREENSAVER_MODULE ("Seethrough", seethrough)
