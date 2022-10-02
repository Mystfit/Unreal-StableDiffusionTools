import unreal

import os

from PIL import Image
import numpy as np

import torch
from torch import autocast
from diffusers import StableDiffusionImg2ImgPipeline

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
    @unreal.ufunction(override=True)
    def InitModel(self):
        with unreal.ScopedSlowTask(1, "Loading model") as load_task:
            load_task.make_dialog(True)
            load_task.enter_progress_frame(1.0, "Loading model")
            self.pipe = StableDiffusionImg2ImgPipeline.from_pretrained(
                "CompVis/stable-diffusion-v1-4", 
                revision="fp16", 
                torch_dtype=torch.float16,
                use_auth_token=True
            )
            self.pipe = self.pipe.to("cuda")
            self.pipe.enable_attention_slicing()

    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, prompt, frame_width, frame_height, guide_frame, mask_frame, strength, iterations, seed):
        result = []
        with autocast("cuda"):
            guide_img = preprocess_init_image(FColorAsPILImage(guide_frame, frame_width, frame_height).convert("RGB"), frame_width, frame_height) if guide_frame else None
            mask_img = preprocess_mask(FColorAsPILImage(mask_frame, frame_width, frame_height).convert("RGB"), frame_width, frame_height) if mask_frame else None
            generator = torch.Generator("cuda").manual_seed(seed) if seed >= 0 else None

            images = self.pipe(prompt=prompt, init_image=guide_img, strength=strength, num_inference_steps=iterations, generator=generator, guidance_scale=7.5).images
            #guide_img.show()
            #images[0].show()
            #guide_img.save(os.path.join(unreal.Paths.project_content_dir(), prompt + "_guide.png"))
            #images[0].save(os.path.join(unreal.Paths.project_content_dir(), prompt + "_output.png"))
            return PILImageToFColorArray(images[0].convert("RGBA"))
