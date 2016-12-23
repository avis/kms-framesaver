/* 
 * ======================================================================================
 * File:        save_frame_as_png.h
 *
 * Purpose:     external interface (API) for code in "save_frame_as_png.c"
 * 
 * History:     1. 2016-11-03   JBendor     Created    
 *              2. 2016-11-24   JBendor     Updated
 *
 * Copyright (c) 2016 TELMATE INC. All Rights Reserved. Proprietary and confidential.
 *               Unauthorized copying of this file is strictly prohibited.
 * ======================================================================================
 */

#ifndef __Save_Frames_As_PNG_H__

#define __Save_Frames_As_PNG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


typedef struct  
{
    char       fmt[9];      // format name (RGB, YUV, etc.)
    void     * pixels;
    uint32_t   depth;       // number of bits per pixel
    uint32_t   width;
    uint32_t   height;
    uint32_t   stride;
} PixmapInfo_t;


typedef struct 
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB24_Pix_t;


typedef struct 
{
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB32_Pix_t;


#define NUM_RGB24_PIXEL_BYTES   (sizeof(RGB24_Pix_t))
#define NUM_RGB32_PIXEL_BYTES   (sizeof(RGB32_Pix_t))
#define NUM_SAMPLE_BITS_R_G_B   (8)


//=======================================================================================
// synopsis: count = convert_BGR_frame_to_RGB(aPixelsPtr, aDepth, aStride, aCols, aRows)
//
// returns number of pixels converted from BGRx to RGBx --- returns zero on failure
//=======================================================================================
extern int convert_BGR_frame_to_RGB(void  * aPixelsPtr, 
                                    int     aDepth,     // must be 24 or 32
                                    int     aStride, 
                                    int     aNumCols, 
                                    int     aNumRows);


//=======================================================================================
// synopsis: result = save_frame_as_PNG(aPathPtr,aFmtPtraPixsPtr,aPixsLng,aStride,aWdt,aHgt)
//
// Writes image frame to a PNG file specified by path;   returns 0 if OK, else error
//=======================================================================================
extern int save_frame_as_PNG(const char * aPathPtr,
                             const char * aFormatPtr,
                             void       * aPixelsPtr,
                             int          aPixmapLng,
                             int          aStrideLng,
                             int          aFrameCols, 
                             int          aFrameRows);


#ifdef __cplusplus
}
#endif  // __cplusplus


#endif // _Save_Frames_As_PNG_H__
