#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/windowman.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    // open the first available window
    window_handle win = window_create();

    // go through the palette
    uint8 cc = 0;
    struct windowevent evt;
    struct window_dim dim = window_getdimensions(win);
    printf("%d x %d\n", dim.width, dim.height);
    while (1)
    {
        if (window_pollevent(win, &evt, 1) == 1)
        {
            cc = cc + 1 % 0xFF;
            window_drawrect(win, 0, 0, dim.width, dim.height, cc);
        }
    }
}