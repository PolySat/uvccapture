#include "libjpeg_soft.h"
#include <dlfcn.h>

#ifndef NO_JPEG_IND

static void *handle = NULL;


struct jpeg_error_mgr *(*jpeg_std_error_ind)(struct jpeg_error_mgr *err) = NULL;
void (*jpeg_CreateCompress_ind)(j_compress_ptr cinfo, int version, size_t structsize) = NULL;
void (*jpeg_stdio_dest_ind)(j_compress_ptr cinfo, FILE * outfile) = NULL;
void (*jpeg_set_defaults_ind)(j_compress_ptr cinfo) = NULL;
void (*jpeg_set_quality_ind)(j_compress_ptr cinfo, int quality,
               boolean force_baseline) = NULL;
void (*jpeg_start_compress_ind)(j_compress_ptr cinfo,
                  boolean write_all_tables) = NULL;
void (*jpeg_write_scanlines_ind)(j_compress_ptr cinfo,
                    JSAMPARRAY scanlines, JDIMENSION num_lines) = NULL;
void (*jpeg_finish_compress_ind)(j_compress_ptr cinfo) = NULL;
void (*jpeg_destroy_compress_ind)(j_compress_ptr cinfo) = NULL;

int libjpeg_avail(void)
{
   if (handle)
      return 1;

   handle = dlopen("libjpeg.so", RTLD_LAZY);
   if (!handle)
      return 0;

   jpeg_std_error_ind = dlsym(handle, "jpeg_std_error");
   if (!jpeg_std_error_ind)
      return 0;

   jpeg_CreateCompress_ind = dlsym(handle, "jpeg_CreateCompress");
   if (!jpeg_CreateCompress_ind)
      return 0;

   jpeg_stdio_dest_ind = dlsym(handle, "jpeg_stdio_dest");
   if (!jpeg_stdio_dest_ind)
      return 0;

   jpeg_set_defaults_ind = dlsym(handle, "jpeg_set_defaults");
   if (!jpeg_set_defaults_ind)
      return 0;

   jpeg_set_quality_ind = dlsym(handle, "jpeg_set_quality");
   if (!jpeg_set_quality_ind)
      return 0;

   jpeg_start_compress_ind = dlsym(handle, "jpeg_start_compress");
   if (!jpeg_start_compress_ind)
      return 0;

   jpeg_write_scanlines_ind = dlsym(handle, "jpeg_write_scanlines");
   if (!jpeg_write_scanlines_ind)
      return 0;

   jpeg_finish_compress_ind = dlsym(handle, "jpeg_finish_compress");
   if (!jpeg_finish_compress_ind)
      return 0;

   jpeg_destroy_compress_ind = dlsym(handle, "jpeg_destroy_compress");
   if (!jpeg_destroy_compress_ind)
      return 0;

   return 1;
};

#endif
