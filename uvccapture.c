
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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <linux/videodev2.h>
#include <assert.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <getopt.h>

#include "v4l2uvc.h"
#include "libjpeg_soft.h"
#include <gpioapi.h>

// #define JPEG_POSIX_DEST

#define DEFAULT_WIDTH 320
#define DEFAULT_HEIGHT 240

static const char version[] = VERSION;
int run = 1;
static char **cameraNames = NULL;

static int flash_gpio_active_val = 1;
static struct gpio_struct flashGpio =  { 0 };

void
sigcatch (int sig)
{
  fprintf (stderr, "Exiting...\n");
  run = 0;
  exit(0);
}

static void flash_off(void)
{
   if (flashGpio.pin > 0) {
      setGPIO(&flashGpio, OUT, !flash_gpio_active_val);
   }
}

static void flash_on(void)
{
   if (flashGpio.pin > 0) {
      setGPIO(&flashGpio, OUT, flash_gpio_active_val);
   }
}

void regsignal(int sig, void (*func)(int)) {
   struct sigaction sa;

   sa.sa_handler = func;
   sa.sa_flags = 0;

   sigaction(sig, &sa, NULL);
}

void
usage (void)
{
  fprintf (stderr, "uvccapture version %s\n", version);
  fprintf (stderr, "Usage is: uvccapture [options]\n");
  fprintf (stderr, "Options:\n");
  fprintf (stderr, "-v\t\tVerbose\n");
  fprintf (stderr, "-m\t\tCapture and save image in raw YUV format\n");
  fprintf (stderr, "-M\t\tOnly capture (don't save) image in raw YUV format\n");
  fprintf (stderr, "-j\t\tCompress YUV to JPEG after capture\n");
  fprintf (stderr, "-b <filename>\t\tCompress YUV to JPEG capture with preexisting image\n");
  fprintf (stderr, "-p\t\tConvert YUV to PPM after capture\n");
  fprintf (stderr, "-L\t\tList Input Sources and exit\n");
  fprintf (stderr, "-N\t\tSet the input camera to the given named camera\n");
  fprintf (stderr, "-R\t\tRotate cameras between images.\n");
  fprintf (stderr, "-T <seconds>\t\tTerminate the process after <seconds> seconds.  Uses SIGALRM.\n");
  fprintf (stderr, "-o <filename>\tOutput filename formax(default: <filename>.<cameraname>.<# in series>.<extention>)\n");
  fprintf (stderr, "-O <filename>\tCompletly override output filename (yuv > ppm > jpg\n");
  fprintf (stderr, "-d <device>\tV4L2 Device(default: /dev/video0)\n");
  fprintf (stderr, "-D <integer>\tDelay (s) before taking first image\n");
  fprintf (stderr, "-F <integer>\tUse GPIO <integer> as flash\n");
  fprintf (stderr, "-f <integer>\tSet flash active value to <integer>\n");
  fprintf (stderr,
	   "-x <width>\tImage Width(must be supported by device)(>960 activates YUYV capture)\n");
  fprintf (stderr,
	   "-y <height>\tImage Height(must be supported by device)(>720 activates YUYV capture)\n");
  fprintf (stderr,
	   "-c <command>\tCommand to run after each image capture(executed as <command> <output_filename>)\n");
  fprintf (stderr,
	   "-t <integer>\tTake continuous shots with <integer> seconds between them (0 for single shot)\n");
  fprintf (stderr,
	   "-q <percentage>\tJPEG Quality Compression Level (activates YUYV capture)\n");
  fprintf (stderr, "-r\tUse read instead of mmap for image capture\n");
  fprintf (stderr,
	   "-w\tWait for capture command to finish before starting next capture\n");
  fprintf (stderr,
	   "-D <integer>\tDelay <integer> seconds before capturing\n");
  fprintf (stderr,
	   "-n <integer>\tTake only <integer> images.  Default is 1.\n");
  fprintf (stderr,
	   "-z \tTake raw capture, writes raw bayer BGGR 10-bit unpacked to 16-bits per color.\n");
  fprintf (stderr, "-e <integer>\tSet manual exposure to <integer> nanoseconds.\n");
  fprintf (stderr, "            \tMax value is integer max or 20-bit unsigned max of VTS * sysclock_ns\n");
  fprintf (stderr, "-g <integer>\tSet manual gain to <integer> / 16. Up to max of 992 (/ 16)\n");
  fprintf (stderr, "            \tMax analog gain is 16x, remaining gain will be digital gain\n");
  fprintf (stderr, "Camera Settings:\n");
  fprintf (stderr, "-A\tUse Auto Exposure, and how many additional seconds for auto exposure\n");
  fprintf (stderr, "-B <integer>\tBrightness, sets black level\n");
  fprintf (stderr, "-C <integer>\tContrast [Not implemented]\n");
  fprintf (stderr, "-S <integer>\tSaturation [Not implemented]\n");
  fprintf (stderr, "-Q\tUse direct mode when saving files (slower)\n");
  exit (8);
}

