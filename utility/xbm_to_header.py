import sys
import os
from pathlib import Path

def flip_endian(in_byte):
    out_byte = 0
    for i in range(8):
        if in_byte & (1<<i):
            out_byte = out_byte | (1<<(7-i))
    return out_byte

def convert_xbm(xbm_array):
    ## Deprecated, use 
    # Create 128x296 image in inverted colors
    # Use Gimp to save as 1-bit .xbm
    # Get the array from that file and feed it to this function
    counter = 0
    for i in xbm_array:
        if counter%16 == 0:
            print('\n\t', end='')
        print("0x%02x, " % flip_endian(i), end='')
        counter += 1

def invert(xbm_array):
    # Create 128x296 image in inverted colors
    # Use Gimp to save as 1-bit .xbm
    # Get the array from that file and feed it to this function
    counter = 0
    for i in xbm_array:
        if counter%16 == 0:
            print('\n\t', end='')
        print("0x%02x, " % flip_endian(i^0xff), end='')
        counter += 1

def rotate_ccw(xbm_filename):
    '''
    Give this an image.xbm that is 296x128 and it will return
    an array that represents a 128x296 version of the image that
    has been rotated 90 degrees counter-clockwise
    '''
    
    with open(xbm_filename) as f:
        content = f.read()
        
    array = [int(x, 16) for x in content.replace('\n', "").strip().split("{")[1].split("}")[0].split(",")]

    flipped_array = [0] * 16 * 296

    for stripe in range(16):
        #process 8 rows at a time for bitwise conversion to bytes
        for block in range(37):
            #process 8 columns at a time
            for bit in range(8):
                out_byte = 0
                for i in range(8):
                    #Iterate each row, masking into bits to form one rotated byte
                    if (array[(36-block) + (i*37) + (stripe*296)] & (1<<(7-bit))):
                        out_byte |= 1<<i
                flipped_array[(block*16*8) + (bit*16) + stripe] = out_byte
    return flipped_array

def invert_array(arr):
    inv_arr = list()
    for n in arr:
        inv_arr.append(n^0xFF)
    return inv_arr

def reverse_endian_array(arr):
    rend_arr = list()
    for n in arr:
        rend_arr.append(flip_endian(n))
    return rend_arr

def write_headerfile(pixel_array, outfile_name, arr_name='magtag_image'):
    '''
    Takes an array of bytes representing a 128x296 image
    and write it as an c header array
    '''

    with open(outfile_name, "w") as f:
        hex_str = "0x{:02X}, "
        counter = 0
        outstring = 'const unsigned char {}[] = {}'.format(arr_name,'{')
        for i in pixel_array:
            if counter%16 == 0:
                f.write(outstring)
                outstring = '\n\t'
            outstring += hex_str.format(i)
            counter += 1
        f.write(outstring)
        f.write('\n};\n')

def rotate_invert_flipendian(xbm_filename):
    arr = rotate_ccw(xbm_filename)
    arr = invert_array(arr)
    arr = reverse_endian_array(arr)
    filename_stub=Path(xbm_filename).stem
    headerfile = os.path.join(os.getcwd(),filename_stub + '.h')
    print("Generating: " + headerfile)
    write_headerfile(arr, headerfile, filename_stub)

def main(argv):
    if len(sys.argv) != 2:
        print("\nUsage: python3 xbm_to_header.py filename.xbm\n")
        print("\tConvert a 296x128 .xbm image file to a header file for the magtag\n")
    else:
        try:
            filename = sys.argv[1]
            rotate_invert_flipendian(sys.argv[1])
        except Exception as e:
            print(e)

if __name__ == "__main__":
   main(sys.argv[1:])

