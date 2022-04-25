#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"
#include "kernel/windowman.h"
#include "user/user.h"

const int gunsprite_width = 6;
const int gunsprite_height = 8;
const uint8 gunsprite[] =
{
    0x00, 0x00, 0x0B, 0x0B, 0x00, 0x00,
    0x00, 0x00, 0x0C, 0x0C, 0x00, 0x00,
    0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x00,
    0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x00,
    0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x00,
    0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x00,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
};

struct coords {
    uint x;
    uint y;
};

static void fire(int bulletx, int bullety, struct coords *bullets)
{
    for (int i = 0; i < 10; ++i)
        if (bullets[i].x == -1 && bullets[i].y == -1)
        {
            bullets[i].x = bulletx, bullets[i].y = bullety;
            break;
        }
}

static int colliding(struct coords c1, struct coords c2, int combinedrad)
{
    uint rsquared = combinedrad * combinedrad;
    uint dx = c1.x - c2.x;
    uint dy = c1.y - c2.y;
    uint distsquared = dx * dx + dy * dy;
    if (distsquared <= rsquared)
        return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    setpriority(1);

    // open the first available window
    window_handle win = window_create();

    struct windowevent evt;
    struct window_dim dim = window_getdimensions(win);

    int gunx = dim.width / 2;
    int guny = (dim.height / 4) * 3;

    const uint bulletrad = 3;
    struct coords bullets[10];

    const uint enemywh = 5;
    struct coords enemies[10];

    for(int i = 0; i < 10; ++i)
    {
        bullets[i].x = -1;
        bullets[i].y = -1;

        int row = i / 5 + 1;
        enemies[i].x = ((i % 5) + row) * (dim.width / 6);
        enemies[i].y = row * (dim.height / 4);
    }
    
    uint score = 0;
    while (score < 10)
    {
        // clear screen
        window_clearscreen(win);

        // handle inputs
        if (window_pollevent(win, &evt, 1) == 1)
        {
            switch (evt.payload)
            {
            case 'a':
                gunx = gunx > 0 ? gunx - 1 : gunx;
                break;
            case 'd':
                gunx = gunx < dim.width ? gunx + 1 : gunx;
                break;
            case ' ':
                fire(gunx + gunsprite_width, guny - 2, bullets);
                break;
            default:
                break;
            }
        }

        // movement
        for(int i = 0; i < 10; ++i)
        {
            // bullets, delete when top of screen reached
            if (bullets[i].x != -1 && bullets[i].y != -1)
                if (--bullets[i].y <= 0)
                    bullets[i].x = -1, bullets[i].y = -1;


        }

        // collisions
        for(int bullet = 0; bullet < 10; ++bullet)
            if (bullets[bullet].x != -1 && bullets[bullet].y != -1)
                for (int enemy = 0; enemy < 10; ++enemy)
                    if (enemies[enemy].x != -1 && enemies[enemy].y != -1 && colliding(bullets[bullet], enemies[enemy], bulletrad + enemywh / 2))
                    {
                        score++;
                        bullets[bullet].x = -1, bullets[bullet].y = -1;
                        enemies[enemy].x = -1, enemies[enemy].y = -1;
                        break;
                    }

        // draw
        window_drawsprite(win, gunx, guny, gunsprite_width, gunsprite_height, gunsprite);
        for(int i = 0; i < 10; ++i)
        {
            if (bullets[i].x != -1 && bullets[i].y != -1)
                window_drawcircle(win, bullets[i].x - bulletrad, bullets[i].y - bulletrad, bulletrad, 0x0C);
            
            if (enemies[i].x != -1 && enemies[i].y != -1)
                window_drawrect(win, enemies[i].x - enemywh, enemies[i].y - enemywh, enemywh, enemywh, 0x0E);
        }

        window_drawchar(win, 8*0, 0, 'S', 0x09);
        window_drawchar(win, 8*1, 0, 'C', 0x09);
        window_drawchar(win, 8*2, 0, 'O', 0x09);
        window_drawchar(win, 8*3, 0, 'R', 0x09);
        window_drawchar(win, 8*4, 0, 'E', 0x09);
        window_drawchar(win, 8*5, 0, ':', 0x09);
        window_drawchar(win, 8*6, 0, ' ', 0x09);
        window_drawchar(win, 8*7, 0, (char)score + 48, 0x09);
    }
    window_destroy(win);
    exit(0);
}