uint8_t rev_byte(uint8_t val)
{
   uint8_t res = 0;
   int i;

   for (i = 0; i < 8; i++)
      if (val & (1 << i))
         res |= (0x80 >> i);

   return res;
}

void print_camera_list(struct vdIn *videoIn)
{
   int i = 0;
   int res = 0;
   struct v4l2_input input;

   printf("Available device inputs:\n");
   while (!res) {
      res = v4l2InputInfo(videoIn, i, &input);
      if (res) {
         if (errno != EINVAL) {
            perror("Reading input information\n");
         }
         break;
      }

      if (input.type == V4L2_INPUT_TYPE_CAMERA)
         printf("  %s\n", input.name);

      i++;
   }
}

int num_cameras(struct vdIn *videoIn)
{
   int i = 0;
   int res = 0;
   struct v4l2_input input;

   while (!res) {
      res = v4l2InputInfo(videoIn, i, &input);
      if (res) {
         if (errno != EINVAL) {
            perror("Reading counting information\n");
         }
         return i;
      }

      if (input.type == V4L2_INPUT_TYPE_CAMERA)
         i++;
   }

   return 0;
}

int setup_cameras(struct vdIn *videoIn)
{
   int i = 0;
   int res = 0;
   struct v4l2_input input;

   int num;

   num = num_cameras(videoIn);
   if (num <= 0)
      return -1;

   cameraNames = malloc(sizeof(char*) * (num + 1));
   cameraNames[num] = 0;

   for (i = 0; i < num; i++) {
      cameraNames[i] = NULL;
      res = v4l2InputInfo(videoIn, i, &input);
      if (res) {
         if (errno != EINVAL) {
            fprintf(stderr, "Camera %d\n", num);
            perror("Reading reading camera information");
         }
         continue;
      }

      if (input.type == V4L2_INPUT_TYPE_CAMERA) {
         cameraNames[i] = strdup((char*)input.name);
      }
   }

   return 0;
}

int
spawn (char *argv[], int wait, int verbose)
{
  pid_t pid;
  int rv;

  switch (pid = fork ()) {
  case -1:
    return -1;
  case 0:
    // CHILD
    execvp (argv[0], argv);
    fprintf (stderr, "Error executing command '%s'\n", argv[0]);
    exit (1);
  default:
    // PARENT
    if (wait == 1) {
      if (verbose >= 1)
	fprintf (stderr, "Waiting for command to finish...");
      waitpid (pid, &rv, 0);
      if (verbose >= 1)
	fprintf (stderr, "\n");
    } else {
      // Clean zombies
      waitpid (-1, &rv, WNOHANG);
      rv = 0;
    }
    break;
  }

  return rv;
}

int save_yuyv3(struct vdIn *vd, const char *filename, int nobuff)
{
   int out;
   uint16_t tmp;
   int tot_size;
   struct timeval start, end;
   unsigned char *line_buff[4096];
   unsigned char *itr;
   int remaining, target;

   gettimeofday(&start, NULL);
   out = open(filename, O_RDWR | O_CREAT | (nobuff ? O_DIRECT:0), 0644);
   printf("** Filename : %s\n\r", filename);
   if (out < 0) {
      perror("Error opening snap.yuv");
      return -1;
   }

   tot_size = sizeof(tmp) + sizeof(tmp) + vd->fbCap;
   ftruncate(out, tot_size);

   tmp = vd->width;
   if (sizeof(tmp) != write(out, &tmp, sizeof(tmp)))
      perror("First write");
   tmp = vd->height;
   if (sizeof(tmp) != write(out, &tmp, sizeof(tmp)))
      perror("Second write");

   remaining = tot_size - sizeof(tmp) - sizeof(tmp);

   itr = vd->framebuffer;
   while (remaining > 0) {
      target = remaining > sizeof(line_buff) ? sizeof(line_buff) : remaining;
      memcpy(line_buff, itr, target);
      if (write(out, line_buff, target) != target) {
         perror("image write");
         close(out);
         return -1;
      }
      itr += target;
      remaining -= target;
   }

   close(out);
   gettimeofday(&end, NULL);

   end.tv_sec -= start.tv_sec;
   end.tv_usec -= start.tv_usec;
   if (end.tv_usec < 0) {
      end.tv_usec += 1000000;
      end.tv_sec -= 1;
   }

   printf("YUV Time: %ld.%06ld\n", end.tv_sec, end.tv_usec);

   return 0;
}

