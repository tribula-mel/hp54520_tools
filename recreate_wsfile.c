/*
 * Recreates WS_FILE
 * Copyright (C) 2015 Tribula Mel <tribula.mel@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <arpa/inet.h>

/* ROM T3.33 defaults */
static char rom_t333_title[32] = "HP54542 ROM V00.13";
#define ROM_T333_START_ADDR      0xa00100
#define ROM_T333_SEC1_ADDR       0xa00100

/* ROM 3.10 defaults */
static char rom_310_title[32]  = "HP54542 ROM V01.14              ";
#define ROM_310_START_ADDR       0xa00100
#define ROM_310_SEC1_ADDR        0xa00100

/* SYSTEM T3.33 defaults */
static char sys_t333_title[32] = "HP54542 SYS VT3.33              ";
#define SYS_T333_START_ADDR      0xa25100
#define SYS_T333_SEC1_ADDR       0xa25000

/* SYSTEM 3.10 defaults */
static char sys_310_title[32]  = "HP54542 SYS V03.10              ";
#define SYS_310_START_ADDR       0xa25100
#define SYS_310_SEC1_ADDR        0xa25000

#define ARG_NUM            4
#define ARG_SEC1_FNAME     0
#define ARG_SEC1_CSUM      1
#define ARG_SEC2_FNAME     2
#define ARG_SEC2_CSUM      3

typedef unsigned short bool;
#define FALSE              0
#define TRUE               1

static bool rom     = FALSE;
static bool tver    = FALSE;

#define OVERHEAD           68

static unsigned int start_addr = 0x0;
static unsigned int sec1_addr  = 0x0;
static unsigned char title[32] = {0};

static void help(void)
{
   fprintf(stderr, "Usage: recreate_wsfile [-r] [-t] sec1 sec1_csum sec2 sec2_csum\n");
   exit(EXIT_FAILURE);
}

static void load_defaults(void)
{
   if (tver) {
      /* firmware T3.33 version defaults */
      if (rom) {
         start_addr = htonl(ROM_T333_START_ADDR);
         sec1_addr = htonl(ROM_T333_SEC1_ADDR);
         memcpy(&title, &rom_t333_title, sizeof(title));
      } else {
         start_addr = htonl(SYS_T333_START_ADDR);
         sec1_addr = htonl(SYS_T333_SEC1_ADDR);
         memcpy(&title, &sys_t333_title, sizeof(title));
      }
   } else {
      /* firmware 3.10 version defaults */
      if (rom) {
         start_addr = htonl(ROM_310_START_ADDR);
         sec1_addr = htonl(ROM_310_SEC1_ADDR);
         memcpy(&title, &rom_310_title, sizeof(title));
      } else {
         start_addr = htonl(SYS_310_START_ADDR);
         sec1_addr = htonl(SYS_310_SEC1_ADDR);
         memcpy(&title, &sys_310_title, sizeof(title));
      }
   }
}

