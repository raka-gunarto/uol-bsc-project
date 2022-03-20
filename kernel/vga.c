#include "vga.h"

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"

#include "bootimg.h"

static volatile int available = 0;
static volatile enum VGA_MODES current_mode;
static volatile int current_width = 0;
static volatile int current_height = 0;

static void pwrite(uint32, uint32, uint8);
static uint8 pread(uint32, uint32);

static volatile uint8 *vga_framebuffer = (uint8 *)VGA_FRAMEBUFFER_BASE;

int vga_putpixel(int x, int y, uint8 p)
{
  if (!available || current_mode != LINEAR_256COLOR_320x200)
    return -1;
  
  if (x < 0 || x >= current_width || y < 0 || y >= current_height)
    return -1;
  
  vga_framebuffer[y * current_width + x] = p;
  return 0;
}

enum VGA_MODES vga_getmode()
{
  return current_mode;
}

int vga_getwidth()
{
  return current_width;
}

int vga_getheight()
{
  return current_height;
}

// to be called by PCI module if VGA hardware is detected
void vga_setavailable()
{
  available = 1;
}

void vgainit(int cpu)
{
  // check if there is a VGA device ready
  if (!available)
    return;

  // check cpu, if 0, init the VGA mode
  if (cpu == 0)
  {
    printf("vga init\n");
    vga_setmode(LINEAR_256COLOR_320x200);
  }

  //-- load the bootimage on screen, offset by CPU
  // find how many images fit in a line on the screen
  static const int xfit = 320 / BOOTIMG_WIDTH;

  // figure out the offset for the image based on the CPU id
  const int xoffset = cpu % xfit;
  const int yoffset = cpu / xfit;

  // copy image to screen framebuffer, taking offset into account
  for (int y = 0; y < BOOTIMG_HEIGHT; y++)
    for (int x = 0; x < BOOTIMG_WIDTH; x++)
      vga_framebuffer[(y * 320) + x + (xoffset * BOOTIMG_WIDTH) + (yoffset * 320 * BOOTIMG_HEIGHT)] = BOOTIMG[y * BOOTIMG_WIDTH + x];
  //--
}

void vga_setmode(enum VGA_MODES mode)
{
  int i;
  for (i = 0; i < VGA_REGISTERS_LEN; ++i) // set register values for mode
    pwrite(VGA_MODE_PORT[i], VGA_MODE_IDX[i], VGA_MODE_VAL[i][mode]);
  current_mode = mode;

  if (mode == LINEAR_256COLOR_320x200) // set 256 palette if graphics mode
  {
    // should have a lookup table, this is fine for now
    current_height = 200;
    current_width = 320;

    pwrite(0x3c8, 0xff, 0x00); // set DAC colour register mode to write
    for (int i = 0; i < 256; i++)
    {
      // take 6 bits from relevant part of palette, shift to uint8 type, and write to reg
      // it is 6 bits because the DAC uses an 18 bit colour space, 6 bits per colour component
      pwrite(0x3c9, 0xff, (VGA_256COL_PALETTE[i] & 0xfc0000) >> 18); // set red val
      pwrite(0x3c9, 0xff, (VGA_256COL_PALETTE[i] & 0x00fc00) >> 10); // set green val
      pwrite(0x3c9, 0xff, (VGA_256COL_PALETTE[i] & 0x0000fc) >> 2);  // set blue val
    }
  }

  if (mode == TEXT) // set font if text mode
  {
    // should have a lookup table, this is fine for now
    current_height = 25;
    current_width = 80;

    // setting graphics controller to linear mode
    pwrite(0x3ce, 0x05, 0x00);
    pwrite(0x3ce, 0x06, 0x04);

    // setting sequencer to linear mode and change to plane 2
    // to set font data
    pwrite(0x3c4, 0x02, 0x04);
    pwrite(0x3c4, 0x04, 0x06);

    // set font data, do every 32 bytes even though each glyph is 16
    // this is because VGA has space for 8x32 fonts but we are using
    // 8x16
    for (int i = 0; i < 4096; i += 16)
      for (int j = 0; j < 16; ++j)
        vga_framebuffer[2 * i + j] = VGA_8x16_FONT[i + j];

    // set sequencer back to normal operation
    pwrite(0x3c4, 0x02, 0x03);
    pwrite(0x3c4, 0x04, 0x02);

    // set graphics controller back to normal operation
    pwrite(0x3ce, 0x05, 0x10);
    pwrite(0x3ce, 0x06, 0x0E);
  }

  pwrite(0x3c0, 0xff, 0x20); // enable display
}

static volatile uint8 idx_mode_set __attribute__((unused));
static volatile uint8 *vga_pio = (uint8 *)PCI_PIO_BASE;
static void pwrite(uint32 port, uint32 idx, uint8 val)
{
  if (idx == 0xFF)
  {
    // special case, this port doesn't use indices
    // direct write
    vga_pio[port] = val;
    return;
  }

  // perform a read op on 0x3DA to set 0x3C0 into index mode
  idx_mode_set = vga_pio[0x3DA];
  __sync_synchronize();

  // write the index to the port, and the value to port + 1 if not special case 0x3C0
  vga_pio[port] = idx;
  __sync_synchronize();
  uint write_offset = (port == 0x3C0) ? (uint32)0 : (uint32)1;
  vga_pio[port + write_offset] = val;
  __sync_synchronize();

  // perform a read op on 0x3DA to set 0x3C0 into index mode after
  idx_mode_set = vga_pio[0x3DA];
}

static uint8 pread(uint32 port, uint32 idx)
{
  if (idx == 0xFF)
    // special case, this port doesn't use indices
    // direct read
    return vga_pio[port];

  // perform a read op on 0x3DA to set 0x3C0 into index mode
  idx_mode_set = vga_pio[0x3DA];
  __sync_synchronize();

  // write the index to the port, and read the value from port + 1
  vga_pio[port] = idx;
  __sync_synchronize();
  uint8 ret = vga_pio[port + 1];
  __sync_synchronize();

  // perform a read op on 0x3DA to set 0x3C0 into index mode after
  idx_mode_set = vga_pio[0x3DA];

  return ret;
}
