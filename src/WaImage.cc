/**
 * @file   WaImage.cc
 * @author David Reveman <c99drn@cs.umu.se>
 * @date   18-Jul-2001 01:45:32
 *
 * @brief Implementation of WaImage and WaImageControl classes
 *
 * These classes are the Image and ImageControl classes from
 * Blackbox Window Manager. Some minor changes has been made to make them
 * fit into the Waimea Window Manager project. Except these small
 * changes the WaImage class and the WaImageControl classes are the same as
 * the Image class and the ImageControl class from Blackbox 0.61.1.
 *
 * Thanks Brad!
 * Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
 *
 * Copyright (C) David Reveman. All rights reserved.
 *
 */

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "WaImage.hh"

#ifdef    HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif // HAVE_SYS_TYPES_H

#ifndef u_int32_t
#  ifdef uint_32_t
typedef uint32_t u_int32_t;
#  else
#    ifdef __uint32_t
typedef __uint32_t u_int32_t;
#    else
typedef unsigned int u_int32_t;
#    endif
#  endif
#endif

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

static unsigned long bsqrt(unsigned long x) {
    if (x <= 0) return 0;
    if (x == 1) return 1;
    
    unsigned long r = x >> 1;
    unsigned long q;
    
    while (1) {
        q = x / r;
        if (q >= r) return r;
        r = (r + q) >> 1;
    }
}

#ifdef XRENDER
bool have_root_pmap = true;

int WaTexture::getOpacity(void) {
    if (have_root_pmap)
        return opacity;
    
    return 0;
}
#endif // XRENDER

void WaColor::setRGB(unsigned short r, unsigned short g, unsigned short b) {
    
#ifdef XRENDER
    xrenderc.red = r;
    xrenderc.green = g;
    xrenderc.blue = b;
    xrenderc.alpha = 0xFFFF;
#endif // XRENDER
    
    if (r == 65535) red = 0xff;
    else red = (r / 0xff);
    if (g == 65535) green = 0xff;
    else green = (g / 0xff);
    if (b == 65535) blue = 0xff;
    else blue = (b / 0xff);        
}

#ifdef XFT
void WaColor::setXftOpacity(unsigned char o) {
    xftc.color.red = xrenderc.red * (100 - o) / 100;
    xftc.color.green = xrenderc.green * (100 - o) / 100;
    xftc.color.blue = xrenderc.blue * (100 - o) / 100;
    xftc.color.alpha = 0xFFFF * (100 - o) / 100;
    xftc.pixel = getPixel();        
}
#endif // XFT

WaImage::WaImage(WaImageControl *c, unsigned int w, unsigned int h) {
    control = c;
    
    width = ((signed) w > 0) ? w : 1;
    height = ((signed) h > 0) ? h : 1;
    
    red = new unsigned char[width * height];
    green = new unsigned char[width * height];
    blue = new unsigned char[width * height];
    
    xtable = ytable = (unsigned int *) 0;
    
    cpc = control->getColorsPerChannel();
    cpccpc = cpc * cpc;
    
    control->getColorTables(&red_table, &green_table, &blue_table,
                            &red_offset, &green_offset, &blue_offset,
                            &red_bits, &green_bits, &blue_bits);
    
    if (control->getVisual()->c_class != TrueColor)
        control->getXColorTable(&colors, &ncolors);
}


WaImage::~WaImage(void) {
    if (red) delete [] red;
    if (green) delete [] green;
    if (blue) delete [] blue;
}


Pixmap WaImage::render(WaTexture *texture) {
    if (texture->getTexture() & WaImage_ParentRelative)
        return ParentRelative;
    else if (texture->getTexture() & WaImage_Solid)
        return render_solid(texture);
    else if (texture->getTexture() & WaImage_Gradient)
        return render_gradient(texture);
    
#ifdef PIXMAP
    else if (texture->getTexture() & WaImage_Pixmap)
        return render_pixmap(texture);
#endif // PIXMAP
    
    return None;
}

#ifdef PIXMAP
Pixmap WaImage::render_pixmap(WaTexture *texture) {
    Pixmap pixmap, mask;

    imlib_context_push(*texture->getContext());
    imlib_context_set_mask(0);
    
    imlib_context_set_image(texture->getPixmap());

    if (texture->getTexture() & WaImage_Tile)
        imlib_render_pixmaps_for_whole_image(&pixmap, &mask);
    else 
        imlib_render_pixmaps_for_whole_image_at_size(&pixmap, &mask, width,
                                                     height);
    imlib_context_pop();
    return pixmap;
}
#endif // PIXMAP


Pixmap WaImage::render_solid(WaTexture *texture) {
    Pixmap pixmap = XCreatePixmap(control->getDisplay(),
                                  control->getDrawable(), width,
                                  height, control->getDepth());
    if (pixmap == None) {
        WARNING << "error creating pixmap" << endl;
        return None;
    }
    
    XGCValues gcv;
    GC gc, hgc, lgc;
    
    gcv.foreground = texture->getColor()->getPixel();
    gcv.fill_style = FillSolid;
    gc = XCreateGC(control->getDisplay(), pixmap,
                   GCForeground | GCFillStyle, &gcv);
    
    gcv.foreground = texture->getHiColor()->getPixel();
    hgc = XCreateGC(control->getDisplay(), pixmap,
                    GCForeground, &gcv);
    
    gcv.foreground = texture->getLoColor()->getPixel();
    lgc = XCreateGC(control->getDisplay(), pixmap,
                    GCForeground, &gcv);
    
    XFillRectangle(control->getDisplay(), pixmap, gc, 0, 0,
                   width, height);
    
#ifdef    INTERLACE
    if (texture->getTexture() & WaImage_Interlaced) {
        gcv.foreground = texture->getColorTo()->getPixel();
        GC igc = XCreateGC(control->getDisplay(), pixmap,
                           GCForeground, &gcv);
        
        register unsigned int i = 0;
        for (; i < height; i += 2)
            XDrawLine(control->getDisplay(), pixmap, igc,
                      0, i, width, i);
        
        XFreeGC(control->getDisplay(), igc);
    }
#endif // INTERLACE
    
    
    if (texture->getTexture() & WaImage_Bevel1) {
        if (texture->getTexture() & WaImage_Raised) {
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      0, height - 1, width - 1, height - 1);
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      width - 1, height - 1, width - 1, 0);
            
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      0, 0, width - 1, 0);
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      0, height - 1, 0, 0);
        } else if (texture->getTexture() & WaImage_Sunken) {
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      0, height - 1, width - 1, height - 1);
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      width - 1, height - 1, width - 1, 0);
            
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      0, 0, width - 1, 0);
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      0, height - 1, 0, 0);
        }
    } else if (texture->getTexture() & WaImage_Bevel2) {
        if (texture->getTexture() & WaImage_Raised) {
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      1, height - 3, width - 3, height - 3);
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      width - 3, height - 3, width - 3, 1);
            
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      1, 1, width - 3, 1);
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      1, height - 3, 1, 1);
        } else if (texture->getTexture() & WaImage_Sunken) {
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      1, height - 3, width - 3, height - 3);
            XDrawLine(control->getDisplay(), pixmap, hgc,
                      width - 3, height - 3, width - 3, 1);
            
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      1, 1, width - 3, 1);
            XDrawLine(control->getDisplay(), pixmap, lgc,
                      1, height - 3, 1, 1);
        }
    }
    
    XFreeGC(control->getDisplay(), gc);
    XFreeGC(control->getDisplay(), hgc);
    XFreeGC(control->getDisplay(), lgc);
    
    return pixmap;
}


Pixmap WaImage::render_gradient(WaTexture *texture) {
    int inverted = 0;
    
#ifdef    INTERLACE
    interlaced = texture->getTexture() & WaImage_Interlaced;
#endif // INTERLACE
    
    if (texture->getTexture() & WaImage_Sunken) {
        from = texture->getColorTo();
        to = texture->getColor();
        
        if (! (texture->getTexture() & WaImage_Invert)) inverted = 1;
    } else {
        from = texture->getColor();
        to = texture->getColorTo();
        
        if (texture->getTexture() & WaImage_Invert) inverted = 1;
    }
    
    control->getGradientBuffers(width, height, &xtable, &ytable);
    
    if (texture->getTexture() & WaImage_Diagonal) dgradient();
    else if (texture->getTexture() & WaImage_Elliptic) egradient();
    else if (texture->getTexture() & WaImage_Horizontal) hgradient();
    else if (texture->getTexture() & WaImage_Pyramid) pgradient();
    else if (texture->getTexture() & WaImage_Rectangle) rgradient();
    else if (texture->getTexture() & WaImage_Vertical) vgradient();
    else if (texture->getTexture() & WaImage_CrossDiagonal) cdgradient();
    else if (texture->getTexture() & WaImage_PipeCross) pcgradient();
    
    if (texture->getTexture() & WaImage_Bevel1) bevel1();
    else if (texture->getTexture() & WaImage_Bevel2) bevel2();
    
    if (inverted) invert();
    
    Pixmap pixmap = renderPixmap();
    
    return pixmap;    
}


