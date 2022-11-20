import unreal
import os, inspect
import numpy as np
import torch
from torch import autocast
import PIL
from PIL import Image
from diffusers import StableDiffusionImg2ImgPipeline, StableDiffusionPipeline, StableDiffusionInpaintPipeline
from diffusionconvertors import FColorAsPILImage, PILImageToFColorArray
from huggingface_hub.utils import HfFolder

try:
    from upsampling import RealESRGANModel
except ImportError as e:
    print("Could not import RealESRGAN upsampler")


def patch_conv(padding_mode):
    cls = torch.nn.Conv2d
    init = cls.__init__

    def __init__(self, *args, **kwargs):
        kwargs["padding_mode"]=padding_mode
        return init(self, *args, **kwargs)

    cls.__init__ = __init__


def preprocess_init_image(image: Image, width: int, height: int):
    image = image.resize((width, height), resample=Image.LANCZOS)
    image = np.array(image).astype(np.float32) / 255.0
    image = image[None].transpose(0, 3, 1, 2)
    image = torch.from_numpy(image)
    return 2.0 * image - 1.0


#def preprocess_mask(mask: Image, width: int, height: int):
#    mask = mask.convert("L")
#    mask = mask.resize((width // 8, height // 8), resample=Image.LANCZOS)
#    mask = np.array(mask).astype(np.float32) / 255.0
#    mask = np.tile(mask, (4, 1, 1))
#    mask = mask[None].transpose(0, 1, 2, 3)  # what does this step do?
#    mask = torch.from_numpy(mask)
#    return mask

def preprocess_image_inpaint(image):
    w, h = image.size
    w, h = map(lambda x: x - x % 32, (w, h))  # resize to integer multiple of 32
    image = image.resize((w, h), resample=PIL.Image.LANCZOS)
    image = np.array(image).astype(np.float32) / 255.0
    image = image[None].transpose(0, 3, 1, 2)
    image = torch.from_numpy(image)
    return 2.0 * image - 1.0


