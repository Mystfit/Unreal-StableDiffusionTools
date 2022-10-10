from PIL import Image
import numpy as np
import unreal

def FColorAsPILImage(color_arr, image_width, image_height):
    pixels = []
    for row_idx in range(image_height):
        row = []
        for col_idx in range(image_width):
            pixel = color_arr[row_idx * image_width + col_idx]
            row.append((pixel.r, pixel.g, pixel.b, pixel.a))
        pixels.append(row)

    pix_arr = np.array(pixels, dtype=np.uint8)
    return Image.fromarray(pix_arr)
    #for color in color_arr:
    #    byte_colors.append(color.r.to_bytes(1, "little"))
    #    byte_colors.append(color.g.to_bytes(1, "little"))
    #    byte_colors.append(color.b.to_bytes(1, "little"))
    #    byte_colors.append(color.a.to_bytes(1, "little"))
    #return Image.frombytes("RGBA", (image_width, image_height), byte_colors, 'raw', 'RGBA')

def PILImageToFColorArray(image):
    output_pixels = []
    for pixel in list(image.getdata()):
        output_pixels.append(unreal.Color(pixel[2], pixel[1], pixel[0], 255))
    return output_pixels
        