XImage *WaImage::renderXImage(void) {
    XImage *image =
        XCreateImage(control->getDisplay(),
                     control->getVisual(), control->getDepth(), ZPixmap, 0, 0,
                     width, height, 32, 0);
    
    if (! image) {
        WARNING << "error creating XImage" << endl;
        return (XImage *) 0;
    }
    
    // insurance policy
    image->data = (char *) 0;
    
    unsigned char *d = new unsigned char[image->bytes_per_line * (height + 1)];
    register unsigned int x, y, dithx, dithy, r, g, b, o, er, eg, eb, offset;

    unsigned char *pixel_data = d, *ppixel_data = d;
    unsigned long pixel;
    
    o = image->bits_per_pixel + ((image->byte_order == MSBFirst) ? 1 : 0);
    
    if (control->doDither() && width > 1 && height > 1) {
        unsigned char dither4[4][4] = { {0, 4, 1, 5},
                                        {6, 2, 7, 3},
                                        {1, 5, 0, 4},
                                        {7, 3, 6, 2} };
        
#ifdef    ORDEREDPSEUDO
        unsigned char dither8[8][8] = { { 0,  32, 8,  40, 2,  34, 10, 42 },
                                        { 48, 16, 56, 24, 50, 18, 58, 26 },
                                        { 12, 44, 4,  36, 14, 46, 6,  38 },
                                        { 60, 28, 52, 20, 62, 30, 54, 22 },
                                        { 3,  35, 11, 43, 1,  33, 9,  41 },
                                        { 51, 19, 59, 27, 49, 17, 57, 25 },
                                        { 15, 47, 7,  39, 13, 45, 5,  37 },
                                        { 63, 31, 55, 23, 61, 29, 53, 21 } };
#endif // ORDEREDPSEUDO
        
        switch (control->getVisual()->c_class) {
            case TrueColor:
                // algorithm: ordered dithering... many many thanks to
                // rasterman (raster@rasterman.com) for telling me about
                // this... portions of this code is based off of his code
                // in Imlib
                for (y = 0, offset = 0; y < height; y++) {
                    dithy = y & 0x3;
                    
                    for (x = 0; x < width; x++, offset++) {
                        dithx = x & 0x3;
                        r = red[offset];
                        g = green[offset];
                        b = blue[offset];
                        
                        er = r & (red_bits - 1);
                        eg = g & (green_bits - 1);
                        eb = b & (blue_bits - 1);
                        
                        r = red_table[r];
                        g = green_table[g];
                        b = blue_table[b];
                        
                        if ((dither4[dithy][dithx] < er) &&
                            (r < red_table[255])) r++;
                        if ((dither4[dithy][dithx] < eg) &&
                            (g < green_table[255])) g++;
                        if ((dither4[dithy][dithx] < eb) &&
                            (b < blue_table[255])) b++;
                        
                        pixel = (r << red_offset) | (g << green_offset) |
                            (b << blue_offset);
                        
                        switch (o) {
                            case  8: //  8bpp
                                *pixel_data++ = pixel;
                                break;
                                
                            case 16: // 16bpp LSB
                                *pixel_data++ = pixel;
                                *pixel_data++ = pixel >> 8;
                                break;
                                
                            case 17: // 16bpp MSB
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel;
                                break;
                                
                            case 24: // 24bpp LSB
                                *pixel_data++ = pixel;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel >> 16;
                                break;
                                
                            case 25: // 24bpp MSB
                                *pixel_data++ = pixel >> 16;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel;
                                break;
                                
                            case 32: // 32bpp LSB
                                *pixel_data++ = pixel;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel >> 16;
                                *pixel_data++ = pixel >> 24;
                                break;
                                
                            case 33: // 32bpp MSB
                                *pixel_data++ = pixel >> 24;
                                *pixel_data++ = pixel >> 16;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel;
                                break;
                        }
                    }
                    pixel_data = (ppixel_data += image->bytes_per_line);
                }
                break;
                
            case StaticColor:
            case PseudoColor: {
#ifndef   ORDEREDPSEUDO
                short *terr,
                    *rerr = new short[width + 2],
                    *gerr = new short[width + 2],
                    *berr = new short[width + 2],
                    *nrerr = new short[width + 2],
                    *ngerr = new short[width + 2],
                    *nberr = new short[width + 2];
                int rr, gg, bb, rer, ger, ber;
                int dd = 255 / control->getColorsPerChannel();
                
                for (x = 0; x < width; x++) {
                    *(rerr + x) = *(red + x);
                    *(gerr + x) = *(green + x);
                    *(berr + x) = *(blue + x);
                }
                
                *(rerr + x) = *(gerr + x) = *(berr + x) = 0;
#endif // ORDEREDPSEUDO
                
                for (y = 0, offset = 0; y < height; y++) {
#ifdef ORDEREDPSEUDO
                    dithy = y & 7;
                    
                    for (x = 0; x < width; x++, offset++) {
                        dithx = x & 7;
                        
                        r = red[offset];
                        g = green[offset];
                        b = blue[offset];
                        
                        er = r & (red_bits - 1);
                        eg = g & (green_bits - 1);
                        eb = b & (blue_bits - 1);
                        
                        r = red_table[r];
                        g = green_table[g];
                        b = blue_table[b];
                        
                        if ((dither8[dithy][dithx] < er) &&
                            (r < red_table[255])) r++;
                        if ((dither8[dithy][dithx] < eg) &&
                            (g < green_table[255])) g++;
                        if ((dither8[dithy][dithx] < eb) &&
                            (b < blue_table[255])) b++;
                        
                        pixel = (r * cpccpc) + (g * cpc) + b;
                        *(pixel_data++) = colors[pixel].pixel;
                    }
                    
                    pixel_data = (ppixel_data += image->bytes_per_line);
#else // !ORDEREDPSEUDO
                    if (y < (height - 1)) {
                        int i = offset + width;
                        for (x = 0; x < width; x++, i++) {
                            *(nrerr + x) = *(red + i);
                            *(ngerr + x) = *(green + i);
                            *(nberr + x) = *(blue + i);
                        }
                        
                        *(nrerr + x) = *(red + (--i));
                        *(ngerr + x) = *(green + i);
                        *(nberr + x) = *(blue + i);
                    }
                    
                    for (x = 0; x < width; x++) {
                        rr = rerr[x];
                        gg = gerr[x];
                        bb = berr[x];
                        
                        if (rr > 255) rr = 255; else if (rr < 0) rr = 0;
                        if (gg > 255) gg = 255; else if (gg < 0) gg = 0;
                        if (bb > 255) bb = 255; else if (bb < 0) bb = 0;
                        
                        r = red_table[rr];
                        g = green_table[gg];
                        b = blue_table[bb];
                        
                        rer = rerr[x] - r*dd;
                        ger = gerr[x] - g*dd;
                        ber = berr[x] - b*dd;
                        
                        pixel = (r * cpccpc) + (g * cpc) + b;
                        *pixel_data++ = colors[pixel].pixel;
                    
                        r = rer >> 1;
                        g = ger >> 1;
                        b = ber >> 1;
                        rerr[x+1] += r;
                        gerr[x+1] += g;
                        berr[x+1] += b;
                        nrerr[x] += r;
                        ngerr[x] += g;
                        nberr[x] += b;
                    }
                    
                    offset += width;
                    
                    pixel_data = (ppixel_data += image->bytes_per_line);
                    
                    terr = rerr;
                    rerr = nrerr;
                    nrerr = terr;
                    
                    terr = gerr;
                    gerr = ngerr;
                    ngerr = terr;
                    
                    terr = berr;
                    berr = nberr;
                    nberr = terr;
#endif // ORDEREDPSUEDO   
                }
#ifndef ORDEREDPSEUDO
                delete [] rerr;
                delete [] gerr;
                delete [] berr;
                delete [] nrerr;
                delete [] ngerr;
                delete [] nberr;
#endif // !ORDEREDPSUEDO
                
                break;
            }
            default:
                WARNING << "unsupported visual" << endl;
                delete [] d;
                XDestroyImage(image);
                return (XImage *) 0;
        }
    }
    else {
        switch (control->getVisual()->c_class) {
            case StaticColor:
            case PseudoColor:
                for (y = 0, offset = 0; y < height; y++) {
                    for (x = 0; x < width; x++, offset++) {
                        r = red_table[red[offset]];
                        g = green_table[green[offset]];
                        b = blue_table[blue[offset]];
                        
                        pixel = (r * cpccpc) + (g * cpc) + b;
                        *pixel_data++ = colors[pixel].pixel;
                    }    
                    pixel_data = (ppixel_data += image->bytes_per_line);
                }
                break;
                
            case TrueColor:
                for (y = 0, offset = 0; y < height; y++) {
                    for (x = 0; x < width; x++, offset++) {
                        r = red_table[red[offset]];
                        g = green_table[green[offset]];
                        b = blue_table[blue[offset]];
                        
                        pixel = (r << red_offset) | (g << green_offset) |
                            (b << blue_offset);
                        
                        switch (o) {
                            case  8: //  8bpp
                                *pixel_data++ = pixel;
                                break;
                                
                            case 16: // 16bpp LSB
                                *pixel_data++ = pixel;
                                *pixel_data++ = pixel >> 8;
                                break;
                                
                            case 17: // 16bpp MSB
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel;
                                break;
                                
                            case 24: // 24bpp LSB
                                *pixel_data++ = pixel;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel >> 16;
                                break;
                                
                            case 25: // 24bpp MSB
                                *pixel_data++ = pixel >> 16;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel;
                                break;
                                
                            case 32: // 32bpp LSB
                                *pixel_data++ = pixel;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel >> 16;
                                *pixel_data++ = pixel >> 24;
                                break;
                                
                            case 33: // 32bpp MSB
                                *pixel_data++ = pixel >> 24;
                                *pixel_data++ = pixel >> 16;
                                *pixel_data++ = pixel >> 8;
                                *pixel_data++ = pixel;
                                break;
                        }
                    }
                    pixel_data = (ppixel_data += image->bytes_per_line);
                }
                break;
                
            case StaticGray:
            case GrayScale:
                for (y = 0, offset = 0; y < height; y++) {
                    for (x = 0; x < width; x++, offset++) {
                        r = *(red_table + *(red + offset));
                        g = *(green_table + *(green + offset));
                        b = *(blue_table + *(blue + offset));
                        
                        g = ((r * 30) + (g * 59) + (b * 11)) / 100;
                        *pixel_data++ = colors[g].pixel;
                    }
                    pixel_data = (ppixel_data += image->bytes_per_line);
                }
                break;
                
            default:
                WARNING << "unsupported visual" << endl;
                delete [] d;
                XDestroyImage(image);
                return (XImage *) 0;
        }
    }
    image->data = (char *) d;
    return image;
}


