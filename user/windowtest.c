#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/windowman.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    // open the first available window
    char *winname = "window0";
    int win = -1;
    int curwin = 0;
    while (win < 0)
    {
        // too high
        if (curwin > 9)
        {
            printf("windowtest: could not find free window\n");
            return 1;
        }
        winname[6] = curwin++ + '0';
        win = open(winname, O_RDWR);
    }

    // go through the palette
    uint8 cc = 0;
    struct stat st;
    fstat(win, &st);
    uint16 *ptr = (uint16 *)&st.size;
    uint size = ptr[0] * ptr[1];
    uint8 *data = malloc(sizeof(uint8) * st.size);
    struct windowevent evt;
    while (1)
    {
        if (read(win, &evt, 1) == 1)
        {
            cc = cc + 1 % 0xFF;
            memset(data, cc, size);
            write(win, data, size);
        }
    }
}