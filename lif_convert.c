/*
 * Converts extracted file (usually WS_FILE) from LIF image
 * WS_FILE is divided into number of segments with following
 * format:
 *
 *    size_ms_byte size_ls_byte data
 *
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
#include <unistd.h>

int main(int argc, char *argv[])
{
   unsigned int size = 0;
   unsigned int overall_size = 0;
   unsigned int segments = 0;
   int in_fd = -1;
   int out_fd = -1;
   int state = -1;
   int i = 0;
   unsigned char byte = 0;
   unsigned char ignore_last = 0;

   if (argc != 3) {
      printf("Usage:\n\n\t lif_convert <input_file> <output_file>\n");
      exit(EXIT_FAILURE);
   }

   in_fd = open(argv[1], O_RDONLY);
   if (in_fd == -1) {
      printf("error opening %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   out_fd = open(argv[2], O_WRONLY | O_CREAT, S_IRWXU);
   if (out_fd == -1) {
      printf("error opening %s\n", argv[2]);
      exit(EXIT_FAILURE);
   }

   for ( ; ; ) {
      state = read(in_fd, &byte, 1);
      if (state == -1) {
         printf("error reading %s\n", argv[1]);
         exit(EXIT_FAILURE);
      }
      if (state == 0) {
         printf("end of %s reached\n", argv[1]);
         exit(EXIT_FAILURE);
      }
      size = byte << 8;

      state = read(in_fd, &byte, 1);
      if (state == -1) {
         printf("error reading %s\n", argv[1]);
         exit(EXIT_FAILURE);
      }
      if (state == 0) {
         printf("end of %s reached\n", argv[1]);
         break;
      }

      size |= byte ;
      if (size == 0xFFFF)
         break;
      if (size & 0x1)
         ignore_last = 1;

      overall_size += size;
      segments++;

      for (i = 0; i < size; i++) {
         state = read(in_fd, &byte, 1);
         if (state == -1) {
            printf("error reading %s\n", argv[1]);
            exit(EXIT_FAILURE);
         }
         state = write(out_fd, &byte, 1);
         if (state == -1) {
            printf("error writing %s\n", argv[2]);
            exit(EXIT_FAILURE);
         }
      }
      if (ignore_last) {
         state = read(in_fd, &byte, 1);
         if (state == -1) {
            printf("error reading %s\n", argv[1]);
            exit(EXIT_FAILURE);
         }
         ignore_last = 0;
      }
   }

   close(in_fd);
   close(out_fd);

   printf("size[%x]\n", overall_size);
   printf("segments[%x]\n", segments);

   exit(EXIT_SUCCESS);
}
