/* 
* ======================================================================================
* File:        save_frames_as_png.c
*
* Purpose:     saves image frames as PNG files
* 
* History:     1. 2016-10-17   JBendor     Created
*              2. 2016-11-24   JBendor     Updated
*
* Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
*               Unauthorized copying of this file is strictly prohibited.
* ======================================================================================
*/

#include "save_frames_as_png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>


//=======================================================================================
// synopsis: pointer = do_get_RGB24_pixel_ptr_at(aPixmapPtr, aCol, aRow)
//
// returns a pointer to the RGB24 pixel at the point (aCol, aRow)
//=======================================================================================
static RGB24_Pix_t * do_get_RGB24_pixel_ptr_at (PixmapInfo_t * aPixmapPtr, int aCol, int aRow)
{
    RGB24_Pix_t   * pixels_ptr = (RGB24_Pix_t *) aPixmapPtr->pixels;

    uint8_t * ptr_col_in_row_0 = (uint8_t *) &pixels_ptr[aCol];

    return (RGB24_Pix_t *) (ptr_col_in_row_0 + (aPixmapPtr->stride * aRow));
}


//=======================================================================================
// synopsis: pointer = do_get_RGB32_pixel_ptr_at(aPixmapPtr, aCol, aRow)
//
// returns a pointer to the RGB32 pixel at the point (aCol, aRow)
//=======================================================================================
static RGB32_Pix_t * do_get_RGB32_pixel_ptr_at (PixmapInfo_t * aPixmapPtr, int aCol, int aRow)
{
    RGB32_Pix_t   * pixels_ptr = (RGB32_Pix_t *) aPixmapPtr->pixels;

    uint8_t * ptr_col_in_row_0 = (uint8_t *) &pixels_ptr[aCol];

    return (RGB32_Pix_t *) (ptr_col_in_row_0 + (aPixmapPtr->stride * aRow));
}


//=======================================================================================
// synopsis: result = do_save_RGB_frame_to_PNG_file (aPixmapPtr, file_path)
//
// Write bitmap to a PNG file specified by path; returns 0 if OK, else error
//=======================================================================================
static int do_save_RGB_frame_to_PNG_file (PixmapInfo_t * aPixmapPtr, const char * file_path)
{
#ifndef _NOT_USING_PNG_LIBRARY_

    png_uint_32 row_idx = 0;

    png_uint_32 is_RGBA = (aPixmapPtr->depth == 32);

    png_structp png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) 
    {
        return -1;
    }

    png_infop info_ptr = png_create_info_struct (png_ptr);
    if (info_ptr == NULL) 
    {
        png_destroy_write_struct (&png_ptr, &info_ptr);
        return -2;
    }

    if (setjmp(png_jmpbuf(png_ptr)) != 0) 
    {
        png_destroy_write_struct (&png_ptr, &info_ptr);
        return -3;
    }

    FILE * fp = fopen (file_path, "wb");
    if (! fp) 
    {
        png_destroy_write_struct (&png_ptr, &info_ptr);
        return -4;
    }

    png_set_IHDR (png_ptr,
                  info_ptr,
                  aPixmapPtr->width,
                  aPixmapPtr->height,
                  NUM_SAMPLE_BITS_R_G_B,
                  (is_RGBA ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB),
                  PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);

    png_uint_32 length = (png_uint_32) ((aPixmapPtr->height + 1) * sizeof(png_bytep));

    png_bytep* row_pointers = png_malloc (png_ptr, length);

    if (is_RGBA)
    {
        for (row_idx = 0;  row_idx < aPixmapPtr->height;  ++row_idx)
        {
            RGB32_Pix_t * row_ptr = do_get_RGB32_pixel_ptr_at(aPixmapPtr, 0, row_idx);

            row_pointers[row_idx] = (png_bytep) row_ptr;
        }
    }
    else
    {
        for (row_idx = 0;  row_idx < aPixmapPtr->height;  ++row_idx)
        {
            RGB24_Pix_t * row_ptr = do_get_RGB24_pixel_ptr_at(aPixmapPtr, 0, row_idx);

            row_pointers[row_idx] = (png_bytep) row_ptr;
        }
    }

    png_init_io (png_ptr, fp);

    png_set_rows (png_ptr, info_ptr, row_pointers);

    png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    png_free (png_ptr, row_pointers);

    fclose (fp);

