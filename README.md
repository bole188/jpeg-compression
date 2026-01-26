# JPEG Compression Implementation

A C-based implementation of the JPEG compression standard, specifically optimized for embedded DSP (Digital Signal Processing) architectures. This project focuses on Discrete Cosine Transform (DCT) algorithms and memory-conscious data handling. It is designed for ADSP-21489 board, SHARC processor. Documentation is given on [this link.](https://www.analog.com/media/en/dsp-documentation/processor-manuals/ADSP-214xx_hwr_rev1.1.pdf)

## Overview

This repository contains a low-level implementation of the JPEG encoding pipeline. The primary goal of this project is to demonstrate efficient image processing techniques, moving from raw pixel data to a compressed bitstream. At the end of the project we analyze the error map image and evaluate the quality of the techniques used for compression and decompression.

The core of the engine is a custom **2D Discrete Cosine Transform (DCT)**
<img width="679" height="92" alt="image" src="https://github.com/user-attachments/assets/09332c02-2e16-490c-b239-4b37b514729a" />\
which has been iteratively optimized for architectures utilizing Dual-Harvard memory structures (separate PM/DM buses).

## Build entities
The project's build process can be divided into the following entities:
*  **Preprocessing**\
     Producing grayscale image pixels out of .bmp image (or .jpg image) which are stored in the C header files.
*  **Main Processing**\
     Implementing JPEG compression in C programming language.
*  **Postprocessing**\
     Transforming the output file to the readable .jpeg image using python scripts. Using python scripts for decompression, as well as for the MSE calculation and error image generation.
-------------------------------------------------------------------
  ## Preprocessing
  In order to get the grayscale pixels of wanted image, we must store those images (.bmp or .jpg format) into the arbitrary directory and run the *[generate_header.py](jpeg_compression/generate_header.py)* with the name of the directory where the input images are located (default=Images).\
  C header files will be named as the corresponding input image and they will be stored strictly in the *Debug* directory.
  The produced header file contains the following information:
*  **Image dimensions**\
     Height and width of the input image are stored.
*  **Pragma directive**\
     States the memory segment in which the following array is stored, hardcoded to the *mem_sram*, described later.
*  **Grayscale pixels**\
     Grayscale pixels are stored into a *data1* variable which is 1 dimensional array of type *const unsigned char*.
  --------------------------------------------------------
  ## Main Processing
  The Main processing pipeline is divided into several sections:
*  **Segmentation**\
     The grayscale image pixels stored in C header file are read and segmented into 8x8 blocks.
*  **Blocks traversal**\
     The following functions operate on blocks individually.
*  **Centering around 0**\
     Each block is centered around 0.
*  **DCT**\
     DCT is executed on each block. This is the core of current processing stage.
*  **Quantization**\
     Previously produced DCT coefficients are divided with values stored in quantization table.
*  **Run-Length Encoding**\
     RLE is used on quantized DCT coefficients to save space.
*  **Huffman Encoding**\
     This part of pipeline maps RL pairs to the predefined Huffman codes.
*  **Serialization**\
     After the program traversed through all blocks, major JPEG markers are written into the output file as well as the most important bitstream which contains our Huffman codes.
-------------------------------------------------------------------------------------------------
  ## Postprocessing
  Since the memory to which the output data is written is word addressed, we need remake the compressed file by reading its every 4th byte.\
  The script used for this is *[read_image.py](jpeg_compression/Debug/read_image.py)* stored in *Debug* directory.\
  Execution of this script requires two arguments:
  *    Path to the input (word adressed) .jpeg file
  *    Path to the output .jpeg file

**Decompression**\
In order to get the decompressed image, the *[decompressing_tool.py](jpeg_compression/Debug/decompressing_tool.py)* must be executed on the output file (first argument). The result is stored in the path noted by the second argument.
**Error map**\
Having both the compressed and the decompressed images, we can generate error map using the *[MSE_calculation.py](jpeg_compression/Debug/MSE_calculation.py)*. Its third argument is the name of the error map image.
