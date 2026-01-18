import numpy as np
from PIL import Image
import argparse
import sys

def calculate_metrics(img1_path, img2_path, diff_path):
    # 1. Load images and convert to Grayscale (L)
    img1 = Image.open(img1_path).convert('L')
    img2 = Image.open(img2_path).convert('L')

    # Ensure they are the same size
    if img1.size != img2.size:
        print(f"Error: Image dimensions do not match! {img1.size} vs {img2.size}")
        sys.exit(1)

    # Convert to numpy arrays for math
    arr1 = np.array(img1).astype(np.float64)
    arr2 = np.array(img2).astype(np.float64)

    # 2. Calculate MSE (Mean Squared Error)
    mse = np.mean((arr1 - arr2) ** 2)
    
    # 3. Calculate PSNR (Peak Signal-to-Noise Ratio)
    if mse == 0:
        psnr = 100 # Identical images
    else:
        max_pixel = 255.0
        psnr = 20 * np.log10(max_pixel / np.sqrt(mse))

    # 4. Generate Difference Map
    # We use absolute difference and multiply by 10 to make 
    # subtle errors visible to the human eye.
    diff = np.abs(arr1 - arr2)
    diff_visual = np.clip(diff * 10, 0, 255).astype(np.uint8)
    Image.fromarray(diff_visual).save(diff_path)

    return mse, psnr

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Calculate Image Compression Error (MSE/PSNR)")
    parser.add_argument("original", help="Path to original lossless image")
    parser.add_argument("reconstructed", help="Path to decompressed image")
    parser.add_argument("diff_out", help="Path to save the visual difference map")
    
    args = parser.parse_args()

    mse, psnr = calculate_metrics(args.original, args.reconstructed, args.diff_out)

    print("-" * 30)
    print(f"MSE:  {mse:.4f}")
    print(f"PSNR: {psnr:.2f} dB")
    print("-" * 30)
    print(f"Difference map saved to: {args.diff_out}")
