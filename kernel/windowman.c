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

volatile static int active_window = ROWS * COLUMNS;

static void
drawborders()
{
    const int width = vga_getwidth();
    const int height = vga_getheight();

    const int activewin_row = active_window / COLUMNS;
    const int activewin_col = active_window % COLUMNS;

    // draw vertical lines
    const int xinterval = width / COLUMNS;
    for (int col = 1; col < COLUMNS; ++col)
        for (int y = 0; y < height; ++y)
            if ((col == activewin_col || col - 1 == activewin_col) && (y / (height / ROWS)) == activewin_row)
                vga_putpixel(xinterval * col, y, 0x28);
            else
                vga_putpixel(xinterval * col, y, 0x0F);

    // draw horizontal lines
    const int yinterval = height / ROWS;
    for (int row = 1; row < ROWS; ++row)
        for (int x = 0; x < width; ++x)
            if ((row == activewin_row || row - 1 == activewin_row) && (x / (width / COLUMNS)) == activewin_col)
                vga_putpixel(x, yinterval * row, 0x28);
            else
                vga_putpixel(x, yinterval * row, 0x0F);
}

static void
clearscreen()
{
    for (int x = 0; x < vga_getwidth(); ++x)
        for (int y = 0; y < vga_getheight(); ++y)
            vga_putpixel(x, y, 0x00);
}

static void
initevtqueue()
{
    for (int i = 0; i < ROWS * COLUMNS; ++i)
        evt_queue[i][0].type = HEAD, evt_queue[i][1].type = TAIL;
}

#define C(x) ((x) - '@')
// uart input interrupts go here
int windowmanintr(int c)
{
    switch (c)
    {
    case C('T'): // control-t switches windows
        if (active_window == ROWS * COLUMNS)
            active_window = 0;
        else
            active_window++;
        drawborders();
        return 0;
    default:                                 // control-t switches windows
        if (active_window == ROWS * COLUMNS) // no active window
            return -1;

        // active window, add to evt queue
        // find tail of evt queue
        for (int tailidx = 0; tailidx < NEVTS; ++tailidx)
        {
            // skip if not tail
            if (evt_queue[active_window][tailidx].type != TAIL)
                continue;

            // queue is full (tail next to head)
            if (evt_queue[active_window][(tailidx + 1) % NEVTS].type == HEAD)
                return 0;

            // add to queue and break
            evt_queue[active_window][tailidx].type = EVT_KEY;
            evt_queue[active_window][tailidx].payload = c;
            evt_queue[active_window][(tailidx + 1) % NEVTS].type = TAIL;
            break;
        }
        return 0;
    }
}

// reads max n events from winow event queue, with window specified by the minor
// number in the device file.
// copies a windowevent struct or array into addr,
int windowmanread(int user_dst, uint64 addr, int n, struct file *f)
{
    if (!f)
        return -1;

    if (f->ip->minor >= (ROWS * COLUMNS)) // invalid window
        return -1;

    // find head of event queue
    for (int headidx = 0; headidx < NEVTS; ++headidx)
    {
        // skip if not head
        if (evt_queue[f->ip->minor][headidx].type != HEAD)
            continue;
        
        // return if queue empty
        if (evt_queue[f->ip->minor][(headidx + 1) % NEVTS].type == TAIL)
            return 0;

        // return all events up to n
        evt_queue[f->ip->minor][headidx].type = EVT_KEY; // set to something else, just not HEAD or TAIL
        int evt_count = 0;
        int current_evtidx = (headidx + 1) % NEVTS;
        uint64 caddr = addr;
        for (;
             evt_count < n && evt_queue[f->ip->minor][current_evtidx].type != TAIL;
             current_evtidx = (current_evtidx + 1) % NEVTS,
             ++evt_count,
             caddr += sizeof(struct windowevent))
        {
            either_copyout(user_dst, caddr, &evt_queue[f->ip->minor][current_evtidx], sizeof(struct windowevent));
            evt_queue[f->ip->minor][current_evtidx].type = HEAD;
        }

        return evt_count;
    }

    return 0;
}

// get the size of a grid window
// first 16 bits are width, last 16 bits are height
uint windowman_getsize()
{
    // bit tricks so we can get the individual width and height
    // in the user end.
    // NOTE: if width or height is > 2^16 in the future it will be an issue
    uint16 width, height;
    width = vga_getwidth() / COLUMNS;
    height = vga_getheight() / ROWS;
    return (width << 16) + height;
}

// write to window specified by the minor number in the device file.
// bytes are taken from memory specified at addr, length n,
// written to the window starting from the offset in the file struct.
// TODO: some point in the future maybe support bit blitting?
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
    int current_x = f->off % (width / COLUMNS);
    int current_y = f->off / (width / COLUMNS);

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

    drawborders();
    return 0;
}

void windowmaninit(void)
{
    int old = intr_get();
    intr_on();
    uint ticks_start = ticks;
    while (ticks - ticks_start < 25)
        ; // wait a while, show the startup image first
    if (!old)
        intr_off();

    // add the window write and window read
    // function pointers to the read/write
    // overrides for WINDOW files.
    devsw[WINDOW].read = windowmanread;
    devsw[WINDOW].write = windowmanwrite;

    initevtqueue();
    clearscreen();
    drawborders();
}