def preprocess_mask_inpaint(mask):
    mask = mask.convert("L")
    w, h = mask.size
    w, h = map(lambda x: x - x % 32, (w, h))  # resize to integer multiple of 32
    mask = mask.resize((w // 8, h // 8), resample=PIL.Image.NEAREST)
    mask = np.array(mask).astype(np.float32) / 255.0
    mask = np.tile(mask, (4, 1, 1))
    mask = mask[None].transpose(0, 1, 2, 3)  # what does this step do?
    mask = 1 - mask  # repaint white, keep black
    mask = torch.from_numpy(mask)
    return mask


@unreal.uclass()
class DiffusersBridge(unreal.StableDiffusionBridge):
    def __init__(self):
        unreal.StableDiffusionBridge.__init__(self)
        self.pipe = None
        self.upsampler = None

    @unreal.ufunction(override=True)
    def LoginUsingToken(self, token):
        try:
            HfFolder.save_token(token)
            return True
        except Exception as e:
             print(f"Could not save HuggingFace token. Exception was {e}")
        return False

    @unreal.ufunction(override=True)
    def GetToken(self):
        try:
            return HfFolder.get_token()
        except Exception as e:
            print(f"Could not get HuggingFace token. Exception was {e}")

        return ""

    @unreal.ufunction(override=True)
    def InitModel(self, model_options):
        self.model_loaded = False

        result = True
        ActivePipeline = StableDiffusionInpaintPipeline if model_options.inpaint else StableDiffusionImg2ImgPipeline
        modelname = model_options.model if model_options.model else "CompVis/stable-diffusion-v1-4"
        kwargs = {
            "torch_dtype": torch.float32 if model_options.precision == "fp32" else torch.float16,
            "use_auth_token": True,
        }
        
        if model_options.revision:
            kwargs["revision"] = model_options.revision
        if model_options.custom_pipeline:
            kwargs["custom_pipeline"] = model_options.custom_pipeline

        patch_conv(padding_mode=model_options.padding_mode)

        # Load model
        #try:
        self.pipe = ActivePipeline.from_pretrained(modelname, **kwargs)
        self.pipe = self.pipe.to("cuda")

        # Performance options for low VRAM gpus
        #self.pipe.enable_sequential_cpu_offload()
        self.pipe.enable_attention_slicing(1)
        self.pipe.enable_xformers_memory_efficient_attention()

        # NSFW filter
        if model_options.allow_nsfw:
            # Backup original NSFW filter
            if not hasattr(self, "orig_NSFW_filter"):
                self.orig_NSFW_filter = self.pipe.safety_checker

            # Dummy passthrough filter
            self.pipe.safety_checker = lambda images, **kwargs: (images, False)
        else:
            if hasattr(self, "orig_NSFW_filter"):
                self.pipe.safety_checker = self.orig_NSFW_filter
            
        self.model_options = model_options
        self.model_loaded = True
        print("Loaded Stable Diffusion model " + modelname)
        #except Exception as e:
            #print("Failed to init Stable Diffusion Img2Image pipeline. Exception was {0}".format(e))
            #result &= False

        return result

    def InitUpsampler(self):
        upsampler = None
        try:
            upsampler = RealESRGANModel.from_pretrained("nateraw/real-esrgan")
            upsampler = upsampler.to("cuda")
        except Exception as e:
            print("Could not load upsampler. Exception was ".format(e))
        print(upsampler)
        return upsampler

    @unreal.ufunction(override=True)
    def ReleaseModel(self):
        if self.pipe:
            del self.pipe
            self.pipe = None
        self.model_loaded = False
        torch.cuda.empty_cache()

    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, input):
        result = unreal.StableDiffusionImageResult()
        guide_img = FColorAsPILImage(input.input_image_pixels, input.options.size_x, input.options.size_y).convert("RGB") if input.input_image_pixels else None
        mask_img = FColorAsPILImage(input.mask_image_pixels, input.options.size_x, input.options.size_y).convert("RGB")  if input.mask_image_pixels else None
        
        if self.model_options.inpaint:
            guide_img = guide_img.resize((512,512))
            mask_img = mask_img.resize((512,512))
            #mask_img.show()
        else:
            guide_img = preprocess_init_image(guide_img, input.options.out_size_x, input.options.out_size_y)
        
        seed = torch.random.seed() if input.options.seed < 0 else input.options.seed
        positive_prompts = ", ".join([f"({split_p.strip()}:{prompt.weight})" if not self.model_options.inpaint else f"{split_p.strip()}" for prompt in input.options.positive_prompts for split_p in prompt.prompt.split(",")])
        negative_prompts = ", ".join([f"({split_p.strip()}:{prompt.weight})" if not self.model_options.inpaint else f"{split_p.strip()}" for prompt in input.options.negative_prompts for split_p in prompt.prompt.split(",")])
        print(positive_prompts)
        print(negative_prompts)

        with torch.inference_mode():
            with autocast("cuda"):
                generator = torch.Generator(device="cuda")
                generator.manual_seed(seed)
                generation_args = {
                    "prompt": positive_prompts,
                    "negative_prompt": negative_prompts,
                    "image" if self.model_options.inpaint else "init_image": guide_img,
                    "width": input.options.out_size_x,
                    "height": input.options.out_size_y,
                    "strength": input.options.strength, 
                    "num_inference_steps": input.options.iterations, 
                    "generator": generator, 
                    "guidance_scale": input.options.guidance_scale, 
                    "callback": self.ImageProgressStep, 
                    "callback_steps": 25
                }
                if self.model_options.inpaint and mask_img:
                    generation_args["mask_image"] = mask_img

                images = self.pipe(**generation_args).images
                image = images[0]

                result.input = input
                result.pixel_data =  PILImageToFColorArray(image.convert("RGBA"))
                result.out_width = image.width
                result.out_height = image.height
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

    @unreal.ufunction(override=True)
    def StartUpsample(self):
        if not hasattr(self, "upsampler"):
            self.upsampler = self.InitUpsampler()

    @unreal.ufunction(override=True)
    def StopUpsample(self):
        if hasattr(self, "upsampler"):
            # Free VRAM after upsample
            if self.upsampler:
                del self.upsampler
                self.upsampler = None
            torch.cuda.empty_cache()

    @unreal.ufunction(override=True)
    def UpsampleImage(self, image_result: unreal.StableDiffusionImageResult):
        upsampled_image = None
        active_upsampler = None
        local_upsampler = None
        if not self.upsampler:
            local_upsampler = self.InitUpsampler()
            active_upsampler = local_upsampler
        else:
            active_upsampler = self.upsampler

        if active_upsampler:
            if not isinstance(image_result, unreal.StableDiffusionImageResult):
                raise ValueError(f"Wrong type passed to upscale. Expected {type(StableDiffusionImageResult)} or List. Received {type(image_result)}")
                
            image = FColorAsPILImage(image_result.pixel_data, image_result.out_width,image_result.out_height).convert("RGB")
            print(f"Upscaling image result from {image_result.out_width}:{image_result.out_height} to {image_result.out_width * 4}:{image_result.out_height * 4}")
            upsampled_image = active_upsampler(image)
        
        # Free local upsampler to restore VRAM
        if local_upsampler:
            if local_upsampler:
                del local_upsampler
            torch.cuda.empty_cache()

        # Build result
        result = unreal.StableDiffusionImageResult()
        result.input = image_result.input
        result.pixel_data =  PILImageToFColorArray(upsampled_image.convert("RGBA"))
        result.out_width = upsampled_image.width
        result.out_height = upsampled_image.height
        result.generated_texture = None
        result.upsampled = True

        return result
