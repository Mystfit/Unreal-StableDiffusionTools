import unreal
import os
import numpy as np
import torch
from torch import autocast
from PIL import Image
from diffusers import StableDiffusionImg2ImgPipeline, StableDiffusionPipeline
from diffusionconvertors import FColorAsPILImage, PILImageToFColorArray

try:
    from .upsampling import RealESRGANModel
except ImportError as e:
    print("Could not import RealESRGAN upsampler")


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


#def split_weighted_subprompts(text):
#    """
#    grabs all text up to the first occurrence of ':' 
#    uses the grabbed text as a sub-prompt, and takes the value following ':' as weight
#    if ':' has no value defined, defaults to 1.0
#    repeats until no text remaining
#    """
#    remaining = len(text)
#    prompts = []
#    weights = []
#    while remaining > 0:
#        if ":" in text:
#            idx = text.index(":") # first occurrence from start
#            # grab up to index as sub-prompt
#            prompt = text[:idx]
#            remaining -= idx
#            # remove from main text
#            text = text[idx+1:]
#            # find value for weight 
#            if " " in text:
#                idx = text.index(" ") # first occurence
#            else: # no space, read to end
#                idx = len(text)
#            if idx != 0:
#                try:
#                    weight = float(text[:idx])
#                except: # couldn't treat as float
#                    print(f"Warning: '{text[:idx]}' is not a value, are you missing a space?")
#                    weight = 1.0
#            else: # no value found
#                weight = 1.0
#            # remove from main text
#            remaining -= idx
#            text = text[idx+1:]
#            # append the sub-prompt and its weight
#            prompts.append(prompt)
#            weights.append(weight)
#        else: # no : found
#            if len(text) > 0: # there is still text though
#                # take remainder as weight 1
#                prompts.append(text)
#                weights.append(1.0)
#            remaining = 0
#    return prompts, weights

##TODO: Implement https://www.reddit.com/r/StableDiffusion/comments/wvolor/is_prompt_weighting_possible/
##Then replace c = model.get_learned_conditioning(prompts) in the txt2img() or img2img() function with:

#subprompts,weights = split_weighted_subprompts(prompt)
#skip_subprompt_normalize = False
#print(f"subprompts: {subprompts}, weights: {weights}")
#if len(subprompts) > 1:
#    c = torch.zeros_like(uc)
#    totalWeight = sum(weights)
#    for i in range(0,len(subprompts)):
#        weight = weights[i]
#        if not skip_subprompt_normalize:
#            weight = weight / totalWeight
#        c = torch.add(opt.c, model.get_learned_conditioning(subprompts[i]), alpha=weight)
#else:
#    c = model.get_learned_conditioning(prompts)


@unreal.uclass()
class DiffusersBridge(unreal.StableDiffusionBridge):
    def __init__(self):
        unreal.StableDiffusionBridge.__init__(self)
        self.pipe = None

    @unreal.ufunction(override=True)
    def InitModel(self, model_options):
        result = False
        ActivePipeline = StableDiffusionImg2ImgPipeline
        modelname = model_options.model if model_options.model else "CompVis/stable-diffusion-v1-4"
        kwargs = {
            "torch_dtype": torch.float32 if model_options.precision == "fp32" else torch.float16,
            "use_auth_token": True,
        }
        
        if model_options.revision:
            kwargs["revision"] = model_options.revision
        if custom_pipeline:
            kwargs["custom_pipeline"] = = model_options.custom_pipeline

        try:
            print("Loading Stable Diffusion model " + modelname)
            self.pipe = ActivePipeline.from_pretrained(modelname, **kwargs)
            self.pipe = self.pipe.to("cuda")
            self.pipe.enable_attention_slicing()

            self.model_options = model_options
            result = True
        except Exception as e:
            print("Failed to init Stable Diffusion Img2Image pipeline. Exception was {0}".format(e))
        return result
    
       try:
            if getattr(self, "upsampler", None) is None:
                self.upsampler = RealESRGANModel.from_pretrained("nateraw/real-esrgan")
            self.upsampler.to("cuda")
       except Exception as e:
            print("Could not load upsampler. Exception was ".format(e))

        
    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, input):
        with autocast("cuda"):
            guide_img = preprocess_init_image(FColorAsPILImage(input.input_image_pixels, input.options.size_x, input.options.size_y).convert("RGB"), input.options.out_size_x, input.options.out_size_y) if input.input_image_pixels else None
            mask_img = preprocess_mask(FColorAsPILImage(input.options.mask_image_pixels, input.options.size_x, input.options.size_y).convert("RGB"), input.options.out_size_x, input.options.out_size_y) if input.mask_image_pixels else None
            generator = torch.Generator("cuda")

            seed = torch.random.seed() if input.options.seed < 0 else input.options.seed
            generator.manual_seed(seed)

            # Split prompts in case commas have snuck in
            positive_prompts = ", ".join([split_p.strip() for p in input.options.positive_prompts for split_p in p.prompt.split(",")])
            negative_prompts = ", ".join([split_p.strip() for p in input.options.negative_prompts for split_p in p.prompt.split(",")])
            print(positive_prompts)
            print(negative_prompts)

            # Pad negative prompts to match batch size
            #if len(negative_prompts) < len(positive_prompts):
            #    for idx in range(len(positive_prompts) - len(negative_prompts)):
            #        negative_prompts.append('')

            images = self.pipe.img2img(
                prompt=positive_prompts, 
                negative_prompt=negative_prompts,
                init_image=guide_img, 
                width=input.options.out_size_x,
                height=input.options.out_size_y,
                strength=input.options.strength, 
                num_inference_steps=input.options.iterations, 
                generator=generator, 
                guidance_scale=input.options.guidance_scale, 
                callback=self.ImageProgressStep, 
                callback_steps=10).images
            image = image[0] if not upsample else self.upsampler(image[0])

        result = unreal.StableDiffusionImageResult()
        result.input = input
        result.pixel_data =  PILImageToFColorArray(image.convert("RGBA"))
        result.generated_texture = None
        return result

    def ImageProgressStep(self, step: int, timestep: int, latents: torch.FloatTensor) -> None:
        adjusted_latents = 1 / 0.18215 * latents
        image = self.pipe.vae.decode(adjusted_latents).sample
        image = (image / 2 + 0.5).clamp(0, 1)
        image = image.detach().cpu().permute(0, 2, 3, 1).numpy()
        image = self.pipe.numpy_to_pil(image)[0]
        pixels = PILImageToFColorArray(image.convert("RGBA"))
        self.update_image_progress("inprogress", step, 0, image.width, image.height, pixels)
