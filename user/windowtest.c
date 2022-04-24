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
            if (evt.payload == 113) // q was inputted
                break;
            cc = cc + 1 % 0xFF;
            window_drawchar(win, 0, 0, 't', cc);
            window_drawchar(win, 8, 0, 'e', cc);
            window_drawchar(win, 16, 0, 's', cc);
            window_drawchar(win, 24, 0, 't', cc);
            window_drawrect(win, 0, 16, dim.width, dim.height, cc);
        }
    }
    window_destroy(win);
    exit(0);
}