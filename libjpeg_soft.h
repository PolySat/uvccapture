#ifndef LIBJPEG_SOFT_H
#define LIBJPEG_SOFT_H

#include <stdio.h>
#include <jpeglib.h>

#if 0

#define NO_JPEG_IND

#define libjpeg_avail(x) 1

#define jpeg_CreateCompress_ind(x,y,z) jpeg_CreateCompres(x,y,z)
#define jpeg_stdio_dest_ind(x,y) jpeg_stdio_dest(x,y)
#define jpeg_set_defaults_ind(x) jpeg_set_defaults(x)
#define jpeg_set_quality_ind(x,y,z) jpeg_set_quality(x,y,z)
#define jpeg_start_compress_ind(x,y) jpeg_start_compress(x,y)

#define jpeg_write_scanlines_ind(x,y,z) jpeg_write_scanlines(x,y,z)
#define jpeg_finish_compress_ind(x) jpeg_finish_compress(x)
#define jpeg_destroy_compress_ind(x) jpeg_destroy_compress(x)
#define jpeg_std_error_ind(x) jpeg_std_error(x)
#define jpeg_create_compress_ind(x) jpeg_create_compress(x)

#else

int libjpeg_avail(void);

extern struct jpeg_error_mgr *(*jpeg_std_error_ind)(struct jpeg_error_mgr *err);
#define jpeg_create_compress_ind(cinfo) \
    (*jpeg_CreateCompress_ind)((cinfo), JPEG_LIB_VERSION, \
             (size_t) sizeof(struct jpeg_compress_struct))

extern void (*jpeg_CreateCompress_ind)(j_compress_ptr cinfo, int version, size_t structsize);
// #define jpeg_stdio_dest_ind(x,y) jpeg_stdio_dest(x,y)
extern void (*jpeg_stdio_dest_ind)(j_compress_ptr cinfo, FILE * outfile);
// #define jpeg_set_defaults_ind(x) jpeg_set_defaults(x)
extern void (*jpeg_set_defaults_ind)(j_compress_ptr cinfo);
// #define jpeg_set_quality_ind(x,y,z) jpeg_set_quality(x,y,z)
extern void (*jpeg_set_quality_ind)(j_compress_ptr cinfo, int quality,
               boolean force_baseline);
// #define jpeg_start_compress_ind(x,y) jpeg_start_compress(x,y)
extern void (*jpeg_start_compress_ind)(j_compress_ptr cinfo,
                  boolean write_all_tables);
extern void (*jpeg_write_scanlines_ind)(j_compress_ptr cinfo,
                    JSAMPARRAY scanlines, JDIMENSION num_lines);
extern void (*jpeg_finish_compress_ind)(j_compress_ptr cinfo);
extern void (*jpeg_destroy_compress_ind)(j_compress_ptr cinfo);

#endif

#endif
