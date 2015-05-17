/*
 * Recreates ROM/SYSTEM LIF files
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

typedef unsigned short bool;
#define FALSE              0
#define TRUE               1

static bool rom     = FALSE;

static short lif_file_type     = 0x8000;
static unsigned char title[6]   = "HFSLIF";
static unsigned char copy[]    = {0x0, 0x0, 0x0, 0x1, 0x10, 0x0, 0x0, 0x0,
                                  0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0,
                                  0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x1,
                                  0x0, 0x0};
static unsigned char copy_1[]  = {0x0, 0x0, 0x0, 0x2, 0x0, 0x0};
static unsigned char copy_2[]  = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x1,
                                  0x0, 0x0, 0x0, 0x80, 0x0, 0x0, 0x0, 0x0,
                                  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0xff};
static unsigned char date[]    = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
static unsigned char ws_file[10] = "WS_FILE   ";
static short rom_file_type     = 0xc30c;
static short sys_file_type     = 0xc30d;

#define ARG_NUM            1
#define ARG_FILE_NAME      0

static void help(void)
{
   fprintf(stderr, "Usage: generate_lif [-r] <input_file>\n");
   exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
   struct stat stats;
   FILE *in_fd  = NULL;
   FILE *out_fd = NULL;
   int opt    = -1;
   int status = 0;
   int size      = 0;
   short nsect     = 0;
   short lif_nsect = 0;
   short i         = 0;
   short word      = 0;
   char out_file[32] = {0};
   char byte         = 0;
   char zero         = 0;

   while ((opt = getopt(argc, argv, "r")) != -1) {
      switch (opt) {
      case 'r':
         rom = TRUE;
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

   status = stat(argv[optind + ARG_FILE_NAME], &stats);
   if (status == -1) {
      fprintf(stderr, "error getting stats for %s\n", argv[optind + ARG_FILE_NAME]);
      exit(EXIT_FAILURE);
   }
   size = stats.st_size;
   if ((size % 0x100) != 0) {
      fprintf(stderr, "length of %s doesn't look right\n", argv[optind + ARG_FILE_NAME]);
      exit(EXIT_FAILURE);
   }
   nsect = htons(size/0x100);
   lif_nsect = htons(size/0x100 + 2);

   in_fd = fopen(argv[optind + ARG_FILE_NAME], "r");
   if (in_fd == NULL) {
      fprintf(stderr, "error opening %s\n", argv[optind + ARG_FILE_NAME]);
      exit(EXIT_FAILURE);
   }

   snprintf(out_file, sizeof(out_file), "%s.%s", (rom? "ROM":"SYSTEM"), "lif");
   out_fd = fopen(out_file, "w");
   if (out_fd == NULL) {
      fprintf(stderr, "error opening %s\n", out_file);
      exit(EXIT_FAILURE);
   }

   /* write the header */
   word = htons(lif_file_type);
   fwrite(&word, 2, 1, out_fd);
   fwrite(&title, sizeof(title), 1, out_fd);
   fwrite(&copy, sizeof(copy), 1, out_fd);
   fwrite(&lif_nsect, 2, 1, out_fd);
   fwrite(&date, sizeof(date), 1, out_fd);
   for (i = 0; i < 206; i++)
      fwrite(&zero, 1, 1, out_fd);
   fwrite(&date, sizeof(date), 1, out_fd);
   for (i = 0; i < 2; i++)
      fwrite(&zero, 1, 1, out_fd);
   fwrite(&ws_file, sizeof(ws_file), 1, out_fd);
   if (rom) {
      word = htons(rom_file_type);
      fwrite(&word, 2, 1, out_fd);
   }
   else {
      word = htons(sys_file_type);
      fwrite(&word, 2, 1, out_fd);
   }
   fwrite(&copy_1, sizeof(copy_1), 1, out_fd);
   fwrite(&nsect, 2, 1, out_fd);
   fwrite(&copy_2, sizeof(copy_2), 1, out_fd);
   for (i = 0; i < 212; i++)
      fwrite(&zero, 1, 1, out_fd);

   /* write the content */
   while (TRUE) {
      fread(&byte, 1, 1, in_fd);
      status = ferror(in_fd);
      if (status) {
         fprintf(stderr, "error reading input file\n");
         exit(EXIT_FAILURE);
      }
      status = feof(in_fd);
      if (status) {
         break;
      }
      fwrite(&byte, 1, 1, out_fd);
   }

   fclose(in_fd);
   fclose(out_fd);

   exit(EXIT_SUCCESS);
}