#else

    do_get_RGB24_pixel_ptr_at(aPixmapPtr, 0, 0);    // supress "unused static" warning
    do_get_RGB32_pixel_ptr_at(aPixmapPtr, 0, 0);    // supress "unused static" warning

#endif

    return 0;
}


//=======================================================================================
// synopsis: result = do_save_RGB24_frame(aPathPtr, aPixmapPtr)
//
// Writes bitmap to a PNG file specified by path; returns 0 if OK, else error
//=======================================================================================
static int do_save_RGB24_frame(const char * aPathPtr, PixmapInfo_t * aPixmapPtr)
{
    void* pixs_ptr = aPixmapPtr->pixels;

    int frame_cols = aPixmapPtr->width;

    int frame_rows = aPixmapPtr->height;

    int row_stride = aPixmapPtr->stride;

    int min_stride = sizeof(RGB24_Pix_t) * frame_cols;

    if ( (frame_cols < 9) || (frame_rows < 9) || (row_stride < min_stride) )
    {
        return -1;
    }

    if (pixs_ptr == NULL)     // create default pattern
    {
        RGB24_Pix_t * pixels_ptr = calloc(row_stride * frame_rows, 1);

        int offset = 0, col_idx = 0, row_idx = -1;

        while ( ++row_idx < frame_rows )
        {
            uint8_t  red = ((row_idx & 0x3) == 1) ? 255 : 0;
            uint8_t  grn = ((row_idx & 0x3) == 2) ? 255 : 0;
            uint8_t  blu = ((row_idx & 0x3) == 3) ? 255 : 0;

            for ( col_idx = -1; ++col_idx < frame_cols; ++offset )
            {
                int group = col_idx % 10;
                pixels_ptr[offset].red   = (group ? red : 127);
                pixels_ptr[offset].green = (group ? grn : 127);
                pixels_ptr[offset].blue  = (group ? blu : 127);
            }
        }

        aPixmapPtr->pixels = pixels_ptr;

        int result = do_save_RGB24_frame(aPathPtr, aPixmapPtr);

        free(pixels_ptr);

        return result;
    }

    int errors = do_save_RGB_frame_to_PNG_file ( aPixmapPtr, aPathPtr );

    return errors;  // 0 is OK, else error
}


//=======================================================================================
// synopsis: result = do_save_RGB32_frame(aPathPtr, aPixmapPtr)
//
// Writes bitmap to a PNG file specified by path; returns 0 if OK, else error
//=======================================================================================
static int do_save_RGB32_frame(const char * aPathPtr, PixmapInfo_t * aPixmapPtr)
{
    void* pixs_ptr = aPixmapPtr->pixels;

    int frame_cols = aPixmapPtr->width;

    int frame_rows = aPixmapPtr->height;

    int row_stride = aPixmapPtr->stride;

    int min_stride = sizeof(RGB32_Pix_t) * frame_cols;

    if ( (frame_cols < 9) || (frame_rows < 9) || (row_stride < min_stride) )
    {
        return -1;
    }

    if (pixs_ptr == NULL)     // create default pattern
    {
        RGB32_Pix_t * pixels_ptr = calloc(row_stride * frame_rows, 1);

        int offset = 0, col_idx = 0, row_idx = -1;

        while ( ++row_idx < frame_rows )
        {
            uint8_t  red = ((row_idx & 0x3) == 1) ? 255 : 0;
            uint8_t  grn = ((row_idx & 0x3) == 2) ? 255 : 0;
            uint8_t  blu = ((row_idx & 0x3) == 3) ? 255 : 0;

            for ( col_idx = -1;  ++col_idx < frame_cols; ++offset )
            {
                int group = col_idx % 10;
                pixels_ptr[offset].red   = (group ? red : 127);
                pixels_ptr[offset].green = (group ? grn : 127);
                pixels_ptr[offset].blue  = (group ? blu : 127);
                pixels_ptr[offset].alpha = 0x00;
            }
        }

        aPixmapPtr->pixels = pixels_ptr;

        int result = do_save_RGB32_frame(aPathPtr, aPixmapPtr);

        free(pixels_ptr);

        return result;
    }

    int errors = do_save_RGB_frame_to_PNG_file ( aPixmapPtr, aPathPtr );

    return errors;  // 0 is OK, else error
}


