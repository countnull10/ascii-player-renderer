# ascii-player-renderer

A lightweight graphic filter written in C that converts any video into ASCII Art in real-time, using a "parasitic" approach by intercepting graphic calls on X11 (also compatible with modern environments via Xwayland)

## How It Works

The project uses the `LD_PRELOAD` technique to hook into a video player (optimized and tested with Parole on XFCE) by intercepting the `XShmPutImage` function of the X11 display server.
Instead of weighing down the system with complex UI or audio libraries, the filter acts directly on the video frames in RAM:
* **Block Sampling**: Divides the frames into grids (e.g., 8x8 pixels).
* **Luminance Conversion**: Calculates grayscale and maps the pixels to a predefined ASCII character ramp.
* **Optimized Rendering**: Leverages branchless C processing and lookup tables (LUT) to keep CPU overhead extremely low.

## Repository Structure
* **`ascii_filter.c`**: The core filter. Intercepts X11 shared images, loads the 8x8 font, and applies the real-time ASCII conversion on pixel blocks.
* **`shrink_gst.c`**: Experimental/support file for handling GStreamer plugins.

## Requirements
* Linux system with an X11 or Xwayland display server.
* GCC compiler with optimization support (e.g., `-mavx2`).
* X11 development libraries (`libx11-dev`, `libxext-dev`).
* The Parole video player (or compatible alternatives).

## Installation & Running

1. Make sure you have installed *Parole* and your 8x8 font structure is ready (`./alt-8x8/alt-8x8`).
2. Clone the repository, compile the shared library, and run the player with `LD_PRELOAD`:

```bash
"gcc -fPIC -shared -o ascii_filter.so ascii_filter.c -ldl -lX11 -lXext -lGL -mavx2"
"LD_PRELOAD=./ascii_filter.so parole"
