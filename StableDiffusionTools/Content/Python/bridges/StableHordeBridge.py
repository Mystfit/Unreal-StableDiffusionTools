import unreal
import os, inspect, random, io
import PIL
from PIL import Image
from diffusionconvertors import FColorAsPILImage, PILImageToFColorArray

import requests
import base64
from io import BytesIO


@unreal.uclass()
class StableHordeBridge(unreal.StableDiffusionBridge):
    def __init__(self):
        unreal.StableDiffusionBridge.__init__(self)

    @unreal.ufunction(override=True)
    def LoginUsingToken(self, token):
        self.set_editor_property("CachedToken", token)
        self.save_properties()
        return True

    @unreal.ufunction(override=True)
    def GetToken(self):
        return self.get_editor_property("CachedToken")

    @unreal.ufunction(override=True)
    def InitModel(self, new_model_options):
        self.set_editor_property("ModelOptions", new_model_options)
        self.model_loaded = True
        return True

    @unreal.ufunction(override=True)
    def ReleaseModel(self):
        pass    

    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, input):
        result = unreal.StableDiffusionImageResult()
        
        guide_img = FColorAsPILImage(input.input_image_pixels, input.options.size_x, input.options.size_y).convert("RGB") if input.input_image_pixels else None
        mask_img = FColorAsPILImage(input.mask_image_pixels, input.options.size_x, input.options.size_y).convert("RGB")  if input.mask_image_pixels else None       
        guide_img = guide_img.resize((512,512))
        mask_img = mask_img.resize((512,512))

        seed = [random.randrange(0, 4294967295)] if input.options.seed < 0 else input.options.seed
        positive_prompts = ", ".join([f"{split_p.strip()}" for prompt in input.options.positive_prompts for split_p in prompt.prompt.split(",")])
        print(positive_prompts)

        # Convert input images to base64 for uploading
        img_buffer = BytesIO()
        guide_img.save(img_buffer, format="JPEG")
        guide_img_str = base64.b64encode(img_buffer.getvalue()).decode("utf-8") 

        mask_buffer = BytesIO()
        mask_img.save(img_buffer, format="JPEG")
        mask_img_str = base64.b64encode(mask_buffer.getvalue()).decode("utf-8")
        
        # Build request and authorization
        request = {
          "prompt": positive_prompts,
          "params": {
            "sampler_name": "k_lms",
            "toggles": [
              1,
              4
            ],
            "cfg_scale": 5,
            "denoising_strength": input.options.strength,
            "seed": str(seed),
            "height": input.options.out_size_y,
            "width": input.options.out_size_x,
            "seed_variation": 1,
            "post_processing": [
              "GFPGAN"
            ],
            "karras": False,
            "steps": input.options.iterations,
            "n": 1
          },
          "nsfw": False,
          "trusted_workers": True,
          "censor_nsfw": False,
          "source_image": guide_img_str,
          "source_processing": "img2img"
        }
        headers = {
            "accept": "application/json",
            "apikey": self.GetToken(),
            "Content-Type": "application/json"
        }

        # Post to Stable Horde
        response = requests.post('https://stablehorde.net/api/v2/generate/sync', json=request, headers=headers)

        # Decode returned base64 image back to a PIL iamge
        print(response.json())
        image = Image.open(BytesIO(base64.b64decode(response.json()["generations"][0]["img"]))).convert("RGBA")

        result.input = input
        result.pixel_data = PILImageToFColorArray(image)
        result.out_width = image.width
        result.out_height = image.height

        return result
