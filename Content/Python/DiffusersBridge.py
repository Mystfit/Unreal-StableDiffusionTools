import unreal
import os
import numpy as np
import huggingface_hub
import torch
from torch import autocast
from PIL import Image
from diffusers import StableDiffusionImg2ImgPipeline, StableDiffusionPipeline
from diffusionconvertors import FColorAsPILImage, PILImageToFColorArray


def preprocess_init_image(image: Image, width: int, height: int):
    image = image.resize((width, height), resample=Image.LANCZOS)
    image = np.array(image).astype(np.float32) / 255.0
    image = image[None].transpose(0, 3, 1, 2)
    image = torch.from_numpy(image)
    return 2.0 * image - 1.0


def preprocess_mask(mask: Image, width: int, height: int):
    mask = mask.convert("L")
    mask = mask.resize((width // 8, height // 8), resample=Image.LANCZOS)
    mask = np.array(mask).astype(np.float32) / 255.0
    mask = np.tile(mask, (4, 1, 1))
    mask = mask[None].transpose(0, 1, 2, 3)  # what does this step do?
    mask = torch.from_numpy(mask)
    return mask


@unreal.uclass()
class DiffusersBridge(unreal.StableDiffusionBridge):
    def __init__(self):
        unreal.StableDiffusionBridge.__init__(self)
        self.pipe = None

    @unreal.ufunction(override=True)
    def InitModel(self, modelname, precision, revision):
        result = False
        try:
            modelname = modelname if modelname else "CompVis/stable-diffusion-v1-4"

            #ActivePipeline = StableDiffusionPipeline
            ActivePipeline = StableDiffusionImg2ImgPipeline

            kwargs = {
                "torch_dtype": torch.float32 if precision == "fp32" else torch.float16,
                "use_auth_token": True
            }
            if revision:
                kwargs["revision"] = precision

            print("Loading Stable Diffusion model " + modelname)
            self.pipe = ActivePipeline.from_pretrained(modelname, **kwargs)
            self.pipe = self.pipe.to("cuda")
            self.pipe.enable_attention_slicing()
            result = True
        except Exception as e:
            print("Failed to init Stable Diffusion Img2Image pipeline. Exception was {0}".format(e))
        return result

    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, prompt, in_frame_width, in_frame_height, frame_width, frame_height, guide_frame, mask_frame, strength, iterations, seed):
        #result = []
        with autocast("cuda"):
            guide_img = preprocess_init_image(FColorAsPILImage(guide_frame, in_frame_width, in_frame_height).convert("RGB"), frame_width, frame_height) if guide_frame else None
            mask_img = preprocess_mask(FColorAsPILImage(mask_frame, in_frame_width, in_frame_height).convert("RGB"), frame_width, frame_height) if mask_frame else None
            generator = torch.Generator("cuda")
            
            seed = torch.random.seed() if seed < 0 else seed
            generator.manual_seed(seed)

            images = self.pipe(prompt=prompt, init_image=guide_img, strength=strength, num_inference_steps=iterations, generator=generator, guidance_scale=7.5, callback=self.ImageProgressStep, callback_steps=10).images

        result = unreal.StableDiffusionImageResult()
        result.prompt = prompt
        result.seed = seed
        result.start_image_size = unreal.IntPoint(in_frame_width, in_frame_height)
        result.generated_image_size = unreal.IntPoint(frame_width, frame_height)
        result.pixel_data =  PILImageToFColorArray(images[0].convert("RGBA"))
        result.generated_texture = None
        #result.input_Texture = None
        #result.mask_texture = None
        #return result
        return result

    def ImageProgressStep(self, step: int, timestep: int, latents: torch.FloatTensor) -> None:
        adjusted_latents = 1 / 0.18215 * latents
        image = self.pipe.vae.decode(adjusted_latents).sample
        image = (image / 2 + 0.5).clamp(0, 1)
        image = image.detach().cpu().permute(0, 2, 3, 1).numpy()
        image = self.pipe.numpy_to_pil(image)[0]
        pixels = PILImageToFColorArray(image.convert("RGBA"))
        self.update_image_progress("inprogress", step, 0, image.width, image.height, pixels)
