
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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
// #include <linux/videodev.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include "v4l2uvc.h"

static int debug = 0;

static unsigned char dht_data[DHT_SIZE] __attribute__((unused)) = {
  0xff, 0xc4, 0x01, 0xa2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
  0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x01, 0x00, 0x03,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
  0x0a, 0x0b, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05,
  0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04,
  0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22,
  0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15,
  0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36,
  0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
  0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
  0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95,
  0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8,
  0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2,
  0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5,
  0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9,
  0xfa, 0x11, 0x00, 0x02, 0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05,
  0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01, 0x02, 0x03, 0x11, 0x04,
  0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22,
  0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33,
  0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25,
  0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36,
  0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
  0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
  0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94,
  0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
  0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
  0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
};

static int init_v4l2 (struct vdIn *vd);
static int init_v4l2_format (struct vdIn *vd);
static int video_disable (struct vdIn *vd);

int
init_videoIn (struct vdIn *vd, char *device)
{
  if (vd == NULL || device == NULL)
    return -1;
  vd->buf.index = -1;
  vd->videodevice = NULL;
  vd->status = NULL;
  vd->pictName = NULL;
  vd->videodevice = (char *) calloc (1, 16 * sizeof (char));
  vd->status = (char *) calloc (1, 100 * sizeof (char));
  vd->pictName = (char *) calloc (1, 80 * sizeof (char));
  snprintf (vd->videodevice, 12, "%s", device);
  vd->toggleAvi = 0;
  vd->getPict = 0;
  vd->signalquit = 1;
  vd->width = 0;
  vd->height = 0;
  vd->formatIn = 0;
  vd->grabmethod = 0;
  vd->formatSet = 0;

  if (init_v4l2 (vd) < 0) {
    fprintf (stderr, " Init v4L2 failed !! exit fatal \n");
    goto error;;
  }

   return 0;

error:
  free (vd->videodevice);
  free (vd->status);
  free (vd->pictName);
  close (vd->fd);
  return -1;
}

int
v4l2TryFormat (struct vdIn *vd, int width, int height,
	      int format)
{
  int ret = 0;

  if (vd == NULL)
    return -1;
  if (width == 0 || height == 0)
    return -1;

  /* set format in */
  memset (&vd->fmt, 0, sizeof (struct v4l2_format));
  vd->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vd->fmt.fmt.pix.width = width;
  vd->fmt.fmt.pix.height = height;
  vd->fmt.fmt.pix.pixelformat = format;
  vd->fmt.fmt.pix.field = V4L2_FIELD_ANY;

  ret = ioctl (vd->fd, VIDIOC_TRY_FMT, &vd->fmt);
  if (ret < 0) {
    fprintf (stderr, "Unable to try format: %d - %s\n", errno, strerror(errno));
    return ret;
  }
  if ((vd->fmt.fmt.pix.width != width) ||
      (vd->fmt.fmt.pix.height != height)) {
    fprintf (stderr, " format asked unavailable get width %d height %d \n",
	     vd->fmt.fmt.pix.width, vd->fmt.fmt.pix.height);
  }
  if (format != vd->fmt.fmt.pix.pixelformat) {
    fprintf (stderr, " pixelformat asked unavailable %d asked for %d \n",
	     vd->fmt.fmt.pix.pixelformat, format);
  }

  return 0;
}

