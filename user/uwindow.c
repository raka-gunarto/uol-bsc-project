#include "kernel/types.h"
#include "kernel/fs.h"
#include "kernel/windowman.h"
#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "user/user.h"

struct window
{
    int fd;
    uint16 width;
    uint16 height;
};

window_handle window_create()
{
    // open root dir
    int rootfd = open("/", 0);
    if (rootfd < 0)
        return 0;

    // find all "window" files
    struct dirent de;
    while (read(rootfd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0) // valid inode?
            continue;

        uint len = strlen(de.name);
        if (len < 7) // string long enough?
            continue;

        // string starts with "window"?
        char temp[7];
        memmove(temp, de.name, 6);
        temp[6] = 0;
        if (strcmp(temp, "window") != 0)
            continue;

        // we have a winner, try opening it
        char path[MAXPATH];
        char *p = path;
        *(p++) = '/';
        memmove(p, de.name, len);
        p[len] = 0;
        int fd = open(path, O_RDWR);

        if (fd >= 0)
        {
            struct window *win = malloc(sizeof(struct window));
            win->fd = fd;

            // extract window sizes
            struct stat st;
            fstat(win, &st);
            win->height = ((uint16 *)&st.size)[0];
            win->width = ((uint16 *)&st.size)[1];

            return (window_handle)win;
        }
    }

    // no window available
    return 0;
}

void window_destroy(window_handle win)
{
    // clear the window
    uint screensize = sizeof(uint8) * win->height * win->width;
    uint8 *buf = malloc(screensize);
    memset(buf, 0, screensize);
    write(win->fd, buf, screensize);
    free(buf);

    // close the window
    close(win->fd);

    // free the window
    free(win);
}

struct window_dim window_getdimensions(window_handle win)
{
    struct window_dim dim = {.height = win->height, .width = win->width};
    return dim;
}

int window_pollevent(window_handle win, struct windowevent *evts, int max_evts)
{
    return read(win->fd, evts, max_evts);
}

static int valid_coords(window_handle win, uint x, uint y)
{
    return x < win->width && y < win->height;
}

void window_drawsprite(window_handle win, uint x, uint y, uint w, uint h, uint8 *data)
{
    // check coords
    if (!valid_coords(win, x, y))
        return;

    // draw the data row by row, allocate only what fits, don't overflow
    int width = (win->width - x) < w ? win->width - x : w;
    int height = (win->height - y) < h ? win->height - y : h;
    uint8 *rowbuf = malloc(sizeof(uint8) * width);
    for (int j = 0; j < height; ++j)
    {
        // copy bytes into rowbuf
        for (int i = 0; i < width; ++i)
            rowbuf[i] = data[j * w + i];

        // write to screen
        seek(win->fd, SEEK_SET, ((y + j) * win->width) + x);
        write(win->fd, rowbuf, width);
    }

    // reset cursor
    seek(win->fd, SEEK_SET, 0);
}

void window_drawrect(window_handle win, uint x, uint y, uint w, uint h, uint8 color)
{
    // check coords
    if (!valid_coords(win, x, y))
        return;

    // draw the data row by row, allocate only what fits, don't overflow
    int width = (win->width - x) < w ? win->width - x : w;
    int height = (win->height - y) < h ? win->height - y : h;
    uint8 *rowbuf = malloc(sizeof(uint8) * width);
    for (int j = 0; j < height; ++j)
    {
        // copy color into rowbuf and write
        memset(rowbuf, color, width);
        seek(win->fd, SEEK_SET, ((y + j) * win->width) + x);
        write(win->fd, rowbuf, width);
    }

    // reset cursor
    seek(win->fd, SEEK_SET, 0);
}

void window_drawchar(window_handle win, uint x, uint y, char c)
{
}

inline static int abs(int x)
{
    return (x < 0) ? -x : x;
}
inline int interpolate(float i, float j, double t)
{
    return (int)(i * t + j * (1 - t));
}
void window_drawline(window_handle win, uint x1, uint y1, uint x2, uint y2, uint8 color)
{
    // check coords
    if (!valid_coords(win, x1, y1) || !valid_coords(win, x2, y2))
        return;

    int steps = (abs(y1 - y2) + abs(x1 - x2));
    float interval = (float)1 / steps;

    float t = 0;
    for (int i = 0; i < steps; i++, t += interval)
    {
        // this is really slow, kernel doesn't support masking / blitting
        // yet so if we don't want to overwrite a rectangular region this is
        // the only way for now
        seek(win->fd, SEEK_SET, interpolate(y1, y2, t) * win->width + interpolate(x1, x2, t));
        write(win->fd, &color, 1);
    }
}

void window_drawcircle(window_handle win, uint x, uint y, uint r, uint8 color)
{
    // find the rectangular bounding box of the circle
    // check coords
    if (!valid_coords(win, x - r, y - r))
        return;

    uint rsquared = r * r;
    for (uint j = y - r; j < y + r && j < win->height; ++j)
        for (uint i = x - r; i < x + r && i < win->width; ++i)
        {
            uint dx = x - i;
            uint dy = y - j;
            uint distsquared = dx * dx + dy * dy;

            if (distsquared <= rsquared)
            {
                seek(win->fd, SEEK_SET, j * win->width + i);
                write(win->fd, &color, 1);
            }
        }
}