/*
* =============================================================================
* YUV encodings have Y,U=Cb,V=Cr samples stored in packed or planar formats. 
* YUV values can have the range [0,255] or limited to [16,235] & [16,240].
* In Packed formats byte-samples are packed into one array of macropixels.
* In Planar formats Y,U,V are in consecutive arrays (U,V are sub-sampled).
* More details are provided at "http://www.fourcc.org/yuv.php".
* 
* ITUR_Convert_YUV_to_RGB(int Y, int U, int V)
*      int32_t Cb = U - 128;
*      int32_t Cr = V - 128;
*      int32_t R  = Y + (1.4065 * Cb);
*      int32_t B  = Y + (1.7790 * Cr);  
*      int32_t G  = Y - (0.3455 * Cb) - (0.7169 * Cr);
*      if (R < 0) R=0; else if (R > 255) R = 255;
*      if (B < 0) B=0; else if (B > 255) B = 255;
*      if (G < 0) G=0; else if (G > 255) G = 255;
*      int32_t RGB = 0xFF000000 | (R << 16) | (G << 8) | B;
* =============================================================================
*/
static char sz_YUV_decoder_error[299] = { '!', 0 };


//=======================================================================================
// synopsis: result = do_decode_YUV420_frame(out_RGB_ptr, YUV_src_ptr, aCols, aRows, aType)
//
// Writes bitmap to a PNG file specified by path; returns 0 if OK, else error
//=======================================================================================
static int32_t do_decode_YUV420_frame(void    *output_RGB_ptr, 
                                      uint8_t *YUV_source_ptr, 
                                      int32_t  numFrameCols, 
                                      int32_t  numFrameRows,
                                      int32_t  srcFrameType) 
{
#define _VERIFY_I420_ARGS___________________NO
#define _STRICT_I420_TYPE_

#define _RGB32_PIXEL_TYPE___________________NO
#define _RGB24_PIXEL_TYPE_

#define CLAMP_8BITS(X) { if (X < 0) { X = 0; } else if (X > 255) { X = 255; } }
#define MAKE_ARGB_8888(pRGB,R,B,G)   ( ++pRGB, pRGB->red=(uint8_t)R, pRGB->blue=(uint8_t)B, pRGB->green=(uint8_t)G )
#define MAKE_RGB24_888(pRGB,R,B,G)   ( ++pRGB, pRGB->red=(uint8_t)R, pRGB->blue=(uint8_t)B, pRGB->green=(uint8_t)G )

#ifdef _RGB24_PIXEL_TYPE_
    #define MAKE_RGB_PIXEL(pRGB,R,G,B)   ( MAKE_RGB24_888(pRGB,R,B,G) )
    RGB24_Pix_t* ptr_RGB_pixel = ((RGB24_Pix_t*) output_RGB_ptr) - 1;
#endif

#ifdef _RGB32_PIXEL_TYPE_
    #define MAKE_RGB_PIXEL(pRGB,R,G,B)   ( MAKE_ARGB_8888(pRGB,R,B,G) )
    RGB32_Pix_t* ptr_RGB_pixel = ((RGB32_Pix_t*) output_RGB_ptr) - 1;
#endif

    int32_t     col_index, row_index, R, G, B, Y1, Y2, U, V, D, E, F;

    int32_t     num_frame_cols = numFrameCols;
    int32_t     num_frame_rows = numFrameRows;
    
    int32_t       Y_row_stride = num_frame_cols;                        // intensity rows stride
    int32_t       Y_frame_size = (Y_row_stride * num_frame_rows);       // intensity frame length

    int32_t       C_row_stride = ((num_frame_cols >> 1) + 0x3) & ~0x3;  // chroma rows stride
    int32_t       C_frame_size = (C_row_stride * num_frame_rows) / 2;   // chroma frame length

    uint8_t    *ptr_Y_encoding = YUV_source_ptr - 1;
    uint8_t    *end_Y_encoding = ptr_Y_encoding + Y_frame_size;
    uint8_t    *ptr_U_encoding = NULL;    
    uint8_t    *ptr_V_encoding = NULL;

    *sz_YUV_decoder_error = 0;    

#ifdef _VERIFY_I420_ARGS_
    if ( (num_frame_cols < 0) || (num_frame_cols & 1) || (srcFrameType < 1) ||
         (num_frame_rows < 0) || (num_frame_rows & 1) || (srcFrameType > 4) )
    {        
        sprintf (sz_YUV_decoder_error, "invalid arguments");
        return -1;        
    }
#endif

#ifdef _STRICT_I420_TYPE_
    if (srcFrameType != 1)
    {
        sprintf (sz_YUV_decoder_error, "expecting I420_IYUV as Frame Type");
        return -2;        
    }
#endif

    for ( row_index = 0;  row_index < num_frame_rows;  ++row_index )
    {
        ptr_U_encoding = end_Y_encoding + ( (row_index >> 1) * C_row_stride );
        ptr_V_encoding = ptr_U_encoding + C_frame_size;

#ifndef _STRICT_I420_TYPE_
        if (srcFrameType == 2) // "I420_YV12"
        {
            ptr_V_encoding  = ptr_U_encoding;
            ptr_U_encoding += C_frame_size;
        }
        else if (srcFrameType == 3) // "I420_NV12"
        {
            ptr_V_encoding = ptr_U_encoding + 1;
        }
        else if (srcFrameType == 4) // "I420_NV21"
        {
            ptr_V_encoding  = ptr_U_encoding;
            ptr_U_encoding += 1;
        }
        else if (srcFrameType != 1) // "I420_IYUV"
        {
            sprintf (sz_YUV_decoder_error, "invalid Frame Type");
            return -3;        
        }
#endif

        for ( col_index = 0;  col_index < num_frame_cols;  col_index += 2 )
        {
            Y1 = ( 0xFF & (int32_t) *(++ptr_Y_encoding) );
            Y2 = ( 0xFF & (int32_t) *(++ptr_Y_encoding) );

            U = ( 0xFF & (int32_t) *(++ptr_U_encoding) ) - 128;
            V = ( 0xFF & (int32_t) *(++ptr_V_encoding) ) - 128;

            D = ( ((88 * U) + (184 * V)) >> 8 );    // approx. ((0.3455 * U) + (0.7169 * V))
            E = ( (455 * U) >> 8 );                 // approx. (1.7790 * U)
            F = ( (360 * V) >> 8 );                 // approx. (1.4065 * V)

            G = Y1 - D;      CLAMP_8BITS( G );
            B = Y1 + E;      CLAMP_8BITS( B );
            R = Y1 + F;      CLAMP_8BITS( R );

            MAKE_RGB_PIXEL(ptr_RGB_pixel, R, G, B);

            G = Y2 - D;      CLAMP_8BITS( G );
            B = Y2 + E;      CLAMP_8BITS( B );
            R = Y2 + F;      CLAMP_8BITS( R );

            MAKE_RGB_PIXEL(ptr_RGB_pixel, R, G, B);
        }
    }

    return Y_frame_size;
}