Pixmap WaImage::renderPixmap(void) {
    Pixmap pixmap =
        XCreatePixmap(control->getDisplay(),
                      control->getDrawable(), width, height,
                      control->getDepth());
    
    if (pixmap == None) {
        WARNING << "error creating pixmap" << endl;
        return None;
    }
    
    XImage *image = renderXImage();
    
    if (! image) {
        XFreePixmap(control->getDisplay(), pixmap);
        return None;
    } else if (! image->data) {
        XDestroyImage(image);
        XFreePixmap(control->getDisplay(), pixmap);
        return None;
    }
    
    XPutImage(control->getDisplay(), pixmap,
              DefaultGC(control->getDisplay(),
                        control->getScreen()),
              image, 0, 0, 0, 0, width, height);
    
    if (image->data) {
        delete [] image->data;
        image->data = NULL;
    }
    
    XDestroyImage(image);
    
    return pixmap;
}


void WaImage::bevel1(void) {
    if (width > 2 && height > 2) {
        unsigned char *pr = red, *pg = green, *pb = blue;
        
        register unsigned char r, g, b, rr ,gg ,bb;
        register unsigned int w = width, h = height - 1, wh = w * h;
        
        while (--w) {
            r = *pr;
            rr = r + (r >> 1);
            if (rr < r) rr = ~0;
            g = *pg;
            gg = g + (g >> 1);
            if (gg < g) gg = ~0;
            b = *pb;
            bb = b + (b >> 1);
            if (bb < b) bb = ~0;
            
            *pr = rr;
            *pg = gg;
            *pb = bb;
            
            r = *(pr + wh);
            rr = (r >> 2) + (r >> 1);
            if (rr > r) rr = 0;
            g = *(pg + wh);
            gg = (g >> 2) + (g >> 1);
            if (gg > g) gg = 0;
            b = *(pb + wh);
            bb = (b >> 2) + (b >> 1);
            if (bb > b) bb = 0;
            
            *((pr++) + wh) = rr;
            *((pg++) + wh) = gg;
            *((pb++) + wh) = bb;
        }
        
        r = *pr;
        rr = r + (r >> 1);
        if (rr < r) rr = ~0;
        g = *pg;
        gg = g + (g >> 1);
        if (gg < g) gg = ~0;
        b = *pb;
        bb = b + (b >> 1);
        if (bb < b) bb = ~0;
        
        *pr = rr;
        *pg = gg;
        *pb = bb;
        
        r = *(pr + wh);
        rr = (r >> 2) + (r >> 1);
        if (rr > r) rr = 0;
        g = *(pg + wh);
        gg = (g >> 2) + (g >> 1);
        if (gg > g) gg = 0;
        b = *(pb + wh);
        bb = (b >> 2) + (b >> 1);
        if (bb > b) bb = 0;
        
        *(pr + wh) = rr;
        *(pg + wh) = gg;
        *(pb + wh) = bb;
        
        pr = red + width;
        pg = green + width;
        pb = blue + width;
        
        while (--h) {
            r = *pr;
            rr = r + (r >> 1);
            if (rr < r) rr = ~0;
            g = *pg;
            gg = g + (g >> 1);
            if (gg < g) gg = ~0;
            b = *pb;
            bb = b + (b >> 1);
            if (bb < b) bb = ~0;
            
            *pr = rr;
            *pg = gg;
            *pb = bb;
            
            pr += width - 1;
            pg += width - 1;
            pb += width - 1;
            
            r = *pr;
            rr = (r >> 2) + (r >> 1);
            if (rr > r) rr = 0;
            g = *pg;
            gg = (g >> 2) + (g >> 1);
            if (gg > g) gg = 0;
            b = *pb;
            bb = (b >> 2) + (b >> 1);
            if (bb > b) bb = 0;
            
            *(pr++) = rr;
            *(pg++) = gg;
            *(pb++) = bb;
        }
        
        r = *pr;
        rr = r + (r >> 1);
        if (rr < r) rr = ~0;
        g = *pg;
        gg = g + (g >> 1);
        if (gg < g) gg = ~0;
        b = *pb;
        bb = b + (b >> 1);
        if (bb < b) bb = ~0;
        
        *pr = rr;
        *pg = gg;
        *pb = bb;
        
        pr += width - 1;
        pg += width - 1;
        pb += width - 1;
        
        r = *pr;
        rr = (r >> 2) + (r >> 1);
        if (rr > r) rr = 0;
        g = *pg;
        gg = (g >> 2) + (g >> 1);
        if (gg > g) gg = 0;
        b = *pb;
        bb = (b >> 2) + (b >> 1);
        if (bb > b) bb = 0;
        
        *pr = rr;
        *pg = gg;
        *pb = bb;
    }
}


