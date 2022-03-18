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

#define ROWS 2
#define COLUMNS 2

// reads max n events from winow event queue, with window specified by the minor
// number in the device file.
// copies a windowevent struct or array into addr,
void
windowmanread(int user_src, uint64 addr, int n, struct file* f)
{
    if (!f)
        return -1;
    
    if (f->ip->minor >= (ROWS * COLUMNS)) // invalid window
        return -1;
}


// write to window specified by the minor number in the device file.
// bytes are taken from memory specified at addr, length n, 
// written to the window starting from the offset in the file struct.
void
windowmanwrite(int user_src, uint64 addr, int n, struct file* f)
{
    if (!f)
        return -1;

    if (f->ip->minor >= (ROWS * COLUMNS)) // invalid window
        return -1;
}

void
windowmaninit(void)
{
    // add the window write and window read
    // function pointers to the read/write
    // overrides for WINDOW files.
    devsw[WINDOW].read = windowmanread;
    devsw[WINDOW].write = windowmanwrite;
}