//=======================================================================================
// synopsis: count = convert_BGR_frame_to_RGB(aPixelsPtr, aDepth, aStride, aCols, aRows)
//
// returns number of pixels converted from BGRx to RGBx --- returns zero on failure
//=======================================================================================
int convert_BGR_frame_to_RGB(void * aPixelsPtr, int aDepth, int aStride, int aCols, int aRows)
{
    int num_pixels = aCols * aRows;

    // verify valid conditions
    if ( (aPixelsPtr == NULL) || (aCols < 1) || (aRows < 1) )
    {
        return 0;
    }
    
    uint8_t * next_row_ptr = ((uint8_t *)aPixelsPtr) + aStride;

    // possibly --- convert 32-bit pixels
    if ( (aDepth == 32) && (aStride >= aCols * 4) )
    {
        while (--aRows >= 0)
        {
            RGB32_Pix_t * pix_ptr = (RGB32_Pix_t *) (next_row_ptr - aStride);
            
            while (next_row_ptr != (uint8_t *) pix_ptr)
            {
                uint8_t other = pix_ptr->blue;
                
                pix_ptr->blue = pix_ptr->red;
                
                pix_ptr->red  = other;                
                
                ++pix_ptr;
            }
            
            next_row_ptr += aStride;
        }        
    
        return num_pixels;
    }

    // possibly --- convert 24-bit pixels
    if ( (aDepth == 24) && (aStride >= aCols * 3) )
    {
        while (--aRows >= 0)
        {
            RGB24_Pix_t * pix_ptr = (RGB24_Pix_t *) (((uint8_t *)next_row_ptr) - aStride);
            
            while (next_row_ptr != (uint8_t *) pix_ptr)
            {
                uint8_t other = pix_ptr->blue;
                
                pix_ptr->blue = pix_ptr->red;
                
                pix_ptr->red  = other;                
                
                ++pix_ptr;
            }
            
            next_row_ptr += aStride;
        }        
    
        return num_pixels;
    }

    // error --- invalid stride or invalid depth    
    return 0;   
}