void WaImage::bevel2(void) {
    if (width > 4 && height > 4) {
        unsigned char r, g, b, rr ,gg ,bb, *pr = red + width + 1,
            *pg = green + width + 1, *pb = blue + width + 1;
        unsigned int w = width - 2, h = height - 1, wh = width * (height - 3);
        
        while (--w) {
            r = *pr;
            rr = r + (r >> 1);
            if (rr < r) rr = ~0;
            g = *pg;
            gg = g + (g >> 1);
            if (gg < g) gg = ~0;
            b = *pb;
            bb = b + (b >> 1);
            if (bb < b) bb = ~0;
            
            *pr = rr;
            *pg = gg;
            *pb = bb;
            
            r = *(pr + wh);
            rr = (r >> 2) + (r >> 1);
            if (rr > r) rr = 0;
            g = *(pg + wh);
            gg = (g >> 2) + (g >> 1);
            if (gg > g) gg = 0;
            b = *(pb + wh);
            bb = (b >> 2) + (b >> 1);
            if (bb > b) bb = 0;
            
            *((pr++) + wh) = rr;
            *((pg++) + wh) = gg;
            *((pb++) + wh) = bb;
        }
        
        pr = red + width;
        pg = green + width;
        pb = blue + width;
        
        while (--h) {
            r = *pr;
            rr = r + (r >> 1);
            if (rr < r) rr = ~0;
            g = *pg;
            gg = g + (g >> 1);
            if (gg < g) gg = ~0;
            b = *pb;
            bb = b + (b >> 1);
            if (bb < b) bb = ~0;
            
            *(++pr) = rr;
            *(++pg) = gg;
            *(++pb) = bb;
            
            pr += width - 3;
            pg += width - 3;
            pb += width - 3;
            
            r = *pr;
            rr = (r >> 2) + (r >> 1);
            if (rr > r) rr = 0;
            g = *pg;
            gg = (g >> 2) + (g >> 1);
            if (gg > g) gg = 0;
            b = *pb;
            bb = (b >> 2) + (b >> 1);
            if (bb > b) bb = 0;
            
            *(pr++) = rr;
            *(pg++) = gg;
            *(pb++) = bb;
            
            pr++; pg++; pb++;
        }
    }
}


void WaImage::invert(void) {
    register unsigned int i, j, wh = (width * height) - 1;
    unsigned char tmp;
    
    for (i = 0, j = wh; j > i; j--, i++) {
        tmp = *(red + j);
        *(red + j) = *(red + i);
        *(red + i) = tmp;
        
        tmp = *(green + j);
        *(green + j) = *(green + i);
        *(green + i) = tmp;
        
        tmp = *(blue + j);
        *(blue + j) = *(blue + i);
        *(blue + i) = tmp;
    }
}


void WaImage::dgradient(void) {
    // diagonal gradient code was written by Mike Cole <mike@mydot.com>
    // modified for interlacing by Brad Hughes
    
    float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
        xr = (float) from->getRed(),
        xg = (float) from->getGreen(),
        xb = (float) from->getBlue();
    unsigned char *pr = red, *pg = green, *pb = blue;
    unsigned int w = width * 2, h = height * 2, *xt = xtable, *yt = ytable;
    
    register unsigned int x, y;
    
    dry = drx = (float) (to->getRed() - from->getRed());
    dgy = dgx = (float) (to->getGreen() - from->getGreen());
    dby = dbx = (float) (to->getBlue() - from->getBlue());
    
    // Create X table
    drx /= w;
    dgx /= w;
    dbx /= w;
    
    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) (xr);
        *(xt++) = (unsigned char) (xg);
        *(xt++) = (unsigned char) (xb);
        
        xr += drx;
        xg += dgx;
        xb += dbx;
    }
    
    // Create Y table
    dry /= h;
    dgy /= h;
    dby /= h;
    
    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) yr);
        *(yt++) = ((unsigned char) yg);
        *(yt++) = ((unsigned char) yb);
        
        yr += dry;
        yg += dgy;
        yb += dby;
    }
    
    // Combine tables to create gradient
    
#ifdef    INTERLACE
    if (! interlaced) {
#endif // INTERLACE
        
        // normal dgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(pr++) = *(xt++) + *(yt);
                *(pg++) = *(xt++) + *(yt + 1);
                *(pb++) = *(xt++) + *(yt + 2);
            }
        }

#ifdef    INTERLACE
    } else {
        // faked interlacing effect
        unsigned char channel, channel2;
        
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = *(xt++) + *(yt);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pr++) = channel2;
                    
                    channel = *(xt++) + *(yt + 1);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pg++) = channel2;
                    
                    channel = *(xt++) + *(yt + 2);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pb++) = channel2;
                } else {
                    channel = *(xt++) + *(yt);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pr++) = channel2;
                    
                    channel = *(xt++) + *(yt + 1);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pg++) = channel2;
                    
                    channel = *(xt++) + *(yt + 2);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pb++) = channel2;
                }
            }
        }
    }
#endif // INTERLACE
    
}


void WaImage::hgradient(void) {
    float drx, dgx, dbx,
        xr = (float) from->getRed(),
        xg = (float) from->getGreen(),
        xb = (float) from->getBlue();
    unsigned char *pr = red, *pg = green, *pb = blue;
    
    register unsigned int x, y;
    
    drx = (float) (to->getRed() - from->getRed());
    dgx = (float) (to->getGreen() - from->getGreen());
    dbx = (float) (to->getBlue() - from->getBlue());

    drx /= width;
    dgx /= width;
    dbx /= width;
    
#ifdef    INTERLACE
    if (interlaced && height > 2) {
        // faked interlacing effect
        unsigned char channel, channel2;
        
        for (x = 0; x < width; x++, pr++, pg++, pb++) {
            channel = (unsigned char) xr;
            channel2 = (channel >> 1) + (channel >> 2);
            if (channel2 > channel) channel2 = 0;
            *pr = channel2;
            
            channel = (unsigned char) xg;
            channel2 = (channel >> 1) + (channel >> 2);
            if (channel2 > channel) channel2 = 0;
            *pg = channel2;
            
            channel = (unsigned char) xb;
            channel2 = (channel >> 1) + (channel >> 2);
            if (channel2 > channel) channel2 = 0;
            *pb = channel2;
            
            
            channel = (unsigned char) xr;
            channel2 = channel + (channel >> 3);
            if (channel2 < channel) channel2 = ~0;
            *(pr + width) = channel2;
            
            channel = (unsigned char) xg;
            channel2 = channel + (channel >> 3);
            if (channel2 < channel) channel2 = ~0;
            *(pg + width) = channel2;
            
            channel = (unsigned char) xb;
            channel2 = channel + (channel >> 3);
            if (channel2 < channel) channel2 = ~0;
            *(pb + width) = channel2;
            
            xr += drx;
            xg += dgx;
            xb += dbx;
        }
        
        pr += width;
        pg += width;
        pb += width;
        
        int offset;
        
        for (y = 2; y < height; y++, pr += width, pg += width, pb += width) {
            if (y & 1) offset = width; else offset = 0;
            
            memcpy(pr, (red + offset), width);
            memcpy(pg, (green + offset), width);
            memcpy(pb, (blue + offset), width);
        }
    } else {
#endif // INTERLACE
        
        // normal hgradient
        for (x = 0; x < width; x++) {
            *(pr++) = (unsigned char) (xr);
            *(pg++) = (unsigned char) (xg);
            *(pb++) = (unsigned char) (xb);
            
            xr += drx;
            xg += dgx;
            xb += dbx;
        }
        
        for (y = 1; y < height; y++, pr += width, pg += width, pb += width) {
            memcpy(pr, red, width);
            memcpy(pg, green, width);
            memcpy(pb, blue, width);
        }
        
#ifdef    INTERLACE
    }
#endif // INTERLACE
    
}


void WaImage::vgradient(void) {
    float dry, dgy, dby,
        yr = (float) from->getRed(),
        yg = (float) from->getGreen(),
        yb = (float) from->getBlue();
    unsigned char *pr = red, *pg = green, *pb = blue;
    
    register unsigned int y;
    
    dry = (float) (to->getRed() - from->getRed());
    dgy = (float) (to->getGreen() - from->getGreen());
    dby = (float) (to->getBlue() - from->getBlue());
    
    dry /= height;
    dgy /= height;
    dby /= height;
  
#ifdef    INTERLACE
    if (interlaced) {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
            if (y & 1) {
                channel = (unsigned char) yr;
                channel2 = (channel >> 1) + (channel >> 2);
                if (channel2 > channel) channel2 = 0;
                memset(pr, channel2, width);

                channel = (unsigned char) yg;
                channel2 = (channel >> 1) + (channel >> 2);
                if (channel2 > channel) channel2 = 0;
                memset(pg, channel2, width);
        
                channel = (unsigned char) yb;
                channel2 = (channel >> 1) + (channel >> 2);
                if (channel2 > channel) channel2 = 0;
                memset(pb, channel2, width);
            } else {
                channel = (unsigned char) yr;
                channel2 = channel + (channel >> 3);
                if (channel2 < channel) channel2 = ~0;
                memset(pr, channel2, width);

                channel = (unsigned char) yg;
                channel2 = channel + (channel >> 3);
                if (channel2 < channel) channel2 = ~0;
                memset(pg, channel2, width);

                channel = (unsigned char) yb;
                channel2 = channel + (channel >> 3);
                if (channel2 < channel) channel2 = ~0;
                memset(pb, channel2, width);
            }

            yr += dry;
            yg += dgy;
            yb += dby;
        }
    } else {
#endif // INTERLACE

        // normal vgradient
        for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
            memset(pr, (unsigned char) yr, width);
            memset(pg, (unsigned char) yg, width);
            memset(pb, (unsigned char) yb, width);

            yr += dry;
            yg += dgy;
            yb += dby;
        }

