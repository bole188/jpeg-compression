def print_and_save_jpeg_bytes(
    jpeg_filename,
    txt_filename,
    bytes_per_line=16
):
    with open(jpeg_filename, "rb") as f:
        data = f.read()

    print(f"File: {jpeg_filename}")
    print(f"Size: {len(data)} bytes\n")

    # NOTE: still text mode, but we only write hex digits
    with open(txt_filename, "w") as txt:

        for i in range(0, len(data), bytes_per_line):
            chunk = data[i:i + bytes_per_line]

            # Hex bytes (for console)
            hex_bytes = " ".join(f"{b:02X}" for b in chunk)

            # ASCII view (console only)
            ascii_bytes = "".join(chr(b) if 32 <= b <= 126 else "." for b in chunk)

            line = f"{i:08X}  {hex_bytes:<{bytes_per_line*3}}  {ascii_bytes}"

            # Print to console (UNCHANGED)
            print(line)

        # ---- RAW HEX FILE OUTPUT ----
        for b in data:
            txt.write(f"{b:02X}")


if __name__ == "__main__":
    import sys

    if len(sys.argv) != 3:
        print("Usage: python print_jpeg_bytes.py <image.jpg> <output.txt>")
        sys.exit(1)

    jpeg_file = sys.argv[1]
    txt_file  = sys.argv[2]

    print_and_save_jpeg_bytes(jpeg_file, txt_file)
