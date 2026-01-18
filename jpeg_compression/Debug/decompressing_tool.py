import numpy as np
from PIL import Image
import sys
import argparse

def apply_idct_manual(block):    
    res = np.zeros((8, 8))
    for x in range(8):
        for y in range(8):
            sum_val = 0
            for u in range(8):
                for v in range(8):
                    cu = 1/np.sqrt(2) if u == 0 else 1
                    cv = 1/np.sqrt(2) if v == 0 else 1
                    sum_val += cu * cv * block[u, v] * \
                               np.cos((2*x+1)*u*np.pi/16) * \
                               np.cos((2*y+1)*v*np.pi/16)
            res[x, y] = 0.25 * sum_val
    return res

class JPEGBitReader:
    def __init__(self, data):
        self.data = data
        self.byte_pos = 0
        self.bit_buffer = 0
        self.bits_left = 0

    def _get_next_byte(self):
        if self.byte_pos >= len(self.data):
            return None
        byte = self.data[self.byte_pos]
        self.byte_pos += 1
        if byte == 0xFF:
            if self.byte_pos < len(self.data) and self.data[self.byte_pos] == 0x00:
                self.byte_pos += 1
        return byte

    def read_bit(self):
        if self.bits_left == 0:
            next_byte = self._get_next_byte()
            if next_byte is None:
                raise EOFError("End of bitstream reached")
            self.bit_buffer = next_byte
            self.bits_left = 8
        
        bit = (self.bit_buffer >> (self.bits_left - 1)) & 1
        self.bits_left -= 1
        return bit

    def read_bits(self, n):
        val = 0
        for _ in range(n):
            val = (val << 1) | self.read_bit()
        return val

    def decode_huffman(self, huffman_table):
        current_code = 0
        for length in range(1, 17):
            current_code = (current_code << 1) | self.read_bit()
            # We index the table by (length, code) to find the symbol
            if (length, current_code) in huffman_table:
                return huffman_table[(length, current_code)]
        raise ValueError("Invalid Huffman code encountered")

def convert_amplitude(bits, length):
    if length == 0: return 0
    if bits >= (1 << (length - 1)):
        return bits
    else:
        return bits - (1 << length) + 1

class JPEGDecompressor:
    def __init__(self, filepath):
        self.filepath = filepath
        self.quant_table = None
        self.huff_dc = {}
        self.huff_ac = {}
        self.width = 0
        self.height = 0
        self.entropy_data = b""

    def build_huffman_dict(self, code_lengths, symbols):
        huff_dict = {}
        code = 0
        symbol_idx = 0
        for i in range(1, 17):
            l = code_lengths[i-1]
            for _ in range(l):
                # Store with a key of (length, code) for unique identification
                huff_dict[(i, code)] = symbols[symbol_idx]
                code += 1
                symbol_idx += 1
            code <<= 1
        return huff_dict

    def inverse_zigzag(self, zigzagged_list):
        index_map = [
             0,  1,  8, 16,  9,  2,  3, 10,
            17, 24, 32, 25, 18, 11,  4,  5,
            12, 19, 26, 33, 40, 48, 41, 34,
            27, 20, 13,  6,  7, 14, 21, 28,
            35, 42, 49, 56, 57, 50, 43, 36,
            29, 22, 15, 23, 30, 37, 44, 51,
            58, 59, 52, 45, 38, 31, 39, 46,
            53, 60, 61, 54, 47, 55, 62, 63
        ]
        block = np.zeros(64)
        for i in range(64):
            block[index_map[i]] = zigzagged_list[i]
        return block.reshape((8, 8))

    def parse_file(self):
        with open(self.filepath, "rb") as f:
            data = f.read()
        pos = 0
        while pos < len(data):
            if data[pos] == 0xFF:
                marker = data[pos + 1]
                if marker == 0xD8: pos += 2 # SOI
                elif marker == 0xD9: break  # EOI
                elif marker == 0xDB: # DQT
                    table_data = data[pos+5 : pos+5+64]
                    self.quant_table = self.inverse_zigzag(list(table_data))
                    pos += 5 + 64
                elif marker == 0xC0: # SOF0
                    self.height = int.from_bytes(data[pos+5:pos+7], "big")
                    self.width = int.from_bytes(data[pos+7:pos+9], "big")
                    pos += 2 + int.from_bytes(data[pos+2:pos+4], "big")
                elif marker == 0xC4: # DHT
                    length = int.from_bytes(data[pos+2:pos+4], "big")
                    info = data[pos+4]
                    is_ac = (info >> 4) & 1
                    lengths = list(data[pos+5:pos+21])
                    syms = list(data[pos+21:pos+2+length])
                    if is_ac: self.huff_ac = self.build_huffman_dict(lengths, syms)
                    else: self.huff_dc = self.build_huffman_dict(lengths, syms)
                    pos += 2 + length
                elif marker == 0xDA: # SOS
                    length = int.from_bytes(data[pos+2:pos+4], "big")
                    start = pos + 2 + length
                    end = data.find(b'\xff\xd9', start)
                    self.entropy_data = data[start:end]
                    break
                else:
                    pos += 2 + int.from_bytes(data[pos+2:pos+4], "big")
            else: pos += 1

    def run_decompression(self):
        self.parse_file()
        reader = JPEGBitReader(self.entropy_data)
        padded_w, padded_h = (self.width + 7)//8*8, (self.height + 7)//8*8
        canvas = np.zeros((padded_h, padded_w), dtype=np.uint8)
        prev_dc = 0
        try:
            for y in range(0, padded_h, 8):
                for x in range(0, padded_w, 8):
                    # DC
                    dc_size = reader.decode_huffman(self.huff_dc)
                    current_dc = prev_dc + convert_amplitude(reader.read_bits(dc_size), dc_size)
                    # AC
                    coeffs = [0]*64; coeffs[0] = current_dc
                    k = 1
                    while k < 64:
                        symbol = reader.decode_huffman(self.huff_ac)
                        if symbol == 0: break
                        if symbol == 0xF0: k += 16; continue
                        run, size = symbol >> 4, symbol & 0x0F
                        k += run
                        coeffs[k] = convert_amplitude(reader.read_bits(size), size)
                        k += 1
                    # Block Transform
                    block = self.inverse_zigzag(coeffs) * self.quant_table
                    pix = apply_idct_manual(block)
                    canvas[y:y+8, x:x+8] = np.clip(pix + 128, 0, 255)
                    prev_dc = current_dc
        except (EOFError, ValueError): pass
        return canvas[:self.height, :self.width]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="JPEG Grayscale Decompressor")
    parser.add_argument("input", help="Path to input .jpeg file")
    parser.add_argument("output", help="Path to save output .jpg/png file")
    args = parser.parse_args()

    try:
        decompressor = JPEGDecompressor(args.input)
        result_array = decompressor.run_decompression()
        
        img = Image.fromarray(result_array)
        img.save(args.output)
        print(f"Success! Image saved to {args.output}")
    except Exception as e:
        print(f"Error: {e}")