int
v4l2SetFormat (struct vdIn *vd, int width, int height,
	      int format, int grabmethod)
{

  if (vd == NULL)
    return -1;
  if (width == 0 || height == 0)
    return -1;
  vd->buf.index = -1;
  if (grabmethod < 0 || grabmethod > 1)
    grabmethod = 1;		//mmap by default;
  vd->toggleAvi = 0;
  vd->getPict = 0;
  vd->signalquit = 1;
  vd->width = width;
  vd->height = height;
  vd->formatIn = format;
  vd->grabmethod = grabmethod;
  if (init_v4l2_format (vd) < 0) {
    fprintf (stderr, " Init v4L2 failed !! exit fatal \n");
    goto error;;
  }
  /* alloc a temp buffer to reconstruct the pict */
  vd->framesizeIn = (vd->width * vd->height << 1);
  switch (vd->formatIn) {
  case V4L2_PIX_FMT_MJPEG:
  case V4L2_PIX_FMT_JPEG:
    vd->tmpbuffer = (unsigned char *) calloc (1, (size_t) vd->framesizeIn);
    if (!vd->tmpbuffer)
      goto error;
    vd->fbCap = vd->width * (vd->height + 8) * 2;
    if (!vd->grabmethod)
      vd->framebuffer =
         (unsigned char *) calloc (1, (size_t) vd->fbCap);
    break;
  case V4L2_PIX_FMT_YUYV:
  case V4L2_PIX_FMT_YVYU:
  case V4L2_PIX_FMT_UYVY:
  case V4L2_PIX_FMT_VYUY:
    vd->fbCap = vd->framesizeIn;
    if (!vd->grabmethod)
      vd->framebuffer = (unsigned char *) calloc (1, (size_t) vd->fbCap);
    break;
  default:
    fprintf (stderr, " should never arrive exit fatal !!\n");
    goto error;
    break;
  }
  if (!vd->framebuffer && !vd->grabmethod) {
     fprintf(stderr, "failed to allocate space for framebuffer\n");
    goto error;
  }
  vd->formatSet = 1;
  return 0;
error:
  // free (vd->videodevice);
  // free (vd->status);
  // free (vd->pictName);
  // close (vd->fd);
  return -1;
}

int v4l2InputInfo(struct vdIn *vd, int index, struct v4l2_input *input)
{
   input->index = index;
   return ioctl (vd->fd, VIDIOC_ENUMINPUT, input);
}

int v4l2SetInput(struct vdIn *vd, const char *inpName)
{
   int i = 0;
   int res = 0;
   struct v4l2_input input;

   while (!res) {
      res = v4l2InputInfo(vd, i, &input);
      if (res)
         return -1;

      if (input.type == V4L2_INPUT_TYPE_CAMERA &&
            !strcmp(inpName, (char*)input.name)) {
         return v4l2SetInputNum(vd, i);
      }
      i++;
   }

   errno = EINVAL;
   return -1;
}

