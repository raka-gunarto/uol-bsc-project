#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#include "kernel/windowman.h"

struct window
{
    int fd;
    uint16 width;
    uint16 height;
};

window_handle window_create()
{
    int rootfd = open("/", 0);
    if (rootfd < 0)
        return 0;
}

struct window_dim window_getdimensions(window_handle win)
{
}

struct windowevent window_pollevent(window_handle win, int max_evts)
{
}

void window_drawsprite(window_handle win, uint x, uint y, uint w, uint h, uint8 *data)
{
}

void window_drawchar(window_handle win, uint x, uint y, char c)
{
}

void window_drawline(window_handle win, uint x1, uint y1, uint x2, uint y2)
{
}

void window_drawrect(window_handle win, uint x, uint y, uint w, uint h, uint8 color)
{
}

void window_drawcircle(window_handle win, uint x, uint y, uint r)
{
}
