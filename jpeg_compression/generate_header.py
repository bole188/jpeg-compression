import os
from PIL import Image
import numpy as np

def process_images_in_directory(input_dir="Images"):
    """
    @brief Finds all BMP and JPG images in the specified directory, converts them to grayscale,
           and generates a corresponding C header file for each image.

    @param input_dir Path to the directory containing BMP/JPG images.
                     Default value is 'Images'.

    This function iterates over all files in the given directory. If a file has
    the extension .bmp or .jpg, it is opened, converted to grayscale, and its pixel
    data is written to a C header file (.h). The array name in the header file is
    derived from the original image filename (without extension).
    """

    # Check if the input directory exists.
    if not os.path.exists(input_dir):
        print(f"Directory does not exist: {input_dir}")
        return

    # Iterate over files in the directory.
    for filename in os.listdir(input_dir):
         # Process only BMP or JPG files (case-insensitive check).
        if filename.lower().endswith((".bmp", ".jpg", ".jpeg")):
            # Construct the full paths for input and header file output.
            bmp_file_path = os.path.join(input_dir, filename)
            header_file_name = os.path.splitext(filename)[0] + ".h"
            header_file_path = os.path.join(os.getcwd(), "Debug", header_file_name)
            array_name = os.path.splitext(filename)[0]

            try:
                # Open and convert the image to 8-bit grayscale.
                img = Image.open(bmp_file_path)
                img = img.convert("L")
                width, height = img.size
                
                # Extract pixel data as a NumPy array of type uint8.
                pixel_data = np.array(img, dtype=np.uint8)

                # Write the C header file with width, height, and pixel data.
                with open(header_file_path, "w") as f:
                    f.write(f"#ifndef {header_file_name.replace('.', '_').upper()}\n")
                    f.write(f"#define {header_file_name.replace('.', '_').upper()}\n\n")
                    f.write("#include <stdint.h>\n\n")
                    
                    f.write(f"const unsigned int test_image_width = {width};\n")
                    f.write(f"const unsigned int test_image_height = {height};\n\n")
                    f.write(f'#pragma section("seg_block3")\n')
                    f.write(f"const unsigned char data1[] = {{\n")
                    
                    # Write pixel data in hexadecimal format.
                    for row in pixel_data:
                        f.write("    " + ", ".join(f"0x{pixel:02X}" for pixel in row) + ",\n")
                    
                    f.write("};\n\n")
                    f.write(f"#endif // {header_file_name.replace('.', '_').upper()}\n")

                print(f"Header successfully generated: {header_file_path}")


            except Exception as e:
                # Catch and report any errors during file I/O or image processing.
                print(f"Error processing {filename}: {e}")


process_images_in_directory("Images")