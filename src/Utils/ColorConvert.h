/** @file ColorConvert.h
 * 
 * Some color management to work with lunaSVG rasterized bitmaps (especially used for 
 * implementing Microsoft's suggestion to deal with Dark Mode icons by inverting luminosity).
 * Article: https://devblogs.microsoft.com/visualstudio/visual-studio-dark-theme/
 * 
 * HSL to RGB (and vice versa) from:
 * https://gist.github.com/ciembor/1494530
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <lunasvg.h>

typedef struct rgb {
    float r, g, b;
} RGB;

typedef struct hsl {
    float h, s, l;
} HSL;

/*
 * Converts an RGB color value to HSL. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes r, g, and b are contained in the set [0, 255] and
 * returns HSL in the set [0, 1].
 */
HSL rgb2hsl(float r, float g, float b);

////////////////////////////////////////////////////////////////////////

/*
 * Converts an HUE to r, g or b.
 * returns float in the set [0, 1].
 */
float hue2rgb(float p, float q, float t);

////////////////////////////////////////////////////////////////////////

/*
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns RGB in the set [0, 255].
 */
RGB hsl2rgb(float h, float s, float l);

// Inverts luminosity from image while also converting colors.
void convertWithInverseLuminosity(lunasvg::Bitmap* bmp, int ri, int gi, int bi, int ai, bool unpremultiply);