#ifdef    INTERLACE
    }
#endif // INTERLACE

}


void WaImage::pgradient(void) {
    // pyramid gradient -  based on original dgradient, written by
    // Mosfet (mosfet@kde.org)
    // adapted from kde sources for Blackbox by Brad Hughes

    float yr, yg, yb, drx, dgx, dbx, dry, dgy, dby,
        xr, xg, xb;
    int rsign, gsign, bsign;
    unsigned char *pr = red, *pg = green, *pb = blue;
    unsigned int tr = to->getRed(), tg = to->getGreen(), tb = to->getBlue(),
        *xt = xtable, *yt = ytable;

    register unsigned int x, y;

    dry = drx = (float) (to->getRed() - from->getRed());
    dgy = dgx = (float) (to->getGreen() - from->getGreen());
    dby = dbx = (float) (to->getBlue() - from->getBlue());

    rsign = (drx < 0) ? -1 : 1;
    gsign = (dgx < 0) ? -1 : 1;
    bsign = (dbx < 0) ? -1 : 1;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
        *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
        *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
        *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
        *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient

#ifdef    INTERLACE
    if (! interlaced) {
#endif // INTERLACE

        // normal pgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(pr++) = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
                *(pg++) = (unsigned char)
                    (tg - (gsign * (*(xt++) + *(yt + 1))));
                *(pb++) = (unsigned char)
                    (tb - (bsign * (*(xt++) + *(yt + 2))));
            }
        }

#ifdef    INTERLACE
    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char)
                        (tr - (rsign * (*(xt++) + *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * (*(xt++) + *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * (*(xt++) + *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pb++) = channel2;
                } else {
                    channel = (unsigned char)
                        (tr - (rsign * (*(xt++) + *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * (*(xt++) + *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * (*(xt++) + *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pb++) = channel2;
                }
            }
        }
    }
#endif // INTERLACE

}


void WaImage::rgradient(void) {
    // rectangle gradient -  based on original dgradient, written by
    // Mosfet (mosfet@kde.org)
    // adapted from kde sources for Blackbox by Brad Hughes

    float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
    int rsign, gsign, bsign;
    unsigned char *pr = red, *pg = green, *pb = blue;
    unsigned int tr = to->getRed(), tg = to->getGreen(), tb = to->getBlue(),
        *xt = xtable, *yt = ytable;

    register unsigned int x, y;

    dry = drx = (float) (to->getRed() - from->getRed());
    dgy = dgx = (float) (to->getGreen() - from->getGreen());
    dby = dbx = (float) (to->getBlue() - from->getBlue());

    rsign = (drx < 0) ? -2 : 2;
    gsign = (dgx < 0) ? -2 : 2;
    bsign = (dbx < 0) ? -2 : 2;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
        *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
        *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
        *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
        *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient

#ifdef    INTERLACE
    if (! interlaced) {
#endif // INTERLACE

        // normal rgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(pr++) = (unsigned char)
                    (tr - (rsign * wamax(*(xt++), *(yt))));
                *(pg++) = (unsigned char)
                    (tg - (gsign * wamax(*(xt++), *(yt + 1))));
                *(pb++) = (unsigned char)
                    (tb - (bsign * wamax(*(xt++), *(yt + 2))));
            }
        }

#ifdef    INTERLACE
    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char)
                        (tr - (rsign * wamax(*(xt++), *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * wamax(*(xt++), *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * wamax(*(xt++), *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pb++) = channel2;
                } else {
                    channel = (unsigned char)
                        (tr - (rsign * wamax(*(xt++), *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * wamax(*(xt++), *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * wamax(*(xt++), *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pb++) = channel2;
                }
            }
        }
    }
#endif // INTERLACE

}


void WaImage::egradient(void) {
    // elliptic gradient -  based on original dgradient, written by
    // Mosfet (mosfet@kde.org)
    // adapted from kde sources for Blackbox by Brad Hughes

    float drx, dgx, dbx, dry, dgy, dby, yr, yg, yb, xr, xg, xb;
    int rsign, gsign, bsign;
    unsigned char *pr = red, *pg = green, *pb = blue;
    unsigned int *xt = xtable, *yt = ytable,
        tr = (unsigned long) to->getRed(),
        tg = (unsigned long) to->getGreen(),
        tb = (unsigned long) to->getBlue();

    register unsigned int x, y;

    dry = drx = (float) (to->getRed() - from->getRed());
    dgy = dgx = (float) (to->getGreen() - from->getGreen());
    dby = dbx = (float) (to->getBlue() - from->getBlue());

    rsign = (drx < 0) ? -1 : 1;
    gsign = (dgx < 0) ? -1 : 1;
    bsign = (dbx < 0) ? -1 : 1;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned long) (xr * xr);
        *(xt++) = (unsigned long) (xg * xg);
        *(xt++) = (unsigned long) (xb * xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = (unsigned long) (yr * yr);
        *(yt++) = (unsigned long) (yg * yg);
        *(yt++) = (unsigned long) (yb * yb);

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient

#ifdef    INTERLACE
    if (! interlaced) {
#endif // INTERLACE

        // normal egradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(pr++) = (unsigned char)
                    (tr - (rsign * control->getSqrt(*(xt++) + *(yt))));
                *(pg++) = (unsigned char)
                    (tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));
                *(pb++) = (unsigned char)
                    (tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));
            }
        }

#ifdef    INTERLACE
    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char)
                        (tr - (rsign * control->getSqrt(*(xt++) + *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pb++) = channel2;
                } else {
                    channel = (unsigned char)
                        (tr - (rsign * control->getSqrt(*(xt++) + *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * control->getSqrt(*(xt++) + *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * control->getSqrt(*(xt++) + *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pb++) = channel2;
                }
            }
        }
    }
#endif // INTERLACE

}


void WaImage::pcgradient(void) {
    // pipe cross gradient -  based on original dgradient, written by
    // Mosfet (mosfet@kde.org)
    // adapted from kde sources for Blackbox by Brad Hughes

    float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
    int rsign, gsign, bsign;
    unsigned char *pr = red, *pg = green, *pb = blue;
    unsigned int *xt = xtable, *yt = ytable,
        tr = to->getRed(),
        tg = to->getGreen(),
        tb = to->getBlue();

    register unsigned int x, y;

    dry = drx = (float) (to->getRed() - from->getRed());
    dgy = dgx = (float) (to->getGreen() - from->getGreen());
    dby = dbx = (float) (to->getBlue() - from->getBlue());

    rsign = (drx < 0) ? -2 : 2;
    gsign = (dgx < 0) ? -2 : 2;
    bsign = (dbx < 0) ? -2 : 2;

    xr = yr = (drx / 2);
    xg = yg = (dgx / 2);
    xb = yb = (dbx / 2);

    // Create X table
    drx /= width;
    dgx /= width;
    dbx /= width;

    for (x = 0; x < width; x++) {
        *(xt++) = (unsigned char) ((xr < 0) ? -xr : xr);
        *(xt++) = (unsigned char) ((xg < 0) ? -xg : xg);
        *(xt++) = (unsigned char) ((xb < 0) ? -xb : xb);

        xr -= drx;
        xg -= dgx;
        xb -= dbx;
    }

    // Create Y table
    dry /= height;
    dgy /= height;
    dby /= height;

    for (y = 0; y < height; y++) {
        *(yt++) = ((unsigned char) ((yr < 0) ? -yr : yr));
        *(yt++) = ((unsigned char) ((yg < 0) ? -yg : yg));
        *(yt++) = ((unsigned char) ((yb < 0) ? -yb : yb));

        yr -= dry;
        yg -= dgy;
        yb -= dby;
    }

    // Combine tables to create gradient

#ifdef    INTERLACE
    if (! interlaced) {
#endif // INTERLACE

        // normal pcgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(pr++) = (unsigned char)
                    (tr - (rsign * wamin(*(xt++), *(yt))));
                *(pg++) = (unsigned char)
                    (tg - (gsign * wamin(*(xt++), *(yt + 1))));
                *(pb++) = (unsigned char)
                    (tb - (bsign * wamin(*(xt++), *(yt + 2))));
            }
        }

#ifdef    INTERLACE
    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = (unsigned char)
                        (tr - (rsign * wamin(*(xt++), *(yt))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (bsign * wamin(*(xt++), *(yt + 1))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (gsign * wamin(*(xt++), *(yt + 2))));
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pb++) = channel2;
                } else {
                    channel = (unsigned char)
                        (tr - (rsign * wamin(*(xt++), *(yt))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pr++) = channel2;

                    channel = (unsigned char)
                        (tg - (gsign * wamin(*(xt++), *(yt + 1))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pg++) = channel2;

                    channel = (unsigned char)
                        (tb - (bsign * wamin(*(xt++), *(yt + 2))));
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pb++) = channel2;
                }
            }
        }
    }
#endif // INTERLACE

}


