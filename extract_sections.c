/*
 * Extracts all sections present in converted WS_FILE
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
   unsigned int section = 0;
   unsigned short generic16 = 0;
   unsigned char generic8 = 0;
   char name[33] = {0};

   if (argc != 2) {
      printf("Usage:\n\textract_sections <file_name>\n");
      exit(EXIT_FAILURE);
   }

   in_fd = open(argv[1], O_RDONLY);
   if (in_fd == -1) {
      printf("error opening %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }

   /* what follows is the generic header of the image:
    *  - length of file
    *  - name and version
    *  - entry address
    */

   /* length of the file, starts at offset 0x0, four bytes */
   state = read(in_fd, &generic32, 4);
   if (state == -1) {
      printf("file corrupted: %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   printf("length of %s in bytes [%x]\n", argv[1], htonl(generic32));

   /* name and version, starts at offset 0x4, 32 bytes, not null terminated */
   state = read(in_fd, &name[0], 32);
   if (state == -1) {
      printf("file corrupted: %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   printf("name and version string [%s]\n", name);
 
   /* entry address, starts at offset 0x24, four bytes */
   state = read(in_fd, &generic32, 4);
   if (state == -1) {
      printf("file corrupted: %s\n", argv[1]);
      exit(EXIT_FAILURE);
   }
   printf("entry address [%x]\n", htonl(generic32));

   /* there might be number of sections:
    *  - FLASH loading address, four bytes
    *  - data/code section length, four bytes
    *  - data/code
    *  - checksum, two bytes
    * next section starts after the checksum
    * zero length means the end of image
    */
   for ( ; ; ) {
      /* FLASH loading address, starts at offset 0x28, four bytes */
      state = read(in_fd, &generic32, 4);
      if (state == -1) {
         printf("file corrupted: %s\n", argv[1]);
         exit(EXIT_FAILURE);
      }
      loading_addr = htonl(generic32);

      /* data/code length, starts at offset 0x2c, four bytes */
      state = read(in_fd, &generic32, 4);
      if (state == -1) {
         printf("file corrupted: %s\n", argv[1]);
         exit(EXIT_FAILURE);
      }
      length = htonl(generic32);
      if (length != 0) {
         section++;
         printf("\tdata/code section[%d]: loading address [%x]\n", section, loading_addr);
         printf("\tdata/code section[%d]: length[%x]\n", section, length);

         memset(&name[0], 0, sizeof(name));
         snprintf(&name[0], sizeof(name), "%s_section_%d_%x", argv[1],
                  section, loading_addr);
         out_fd = open(name, O_WRONLY | O_CREAT, S_IRWXU);
         if (out_fd == -1) {
            printf("error opening %s\n", argv[2]);
            exit(EXIT_FAILURE);
         }
      } else {
         printf("we reached the end of the image\n");
         break;
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

      /* checksum, starts at offset 0x30 + section length, two bytes */
      state = read(in_fd, &generic16, 2);
      if (state == -1) {
         printf("file corrupted: %s\n", argv[1]);
         exit(EXIT_FAILURE);
      }
      /* crc:16,8005,0,true,true,0
       * poly x^16 + x^15 + x^2 + 1
       */
      printf("\tcrc16     section[%d]: checksum[%x]\n", section, htons(generic16));
   }

   close(in_fd);

   exit(EXIT_SUCCESS);
}
