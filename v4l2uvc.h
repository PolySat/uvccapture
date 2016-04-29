
/*******************************************************************************
#             uvccapture: USB UVC Video Class Snapshot Software                #
#This package work with the Logitech UVC based webcams with the mjpeg feature  #
#                                                                              #
#       Orginally Copyright (C) 2005 2006 Laurent Pinchart &&  Michel Xhaard   #
#       Modifications Copyright (C) 2006 Gabriel A. Devenyi                    #
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

#define NB_BUFFER 16
#define DHT_SIZE 420

#ifndef V4L2_CID_BACKLIGHT_COMPENSATION
#define V4L2_CID_BACKLIGHT_COMPENSATION	(V4L2_CID_PRIVATE_BASE+0)
#endif 

#ifndef V4L2_CID_POWER_LINE_FREQUENCY
#define V4L2_CID_POWER_LINE_FREQUENCY	(V4L2_CID_PRIVATE_BASE+1)
#endif

#ifndef V4L2_CID_SHARPNESS
#define V4L2_CID_SHARPNESS		(V4L2_CID_PRIVATE_BASE+2)
#endif

#ifndef V4L2_CID_HUE_AUTO
#define V4L2_CID_HUE_AUTO		(V4L2_CID_PRIVATE_BASE+3)
#endif

#ifndef V4L2_CID_FOCUS_AUTO
#define V4L2_CID_FOCUS_AUTO		(V4L2_CID_PRIVATE_BASE+4)
#endif

#ifndef V4L2_CID_FOCUS_ABSOLUTE
#define V4L2_CID_FOCUS_ABSOLUTE		(V4L2_CID_PRIVATE_BASE+5)
#endif

#ifndef V4L2_CID_FOCUS_RELATIVE
#define V4L2_CID_FOCUS_RELATIVE		(V4L2_CID_PRIVATE_BASE+6)
#endif

#define V4L2_CID_PANTILT_RELATIVE	(V4L2_CID_PRIVATE_BASE+7)
#define V4L2_CID_PANTILT_RESET		(V4L2_CID_PRIVATE_BASE+8)

struct vdIn {
  int fd;
  char *videodevice;
  char *status;
  char *pictName;
  struct v4l2_capability cap;
  struct v4l2_format fmt;
  struct v4l2_buffer buf;
  struct v4l2_requestbuffers rb;
  void *mem[NB_BUFFER];
  unsigned char *tmpbuffer;
  unsigned char *framebuffer;
  int fbCap;
  int isstreaming;
  int grabmethod;
  int width;
  int height;
  int formatIn;
  int formatOut;
  int framesizeIn;
  int signalquit;
  int toggleAvi;
  int getPict;
  int nr_buffer;
  int formatSet;
};

int init_videoIn (struct vdIn *vd, char *device);
int uvcGrab (struct vdIn *vd);
int close_v4l2 (struct vdIn *vd);

int v4l2SetFormat (struct vdIn *vd, int width, int height, int format,
      int grabmethod);
int v4l2TryFormat (struct vdIn *vd, int width, int height, int format);
int v4l2GetControl (struct vdIn *vd, int control);
int v4l2SetControl (struct vdIn *vd, int control, int value);
int v4l2UpControl (struct vdIn *vd, int control);
int v4l2DownControl (struct vdIn *vd, int control);
int v4l2ToggleControl (struct vdIn *vd, int control);
int v4l2ResetControl (struct vdIn *vd, int control);
int v4l2ResetPanTilt (struct vdIn *vd, int pantilt);
int v4L2UpDownPan (struct vdIn *vd, short inc);
int v4L2UpDownTilt (struct vdIn *vd, short inc);
int v4l2InputInfo(struct vdIn *vd, int index, struct v4l2_input *input);
int v4l2SetInput(struct vdIn *vd, const char *inpName);
int v4l2SetInputNum(struct vdIn *vd, int num);
int v4l2NextInput(struct vdIn *vd);
int v4l2GetInput(struct vdIn *vd);
