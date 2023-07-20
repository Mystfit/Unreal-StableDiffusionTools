import unreal
import os, inspect, random, io
import PIL
from PIL import Image
from diffusionconvertors import FColorAsPILImage, PILImageToTexture
import requests
import base64
from io import BytesIO


@unreal.uclass()
class StableHordeBridge(unreal.StableDiffusionBridge):
    API_URL = "https://stablehorde.net/api/v2"

    def __init__(self):
        unreal.StableDiffusionBridge.__init__(self)

    @unreal.ufunction(override=True)
    def GetTokenWebsiteHint(self):
        return "https://stablehorde.net/register"

    @unreal.ufunction(override=True)
    def GetRequiresToken(self):
        return True

    @unreal.ufunction(override=True)
    def InitModel(self, new_model_options, new_pipeline_options, lora_asset, textual_inversion_asset, layers, allow_nsfw, padding_mode):
        result = unreal.StableDiffusionModelInitResult()
        self.model_loaded = True
        headers = {
            "accept": "application/json",
            "apikey": self.get_token()
        }
        response = requests.get(f"{StableHordeBridge.API_URL}/find_user", headers=headers)
        if response.status_code != 200:
            unreal.log_error(f"No Stable Horde user found with the provided token {self.get_token()}. Response code was {response.status_code} and message was \"{response.json()['message']}\"")
            self.model_loaded = False

        self.set_editor_property("ModelOptions", new_model_options)
        self.set_editor_property("PipelineOptions", new_pipeline_options)

        result.model_status = unreal.ModelStatus.LOADED
        self.set_editor_property("ModelStatus", result)
        return result

    @unreal.ufunction(override=True)
    def ReleaseModel(self):
        pass    

    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, input, out_texture, preview_texture):
        result = unreal.StableDiffusionImageResult()
        
        guide_img = FColorAsPILImage(input.input_image_pixels, input.options.size_x, input.options.size_y).convert("RGB") if input.input_image_pixels else None
        mask_img = FColorAsPILImage(input.mask_image_pixels, input.options.size_x, input.options.size_y).convert("RGB")  if input.mask_image_pixels else None       
        guide_img = guide_img.resize((512,512))
        mask_img = mask_img.resize((512,512))

        seed = [random.randrange(0, 4294967295)] if input.options.seed < 0 else input.options.seed
        positive_prompts = ", ".join([f"{split_p.strip()}" for prompt in input.options.positive_prompts for split_p in prompt.prompt.split(",")])
        print(positive_prompts)
        if not positive_prompts:
            unreal.log_error("Stable Horde does not work with empty prompts")
            return result

        # Round strength to two decimal places
        strength = 0.01 * round(input.options.strength / 0.01)

        # Convert input images to base64 for uploading
        img_buffer = BytesIO()
        guide_img.save(img_buffer, format="JPEG")
        guide_img_str = base64.b64encode(img_buffer.getvalue()).decode("utf-8") 

        mask_buffer = BytesIO()
        mask_img.save(img_buffer, format="JPEG")
        mask_img_str = base64.b64encode(mask_buffer.getvalue()).decode("utf-8")

        model_options = self.get_editor_property("ModelOptions")
        
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
            "denoising_strength": strength,
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
          "nsfw": model_options.allow_nsfw,
          "trusted_workers": True,
          "censor_nsfw": not model_options.allow_nsfw,
          "source_image": guide_img_str,
          "source_processing": "img2img"
        }
        headers = {
            "accept": "application/json",
            "apikey": self.get_token(),
            "Content-Type": "application/json"
        }

        # Post to Stable Horde
        response = requests.post(f"{StableHordeBridge.API_URL}/generate/sync", json=request, headers=headers)

        # Decode returned base64 image back to a PIL iamge
        image = None
        if response.status_code == 200:
            image = Image.open(BytesIO(base64.b64decode(response.json()["generations"][0]["img"]))).convert("RGBA")
        else:
            unreal.log_error(f"Stable Horde returned an error. Status code was {response.status_code}. Message was {response.json()['message']}")
            unreal.log_error(f"Error was {response.json()['errors']}")
            unreal.log_error(f"Input headers were {headers}")
            unreal.log_error(f"Input data was {request}")

        result.input = input
        result.pixel_data = PILImageToTexture(image.convert("RGBA"), out_texture)
        result.out_width = image.width
        result.out_height = image.height

        return result

    @unreal.ufunction(override=True)
    def StartUpsample(self):
        pass

    @unreal.ufunction(override=True)
    def StopUpsample(self):
        pass

    @unreal.ufunction(override=True)
    def UpsampleImage(self, image_result: unreal.StableDiffusionImageResult, out_texture):
        unreal.log_warning("Upsampler not yet implemented for Stable Horde bridge")

        # Build result
        result = unreal.StableDiffusionImageResult()
        result.input = image_result.input
        result.pixel_data = image_result.pixel_data
        result.out_width = image_result.out_width
        result.out_height = image_result.out_height
        result.generated_texture = None
        result.upsampled = True

        return result