int v4l2SetInputNum(struct vdIn *vd, int index)
{
   int res;
   int i;

   if (vd->isstreaming) {
      video_disable(vd);

      res = fcntl(vd->fd, F_GETFL);
      if (res < 0) {
         perror("fnctl GETFL1");
         return -1;
      }

      res = fcntl(vd->fd, F_SETFL, res | O_NONBLOCK);
      if (res < 0) {
         perror("fnctl SETFL1");
         return -1;
      }

      for (i = 0; i < 10000; i++) {
         vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
         vd->buf.memory = V4L2_MEMORY_MMAP;
         res = ioctl (vd->fd, VIDIOC_DQBUF, &vd->buf);
         if (res < 0) {
            if (errno == EAGAIN)
               break;

            perror("DQBUF InputNum 1");
            fprintf (stderr, "Unable to dequeue buffer (%d) - %s\n", errno, strerror(errno));
            return -1;
         }
      }

      res = fcntl(vd->fd, F_GETFL);
      if (res < 0) {
         perror("fnctl GETFL2");
         return -1;
      }

      res = fcntl(vd->fd, F_SETFL, res & (~O_NONBLOCK));
      if (res < 0) {
         perror("fnctl SETFL2");
         return -1;
      }

      for (i = 0; i < vd->rb.count; ++i) {
      memset (&vd->buf, 0, sizeof (struct v4l2_buffer));
      vd->buf.index = i;
      vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      vd->buf.memory = V4L2_MEMORY_MMAP;
      res = ioctl (vd->fd, VIDIOC_QBUF, &vd->buf);
      if (res < 0) {
         fprintf (stderr, "Unable to queue buffer (%d) - %s\n", errno, strerror(errno));
         return -1;
         }
      }

      vd->buf.index = -1;
   }

   res = ioctl (vd->fd, VIDIOC_S_INPUT, &index);
   if (res)
      return res;

   if (vd->formatSet) {
      // vd->width = width;
      // vd->height = height;
      // vd->formatIn = format;

      /* set format in */
      memset (&vd->fmt, 0, sizeof (struct v4l2_format));
      vd->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      vd->fmt.fmt.pix.width = vd->width;
      vd->fmt.fmt.pix.height = vd->height;
      vd->fmt.fmt.pix.pixelformat = vd->formatIn;
      vd->fmt.fmt.pix.field = V4L2_FIELD_ANY;
      res = ioctl (vd->fd, VIDIOC_S_FMT, &vd->fmt);
      if (res < 0) {
         fprintf (stderr, "Unable to reset format: %d - %s\n", errno, strerror(errno));
         return res;
      }
      if ((vd->fmt.fmt.pix.width != vd->width) ||
            (vd->fmt.fmt.pix.height != vd->height)) {
         fprintf (stderr, " format asked unavailable get width %d height %d when changing cameras\n",
	         vd->fmt.fmt.pix.width, vd->fmt.fmt.pix.height);
         vd->width = vd->fmt.fmt.pix.width;
         vd->height = vd->fmt.fmt.pix.height;
      }
      if (vd->formatIn != vd->fmt.fmt.pix.pixelformat) {
         fprintf (stderr, " pixelformat asked unavailable %d asked for %d when changing cameras\n",
	         vd->fmt.fmt.pix.pixelformat, vd->formatIn);
         vd->formatIn = vd->fmt.fmt.pix.pixelformat;
      }
  }

   return 0;
}

int v4l2GetInput(struct vdIn *vd)
{
   int i = 0;
   int res = 0;

   res = ioctl (vd->fd, VIDIOC_G_INPUT, &i);
   if (res)
      return res;

   return i;
}

int v4l2NextInput(struct vdIn *vd)
{
   int i = 0;
   int res = 0;

   res = ioctl (vd->fd, VIDIOC_G_INPUT, &i);
   if (res)
      return res;

   i++;
   res = v4l2SetInputNum (vd, i);
   if (res) {
      if (errno == EINVAL) {
         i = 0;
         return v4l2SetInputNum (vd, i);
      }

      return res;
   }

   return 0;
}