//=======================================================================================
// synopsis: result = save_frame_as_PNG(aPathPtr, aFmtPtr, aPixsPtr, aPixsLng, aStride, aWdt, aHgt)
//
// Writes image frame to a PNG file specified by path;   returns 0 if OK, else error
//=======================================================================================
int save_frame_as_PNG(const char * aPathPtr,
                      const char * aFormatPtr,
                      void       * aPixelsPtr,
                      int          aPixmapLng,
                      int          aStrideLng,
                      int          aFrameCols,
                      int          aFrameRows)
{
    PixmapInfo_t pixmap_info;

    int num_frame_pixels = (aFrameRows < 1) ? 0 : (aFrameRows * aFrameCols);

    int  num_pixel_bytes = (aFrameCols < 1) ? 0 : (aStrideLng / aFrameCols);

    if ( (aPathPtr == NULL)     || 
         (aFormatPtr == NULL)   || 
         (num_pixel_bytes < 1)  || 
         (num_pixel_bytes > 4)  ||
         (num_frame_pixels < 0) )
    {
        return -10;
    }

    pixmap_info.depth  = num_pixel_bytes * 8;
    pixmap_info.width  = aFrameCols;
    pixmap_info.height = aFrameRows;
    pixmap_info.stride = aStrideLng;
    pixmap_info.pixels = aPixelsPtr;

    strncpy( pixmap_info.fmt, aFormatPtr, sizeof(pixmap_info.fmt) - 1 );

    if ( strstr(aPathPtr, ".RAW.") != NULL )
    {
        FILE * fp = (aPixelsPtr == NULL) ? NULL : fopen (aPathPtr, "wb");

        if (fp != NULL) 
        {
            fwrite(aPixelsPtr, aPixmapLng, 1, fp);
            fclose(fp);
        }

        return (fp ? 0 : -20);
    }

    if (strstr(aFormatPtr, "RGB") != NULL)   // RGB 
    {
        int result = -1;

        if (num_pixel_bytes == 3)       // RGB_24
        {
            result = do_save_RGB24_frame(aPathPtr, &pixmap_info);
        }
        else if (num_pixel_bytes == 4)  // RGB_32
        {
            result = do_save_RGB32_frame(aPathPtr, &pixmap_info);
        }
        else if (num_pixel_bytes == 2)  // RGB_16
        {
            ;   // TODO --- NOT YET NEEDED
        }

        return (result ? -30 : 0);
    }

    if (strstr(aFormatPtr, "I420") != NULL)   // YUV 
    {
        RGB24_Pix_t * ptr_RGB24_pixels = malloc(sizeof(RGB24_Pix_t) * num_frame_pixels);

        int result = do_decode_YUV420_frame(ptr_RGB24_pixels, aPixelsPtr, aFrameCols, aFrameRows, 1);

        if (result == num_frame_pixels)
        {
            pixmap_info.depth  = 24;
            pixmap_info.stride = aFrameCols * 3;
            pixmap_info.pixels = ptr_RGB24_pixels;

            result = do_save_RGB24_frame(aPathPtr, &pixmap_info);
        }

        free(ptr_RGB24_pixels);

        return (result ? -60 : 0);
    }

    return -100;
}
