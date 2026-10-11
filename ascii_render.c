#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define GRID_SIZE 8
#define FONT_W    8
#define FONT_H    8

// Function pointer for the XShmPutImage function
typedef int (*XShmPutImage_t)(Display*, Drawable, GC, XImage*, int, int, int, int, unsigned int, unsigned int, Bool);

// ASCII ramp ordered by visual density (from darkest/densest to lightest/empty)
static const char ascii_ramp[] = " .:1?L$5%#";
static const int ramp_len = 10;

unsigned char *font_data = NULL;

// Load the 8x8 font file used for character rendering
void load_font() {
    FILE *f = fopen("./alt-8x8/alt-8x8", "rb");
    if (f) {
        font_data = malloc(256 * 8); 
        if (fread(font_data, 1, 256 * 8, f) != 256 * 8) {
            fprintf(stderr, "Error reading the font file\n");
        }
        fclose(f);
    } else {
        fprintf(stderr, "Font file not found!\n");
    }
}

// Intercepted XShmPutImage function
int XShmPutImage(Display *display, Drawable d, GC gc, XImage *image,
                int src_x, int src_y, int dest_x, int dest_y,
                unsigned int width, unsigned int height, Bool send_event) {
    
    // Resolve the original XShmPutImage function pointer dynamically
    static XShmPutImage_t real_XShmPutImage = NULL;
    if (!real_XShmPutImage) real_XShmPutImage = (XShmPutImage_t)dlsym(RTLD_NEXT, "XShmPutImage");
    
    // Load font data on the first available frame
    if (!font_data) {
        load_font();
        if (!font_data) return real_XShmPutImage(display, d, gc, image, src_x, src_y, dest_x, dest_y, width, height, send_event);
    }

    // Static lookup table to map grayscale values (0-255) to ASCII characters
    static unsigned char gray_to_char[256];
    static int lut_initialized = 0;
    if (!lut_initialized) {
        for (int i = 0; i < 256; i++) {
            gray_to_char[i] = ascii_ramp[(i * (ramp_len - 1)) / 255];
        }
        lut_initialized = 1;
    }

    uint32_t *pixels = (uint32_t *)image->data;
    if (!pixels) return real_XShmPutImage(display, d, gc, image, src_x, src_y, dest_x, dest_y, width, height, send_event);
    
    int stride_pixels = image->bytes_per_line / 4;
    int center_offset = (GRID_SIZE >> 1) * stride_pixels + (GRID_SIZE >> 1);

    // Process the video frame block by block
    for (int y = 0; y <= (int)height - GRID_SIZE; y += GRID_SIZE) {
        uint32_t *line_start_ptr = &pixels[y * stride_pixels];

        for (int x = 0; x <= (int)width - GRID_SIZE; x += GRID_SIZE) {
            uint32_t *block_ptr = line_start_ptr + x; 
            uint32_t center_pixel = block_ptr[center_offset];

            // Extract RGB color components from the center pixel
            unsigned char b = center_pixel & 0xFF;
            unsigned char g = (center_pixel >> 8) & 0xFF;
            unsigned char r = (center_pixel >> 16) & 0xFF;

            // Compute grayscale value and map it to an ASCII character
            int gray = ((r << 1) + (g << 2) + g + b) >> 3;
            unsigned char c = gray_to_char[gray];
            const unsigned char *char_glyph = &font_data[c * 8];

            // Branchless rendering: draw the font glyph onto the pixel block
            for (int i = 0; i < 8; i++) {
                uint32_t *dest = &block_ptr[i * stride_pixels];
                unsigned char bits = char_glyph[i];

                dest[0] = center_pixel & -( (bits >> 7) & 1 );
                dest[1] = center_pixel & -( (bits >> 6) & 1 );
                dest[2] = center_pixel & -( (bits >> 5) & 1 );
                dest[3] = center_pixel & -( (bits >> 4) & 1 );
                dest[4] = center_pixel & -( (bits >> 3) & 1 );
                dest[5] = center_pixel & -( (bits >> 2) & 1 );
                dest[6] = center_pixel & -( (bits >> 1) & 1 );
                dest[7] = center_pixel & -( (bits >> 0) & 1 );
            }
        }
    }

    // Forward the modified image to the actual X11 display server
    return real_XShmPutImage(display, d, gc, image, src_x, src_y, dest_x, dest_y, width, height, send_event);
}
