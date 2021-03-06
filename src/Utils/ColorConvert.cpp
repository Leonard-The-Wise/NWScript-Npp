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

#include "ColorConvert.h"

/*
 * Converts an RGB color value to HSL. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes r, g, and b are contained in the set [0, 255] and
 * returns HSL in the set [0, 1].
 */
HSL rgb2hsl(float r, float g, float b) {

    HSL result;

    r /= 255;
    g /= 255;
    b /= 255;

    float max = std::max(std::max(r, g), b);
    float min = std::min(std::min(r, g), b);

    result.h = result.s = result.l = (max + min) / 2;

    if (max == min) {
        result.h = result.s = 0; // achromatic
    }
    else {
        float d = max - min;
        result.s = (result.l > 0.5) ? d / (2 - max - min) : d / (max + min);

        if (max == r) {
            result.h = (g - b) / d + (g < b ? 6 : 0);
        }
        else if (max == g) {
            result.h = (b - r) / d + 2;
        }
        else if (max == b) {
            result.h = (r - g) / d + 4;
        }

        result.h /= 6;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////

/*
 * Converts an HUE to r, g or b.
 * returns float in the set [0, 1].
 */
float hue2rgb(float p, float q, float t) {

    if (t < 0.0f)
        t += 1.0f;
    if (t > 1.0f)
        t -= 1.0f;
    if (t < 1.0f / 6.0f)
        return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f)
        return q;
    if (t < 2.0f / 3.0f)
        return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;

    return p;

}

////////////////////////////////////////////////////////////////////////

/*
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes h, s, and l are contained in the set [0, 1] and
 * returns RGB in the set [0, 255].
 */
RGB hsl2rgb(float h, float s, float l) {

    RGB result;

    if (0 == s) {
        result.r = result.g = result.b = l; // achromatic
    }
    else {
        float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        result.r = hue2rgb(p, q, h + 1.0f / 3.0f) * 255.0f;
        result.g = hue2rgb(p, q, h) * 255.0f;
        result.b = hue2rgb(p, q, h - 1.0f / 3.0f) * 255.0f;
    }

    return result;

}

// Inverts luminosity from image while also converting colors.
void convertWithInverseLuminosity(lunasvg::Bitmap* bmp, int ri, int gi, int bi, int ai, bool unpremultiply)
{
    auto width = bmp->width();
    auto height = bmp->height();
    auto stride = bmp->stride();
    auto rowData = bmp->data();

    HSL hslAdjust;
    RGB result;
    for (std::uint32_t y = 0; y < height; y++)
    {
        auto data = rowData;
        for (std::uint32_t x = 0; x < width; x++)
        {
            auto a = data[3];
            auto r = data[2];
            auto g = data[1];
            auto b = data[0];

            if (unpremultiply && a != 0)
            {
                r = (r * 255) / a;
                g = (g * 255) / a;
                b = (b * 255) / a;
            }

            hslAdjust = rgb2hsl((float)r, (float)g, (float)b);
            hslAdjust.l = 1 - hslAdjust.l;
            result = hsl2rgb(hslAdjust.h, hslAdjust.s, hslAdjust.l);

            // Work with integers, better performance...
            r = (uint8_t)result.r;
            g = (uint8_t)result.g;
            b = (uint8_t)result.b;

            // Also, we invert grayscales of low color intensity
            if (r == g && r == b && r < 128)
            {
                r = 255 - r;
                g = 255 - g;
                b = 255 - b;
            }

            data[ai] = a;
            data[ri] = r;
            data[gi] = g;
            data[bi] = b;

            data += 4;
        }
        rowData += stride;
    }
}