static int
init_v4l2 (struct vdIn *vd)
{
  int ret = 0;

  if ((vd->fd = open (vd->videodevice, O_RDWR)) == -1) {
    perror ("ERROR opening V4L interface");
    exit (1);
  }
  memset (&vd->cap, 0, sizeof (struct v4l2_capability));
  ret = ioctl (vd->fd, VIDIOC_QUERYCAP, &vd->cap);
  if (ret < 0) {
    fprintf (stderr, "Error opening device %s: unable to query device.\n",
	     vd->videodevice);
    return -1;
  }

  if ((vd->cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
    fprintf (stderr,
	     "Error opening device %s: video capture not supported.\n",
	     vd->videodevice);
    return -1;
  }
  if (vd->grabmethod) {
    if (!(vd->cap.capabilities & V4L2_CAP_STREAMING)) {
      fprintf (stderr, "%s does not support streaming i/o\n",
	       vd->videodevice);
      return -1;
    }
  } else {
    if (!(vd->cap.capabilities & V4L2_CAP_READWRITE)) {
      fprintf (stderr, "%s does not support read i/o\n", vd->videodevice);
      return -1;
    }
  }

  return 0;
}

static int
init_v4l2_format (struct vdIn *vd)
{
  int i;
  int ret = 0;

  /* set format in */
  memset (&vd->fmt, 0, sizeof (struct v4l2_format));
  vd->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vd->fmt.fmt.pix.width = vd->width;
  vd->fmt.fmt.pix.height = vd->height;
  vd->fmt.fmt.pix.pixelformat = vd->formatIn;
  vd->fmt.fmt.pix.field = V4L2_FIELD_ANY;

  if (vd->width == -1 || vd->height == -1 || vd->formatIn == -1) {
   ret = ioctl (vd->fd, VIDIOC_G_FMT, &vd->fmt);
   if (ret < 0) {
      fprintf (stderr, "Unable to get format: %d - %s\n", errno, strerror(errno));
      goto fatal;
   }
  }
  else {
   ret = ioctl (vd->fd, VIDIOC_S_FMT, &vd->fmt);
   if (ret < 0) {
      fprintf (stderr, "Unable to set format: %d - %s\n", errno, strerror(errno));
      goto fatal;
   }
  }
  if ((vd->fmt.fmt.pix.width != vd->width) ||
      (vd->fmt.fmt.pix.height != vd->height)) {
    fprintf (stderr, " format asked unavailable get width %d height %d \n",
	     vd->fmt.fmt.pix.width, vd->fmt.fmt.pix.height);
    vd->width = vd->fmt.fmt.pix.width;
    vd->height = vd->fmt.fmt.pix.height;
    /* look the format is not part of the deal ??? */
    //vd->formatIn = vd->fmt.fmt.pix.pixelformat;
  }
  if (vd->formatIn != vd->fmt.fmt.pix.pixelformat) {
    fprintf (stderr, " pixelformat asked unavailable %d asked for %d \n",
	     vd->fmt.fmt.pix.pixelformat, vd->formatIn);
    vd->formatIn = vd->fmt.fmt.pix.pixelformat;
  }

  if (vd->grabmethod) {
  /* request buffers */
  memset (&vd->rb, 0, sizeof (struct v4l2_requestbuffers));
  vd->rb.count = NB_BUFFER;
  // vd->rb.count = 1;
  vd->rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vd->rb.memory = V4L2_MEMORY_MMAP;

  ret = ioctl (vd->fd, VIDIOC_REQBUFS, &vd->rb);
  if (ret < 0) {
    fprintf (stderr, "Unable to allocate buffers: %d - %s\n", errno, strerror(errno));
    goto fatal;
  }
  /* map the buffers */
  for (i = 0; i < vd->rb.count; i++) {
    memset (&vd->buf, 0, sizeof (struct v4l2_buffer));
    vd->buf.index = i;
    vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl (vd->fd, VIDIOC_QUERYBUF, &vd->buf);
    if (ret < 0) {
      fprintf (stderr, "Unable to query buffer (%d) - %s\n", errno, strerror(errno));
      goto fatal;
    }
    if (debug)
      fprintf (stderr, "length: %u offset: %u\n", vd->buf.length,
	       vd->buf.m.offset);

      fprintf (stderr, "mapping %d length: %u offset: %u\n", i, vd->buf.length,
	       vd->buf.m.offset);
    vd->mem[i] = mmap (0 /* start anywhere */ ,
		       vd->buf.length, PROT_READ, MAP_SHARED, vd->fd,
		       vd->buf.m.offset);
    if (vd->mem[i] == MAP_FAILED) {
      fprintf (stderr, "Unable to map buffer (%d) - %s\n", errno, strerror(errno));
      goto fatal;
    }
    if (debug)
      fprintf (stderr, "Buffer mapped at address %p.\n", vd->mem[i]);
  }
  /* Queue the buffers. */
  for (i = 0; i < vd->rb.count; ++i) {
    memset (&vd->buf, 0, sizeof (struct v4l2_buffer));
    vd->buf.index = i;
    vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vd->buf.memory = V4L2_MEMORY_MMAP;
    ret = ioctl (vd->fd, VIDIOC_QBUF, &vd->buf);
    if (ret < 0) {
      fprintf (stderr, "Unable to queue buffer (%d) - %s\n", errno, strerror(errno));
      goto fatal;
    }
  }
  memset (&vd->buf, 0, sizeof (struct v4l2_buffer));
  vd->buf.index = -1;
  }
  else { // !grabmethod
  }
  return 0;
fatal:
  return -1;

}

static int
video_enable (struct vdIn *vd)
{
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int ret;

  ret = ioctl (vd->fd, VIDIOC_STREAMON, &type);
  if (ret < 0) {
    fprintf (stderr, "Unable to %s capture: %d- %s\n", "start", errno, strerror(errno));
    return ret;
  }
  vd->isstreaming = 1;
  return 0;
}

static int
video_disable (struct vdIn *vd)
{
  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int ret;

  ret = ioctl (vd->fd, VIDIOC_STREAMOFF, &type);
  if (ret < 0) {
    fprintf (stderr, "Unable to %s capture: %d - %s\n", "stop", errno, strerror(errno));
    return ret;
  }
  vd->isstreaming = 0;
  return 0;
}

int
uvcGrabRead (struct vdIn *vd)
{
   int readLen;

  if (vd->formatIn != V4L2_PIX_FMT_YUYV &&
      vd->formatIn != V4L2_PIX_FMT_YVYU &&
      vd->formatIn != V4L2_PIX_FMT_UYVY &&
      vd->formatIn != V4L2_PIX_FMT_VYUY &&
      vd->formatIn != V4L2_PIX_FMT_JPEG &&
      vd->formatIn != V4L2_PIX_FMT_MJPEG)
     goto err;

  printf("Reading %d bytes!\n", vd->fbCap);
  readLen = read(vd->fd, vd->framebuffer, vd->fbCap);
  /* if (vd->fbCap != readLen) {
    fprintf (stderr, "Unable to read entire image into buffer (%d) - %s\n", errno, strerror(errno));
     goto err;
  } */
  vd->buf.bytesused = readLen;

  printf("Read %d bytes!\n", readLen);
  return 0;

err:
  vd->signalquit = 0;
  return -1;
}

int
uvcGrab (struct vdIn *vd)
{
#define HEADERFRAME1 0xaf
  int ret;

  if (!vd->grabmethod)
     return uvcGrabRead(vd);

  if (vd->buf.index != -1) {
     vd->framebuffer = NULL;
     // printf("queue buff %d\n", vd->buf.index);
     vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
     vd->buf.memory = V4L2_MEMORY_MMAP;
   ret = ioctl (vd->fd, VIDIOC_QBUF, &vd->buf);
   if (ret < 0) {
      fprintf (stderr, "Unable to requeue buffer (%d) - %s\n", errno, strerror(errno));
      // goto err;
      ret = ioctl (vd->fd, VIDIOC_QBUF, &vd->buf);
      if (ret < 0) {
         fprintf (stderr, "Unable to requeue buffer (%d) - %s\n", errno, strerror(errno));
         goto err;
      }
   }
  }

  if (!vd->isstreaming) {
    if (video_enable (vd))
      goto err;
  }

  memset (&vd->buf, 0, sizeof (struct v4l2_buffer));
  vd->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  vd->buf.memory = V4L2_MEMORY_MMAP;
  ret = ioctl (vd->fd, VIDIOC_DQBUF, &vd->buf);
  if (ret < 0) {
    fprintf (stderr, "Unable to dequeue buffer (%d) - %s\n", errno, strerror(errno));
    goto err;
  }

  // Special case where the video device turns off for a single buffer to prevent
  // shared memory corruption issues
  if (vd->rb.count == 1)
      video_disable (vd);

  switch (vd->formatIn) {
  case V4L2_PIX_FMT_MJPEG:
  case V4L2_PIX_FMT_JPEG:

     /*
    memcpy (vd->tmpbuffer, vd->mem[vd->buf.index], HEADERFRAME1);
    memcpy (vd->tmpbuffer + HEADERFRAME1, dht_data, DHT_SIZE);
    memcpy (vd->tmpbuffer + HEADERFRAME1 + DHT_SIZE,
	    vd->mem[vd->buf.index] + HEADERFRAME1,
	    (vd->buf.bytesused - HEADERFRAME1));
       */
    if (debug)
      fprintf (stderr, "bytes in used %d \n", vd->buf.bytesused);
    vd->framebuffer = vd->mem[vd->buf.index];
    break;

  case V4L2_PIX_FMT_YUYV:
  case V4L2_PIX_FMT_YVYU:
  case V4L2_PIX_FMT_UYVY:
  case V4L2_PIX_FMT_VYUY:
    vd->framebuffer = vd->mem[vd->buf.index];

    /* if (vd->buf.bytesused > vd->framesizeIn)
      memcpy (vd->framebuffer, vd->mem[vd->buf.index],
	      (size_t) vd->framesizeIn);
    else
      memcpy (vd->framebuffer, vd->mem[vd->buf.index],
	      (size_t) vd->buf.bytesused); */
    break;
  default:
    goto err;
    break;
  }

  return 0;
err:
  vd->signalquit = 0;
  return -1;
}

int
close_v4l2 (struct vdIn *vd)
{
  int i;

  if (vd->buf.index != -1) {
     vd->framebuffer = NULL;
     ioctl (vd->fd, VIDIOC_QBUF, &vd->buf);
  }

  if (vd->isstreaming)
    video_disable (vd);

  /* If the memory maps are not released the device will remain opened even
     after a call to close(); */
  for (i = 0; i < vd->rb.count; i++) {
     if (vd->mem[i])
        munmap (vd->mem[i], vd->buf.length);
  }

  if (vd->tmpbuffer)
    free (vd->tmpbuffer);
  vd->tmpbuffer = NULL;
  if (vd->framebuffer)
   free (vd->framebuffer);
  vd->framebuffer = NULL;
  free (vd->videodevice);
  free (vd->status);
  free (vd->pictName);
  vd->videodevice = NULL;
  vd->status = NULL;
  vd->pictName = NULL;
  close (vd->fd);
  return 0;
}

/* return >= 0 ok otherwhise -1 */
static int
isv4l2Control (struct vdIn *vd, int control, struct v4l2_queryctrl *queryctrl)
{
  int err = 0;

  queryctrl->id = control;
  if ((err = ioctl (vd->fd, VIDIOC_QUERYCTRL, queryctrl)) < 0) {
    fprintf (stderr, "ioctl querycontrol %d error %d  - %s\n", queryctrl->id, errno, strerror(errno));
  } else if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED) {
    fprintf (stderr, "control %s disabled \n", (char *) queryctrl->name);
  } else if (queryctrl->flags & V4L2_CTRL_TYPE_BOOLEAN) {
    return 1;
  } else if (queryctrl->type & V4L2_CTRL_TYPE_INTEGER) {
    return 0;
  } else {
    fprintf (stderr, "contol %s (%d) unsupported  \n", (char *) queryctrl->name, queryctrl->id);
  }
  return -1;
}

