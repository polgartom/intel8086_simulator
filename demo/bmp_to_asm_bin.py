from PIL import Image
import sys
import struct

header = "" 
def read_rows(path):
    global header

    image_file = open(path, "rb")
    # Blindly skip the BMP header.
    header = image_file.read(72)

    # We need to read pixels in as rows to later swap the order
    # since BMP stores pixels starting at the bottom left.
    rows = []
    row = []
    pixel_index = 0

    width = 128
    height = 128
    byte = 2

    r = []

    while True:
        if pixel_index == width:
            pixel_index = 0
            rows.insert(0, row)
            if len(row) != width * byte:
                raise Exception("Row length is not width*byte but " + str(len(row)) + " / byte = " + str(len(row) / byte))
            row = []
        pixel_index += 1

        b1 = image_file.read(1)
        b2 = image_file.read(1)

        if len(b1) == 0:
            # This is expected to happen when we've read everything.
            break

        if len(b2) == 0:
            break

        b1_num = ord(b1)
        b2_num = ord(b2)

        r.append( ((b2_num<<8)|b1_num) & 0xFFFF )

        row.append(b1_num)
        row.append(b2_num)

    image_file.close()

    return r

f = sys.argv[1]
val = read_rows(f)

x = map(str, val)
with open('./jura.dat', "w") as f:
    for i, v in enumerate(x):
        s = ''
        if (i % 128) == 0:
            if (i != 0): s = "\n"
            s += "dw "
        else:
            s = ', '
        s += v
        
        f.write(s)

"""
with open('./out.bmp', 'wb') as f:
    f.write(header)
    for b in val:
        f.write(struct.pack('H', b)) 
"""