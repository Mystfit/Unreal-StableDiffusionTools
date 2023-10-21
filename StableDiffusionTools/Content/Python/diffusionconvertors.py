from typing import Union
from PIL import Image
import PIL
import numpy as np
import math
import unreal

def FColorAsNumpyArray(color_arr: Union[list[unreal.Color], list[unreal.LinearColor]], image_width: int, image_height: int) -> PIL.Image:
    pixels = []

    if not len(color_arr):
        return pixels

    # Detect type hint from first pixel (janky until I figure out type hint introspection)
    is_linear = isinstance(color_arr[0],  unreal.LinearColor)

    for row_idx in range(image_height):
        row = []
        for col_idx in range(image_width):
            pixel = color_arr[row_idx * image_width + col_idx]
            row.append((pixel.r, pixel.g, pixel.b, pixel.a))
        pixels.append(row)
    pix_arr = np.array(pixels, dtype=np.float32) if is_linear else np.array(pixels, dtype=np.uint8)
    return pix_arr

def FColorAsPILImage(color_arr: Union[list[unreal.Color], list[unreal.LinearColor]], image_width: int, image_height: int) -> PIL.Image:
    return Image.fromarray(FColorAsNumpyArray(color_arr, image_width, image_height))

def PILImageToFColorArray(image: PIL.Image.Image) -> list[unreal.Color]:
    output_pixels = []
    for pixel in list(image.convert("RGBA").getdata()):
        output_pixels.append(unreal.Color(pixel[2], pixel[1], pixel[0], pixel[3]))
    return output_pixels

def PILImageToFLinearColorArray(image: PIL.Image.Image) -> list[unreal.LinearColor]:
    output_pixels = []
    for pixel in image.getdata():
        output_pixels.append(unreal.LinearColor(pixel[2], pixel[1], pixel[0], pixel[3]))
    return output_pixels

def NumpyArrayToFLinearColorArray(image_arr: np.array) -> list[unreal.LinearColor]:
    output_pixels = []
    print(f"Numpy array shape is :{image_arr.shape}")
    for row in range(image_arr.shape[0]):
        for col in range(image_arr.shape[1]):
            pixel = image_arr[row][col]
            output_pixels.append(unreal.LinearColor(pixel[0].item(), pixel[1].item(), pixel[2].item(), pixel[3].item()))
    return output_pixels
        
def UpdateTexture(image: Union[PIL.Image.Image, np.array], out_texture: unreal.Texture2D, defer_update: bool, is_linear: bool = False) -> None:
    if image is None or not out_texture:
        print("Can't convert image. No destination texture provided")
        return None

    pixels = []
    width = 0
    height = 0

    if isinstance(image, np.ndarray):
        pixels = NumpyArrayToFLinearColorArray(image)
        width = image.shape[1]
        height = image.shape[0]
    elif isinstance(image, PIL.Image.Image):
        pixels = PILImageToFLinearColorArray(image) if is_linear else PILImageToFColorArray(image)
        width = image.width
        height = image.height

    print(f"UpdateTexture received Image with width:{width} height:{height}")
    if is_linear:
        unreal.StableDiffusionBlueprintLibrary.color_float_buffer_to_texture(pixels, unreal.IntPoint(width, height), out_texture, defer_update)
    else:
        unreal.StableDiffusionBlueprintLibrary.color_buffer_to_texture(pixels, unreal.IntPoint(width, height), out_texture, defer_update)
