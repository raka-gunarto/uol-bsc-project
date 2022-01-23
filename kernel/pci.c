#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "memlayout.h"

static void setup_vga_card(volatile uint32 *);

void pciinit(void)
{
  uint16 bus; // bus index
  uint8 dev;  // device index

  //-- look through every device (brute-force scan)
  // there are 256 buses, 32 potential devices per bus
  //
  // this is actually very simplified and uses the legacy
  // access method, only looking at PCI Segment Group 0
  // (out of up to 65536 groups)
  for (bus = 0; bus < 256; ++bus)
    for (dev = 0; dev < 32; ++dev)
    {
      //-- calculate offsets
      // volatile pointer because this address is not RAM
      // it's memory mapped PCI registers which can change
      // at any time
      uint32 header_offset = (bus << 16) | (dev << 11);
      volatile uint32 *header = (volatile uint32 *)(PCI_ECAM_BASE + header_offset);

      //-- switch to handle known PCI devices
      // first 32 bits of the PCI header is the
      // device ID and the vendor ID, combined
      // becomes the PCI ID
      switch (header[0])
      {
      case 0x11111234:
        setup_vga_card(header);
        break;
      default:
        if (header[0] != 0xFFFFFFFF)
          printf("pciinit: unknown PCI device: 0x%x\n", header[0]);
        break;
      }
    }
}

static void setup_vga_card(volatile uint32 *header)
{
  //-- set bits in the command register
  // Bit 0 - respond to I/O accesses
  // Bit 1 - respond the memory accesses
  // Bit 2 - enable bus mastering (allow card to access other components without CPU intervention)
  header[1] = 0b111;

  __sync_synchronize(); // ensure order of writes aren't modified by optimisation

  // map the framebuffer in our specified base addr
  header[4] = VGA_FRAMEBUFFER_BASE;

  // tell our vga module it's ready to perform graphics initialisation
  vgainit();
}