int save_yuyv(struct vdIn *vd, const char *filename, int nobuff)
{
   int out;
   uint16_t tmp;
   int tot_size;
   struct timeval start, end;

   gettimeofday(&start, NULL);
   out = open(filename, O_RDWR | O_CREAT | (nobuff ? O_DIRECT:0), 0644);
   printf("** Filename : %s\n\r", filename);
   if (out < 0) {
      perror("Error opening snap.yuv");
      return -1;
   }

   tot_size = sizeof(tmp) + sizeof(tmp) + vd->fbCap;
   ftruncate(out, tot_size);

   tmp = vd->width;
   if (sizeof(tmp) != write(out, &tmp, sizeof(tmp)))
      perror("First write");
   tmp = vd->height;
   if (sizeof(tmp) != write(out, &tmp, sizeof(tmp)))
      perror("Second write");

   if (write(out, vd->framebuffer, vd->fbCap) != vd->fbCap)
      perror("image write");

   close(out);
   gettimeofday(&end, NULL);

   end.tv_sec -= start.tv_sec;
   end.tv_usec -= start.tv_usec;
   if (end.tv_usec < 0) {
      end.tv_usec += 1000000;
      end.tv_sec -= 1;
   }

   printf("YUV Time: %ld.%06ld\n", end.tv_sec, end.tv_usec);

   return 0;
}

int save_raw_bayer(struct vdIn *vd, const char *filename)
{
   int out;

   out = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
   if (out < 0) {
      perror("Error opening raw bayer output file");
      return -1;
   }
   if (vd->buf.bytesused != write(out, vd->framebuffer, vd->buf.bytesused)) {
      perror("short raw bayer write");
      close(out);
      return -1;
   }
   close(out);

   return 0;
}


int
save_raw_jpeg (struct vdIn *vd, const char *filename)
{
   int out;

   out = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);
   if (out < 0) {
      perror("Error opening jpeg output file");
      return -1;
   }
   if (vd->buf.bytesused != write(out, vd->framebuffer, vd->buf.bytesused)) {
      perror("short jpeg write");
      close(out);
      return -1;
   }
   close(out);

   return 0;
}

#ifdef JPEG_POSIX_DEST

typedef struct {
     struct jpeg_destination_mgr pub; /* public fields */
     JOCTET buffer[4096];
     int fd;
} posix_dest_mgr;

static void posix_init_destination (j_compress_ptr cinfo)
{
   posix_dest_mgr *dest = (posix_dest_mgr*) cinfo->dest;

   dest->pub.next_output_byte = dest->buffer;
   dest->pub.free_in_buffer = sizeof(dest->buffer);
}

static boolean posix_empty_output_buffer (j_compress_ptr cinfo)
{
   posix_dest_mgr *dest = (posix_dest_mgr*) cinfo->dest;

   if (sizeof(dest->buffer) != write(dest->fd, dest->buffer,
            sizeof(dest->buffer))) {
      perror("short jpeg write");
      return FALSE;
   }

   dest->pub.next_output_byte = dest->buffer;
   dest->pub.free_in_buffer = sizeof(dest->buffer);
   return TRUE;
}

void posix_term_destination (j_compress_ptr cinfo)
{
   posix_dest_mgr *dest = (posix_dest_mgr*) cinfo->dest;
   size_t datacount = sizeof(dest->buffer) - dest->pub.free_in_buffer;

   /* Write any data remaining in the buffer */
   if (datacount > 0) {
      if (write(dest->fd, dest->buffer, datacount) != datacount) {
         perror("Short JPEG write");
      }
   }
}

#endif

int
compress_yuyv_to_jpeg (struct vdIn *vd, const char *filename, int quality)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];
  unsigned char *line_buffer, *yuyv;
  int z;
  struct timeval start, end;
#ifdef JPEG_POSIX_DEST
  posix_dest_mgr dest;
#endif

  // gsPpm = fopen("snap1-gs.ppm", "w");
  // assert(gsPpm);
  // fprintf(gsPpm, "P2\n%d %d\n255\n",  vd->width, vd->height);

  gettimeofday(&start, NULL);
  // printf("Compressing to jpeg!\n");
#ifdef JPEG_POSIX_DEST
  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
     perror("Error opening PPM file");
     return -1;
  }
   dest.pub.init_destination = posix_init_destination;
   dest.pub.empty_output_buffer = posix_empty_output_buffer;
   dest.pub.term_destination = posix_term_destination;
   dest.fd = fd;
#else
  FILE *file = fopen (filename, "wb");
  if (!file) {
     perror ("Error opening output file");
     return -1;
  }
#endif

  line_buffer = calloc (vd->width * 3, 1);
  yuyv = vd->framebuffer;

  cinfo.err = jpeg_std_error_ind (&jerr);
  jpeg_create_compress_ind (&cinfo);
#ifdef JPEG_POSIX_DEST
  cinfo.dest = &dest.pub;
#else
  jpeg_stdio_dest_ind (&cinfo, file);
