/*
 * Extracts subsection which is actually a jump table
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
#include <string.h>

#include <arpa/inet.h>

int main(int argc, char *argv[])
{
   int in_fd = -1;
   int out_fd = -1;
   int state = -1;
   unsigned int i = 0;
   unsigned int length = 0;
   unsigned int loading_addr = 0;
   unsigned int generic32 = 0;
   unsigned char generic8 = 0;
   char signature[3] = {0};
   char name[128] = {0};

   if (argc != 2) {
      printf("Usage:\n\textract_subsection <file_name>\n");
      exit(EXIT_FAILURE);
   }

   in_fd = open(argv[1], O_RDONLY);
   if (in_fd == -1) {
      printf("error opening %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }

   /* signature, starts at offset 0x0, 2 bytes */
   state = read(in_fd, &signature[0], 2);
   if (state == -1) {
      printf("file corrupted: %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   printf("signature [%s]\n", signature);
   if (strncmp(signature, "SC", 2)) {
      printf("file %s doesn't look like subsection\n", argv[1]);
      exit(EXIT_FAILURE);
   }
 
   /* length of subsection, starts at offset 0x2, four bytes */
   state = read(in_fd, &generic32, 4);
   if (state == -1) {
      printf("file corrupted: %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   printf("subsection length [%x]\n", htonl(generic32));
   length = htonl(generic32);

   /* loading address, starts at offset 0x6, four bytes */
   state = read(in_fd, &generic32, 4);
   if (state == -1) {
      printf("file corrupted: %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   loading_addr = htonl(generic32);

   snprintf(&name[0], sizeof(name), "%s_sub_%x", argv[1], loading_addr);
   out_fd = open(name, O_WRONLY | O_CREAT, S_IRWXU);
   if (out_fd == -1) {
      printf("error opening %s\n", argv[2]);
      exit(EXIT_FAILURE);
   }

   /* this is now data/code section */
   for (i = 0; i < length; i++) {
      state = read(in_fd, &generic8, 1);
      if (state == -1) {
         printf("file corrupted: %s\n", argv[1]);
         exit(EXIT_FAILURE);
      }
      if (state == 0) {
         printf("end of %s reached\n", argv[1]);
         break;
      }
      state = write(out_fd, &generic8, 1);
      if (state == -1) {
         printf("error writing: %s\n", name);
         exit(EXIT_FAILURE);
      }
   }

   printf("\twrote file: %s\n", name);

   close(out_fd);
   close(in_fd);

   exit(EXIT_SUCCESS);
}
