#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "windowman.h"

#define ROWS 2
#define COLUMNS 2

struct windowevent evt_queue[ROWS * COLUMNS][NEVTS];

static void
drawborders()
{
    const int width = vga_getwidth();
    const int height = vga_getheight();

    // draw vertical lines
    const int xinterval = width / COLUMNS;
    for (int row = 1; row < ROWS; ++row)
        for (int y = 0; y < height; ++y)
            vga_putpixel(xinterval * row, y, 0x0F);

    // draw horizontal lines
    const int yinterval = height / ROWS;
    for (int col = 1; col < COLUMNS; ++col)
        for (int x = 0; x < width; ++x)
            vga_putpixel(x, yinterval * col, 0x0F);
}

static void
clearscreen()
{
    for (int x = 0; x < vga_getwidth(); ++x)
        for (int y = 0; y < vga_getheight(); ++y)
            vga_putpixel(x, y, 0x00);
}

// reads max n events from winow event queue, with window specified by the minor
// number in the device file.
// copies a windowevent struct or array into addr,
int windowmanread(int user_src, uint64 addr, int n, struct file *f)
{
    if (!f)
        return -1;

    if (f->ip->minor >= (ROWS * COLUMNS)) // invalid window
        return -1;

    return 0;
}

// get the size of a grid window
int windowman_getsize()
{
    return (vga_getwidth() / COLUMNS) * (vga_getheight() / ROWS);
}

// write to window specified by the minor number in the device file.
// bytes are taken from memory specified at addr, length n,
// written to the window starting from the offset in the file struct.
int windowmanwrite(int user_src, uint64 addr, int n, struct file *f)
{
    if (!f)
        return -1;

    if (f->ip->minor >= (ROWS * COLUMNS)) // invalid window
        return -1;

    // check out of bounds
    const int width = vga_getwidth();
    const int height = vga_getheight();
    if (f->off + n > (width / COLUMNS) * (height / ROWS))
        return -1;

    // get framebuffer for direct access
    volatile uint8 *framebuffer = vga_getframebuffer();

    // calculate starting offsets for window
    const int xoffset = (width / COLUMNS) * (f->ip->minor % COLUMNS);
    const int yoffset = (height / ROWS) * (f->ip->minor / COLUMNS);

    // calculate initial offset in window if f->off isn't 0
    int bytes_left = n;
    int current_x = f->off % width;
    int current_y = f->off / width;

    while (bytes_left > 0)
    {
        // check bytes left will overflow this row
        if (bytes_left + current_x >= (width / COLUMNS))
        {
            either_copyin(
                framebuffer + (xoffset + current_x) + (yoffset + current_y) * width,
                user_src,
                addr,
                (width / COLUMNS) - current_x);
            bytes_left -= (width / COLUMNS) - current_x;
            current_x = 0;
            current_y++;
            continue;
        }

        // fits in this row
        either_copyin(
            framebuffer + (xoffset + current_x) + (yoffset + current_y) * width,
            user_src,
            addr,
            bytes_left);
        bytes_left = 0;
    }

    return 0;
}

void windowmaninit(void)
{
    // add the window write and window read
    // function pointers to the read/write
    // overrides for WINDOW files.
    devsw[WINDOW].read = windowmanread;
    devsw[WINDOW].write = windowmanwrite;

    clearscreen();
    drawborders();
}