#endif

  cinfo.image_width = vd->width;
  cinfo.image_height = vd->height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults_ind (&cinfo);
  jpeg_set_quality_ind (&cinfo, quality, TRUE);

  jpeg_start_compress_ind (&cinfo, TRUE);

  z = 0;
  while (cinfo.next_scanline < cinfo.image_height) {
    int x;
    unsigned char *ptr = line_buffer;

    for (x = 0; x < vd->width; x++) {
      int r, g, b;
      int y = 0, u = 0, v = 0;

      if (vd->formatIn == V4L2_PIX_FMT_YUYV) {
         if (!z)
	         y = yuyv[0] << 8;
         else
	         y = yuyv[2] << 8;
         u = yuyv[1] - 128;
         v = yuyv[3] - 128;
      } 
      else if (vd->formatIn == V4L2_PIX_FMT_YVYU) {
         if (!z)
	         y = yuyv[0] << 8;
         else
	         y = yuyv[2] << 8;
         v = yuyv[1] - 128;
         u = yuyv[3] - 128;
      }
      else if (vd->formatIn == V4L2_PIX_FMT_UYVY) {
         if (!z)
	         y = yuyv[1] << 8;
         else
	         y = yuyv[3] << 8;
         u = yuyv[0] - 128;
         v = yuyv[2] - 128;
      }
      else if (vd->formatIn == V4L2_PIX_FMT_VYUY) {
         // Atmel uses this one
         if (!z)
	         y = yuyv[1] << 8;
         else
	         y = yuyv[3] << 8;
         u = yuyv[2] - 128;
         v = yuyv[0] - 128;
      }


      r = (y + (359 * v)) >> 8;
      g = (y - (88 * u) - (183 * v)) >> 8;
      b = (y + (454 * u)) >> 8;

      *(ptr++) = (r > 255) ? 255 : ((r < 0) ? 0 : r);
      *(ptr++) = (g > 255) ? 255 : ((g < 0) ? 0 : g);
      *(ptr++) = (b > 255) ? 255 : ((b < 0) ? 0 : b);

      if (z++) {
	z = 0;
	yuyv += 4;
      }
    }

    row_pointer[0] = line_buffer;
    jpeg_write_scanlines_ind (&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress_ind (&cinfo);
  jpeg_destroy_compress_ind (&cinfo);

  free (line_buffer);
#ifdef JPEG_POSIX_DEST
  close(fd);
#else
  fclose(file);
#endif

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

int
convert_yuyv_to_ppm (struct vdIn *vd, char * filename)
{
  unsigned char *yuyv;
  int z;
  int scanline = 0;
  int fd;
  char header[512];
  unsigned char *line_buffer;
  struct timeval start, end;
  int tot_len;

  // gsPpm = fopen("snap1-gs.ppm", "w");
  // assert(gsPpm);
  // fprintf(gsPpm, "P2\n%d %d\n255\n",  vd->width, vd->height);
  // printf("Converting to PPM!\n");
  gettimeofday(&start, NULL);
  fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
     perror("Error opening PPM file");
     return -1;
  }

  snprintf(header, sizeof(header), "P6\n%d %d\n255\n",
        vd->width, vd->height);
  header[sizeof(header)-1] = 0;

  tot_len = strlen(header) + vd->width * vd->height * 3;
  ftruncate(fd, tot_len);

  line_buffer = malloc (vd->width * 3);
  /*
  line_buffer = mmap(NULL, tot_len, PROT_WRITE,
        MAP_FILE | MAP_SHARED, fd, 0);
  if (!line_buffer) {
     perror("mmap");
     return -1;
  }
  */
  yuyv = vd->framebuffer;

  // memcpy(ptr, header, strlen(header));
  // ptr += strlen(header);
  write(fd, header, strlen(header));

  z = 0;
  for (scanline = 0; scanline < vd->height; scanline++) {
    int x;
    unsigned char *ptr = (unsigned char*)line_buffer;

    for (x = 0; x < vd->width; x++) {
      int r, g, b;
      int y = 0, u = 0, v = 0;

      if (vd->formatIn == V4L2_PIX_FMT_YUYV) {
         if (!z)
	         y = yuyv[0] << 8;
         else
	         y = yuyv[2] << 8;
         u = yuyv[1] - 128;
         v = yuyv[3] - 128;
      } 
      else if (vd->formatIn == V4L2_PIX_FMT_YVYU) {
         if (!z)
	         y = yuyv[0] << 8;
         else
	         y = yuyv[2] << 8;
         v = yuyv[1] - 128;
         u = yuyv[3] - 128;
      }
      else if (vd->formatIn == V4L2_PIX_FMT_UYVY) {
         if (!z)
	         y = yuyv[1] << 8;
         else
	         y = yuyv[3] << 8;
         u = yuyv[0] - 128;
         v = yuyv[2] - 128;
      }
      else if (vd->formatIn == V4L2_PIX_FMT_VYUY) {
         // Atmel uses this one
         if (!z)
	         y = yuyv[1] << 8;
         else
	         y = yuyv[3] << 8;
         u = yuyv[2] - 128;
         v = yuyv[0] - 128;
      }


      r = (y + (359 * v)) >> 8;
      *ptr++ = (r > 255) ? 255 : ((r < 0) ? 0 : r);

      g = (y - (88 * u) - (183 * v)) >> 8;
      *ptr++ = (g > 255) ? 255 : ((g < 0) ? 0 : g);

      b = (y + (454 * u)) >> 8;
      *ptr++ = (b > 255) ? 255 : ((b < 0) ? 0 : b);

      if (z++) {
	    z = 0;
	    yuyv += 4;
      }
    }

    write(fd, line_buffer, 3*vd->width);
  }
  free (line_buffer);
  // munmap(line_buffer, tot_len);
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

#define OV3642_CID_AEC_CONVERGE (V4L2_CID_PRIVATE_BASE + 1)

int wait_for_auto_exposure_control(struct vdIn *videoIn, int ov_autoexpo)
{
   struct timeval now, ag_end, select_delay;

   if (ov_autoexpo) {

     // diable manual exposure control
     v4l2ResetControl(videoIn, V4L2_CID_EXPOSURE);

     gettimeofday(&ag_end, NULL);
     ag_end.tv_sec += ov_autoexpo;

     do {
       int val = v4l2GetControl (videoIn, OV3642_CID_AEC_CONVERGE);
       if (val >= 0) {
	 printf("AEC Returned %d\n", val);
	 return 0;
       }
       if (val != -EAGAIN) {
	 printf("AEC returned error %s\n", strerror(errno));
	 return -errno;
       }
       select_delay.tv_sec = 0;
       select_delay.tv_usec = 250000;
       select(0, NULL, NULL, NULL, &select_delay);
       gettimeofday(&now, NULL);
     } while (timercmp(&now, &ag_end, <));
   }

   return 0;
}


int
main (int argc, char *argv[])
{
  char *videodevice = "/dev/video0";
  char *outputfile = "snap";
  char *outputfile_override = NULL;
  char *yuyv_file = NULL;
  char *post_capture_command[3];
  int format = V4L2_PIX_FMT_MJPEG;
  int grabmethod = 1;
  int width = 320;
  int height = 240;
  int brightness = 0, contrast = 0, saturation = 0;
  int verbose = 0;
  int delay = 0;
  int quality = 95;
  int ov_autoexpo = 0;
  int post_capture_command_wait = 0;
  time_t ref_time, img_time;
  struct vdIn *videoIn;
  int start_delay = 0;
  int image_cnt = 1, target_img_cnt = 1;
  struct timeval start_time, stop_time;
  int list_cameras = 0;
  const char *camera_name = NULL;
  int rotate = 0;
  int cam_index = -1;
  char outnameBuff[1024];
  char ppmOutnameBuff[1024];
  char yuvOutnameBuff[1024];
  char rawOutnameBuff[1024];
  int soft_compress_yuv = 0;
  int convert_yuv_to_ppm = 0;
  int alarm_time = -1;
  int save_yuyv_data = 1;
  int error = 0;
  int dbg = 0;
  int nobuff = 0;
  int ov_manual_exposure = 0;
  int ov_manual_gain = 0;
  int exposure_ns = 0;
  int gain = 0;

  int opt;

  (void) regsignal (SIGINT, sigcatch);
  (void) regsignal (SIGQUIT, sigcatch);
  (void) regsignal (SIGKILL, sigcatch);
  (void) regsignal (SIGTERM, sigcatch);
  (void) regsignal (SIGABRT, sigcatch);
  (void) regsignal (SIGTRAP, sigcatch);
  (void) regsignal (SIGALRM, sigcatch);
  // signal(SIGALRM, SIG_DFL);

  // set post_capture_command to default values
  post_capture_command[0] = NULL;
  post_capture_command[1] = NULL;
  post_capture_command[2] = NULL;

  while((opt = getopt(argc, argv, "b:c:d:e:f:g:hjmn:o:pq:rtvwzx:y:A:B:C:D:F:LMN:O:QRS:T:Z:")) != -1) {
     switch(opt){

         case 'b':
            yuyv_file = optarg;
            break;

         case 'c':
            post_capture_command[0] = optarg;
            break;

         case 'd':
            videodevice = optarg;
            break;

	 case 'e':
	    ov_manual_exposure = 1;
	    exposure_ns = atoi(optarg);
	    break;

         case 'f':
            flash_gpio_active_val = atoi(optarg);
            break;

	 case 'g':
	    ov_manual_gain = 1;
	    gain = atoi(optarg);
	    break;
         
         case 'h':
            usage ();
            break;

         case 'j':
            soft_compress_yuv = 1;
            break;

         case 'm':
            format = V4L2_PIX_FMT_YUYV;
            save_yuyv_data = 1;
            break;

         case 'z':
            format = V4L2_PIX_FMT_SBGGR16;
            break;
         
         case 'n':
            target_img_cnt = image_cnt = atoi(optarg);
            break;

         case 'o':
            outputfile = optarg;
            break;

         case 'p':
            convert_yuv_to_ppm = 1;
            break;

         case 'q':
            quality = atoi(optarg);
            break;

         case 'r':
            grabmethod = 0;
            break;

         case 't':
            delay = atoi (optarg);
            break;

         case 'v':
            verbose++;
            break;

         case 'w':
            post_capture_command_wait = 1;
            break;

         case 'x':
            width = atoi (optarg);
            break;

         case 'y':
            height = atoi (optarg);
            break;

         case 'A':
            ov_autoexpo = atoi (optarg);
            break;

         case 'B':
            brightness = atoi (optarg);
            break;

         case 'C':
            contrast = atoi (optarg);
            break;

         case 'D':
            start_delay = atoi (optarg);
            break;

         case 'F':
            initGPIO(0, atoi(optarg), &flashGpio);
            break;

         case 'L':
            list_cameras = 1;
            break;
         
         case 'M':
            format = V4L2_PIX_FMT_YUYV;
            save_yuyv_data = 0;
            break;
         
         case 'N':
            camera_name = optarg;
            break;
         
         case 'O':
            outputfile_override = optarg;
            break;

         case 'Q':
            nobuff = 1;
            break;

         case 'R':
            rotate = 1;
            break;

         case 'S':
            saturation = atoi (optarg);
            break;
         
         case 'T':
            alarm_time = atoi(optarg);
            break;

         case 'Z':
            dbg = atoi(optarg);
            break;

         case '?':
            if (optopt == 'c')
               fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
               fprintf (stderr,
                        "Unknown option character `\\x%x'.\n",
                        optopt);
            return 1;
         default:
            fprintf(stderr, "Error in getopt: %d\n", opt);
            break;
     }
  }

  atexit(flash_off);

  if (alarm_time > 0)
     alarm(alarm_time);

  if (post_capture_command[0])
    post_capture_command[1] = outputfile;

  if (verbose >= 1) {
    fprintf (stderr, "Using videodevice: %s\n", videodevice);
    if(outputfile_override){
      fprintf (stderr, "Saving images to: %s\n", outputfile_override);
    }else{
      fprintf (stderr, "Saving images with prefix: %s\n", outputfile);
    }
    fprintf (stderr, "Image size: %dx%d\n", width, height);
    fprintf (stderr, "Taking snapshot every %d seconds\n", delay);
    if (grabmethod == 1)
      fprintf (stderr, "Taking images using mmap\n");
    else
      fprintf (stderr, "Taking images using read\n");
    if (post_capture_command[0])
      fprintf (stderr, "Executing '%s' after each image capture\n",
	       post_capture_command[0]);
  }
  videoIn = (struct vdIn *) calloc (1, sizeof (struct vdIn));
  if (init_videoIn (videoIn, (char *) videodevice) < 0)
  {
     fprintf(stderr, "Error in init_videoIn\n");
    exit (1);
  }

  // Only converting image?
  if (yuyv_file) {
     printf("*** Converting yuyv image\n\r");
     if (libjpeg_avail())
	     error = compress_yuyv_to_jpeg (videoIn, yuyv_file, quality);
     close_v4l2 (videoIn);
     free (videoIn);
     return error;
  }

  if (list_cameras) {
     print_camera_list(videoIn);
     return 0;
  }

  if (setup_cameras(videoIn)) {
     perror("Error reading camera names");
     return 1;
  }

  if (camera_name) {
     if (v4l2SetInput(videoIn, camera_name)) {
        perror("Error selecting camera");
        fprintf(stderr, "\nCamera choice:%s\nVerify spelling and upper/lower case.\n", camera_name);
        print_camera_list(videoIn);
        return 2;
     }
  }

  cam_index = v4l2GetInput(videoIn);
  if (cam_index < 0) {
     perror("Error reading camera index\n");
     return 3;
  }
  else if (verbose >= 1) {
      fprintf (stderr, "Using camera with index %d\n", cam_index);
  }

  if (dbg == 1) {
   if (verbose >= 1)
      fprintf (stderr, "Debug exit 1\n");
   exit(0);
  }


  if (v4l2TryFormat(videoIn, width, height, format) < 0)
  {
     fprintf(stderr, "Error in v4l2TryFormat\n");
  }

  if(width <= 0){
     fprintf(stderr, "Invalid width, setting to defualt \n");
     width = DEFAULT_WIDTH;
  }

  if(height <= 0){
     fprintf(stderr, "Invalid height, setting to defualt \n");
     height = DEFAULT_HEIGHT;
  }

  if (dbg == 2) {
   if (verbose >= 1)
      fprintf (stderr, "Debug exit 2\n");
   exit(0);
  }

  if (v4l2SetFormat(videoIn, width, height, format, grabmethod) < 0)
  {
     perror("Error in v4l2SetFormat");
    exit (4);
  }

  if (dbg == 3) {
   if (verbose >= 1)
      fprintf (stderr, "Debug exit 3\n");
   exit(0);
  }

  if (verbose >= 1)
    fprintf (stderr, "Resetting camera settings\n");
  v4l2ResetControl (videoIn, V4L2_CID_BRIGHTNESS);

  if (dbg == 4) {
   if (verbose >= 1)
      fprintf (stderr, "Debug exit 4\n");
   exit(0);
  }

  if (verbose >= 1)
    fprintf (stderr, "Before brightness\n");

  //Setup Camera Parameters
  if (brightness != 0) {
    if (verbose >= 1)
      fprintf (stderr, "Setting camera brightness to %d\n", brightness);

    // black level
    v4l2SetControl (videoIn, V4L2_CID_BRIGHTNESS, brightness);
  } else if (verbose >= 1) {
    fprintf (stderr, "Camera brightness level is %d\n",
	     v4l2GetControl (videoIn, V4L2_CID_BRIGHTNESS));
  }

  if (ov_manual_exposure) {
    fprintf(stderr, "Setting manual exposure to %dns\n", exposure_ns);
    fprintf(stderr, "(note: exposure may be clamped depending on camera clock rate and line width)\n");
    ov_autoexpo = 0;

    v4l2SetControl(videoIn, V4L2_CID_EXPOSURE, exposure_ns);
  } else {
    v4l2ResetControl(videoIn, V4L2_CID_EXPOSURE);
  }

  if (ov_manual_gain) {
    fprintf(stderr, "Setting manual gain to %d / 16\n", gain);
    fprintf(stderr, "(note: max analog gain is 64x, digital makes up any remaining gain)\n");

    v4l2SetControl(videoIn, V4L2_CID_GAIN, gain);
  } else {
    v4l2ResetControl(videoIn, V4L2_CID_GAIN);
  }

  if (verbose >= 1)
    fprintf (stderr, "before AEC %d\n", ov_autoexpo);

  flash_on();
  wait_for_auto_exposure_control(videoIn, ov_autoexpo);

  if (verbose >= 1)
    fprintf (stderr, "before start delay %d\n", start_delay);

  if (start_delay > 0)
     sleep(start_delay);

  ref_time = time (NULL);
  gettimeofday(&start_time, NULL);

  while (run) {
    if (verbose >= 2)
      fprintf (stderr, "Grabbing frame\n");
    img_time = time (NULL);
    if (uvcGrab (videoIn) < 0) {
      perror("Error grabbing");
      close_v4l2 (videoIn);
      free (videoIn);
      exit (5);
    }

    if (delay == 0 || image_cnt == target_img_cnt ||
               (difftime (time (NULL), ref_time) > delay)) {
      ref_time = img_time;
      
         if(outputfile_override){
            sprintf(outnameBuff, "%s", outputfile_override);
            sprintf(ppmOutnameBuff, "%s", outputfile_override);
            sprintf(yuvOutnameBuff, "%s", outputfile_override);
            sprintf(rawOutnameBuff, "%s", outputfile_override);
         }else{
            sprintf(outnameBuff, "%s.%s.%d.jpg", outputfile, cameraNames[cam_index], target_img_cnt - image_cnt);
            sprintf(ppmOutnameBuff, "%s.%s.%d.ppm", outputfile, cameraNames[cam_index], target_img_cnt - image_cnt);
            sprintf(yuvOutnameBuff, "%s.%s.%d.yuv", outputfile, cameraNames[cam_index], target_img_cnt - image_cnt);
            sprintf(rawOutnameBuff, "%s.%s.%d.raw", outputfile, cameraNames[cam_index], target_img_cnt - image_cnt);
         }
      
      if (verbose >= 1)
	      fprintf (stderr, "Saving image to: %s / %s / %s / %s\n", 
			      outnameBuff, rawOutnameBuff, yuvOutnameBuff, ppmOutnameBuff);
	switch (videoIn->formatIn) {
	case V4L2_PIX_FMT_YUYV:
	case V4L2_PIX_FMT_YVYU:
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_VYUY:
      if (save_yuyv_data)
         error = error || save_yuyv(videoIn, yuvOutnameBuff, nobuff);
      if (convert_yuv_to_ppm)
         error = error || convert_yuyv_to_ppm(videoIn, ppmOutnameBuff);
      if (soft_compress_yuv && libjpeg_avail())
	      error = error || compress_yuyv_to_jpeg (videoIn, outnameBuff, quality);
      
	  break;
	case V4L2_PIX_FMT_SBGGR16:
     	      error = error || save_raw_bayer(videoIn, rawOutnameBuff);
	  break;

   case V4L2_PIX_FMT_MJPEG:
   case V4L2_PIX_FMT_JPEG:
     error = error || save_raw_jpeg(videoIn, outnameBuff);
     break;

	default:
	  break;
	}
	videoIn->getPict = 0;

      if (post_capture_command[0]) {
         if (verbose >= 1)
           fprintf (stderr, "Executing '%s %s'\n", post_capture_command[0],
               post_capture_command[1]);
         if (spawn (post_capture_command, post_capture_command_wait, verbose)) {
           fprintf (stderr, "Command exited with error\n");
           close_v4l2 (videoIn);
           free (videoIn);
           exit (6);
         }
      }

      if (--image_cnt == 0)
        break;

      if (rotate) {
         if (!cameraNames[++cam_index]) {
            cam_index = 0;
         }
         if (v4l2SetInputNum(videoIn, cam_index)) {
            perror("Error selecting input");
            if (!error)
               error = 7;
            run = 0;
         }
   
         /*
         if (v4l2SetFormat(videoIn, width, height, format, grabmethod) < 0)
         {
            fprintf(stderr, "Error in v4l2SetFormat\n");
            exit (1);
         }
         */
      }
         wait_for_auto_exposure_control(videoIn, ov_autoexpo);
    }
  }
  flash_off();
  gettimeofday(&stop_time, NULL);
  close_v4l2 (videoIn);
  free (videoIn);

  stop_time.tv_sec -= start_time.tv_sec;
  while (stop_time.tv_usec < start_time.tv_usec) {
     stop_time.tv_sec--;
     stop_time.tv_usec += 1000000;
  }
  float secs = stop_time.tv_sec + stop_time.tv_usec / 1000000.0;

  if (secs != 0)
   printf("FPS: %f\n", (target_img_cnt - image_cnt) / secs);

  return error;
}

/** Simple capture command to be used by payload processes */
int simple_capture(char *outputfile_override, int brightness, int ov_autoexpo) {

   int grabmethod = 1;
   char *videodevice = "/dev/video0";
   int format = V4L2_PIX_FMT_YUYV;
   int width = 320;
   int height = 240;
   struct vdIn *videoIn;
   const char *camera_name = NULL;
   char yuvOutnameBuff[1024];
   int error = 0;
   int nobuff = 0;
   int cam_index = -1;

   videoIn = (struct vdIn *) calloc (1, sizeof (struct vdIn));
   if (init_videoIn (videoIn, (char *) videodevice) < 0)
   {
      fprintf(stderr, "Error in init_videoIn\n");
      return 1;
   }
  
   if (setup_cameras(videoIn)) {
      perror("Error reading camera names");
      return 2;
   }

   cam_index = v4l2GetInput(videoIn);
   if (cam_index < 0) {
      perror("Error reading camera index\n");
      return 3;
   }

   if (v4l2TryFormat(videoIn, width, height, format) < 0)
   {
      fprintf(stderr, "Error in v4l2TryFormat\n");
   }


   if(width <= 0){
      fprintf(stderr, "Invalid width, setting to defualt \n");
      width = DEFAULT_WIDTH;
   }

   if(height <= 0){
      fprintf(stderr, "Invalid height, setting to defualt \n");
      height = DEFAULT_HEIGHT;
   }

   if (v4l2SetFormat(videoIn, width, height, format, grabmethod) < 0)
   {
      perror("Error in v4l2SetFormat");
      return 4;
   }

   v4l2ResetControl (videoIn, V4L2_CID_BRIGHTNESS);

   if (brightness != 0) {
      v4l2SetControl (videoIn, V4L2_CID_BRIGHTNESS, brightness);
   } 

   flash_on();
   wait_for_auto_exposure_control(videoIn, ov_autoexpo);

   if (uvcGrab (videoIn) < 0) {
      perror("Error grabbing");
      close_v4l2 (videoIn);
      free (videoIn);
      return 5;
   }

   error = error || save_yuyv(videoIn, outputfile_override, nobuff);

   videoIn->getPict = 0;

   flash_off();
   close_v4l2 (videoIn);
   free (videoIn);

   return error;
}


/** Thumbnail */
/**
// Only converting image?
  if (yuyv_file) {
     printf("*** Converting yuyv image\n\r");
     if (libjpeg_avail())
	     error = compress_yuyv_to_jpeg (videoIn, yuyv_file, quality);
     close_v4l2 (videoIn);
     free (videoIn);
     return error;
  }
*/