void WaImage::cdgradient(void) {
    // cross diagonal gradient -  based on original dgradient, written by
    // Mosfet (mosfet@kde.org)
    // adapted from kde sources for Blackbox by Brad Hughes

    float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
        xr = (float) from->getRed(),
        xg = (float) from->getGreen(),
        xb = (float) from->getBlue();
    unsigned char *pr = red, *pg = green, *pb = blue;
    unsigned int w = width * 2, h = height * 2, *xt, *yt;

    register unsigned int x, y;

    dry = drx = (float) (to->getRed() - from->getRed());
    dgy = dgx = (float) (to->getGreen() - from->getGreen());
    dby = dbx = (float) (to->getBlue() - from->getBlue());

    // Create X table
    drx /= w;
    dgx /= w;
    dbx /= w;

    for (xt = (xtable + (width * 3) - 1), x = 0; x < width; x++) {
        *(xt--) = (unsigned char) xb;
        *(xt--) = (unsigned char) xg;
        *(xt--) = (unsigned char) xr;

        xr += drx;
        xg += dgx;
        xb += dbx;
    }

    // Create Y table
    dry /= h;
    dgy /= h;
    dby /= h;

    for (yt = ytable, y = 0; y < height; y++) {
        *(yt++) = (unsigned char) yr;
        *(yt++) = (unsigned char) yg;
        *(yt++) = (unsigned char) yb;

        yr += dry;
        yg += dgy;
        yb += dby;
    }

    // Combine tables to create gradient

#ifdef    INTERLACE
    if (! interlaced) {
#endif // INTERLACE

        // normal cdgradient
        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                *(pr++) = *(xt++) + *(yt);
                *(pg++) = *(xt++) + *(yt + 1);
                *(pb++) = *(xt++) + *(yt + 2);
            }
        }

#ifdef    INTERLACE
    } else {
        // faked interlacing effect
        unsigned char channel, channel2;

        for (yt = ytable, y = 0; y < height; y++, yt += 3) {
            for (xt = xtable, x = 0; x < width; x++) {
                if (y & 1) {
                    channel = *(xt++) + *(yt);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pr++) = channel2;

                    channel = *(xt++) + *(yt + 1);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pg++) = channel2;

                    channel = *(xt++) + *(yt + 2);
                    channel2 = (channel >> 1) + (channel >> 2);
                    if (channel2 > channel) channel2 = 0;
                    *(pb++) = channel2;
                } else {
                    channel = *(xt++) + *(yt);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pr++) = channel2;

                    channel = *(xt++) + *(yt + 1);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pg++) = channel2;

                    channel = *(xt++) + *(yt + 2);
                    channel2 = channel + (channel >> 3);
                    if (channel2 < channel) channel2 = ~0;
                    *(pb++) = channel2;
                }
            }
        }
    }
#endif // INTERLACE
}


