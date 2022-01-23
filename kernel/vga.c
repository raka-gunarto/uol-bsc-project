#include "vga.h"

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"

#include "bootimg.h"

static void pwrite(uint32, uint32, uint8);
static uint8 pread(uint32, uint32);

static volatile uint8 *vga_framebuffer = (uint8 *)VGA_FRAMEBUFFER_BASE;
void vgainit(void)
{
  printf("vga init\n");

  vga_setmode(LINEAR_256COLOR_320x200);
  for (int y = 0; y < BOOTIMG_HEIGHT; y++)
    for (int x = 0; x < BOOTIMG_WIDTH; x++)
      vga_framebuffer[(y + (100 - BOOTIMG_HEIGHT / 2)) * 320 + x + (160 - BOOTIMG_WIDTH / 2)] = BOOTIMG[y * 91 + x];
}

void vga_setmode(enum VGA_MODES mode)
{
  int i;
  for (i = 0; i < VGA_REGISTERS_LEN; ++i) // set register values for mode
    pwrite(VGA_MODE_PORT[i], VGA_MODE_IDX[i], VGA_MODE_VAL[i][mode]);

  if (mode == LINEAR_256COLOR_320x200) // set 256 palette if graphics mode
  {
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
