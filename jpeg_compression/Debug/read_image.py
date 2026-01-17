import sys

if len(sys.argv) != 3:
    print("Usage: python copy_every_4th_byte.py <input.jpg> <output.jpg>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

with open(input_file, "rb") as fin, open(output_file, "wb") as fout:
    index = 0
    while True:
        byte = fin.read(1)
        if not byte:
            break

        if index % 4 == 0:
            fout.write(byte)

        index += 1

print("Done.")
