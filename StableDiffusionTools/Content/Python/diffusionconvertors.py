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

def PILImageToFColorArray(image):
    output_pixels = []
    for pixel in list(image.getdata()):
        output_pixels.append(unreal.Color(pixel[2], pixel[1], pixel[0], 255))
    return output_pixels
        
def PILImageToTexture(image, out_texture, defer_update):
    if not image or not out_texture:
        return None

    pixels = PILImageToFColorArray(image)
    return unreal.StableDiffusionBlueprintLibrary.color_buffer_to_texture(pixels, unreal.IntPoint(image.width, image.height), out_texture, defer_update)
