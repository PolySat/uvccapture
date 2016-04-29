
/*******************************************************************************
#             uvccapture: USB UVC Video Class Snapshot Software                #
#This package work with the Logitech UVC based webcams with the mjpeg feature  #
#.                                                                             #
# 	Orginally Copyright (C) 2005 2006 Laurent Pinchart &&  Michel Xhaard   #
#       Modifications Copyright (C) 2006  Gabriel A. Devenyi                   #
#                                                                              #
# This program is free software; you can redistribute it and/or modify         #
# it under the terms of the GNU General Public License as published by         #
# the Free Software Foundation; either version 2 of the License, or            #
# (at your option) any later version.                                          #
#                                                                              #
# This program is distributed in the hope that it will be useful,              #
# but WITHOUT ANY WARRANTY; without even the implied warranty of               #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                #
# GNU General Public License for more details.                                 #
#                                                                              #
# You should have received a copy of the GNU General Public License            #
# along with this program; if not, write to the Free Software                  #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA    #
#                                                                              #
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <jpeglib.h>
#include <time.h>
#include <assert.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <strings.h>

void usage (const char *app)
{
  fprintf (stderr, "Usage is: %s [options] <in file>\n", app);
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "-j\t\tCompress input YUV to JPEG\n");
  fprintf (stderr, "-p\t\tConvert input YUV to PPM\n");
  exit (8);
}

int compress_yuyv_to_jpeg (const char *basename, int quality, uint16_t width, uint16_t height, void *data)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  unsigned char *line_buffer, *yuyv;
  int z;
  struct timeval start, end;
  char outname[1024];

  sprintf(outname, "%s.jpg", basename);
  gettimeofday(&start, NULL);
  FILE *file = fopen (outname, "wb");
  if (!file) {
     perror ("Error opening output file");
     return -1;
  }

  line_buffer = calloc (width * 3, 1);
  yuyv = data;

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_compress (&cinfo);
  jpeg_stdio_dest (&cinfo, file);

  cinfo.image_width = width;
  cinfo.image_height = height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);

  jpeg_start_compress (&cinfo, TRUE);

  z = 0;
  while (cinfo.next_scanline < cinfo.image_height) {
    int x;
    unsigned char *ptr = line_buffer;

    for (x = 0; x < width; x++) {
      int r, g, b;
      int y = 0, u = 0, v = 0;

         // Atmel uses this one
         if (!z)
	         y = yuyv[1] << 8;
         else
	         y = yuyv[3] << 8;
         u = yuyv[2] - 128;
         v = yuyv[0] - 128;


      r = (y + (359 * v)) >> 8;
      g = (y - (88 * u) - (183 * v)) >> 8;
      b = (y + (454 * u)) >> 8;
#define CAP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)) )

      *(ptr++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
      *(ptr++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
      *(ptr++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);

      if (z++) {
	z = 0;
	yuyv += 4;
      }
    }

    row_pointer[0] = line_buffer;
    jpeg_write_scanlines (&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress (&cinfo);
  jpeg_destroy_compress (&cinfo);

  free (line_buffer);
  fclose(file);

  gettimeofday(&end, NULL);
  end.tv_sec -= start.tv_sec;
  end.tv_usec -= start.tv_usec;
  if (end.tv_usec < 0) {
     end.tv_usec += 1000000;
     end.tv_sec -= 1;
  }

  printf("JPEG Time: %ld.%06ld\n", end.tv_sec, end.tv_usec);

  return (0);
}

int convert_yuyv_to_ppm (const char *basename, uint16_t width, 
      uint16_t height, void *data)
{
  unsigned char *yuyv;
  int z;
  int scanline = 0;
  int fd;
  char header[512];
  unsigned char *line_buffer;
  struct timeval start, end;
  int tot_len;
  char outname[1024];

  sprintf(outname, "%s.ppm", basename);

  gettimeofday(&start, NULL);
  fd = open(outname, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
     perror("Error opening PPM file");
     return -1;
  }

  snprintf(header, sizeof(header), "P6\n%d %d\n255\n",
        width, height);
  header[sizeof(header)-1] = 0;

  tot_len = strlen(header) + width * height * 3;
  ftruncate(fd, tot_len);

  line_buffer = malloc (width * 3);
  yuyv = data;

  write(fd, header, strlen(header));

  z = 0;
  for (scanline = 0; scanline < height; scanline++) {
    int x;
    unsigned char *ptr = (unsigned char*)line_buffer;

    for (x = 0; x < width; x++) {
      int r, g, b;
      int y = 0, u = 0, v = 0;

         // Atmel uses this one
         if (!z)
	         y = yuyv[1] << 8;
         else
	         y = yuyv[3] << 8;
         u = yuyv[2] - 128;
         v = yuyv[0] - 128;


      r = (y + (359 * v)) >> 8;
      *ptr++ = (r > 255) ? 255 : ((r < 0) ? 0 : r);

      g = (y - (88 * u) - (183 * v)) >> 8;
      *ptr++ = (g > 255) ? 255 : ((g < 0) ? 0 : g);

      b = (y + (454 * u)) >> 8;
      *ptr++ = (b > 255) ? 255 : ((b < 0) ? 0 : b);
#define CAP(x) ((x) < 0 ? 0 : ((x) > 255 ? 255 : (x)) )

      if (z++) {
	z = 0;
	yuyv += 4;
      }
    }

  write(fd, line_buffer, 3*width);
  }
  write(fd, line_buffer, 3*width);
  free (line_buffer);
  close(fd);
  gettimeofday(&end, NULL);

  end.tv_sec -= start.tv_sec;
  end.tv_usec -= start.tv_usec;
  if (end.tv_usec < 0) {
     end.tv_usec += 1000000;
     end.tv_sec -= 1;
  }

  printf("PPM Time: %ld.%06ld\n", end.tv_sec, end.tv_usec);

  return (0);
}

int
main (int argc, char *argv[])
{
  char *input_file = NULL;
  struct timeval start_time, stop_time;
  char outnameBuff[1024];
  int quality = 95;

  int soft_compress_yuv = 0;
  int convert_yuv_to_ppm = 0;

  char *sep;
  int inp;
  uint16_t width, height;
  void *pixels;


  //Options Parsing (FIXME)
  while ((argc > 1) && (argv[1][0] == '-')) {
    switch (argv[1][1]) {
    case 'j':
      soft_compress_yuv = 1;
      break;

    case 'p':
      convert_yuv_to_ppm = 1;
      break;

    case 'h':
      usage ("yuv_decode");
      break;

    default:
      fprintf (stderr, "Unknown option %s \n", argv[1]);
      usage ("yuv_decode");
    }
    ++argv;
    --argc;
  }

  if (argc <= 1)
     usage("yuv_decode");

  input_file = argv[1];
  strcpy(outnameBuff, input_file);
  /*
  sep = rindex(input_file, '.');
  if (!sep)
     strcpy(outnameBuff, input_file);
  else {
     memset(outnameBuff, 0, sizeof(outnameBuff));
     memcpy(outnameBuff, input_file, sep - input_file);
  }
  */

  inp = open(input_file, O_RDONLY);
  if (inp < 0) {
     perror("Error opening input file");
     exit(0);
  }

  if (sizeof(width) != read(inp, &width, sizeof(width))) {
     perror("Read width");
     exit(0);
  }

  if (sizeof(height) != read(inp, &height, sizeof(height))) {
     perror("Read height");
     exit(0);
  }

  pixels = mmap(NULL, width * height * 2 + sizeof(width) + sizeof(height), PROT_READ, MAP_SHARED | MAP_FILE, inp, 0);
  if (MAP_FAILED == pixels) {
     perror("mmap");
     exit(0);
  }

  printf("%s (%d x %d)\n", outnameBuff, width, height);


   if (soft_compress_yuv)
      compress_yuyv_to_jpeg (outnameBuff, quality, width, height, pixels + sizeof(width) + sizeof(height));

   if (convert_yuv_to_ppm)
      convert_yuyv_to_ppm(outnameBuff, width, height, pixels + sizeof(width) + sizeof(height));

  munmap(pixels, width * height * 2);

  return 0;
}