int
v4l2GetControl (struct vdIn *vd, int control)
{
  struct v4l2_queryctrl queryctrl;
  struct v4l2_control control_s;
  int err;

  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;
  control_s.id = control;
  if ((err = ioctl (vd->fd, VIDIOC_G_CTRL, &control_s)) < 0) {
     if (errno != EAGAIN)
      fprintf (stderr, "ioctl get control (%d) error: %s\n", control, strerror(errno));
    return -errno;
  }
  return control_s.value;
}

int
v4l2SetControl (struct vdIn *vd, int control, int value)
{
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  int min, max, step, val_def;
  int err;

  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;
  min = queryctrl.minimum;
  max = queryctrl.maximum;
  step = queryctrl.step;
  val_def = queryctrl.default_value;
  if ((value >= min) && (value <= max)) {
    control_s.id = control;
    control_s.value = value;
    if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
      fprintf (stderr, "ioctl set control error\n");
      return -1;
    }
  }
  return 0;
}

int
v4l2UpControl (struct vdIn *vd, int control)
{
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  int min, max, current, step, val_def;
  int err;

  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;
  min = queryctrl.minimum;
  max = queryctrl.maximum;
  step = queryctrl.step;
  val_def = queryctrl.default_value;
  current = v4l2GetControl (vd, control);
  current += step;
  if (current <= max) {
    control_s.id = control;
    control_s.value = current;
    if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
      fprintf (stderr, "ioctl set control error\n");
      return -1;
    }
  }
  return control_s.value;
}

