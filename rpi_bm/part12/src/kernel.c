#include "common.h"
#include "mini_uart.h"
#include "printf.h"
#include "irq.h"
#include "timer.h"
#include "i2c.h"
#include "spi.h"
#include "led_display.h"
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"

int strncmp(const char *s1, const char *s2, unsigned short n)
{
    unsigned char u1, u2;

    while (n-- > 0)
    {
       u1 = (unsigned char) *s1++;
       u2 = (unsigned char) *s2++;
       if (u1 != u2) return u1 - u2;
       if (u1 == '\0') return 0;
    }

    return 0;
}

// NETWORKING GLOBALS AND FUNCTIONS

ENC_HandleTypeDef handle;

// MAC address to be assigned to the ENC28J60
unsigned char myMAC[6] = { 0xc0, 0xff, 0xee, 0xc0, 0xff, 0xee };

// IP address to be assigned to the ENC28J60
unsigned char deviceIP[4] = { 192, 168, 0, 66 };

void init_network(void)
{
   handle.Init.DuplexMode = ETH_MODE_HALFDUPLEX;
   handle.Init.MACAddr = myMAC;
   handle.Init.ChecksumMode = ETH_CHECKSUM_BY_HARDWARE;
   handle.Init.InterruptEnableBits = EIE_LINKIE | EIE_PKTIE;

   printf("Starting network up.");
   if (!ENC_Start(&handle)) {
      printf("Could not initialise network card.");
   } else {
      printf("Setting MAC address to C0:FF:EE:C0:FF:EE.");

      ENC_SetMacAddr(&handle);

      printf("Network card successfully initialised.");
   }

   printf("Waiting for ifup... ");
   while (!(handle.LinkStatus & PHSTAT2_LSTAT)) ENC_IRQHandler(&handle);
   printf("done.");

   // Re-enable global interrupts
   ENC_EnableInterrupts(EIE_INTIE);

   printf("Initialising the TCP stack... ");
   init_udp_or_www_server(myMAC, deviceIP);
   printf("done.");
}

void enc28j60PacketSend(unsigned short buflen, void *buffer) {
   if (ENC_RestoreTXBuffer(&handle, buflen) == 0) {
      ENC_WriteBuffer((unsigned char *) buffer, buflen);
      handle.transmitLength = buflen;
      ENC_Transmit(&handle);
   }
}

void putc(void *p, char c) {
    if (c == '\n') {
        uart_send('\r');
    }

    uart_send(c);
}

u32 get_el();

void serve(void)
{
   while (1) {
      while (!ENC_GetReceivedFrame(&handle));

      uint8_t *buf = (uint8_t *)handle.RxFrameInfos.buffer;
      uint16_t len = handle.RxFrameInfos.length;
      uint16_t dat_p = packetloop_arp_icmp_tcp(buf, len);

      if (dat_p != 0) {
         printf("Incoming web request... ");

         if (strncmp("GET ", (char *)&(buf[dat_p]), 4) != 0) {
            printf("not GET");
            dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>ERROR</h1>");
         } else if (strncmp("echo ", (char *)&(buf[dat_p]), 5) != 0) {
            dat_p = fill_tcp_data(buf, 0, (char *)&buf[dat_p + 5]);
         } else {
            if (strncmp("/ ", (char *)&(buf[dat_p+4]), 2) == 0) {
               // just one web page in the "root directory" of the web server
               printf("GET root");
               dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello world!</h1>");
            } else {
               // just one web page not in the "root directory" of the web server
               printf("GET not root");
               dat_p = fill_tcp_data(buf, 0, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Goodbye cruel world.</h1>");
            }
         }

         www_server_reply(buf, dat_p); // send web page data
      }
   }
}


void kernel_main() {
    uart_init();
    init_printf(0, putc);
    printf("\nRasperry PI Bare Metal OS Initializing...\n");

    irq_init_vectors();
    enable_interrupt_controller();
    irq_enable();
    timer_init();

#if RPI_VERSION == 3
    printf("\tBoard: Raspberry PI 3\n");
#endif

#if RPI_VERSION == 4
    printf("\tBoard: Raspberry PI 4\n");
#endif

    printf("\nException Level: %d\n", get_el());

    printf("Sleeping 2000 ms...\n");
    timer_sleep(2000);
    printf("Hi\n");

    

    // printf("Initializing I2C...\n");
    // i2c_init();

    // for (u8 i=0x20; i<0x30; i++) {
    //     if (i2c_send(i, &i, 1) == I2CS_SUCCESS) {
    //         //we know there is an i2c device here now.
    //         printf("Found device at address 0x%X\n", i);
    //     }
    // }

    printf("Initializing SPI...\n");
    spi_init();

    init_network();

    serve();

    // printf("Initializing Display...\n");
    // led_display_init();
    // timer_sleep(2000);

    // led_display_clear();

    // printf("Cleared\n");
    
    // for (int i=0; i<=9; i++) {
    //     for (int d=0; d<8; d++) {
    //         led_display_set_digit(d, i, false);
    //         timer_sleep(200);
    //     }
    // }

    // printf("Intensifying...\n");

    // for (int i=0; i<16; i++) {
    //     printf("Intensity: %d\n", i);
    //     led_display_intensity(i);
    //     timer_sleep(200);
    // }

    // led_display_clear();
    // timer_sleep(2000);

    // //HELLO
    // led_display_send_command(LD_DIGIT4, 0b00110111);
    // led_display_send_command(LD_DIGIT3, 0b01001111);
    // led_display_send_command(LD_DIGIT2, 0b00001110);
    // led_display_send_command(LD_DIGIT1, 0b00001110);
    // led_display_send_command(LD_DIGIT0, 0b01111110);


    // printf("Shutting down...\n");
    // timer_sleep(2000);
    // led_display_send_command(LD_SHUTDOWN, 0);

    printf("DONE!\n");

    while(1) {
        uart_send(uart_recv());
    }
}