WaImageControl::WaImageControl(Display *dpy, WaScreen *scrn, bool _dither,
                               int _cpc, unsigned long cmax) {
    display       = dpy;
    screen_number = scrn->screen_number;
    screen_depth  = DefaultDepth(display, screen_number);
    window        = RootWindow(display, screen_number);
    colormap      = DefaultColormap(display, screen_number);
    visual        = DefaultVisual(display, screen_number);
    wascreen      = scrn;
    
    setDither(_dither);
    setColorsPerChannel(_cpc);
    
    cache_max = cmax;
    
    colors = (XColor *) 0;
    ncolors = 0;
        
    grad_xbuffer = grad_ybuffer = (unsigned int *) 0;
    grad_buffer_width = grad_buffer_height = 0;
    
    sqrt_table = (unsigned long *) 0;
    
    int count;
    XPixmapFormatValues *pmv = XListPixmapFormats(display,
                                                  &count);    
    if (pmv) {
        bits_per_pixel = 0;
        for (int i = 0; i < count; i++)
            if (pmv[i].depth == screen_depth) {
                bits_per_pixel = pmv[i].bits_per_pixel;
                break;
            } 
        XFree(pmv);
    }
    
    if (bits_per_pixel == 0) bits_per_pixel = screen_depth;
    if (bits_per_pixel >= 24) setDither(false);
    
    red_offset = green_offset = blue_offset = 0;
    
    switch (getVisual()->c_class) {
        case TrueColor: {
            int i;
            
            // compute color tables
            unsigned long red_mask = getVisual()->red_mask,
                green_mask = getVisual()->green_mask,
                blue_mask = getVisual()->blue_mask;
            
            while (! (red_mask & 1)) { red_offset++; red_mask >>= 1; }
            while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
            while (! (blue_mask & 1)) { blue_offset++; blue_mask >>= 1; }
            
            red_bits = 255 / red_mask;
            green_bits = 255 / green_mask;
            blue_bits = 255 / blue_mask;
            
            for (i = 0; i < 256; i++) {
                red_color_table[i] = i / red_bits;
                green_color_table[i] = i / green_bits;
                blue_color_table[i] = i / blue_bits;
            }
            break;
        }
        case PseudoColor:
        case StaticColor: {
            ncolors = colors_per_channel * colors_per_channel *
                colors_per_channel;
            
            if (ncolors > (1 << screen_depth)) {
                colors_per_channel = (1 << screen_depth) / 3;
                ncolors = colors_per_channel * colors_per_channel *
                    colors_per_channel;
            }
            
            if (colors_per_channel < 2 || ncolors > (1 << screen_depth)) {
                WARNING << "invalid colormap size " << ncolors << " (" <<
                    colors_per_channel << "/" << colors_per_channel <<
                    colors_per_channel << " - reducing" << endl;
                
                colors_per_channel = (1 << screen_depth) / 3;
            }
            
            colors = new XColor[ncolors];
            if (! colors) {
                ERROR << "error allocating colormap" << endl; quit(1);
            }
            
#ifdef    ORDEREDPSEUDO
            int i = 0, ii, p, r, g, b, bits = 256 / colors_per_channel;
#else // !ORDEREDPSEUDO
            int i = 0, ii, p, r, g, b, bits = 255 / (colors_per_channel - 1);
#endif // ORDEREDPSEUDO
            
            red_bits = green_bits = blue_bits = bits;
            
            for (i = 0; i < 256; i++)
                red_color_table[i] = green_color_table[i] =
                    blue_color_table[i] = i / bits;
            
            for (r = 0, i = 0; r < colors_per_channel; r++) {
                for (g = 0; g < colors_per_channel; g++) {
                    for (b = 0; b < colors_per_channel; b++, i++) {
                        colors[i].red = (r * 0xffff) /
                            (colors_per_channel - 1);
                        colors[i].green = (g * 0xffff) /
                            (colors_per_channel - 1);
                        colors[i].blue = (b * 0xffff) /
                            (colors_per_channel - 1);
                        colors[i].flags = DoRed|DoGreen|DoBlue;
                    }
                }
            }
            
            XGrabServer(display);
            
            for (i = 0; i < ncolors; i++)
                if (! XAllocColor(display, colormap, &colors[i])) {
                    WARNING << "couldn't alloc color " << colors[i].red <<
                        " " << colors[i].green << " " << colors[i].blue <<
                        endl;
                    colors[i].flags = 0;
                } else
                    colors[i].flags = DoRed|DoGreen|DoBlue;
            
            XUngrabServer(display);
            
            XColor icolors[256];
            int incolors = (((1 << screen_depth) > 256) ? 256 :
                            (1 << screen_depth));
                
            for (i = 0; i < incolors; i++)
                icolors[i].pixel = i;
            
            XQueryColors(display, colormap, icolors, incolors);
            for (i = 0; i < ncolors; i++) {
                if (! colors[i].flags) {
                    unsigned long chk = 0xffffffff, pixel, close = 0;
                        
                    p = 2;
                    while (p--) {
                        for (ii = 0; ii < incolors; ii++) {
                            r = (colors[i].red - icolors[i].red) >> 8;
                            g = (colors[i].green - icolors[i].green) >> 8;
                            b = (colors[i].blue - icolors[i].blue) >> 8;
                            pixel = (r * r) + (g * g) + (b * b);
                                
                            if (pixel < chk) {
                                chk = pixel;
                                close = ii;
                            }
                            
                            colors[i].red = icolors[close].red;
                            colors[i].green = icolors[close].green;
                            colors[i].blue = icolors[close].blue;
                                
                            if (XAllocColor(display, colormap,
                                            &colors[i])) {
                                colors[i].flags = DoRed|DoGreen|DoBlue;
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }
        case GrayScale:
        case StaticGray: {
            if (getVisual()->c_class == StaticGray) {
                ncolors = 1 << screen_depth;
            }
            else {
                ncolors = colors_per_channel * colors_per_channel *
                    colors_per_channel;
                
                if (ncolors > (1 << screen_depth)) {
                    colors_per_channel = (1 << screen_depth) / 3;
                    ncolors =
                        colors_per_channel * colors_per_channel *
                        colors_per_channel;
                }
            }
            
            if (colors_per_channel < 2 || ncolors > (1 << screen_depth)) {
                WARNING << "invalid colormap size " << ncolors << " (" <<
                    colors_per_channel << "/" << colors_per_channel <<
                    colors_per_channel << " - reducing" << endl;
                
                colors_per_channel = (1 << screen_depth) / 3;
            }
            
            colors = new XColor[ncolors];
            if (! colors) {
                ERROR << "error allocating colormap" << endl; quit(1);
            }
            
            int i = 0, ii, p, bits = 255 / (colors_per_channel - 1);
            red_bits = green_bits = blue_bits = bits;
            
            for (i = 0; i < 256; i++)
                red_color_table[i] = green_color_table[i] =
                    blue_color_table[i] = i / bits;
            
            XGrabServer(display);
            for (i = 0; i < ncolors; i++) {
                colors[i].red = (i * 0xffff) / (colors_per_channel - 1);
                colors[i].green = (i * 0xffff) / (colors_per_channel - 1);
                colors[i].blue = (i * 0xffff) / (colors_per_channel - 1);;
                colors[i].flags = DoRed|DoGreen|DoBlue;
                
                if (! XAllocColor(display, colormap, &colors[i])) {
                    WARNING << "couldn't alloc color " << colors[i].red <<
                        " " << colors[i].green << " " << colors[i].blue <<
                        endl;
                    colors[i].flags = 0;
                } else
                    colors[i].flags = DoRed|DoGreen|DoBlue;
            }
            
            XUngrabServer(display);
            
            XColor icolors[256];
            int incolors = (((1 << screen_depth) > 256) ? 256 :
                            (1 << screen_depth));
            
            for (i = 0; i < incolors; i++)
                icolors[i].pixel = i;
            
            XQueryColors(display, colormap, icolors, incolors);
            for (i = 0; i < ncolors; i++) {
                if (! colors[i].flags) {
                    unsigned long chk = 0xffffffff, pixel, close = 0;
                    
                    p = 2;
                    while (p--) {
                        for (ii = 0; ii < incolors; ii++) {
                            int r = (colors[i].red - icolors[i].red) >> 8;
                            int g = (colors[i].green - icolors[i].green) >> 8;
                            int b = (colors[i].blue - icolors[i].blue) >> 8;
                            pixel = (r * r) + (g * g) + (b * b);
                            
                            if (pixel < chk) {
                                chk = pixel;
                                close = ii;
                            }
                            
                            colors[i].red = icolors[close].red;
                            colors[i].green = icolors[close].green;
                            colors[i].blue = icolors[close].blue;
                            
                            if (XAllocColor(display, colormap,
                                            &colors[i])) {
                                colors[i].flags = DoRed|DoGreen|DoBlue;
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }    
        default:
            ERROR << "unsupported visual " << getVisual()->c_class << endl;
            quit(1);
    }
    cache = new list<Cache *>;
}


WaImageControl::~WaImageControl(void) {
    XSync(wascreen->display, false);
    if (sqrt_table) {
        delete [] sqrt_table;
    }
    if (grad_xbuffer) {
        delete [] grad_xbuffer;
    }
    if (grad_ybuffer) {
        delete [] grad_ybuffer;
    }
    if (colors) {
        unsigned long *pixels = new unsigned long [ncolors];   
        int i;
        for (i = 0; i < ncolors; i++)
            *(pixels + i) = (*(colors + i)).pixel;
        
        XFreeColors(display, colormap, pixels, ncolors, 0);
        
        delete [] colors;
    }
    if (! cache->empty()) {
        int i, n = cache->size();
        
        for (i = 0; i < n; i++) {
            Cache *tmp = cache->front();
            XFreePixmap(display, tmp->pixmap);
            cache->pop_front();
            delete tmp;
        }
    }
    delete cache;
    XSync(wascreen->display, false);
    XSync(wascreen->pdisplay, false);
}


Pixmap WaImageControl::searchCache(unsigned int width, unsigned int height,
                                   unsigned long texture,
                                   WaColor *c1, WaColor *c2) {
    if (! cache->empty()) {
        list<Cache *>::iterator it = cache->begin();
        for (; it != cache->end(); ++it) {
            if (((*it)->width == width) &&
                ((*it)->height == height) &&
                ((*it)->texture == texture) &&
                ((*it)->pixel1 == c1->getPixel()))
                if (texture & WaImage_Gradient) {
                    if ((*it)->pixel2 == c2->getPixel()) {
                        (*it)->count++;
                        return (*it)->pixmap;
                    }
                } else {
                    (*it)->count++;
                    return (*it)->pixmap;
                }
        }
    }
    return None;
}


Pixmap WaImageControl::renderImage(unsigned int width, unsigned int height,
                                   WaTexture *texture, Pixmap parent,
                                   unsigned int src_x, unsigned int src_y,
                                   Pixmap dest) {
    Pixmap retp;
    if (texture->getTexture() & WaImage_ParentRelative) return ParentRelative;
    
    XSync(wascreen->display, false);
    
    Pixmap pixmap = searchCache(width, height, texture->getTexture(),
                                texture->getColor(), texture->getColorTo());
    if (pixmap) {
        
#ifdef XRENDER
        retp = xrender(pixmap, width, height, texture, parent, src_x, src_y,
                       dest);
#else // !XRENDER
        retp = pixmap;
#endif // XRENDER
        
        XSync(wascreen->display, false);
        XSync(wascreen->pdisplay, false);
        return retp;
    }

    WaImage image(this, width, height);
    pixmap = image.render(texture);

#ifdef PIXMAP
    if (texture->getTexture() & WaImage_Pixmap) {
        
#ifdef XRENDER
        retp = xrender(pixmap, width, height, texture, parent, src_x, src_y,
                       dest);
#else // !XRENDER
        retp = pixmap;
#endif // XRENDER

        XSync(wascreen->display, false);
        XSync(wascreen->pdisplay, false);
        return retp;
    }
#endif // PIXMAP

    if (pixmap) {
        Cache *tmp = new Cache;
        
        tmp->pixmap = pixmap;
        tmp->width = width;
        tmp->height = height;
        tmp->count = 1;
        tmp->texture = texture->getTexture();
        tmp->pixel1 = texture->getColor()->getPixel();
        
        if (texture->getTexture() & WaImage_Gradient)
            tmp->pixel2 = texture->getColorTo()->getPixel();
        else
            tmp->pixel2 = 0l;
        
        cache->push_front(tmp);
        
        if ((unsigned) cache->size() > cache_max)
            timeout();
                
#ifdef XRENDER
        retp = xrender(pixmap, width, height, texture, parent, src_x, src_y,
                       dest);
#else // !XRENDER
        retp = pixmap;
#endif // XRENDER

        XSync(wascreen->display, false);
        XSync(wascreen->pdisplay, false);
        return retp;
    }
    XSync(wascreen->display, false);
    XSync(wascreen->pdisplay, false);
    return None;
}


void WaImageControl::removeImage(Pixmap pixmap) {
    if (pixmap) {
        list<Cache *>::iterator it = cache->begin();
        for (; it != cache->end(); ++it) {
            if ((*it)->pixmap == pixmap) {
                Cache *tmp = *it;
                
                if (tmp->count) {
                    tmp->count--;   
                    if (! tmp->count) timeout();
                }                
                return;
            }
        }
    }
}

unsigned long WaImageControl::getColor(const char *colorname,
                                       unsigned short *r, unsigned short *g,
                                       unsigned short *b) {
    XColor color;
    color.pixel = 0;
    
    if (! XParseColor(display, colormap, colorname, &color))
        WARNING << "color parse error: \"" << colorname << "\"" << endl;
    else if (! XAllocColor(display, colormap, &color))
        WARNING << "color alloc error: \"" << colorname << "\"" << endl;
    
    *r = color.red;
    *g = color.green;
    *b = color.blue;
    
    return color.pixel;
}

unsigned long WaImageControl::getColor(const char *colorname) {
    XColor color;
    color.pixel = 0;
    
    if (! XParseColor(display, colormap, colorname, &color))
        WARNING << "color parse error: \"" << colorname << "\"" << endl;
    else if (! XAllocColor(display, colormap, &color))
        WARNING << "color alloc error: \"" << colorname << "\"" << endl;
    
    return color.pixel;
}

void WaImageControl::getColorTables(unsigned char **rmt, unsigned char **gmt,
                                    unsigned char **bmt,
                                    int *roff, int *goff, int *boff,
                                    int *rbit, int *gbit, int *bbit) {
    if (rmt) *rmt = red_color_table;
    if (gmt) *gmt = green_color_table;
    if (bmt) *bmt = blue_color_table;
    
    if (roff) *roff = red_offset;
    if (goff) *goff = green_offset;
    if (boff) *boff = blue_offset;
    
    if (rbit) *rbit = red_bits;
    if (gbit) *gbit = green_bits;
    if (bbit) *bbit = blue_bits;
}

void WaImageControl::getXColorTable(XColor **c, int *n) {
    if (c) *c = colors;
    if (n) *n = ncolors;
}

void WaImageControl::getGradientBuffers(unsigned int w,
                                        unsigned int h,
                                        unsigned int **xbuf,
                                        unsigned int **ybuf) {
    if (w > grad_buffer_width) {
        if (grad_xbuffer) {
            delete [] grad_xbuffer;
        }
        
        grad_buffer_width = w;
        
        grad_xbuffer = new unsigned int[grad_buffer_width * 3];
    }
    
    if (h > grad_buffer_height) {
        if (grad_ybuffer) {
            delete [] grad_ybuffer;
        }
        
        grad_buffer_height = h;
        
        grad_ybuffer = new unsigned int[grad_buffer_height * 3];
    }
    
    *xbuf = grad_xbuffer;
    *ybuf = grad_ybuffer;
}

void WaImageControl::installRootColormap(void) {
    XSync(wascreen->display, false);
    
    bool install = true;
    int i = 0, ncmap = 0;
    Colormap *cmaps =
        XListInstalledColormaps(display, window, &ncmap);

    if (cmaps) {
        for (i = 0; i < ncmap; i++)
            if (*(cmaps + i) == colormap)
                install = false;
        
        if (install)
            XInstallColormap(display, colormap);
        
        XFree(cmaps);
    }   
    XSync(wascreen->display, false);
    XSync(wascreen->pdisplay, false);
}

void WaImageControl::setColorsPerChannel(int cpc) {
    if (cpc < 2) cpc = 2;
    if (cpc > 6) cpc = 6;
    
    colors_per_channel = cpc;
}

unsigned long WaImageControl::getSqrt(unsigned int x) {
    if (! sqrt_table) {
        // build sqrt table for use with elliptic gradient
        
        sqrt_table = new unsigned long[(256 * 256 * 2) + 1];
        int i = 0;
        
        for (; i < (256 * 256 * 2); i++)
            *(sqrt_table + i) = bsqrt(i);
    }   
    return (*(sqrt_table + x));
}

void WaImageControl::parseTexture(WaTexture *texture, char *t) {
    if ((! texture) || (! t)) return;
    
    int t_len = strlen(t) + 1, i;
    char *ts = new char[t_len];
    if (! ts) return;
    
    // convert to lower case
    for (i = 0; i < t_len; i++)
        *(ts + i) = tolower(*(t + i));

    if (strstr(ts, "parentrelative")) {
        texture->setTexture(WaImage_ParentRelative);
        
#ifdef PIXMAP
    } else if (strstr(ts, "pixmap")) {
        texture->setTexture(0);
        texture->addTexture(WaImage_Pixmap);
        if (strstr(ts, "scaled"))
            texture->addTexture(WaImage_Scale);
        else if (strstr(ts, "stretched"))
            texture->addTexture(WaImage_Stretch);
        else
            texture->addTexture(WaImage_Tile);
#endif // PIXMAP
        
    } else {
        texture->setTexture(0);
        
        if (strstr(ts, "solid"))
            texture->addTexture(WaImage_Solid);
        else if (strstr(ts, "gradient")) {
            texture->addTexture(WaImage_Gradient);
            if (strstr(ts, "crossdiagonal"))
                texture->addTexture(WaImage_CrossDiagonal);
            else if (strstr(ts, "rectangle"))
                texture->addTexture(WaImage_Rectangle);
            else if (strstr(ts, "pyramid"))
                texture->addTexture(WaImage_Pyramid);
            else if (strstr(ts, "pipecross"))
                texture->addTexture(WaImage_PipeCross);
            else if (strstr(ts, "elliptic"))
                texture->addTexture(WaImage_Elliptic);
            else if (strstr(ts, "diagonal"))
                texture->addTexture(WaImage_Diagonal);
            else if (strstr(ts, "horizontal"))
                texture->addTexture(WaImage_Horizontal);
            else if (strstr(ts, "vertical"))
                texture->addTexture(WaImage_Vertical);
            else
                texture->addTexture(WaImage_Diagonal);
        } else
            texture->addTexture(WaImage_Solid);
        
        if (strstr(ts, "raised"))
            texture->addTexture(WaImage_Raised);
        else if (strstr(ts, "sunken"))
            texture->addTexture(WaImage_Sunken);
        else if (strstr(ts, "flat"))
            texture->addTexture(WaImage_Flat);
        else
            texture->addTexture(WaImage_Raised);
        
        if (! (texture->getTexture() & WaImage_Flat))
            if (strstr(ts, "bevel2"))
                texture->addTexture(WaImage_Bevel2);
            else
                texture->addTexture(WaImage_Bevel1);
        
#ifdef    INTERLACE
        if (strstr(ts, "interlaced"))
            texture->addTexture(WaImage_Interlaced);
#endif // INTERLACE
    }
    
    delete [] ts;
}

void WaImageControl::parseColor(WaColor *color, char *c) {
    if (! color) return;

    if (color->isAllocated()) {
        unsigned long pixel = color->getPixel();
        
        XFreeColors(display, colormap, &pixel, 1, 0);
        
        color->setPixel(0l);
        color->setRGB(0, 0, 0);
        color->setAllocated(false);
    }
    if (c) {
        unsigned short r, g, b;    
       
        color->setPixel(getColor(c, &r, &g, &b));
        color->setRGB(r, g, b);
        color->setAllocated(true);
    }
}

void WaImageControl::timeout(void) {
    list<Cache *>::iterator it = cache->begin();
    for (; it != cache->end(); ++it) {
        Cache *tmp = *it;
        
        if (tmp->count <= 0) {
            XFreePixmap(display, tmp->pixmap);
            cache->remove(tmp);
            delete tmp;
        }
    }
}

#ifdef XRENDER
Pixmap WaImageControl::xrender(Pixmap p, unsigned int width,
                               unsigned int height, WaTexture *texture,
                               Pixmap parent, unsigned int src_x,
                               unsigned int src_y,
                               Pixmap dest) {
    Picture src_pict, dest_pict;
    XRenderPictFormat *format;
    
    if ((! texture->getOpacity()) || parent == None || dest == None)
        return p;

    XSync(wascreen->display, false);
    
    GC gc = DefaultGC(display, screen_number);
    XCopyArea(display, parent, dest, gc, src_x, src_y, width,
              height,  0, 0);

    if (texture->getOpacity() == 255) return dest;

    format = XRenderFindVisualFormat(display, visual);
    if (p == None)
        src_pict = texture->getSolidPicture();
    else
        src_pict = XRenderCreatePicture(display, (Drawable) p, format, 0, 0);
    dest_pict = XRenderCreatePicture(display, (Drawable) dest, format, 0, 0);
    XRenderComposite(display, PictOpOver, src_pict,
                     texture->getAlphaPicture(), dest_pict, 0, 0, 0, 0, 0, 0,
                     width, height);
    if (p != None) XRenderFreePicture(display, src_pict);
    XRenderFreePicture(display, dest_pict);
    XSync(wascreen->display, false);
    XSync(wascreen->pdisplay, false);
    return dest;
}

void WaImageControl::setXRootPMapId(bool hrp) { have_root_pmap = hrp; }

#endif // XRENDER