int main(int argc, char *argv[])
{
   struct stat stats;
   FILE *sec1_fd = NULL;
   FILE *sec2_fd = NULL;
   FILE *sec_fd  = NULL;
   FILE *out_fd  = NULL;
   FILE *mem_fd  = NULL;
   FILE *head_fd = NULL;
   FILE *term_fd = NULL;
   unsigned int sec2_addr = 0x0;
   unsigned int sec1_len  = 0;
   unsigned int sec2_len  = 0;
   unsigned int len       = 0;
   int opt    = -1;
   int status = 0;
   short count   = 0, count_be = 0;
   short the_end = 4;
   short sec1_cs = 0;
   short sec2_cs = 0;
   short n = 0, i = 0;
   short term     = 0xffff;
   char pad          = 0x78;
   char byte         = 0;
   char ignored_sys  = 0x53;
   char ignored_rom  = 0xa1;
   char ws_file[12]  = {0};
   char buffer[254]  = {0};
   char sec2_h[10]   = {0};
   char sec_term[10] = {0};
   bool sec_eof      = FALSE;

   while ((opt = getopt(argc, argv, "rt")) != -1) {
      switch (opt) {
      case 'r':
         rom = TRUE;
         break;
      case 't':
         tver = TRUE;
         break;
      default: /* '?' */
         help();
      }
   }

   if (optind >= argc) {
      fprintf(stderr, "Expected arguments after options\n");
      help();
   }

   if ((argc - optind) != ARG_NUM) {
      help();
   }

   load_defaults();

   sec1_cs = htons(strtol(argv[optind + ARG_SEC1_CSUM], NULL, 16));
   sec2_cs = htons(strtol(argv[optind + ARG_SEC2_CSUM], NULL, 16));

   status = stat(argv[optind + ARG_SEC1_FNAME], &stats);
   if (status == -1) {
      fprintf(stderr, "error getting stats for %s\n", argv[optind + ARG_SEC1_FNAME]);
      exit(EXIT_FAILURE);
   }
   sec1_len = htonl(stats.st_size);
   status = stat(argv[optind + ARG_SEC2_FNAME], &stats);
   if (status == -1) {
      fprintf(stderr, "error getting stats for %s\n", argv[optind + ARG_SEC2_FNAME]);
      exit(EXIT_FAILURE);
   }
   sec2_len = htonl(stats.st_size);
   len = sec1_len + sec2_len + htonl(OVERHEAD);
   sec2_addr = sec1_addr + sec1_len;

   sec1_fd = fopen(argv[optind + ARG_SEC1_FNAME], "r");
   if (sec1_fd == NULL) {
      fprintf(stderr, "error opening %s\n", argv[optind + ARG_SEC1_FNAME]);
      exit(EXIT_FAILURE);
   }

   sec2_fd = fopen(argv[optind + ARG_SEC2_FNAME], "r");
   if (sec2_fd == NULL) {
      fprintf(stderr, "error opening %s\n", argv[optind + ARG_SEC2_FNAME]);
      exit(EXIT_FAILURE);
   }

   snprintf(ws_file, sizeof(ws_file), "%s.%s", "WS_FILE", (rom? "rom":"sys"));
   out_fd = fopen(ws_file, "w");
   if (out_fd == NULL) {
      fprintf(stderr, "error opening %s\n", ws_file);
      exit(EXIT_FAILURE);
   }

   head_fd = fmemopen(sec2_h, sizeof(sec2_h), "rb+");
   if (head_fd == NULL) {
      fprintf(stderr, "error opening header buffer\n");
      exit(EXIT_FAILURE);
   }
   /* it is convenient to serialize following data */
   fwrite(&sec1_cs, 2, 1, head_fd);
   fwrite(&sec2_addr, 4, 1, head_fd);
   fwrite(&sec2_len, 4, 1, head_fd);
   rewind(head_fd);

   term_fd = fmemopen(sec_term, sizeof(sec_term), "rb+");
   if (term_fd == NULL) {
      fprintf(stderr, "error opening terminate buffer\n");
      exit(EXIT_FAILURE);
   }
   /* it is convenient to serialize following data */
   fwrite(&sec2_cs, 2, 1, term_fd);
   rewind(term_fd);

   mem_fd = fmemopen(buffer, sizeof(buffer), "rb+");
   if (mem_fd == NULL) {
      fprintf(stderr, "error opening memory buffer\n");
      exit(EXIT_FAILURE);
   }

   /* write the header */
   fwrite(&len, 4, 1, mem_fd);
   count += 4;
   fwrite(&title, 32, 1, mem_fd);
   count += 32;
   fwrite(&start_addr, 4, 1, mem_fd);
   count += 4;
   /* write the section 1 header */
   fwrite(&sec1_addr, 4, 1, mem_fd);
   count += 4;
   fwrite(&sec1_len, 4, 1, mem_fd);
   count += 4;

   sec_fd = sec1_fd;

   /* read section 1 */
   while (TRUE) {
      /* collect at max 254 bytes */
      while (TRUE) {
         if (count == 0xfe)
            break;
         fread(&byte, 1, 1, sec_fd);
         status = ferror(sec_fd);
         if (status) {
            fprintf(stderr, "error reading section 1 file\n");
            exit(EXIT_FAILURE);
         }
         status = feof(sec_fd);
         if (status) {
            sec_eof = TRUE;
            break;
         }
         fwrite(&byte, 1, 1, mem_fd);
         count++;
      }
      if (!sec_eof) {
         /* flush 0xfe bytes into file */
         count_be = htons(count);
         fwrite(&count_be, 2, 1, out_fd);
         rewind(mem_fd);
         fwrite(&buffer, count, 1, out_fd);
         count = 0;
      } else {
         switch(--the_end) {
         case 3:
            sec_fd = head_fd;
            break;
         case 2:
            sec_fd = sec2_fd;
            break;
         case 1:
            sec_fd = term_fd;
            break;
         }

         if (the_end == 0) {
            count_be = htons(count);
            fwrite(&count_be, 2, 1, out_fd);
            rewind(mem_fd);
            fwrite(&buffer, count, 1, out_fd);
            if (count & 0x01) {
               /* this byte is ignored in case the length of final
                * section is odd according to LIF spec; it is the
                * same number as seen in V3.33 SYSTEM & ROM files
                */
               if (rom) 
                  fwrite(&ignored_rom, 1, 1, out_fd);
               else
                  fwrite(&ignored_sys, 1, 1, out_fd);
               n = -1;
            }
            fwrite(&term, 2, 1, out_fd);
            /* pad the last section up to 256 bytes */
            n += 0x100 - count - 4;
            for (i = 0; i < n; i++)
               fwrite(&pad, 1, 1, out_fd);
            break;
         }

         sec_eof = FALSE;
      }
   }

   fclose(sec1_fd);
   fclose(sec2_fd);
   fclose(out_fd);
   fclose(mem_fd);
   fclose(head_fd);
   fclose(term_fd);

   exit(EXIT_SUCCESS);
}