int
v4l2DownControl (struct vdIn *vd, int control)
{
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  int min, max, current, step, val_def;
  int err;

  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;
  min = queryctrl.minimum;
  max = queryctrl.maximum;
  step = queryctrl.step;
  val_def = queryctrl.default_value;
  current = v4l2GetControl (vd, control);
  current -= step;
  if (current >= min) {
    control_s.id = control;
    control_s.value = current;
    if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
      fprintf (stderr, "ioctl set control error\n");
      return -1;
    }
  }
  return control_s.value;
}

int
v4l2ToggleControl (struct vdIn *vd, int control)
{
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  int current;
  int err;

  if (isv4l2Control (vd, control, &queryctrl) != 1)
    return -1;
  current = v4l2GetControl (vd, control);
  control_s.id = control;
  control_s.value = !current;
  if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
    fprintf (stderr, "ioctl toggle control error\n");
    return -1;
  }
  return control_s.value;
}

int
v4l2ResetControl (struct vdIn *vd, int control)
{
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  int val_def;
  int err;

  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;
  val_def = queryctrl.default_value;
  control_s.id = control;
  control_s.value = val_def;
  if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
    fprintf (stderr, "ioctl reset control error\n");
    return -1;
  }

  return 0;
}

int
v4l2ResetPanTilt (struct vdIn *vd, int pantilt)
{
  int control = V4L2_CID_PANTILT_RESET;
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  unsigned char val;
  int err;

  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;
  val = (unsigned char) pantilt;
  control_s.id = control;
  control_s.value = val;
  if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
    fprintf (stderr, "ioctl reset Pan control error\n");
    return -1;
  }

  return 0;
}
union pantilt {
  struct {
    short pan;
    short tilt;
  } s16;
  int value;
} pantilt;

int
v4L2UpDownPan (struct vdIn *vd, short inc)
{
  int control = V4L2_CID_PANTILT_RELATIVE;
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  int err;

  union pantilt pan;

  control_s.id = control;
  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;

  pan.s16.pan = inc;
  pan.s16.tilt = 0;

  control_s.value = pan.value;
  if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
    fprintf (stderr, "ioctl pan updown control error\n");
    return -1;
  }
  return 0;
}

int
v4L2UpDownTilt (struct vdIn *vd, short inc)
{
  int control = V4L2_CID_PANTILT_RELATIVE;
  struct v4l2_control control_s;
  struct v4l2_queryctrl queryctrl;
  int err;
  union pantilt pan;

  control_s.id = control;
  if (isv4l2Control (vd, control, &queryctrl) < 0)
    return -1;

  pan.s16.pan = 0;
  pan.s16.tilt = inc;

  control_s.value = pan.value;
  if ((err = ioctl (vd->fd, VIDIOC_S_CTRL, &control_s)) < 0) {
    fprintf (stderr, "ioctl tiltupdown control error\n");
    return -1;
  }
  return 0;
}
