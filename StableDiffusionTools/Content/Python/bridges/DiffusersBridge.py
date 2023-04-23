from asyncio.windows_utils import pipe
import pipes
from turtle import update
import unreal
import os, inspect, importlib, random, threading, ctypes, time, traceback, pprint

import numpy as np

import safetensors
import torch
from torch import autocast
from torchvision.transforms.functional import rgb_to_grayscale
import PIL
from PIL import Image
from transformers import CLIPFeatureExtractor
from compel import Compel
import diffusers
from diffusers import StableDiffusionImg2ImgPipeline, StableDiffusionPipeline, StableDiffusionInpaintPipeline, StableDiffusionDepth2ImgPipeline, StableDiffusionUpscalePipeline
from diffusers.pipelines.stable_diffusion.safety_checker import StableDiffusionSafetyChecker
from diffusers.schedulers.scheduling_utils import SchedulerMixin
from diffusionconvertors import FColorAsPILImage, PILImageToFColorArray
from huggingface_hub.utils import HfFolder

try:
    from upsampling import RealESRGANModel
except ImportError as e:
    print("Could not import RealESRGAN upsampler")


# Globally disable progress bars to avoid polluting the Unreal log
diffusers.utils.logging.disable_progress_bar()

## Global disable torch loading. Use safetensors
#def _raise_torch_load_err():
#    raise RuntimeError("I don't want to use pickle")

#torch.load = lambda *args, **kwargs: _raise_torch_load_err()


def patch_conv(padding_mode):
    cls = torch.nn.Conv2d
    init = cls.__init__

    def __init__(self, *args, **kwargs):
        kwargs["padding_mode"]=padding_mode.name.lower()
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

def preprocess_normalmap(image: Image, width: int, height: int):
    image = image.resize((width, height), resample=Image.LANCZOS)
    image = np.array(image)
    #image = image[...,::-1]
    image = Image.fromarray(image)
    return image

def preprocess_image_inpaint(image):
    w, h = image.size
    w, h = map(lambda x: x - x % 32, (w, h))  # resize to integer multiple of 32
    image = image.resize((w, h), resample=PIL.Image.LANCZOS)
    image = np.array(image).astype(np.float32) / 255.0
    image = image[None].transpose(0, 3, 1, 2)
    image = torch.from_numpy(image)
    return 2.0 * image - 1.0

def preprocess_image_depth(image):
    w, h = image.size
    w, h = map(lambda x: x - x % 32, (w, h))  # resize to integer multiple of 32
    image = image.convert("RGB")
    image = image.resize((w, h), resample=PIL.Image.LANCZOS)
    image = np.array(image).astype(np.float32) / 255.0
    image = image[None].transpose(0, 3, 1, 2)
    #image = 1 - image
    image = torch.from_numpy(image)
    image = rgb_to_grayscale(image, 1)
    print(f"Presqueeze: Depthmap has shape {image.shape}")
    image = image.squeeze(1)
    print(f"Postqueeze: Depthmap has shape {image.shape}")
    return image


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


class AbortableExecutor(threading.Thread):
    def __init__(self, name, func):
        threading.Thread.__init__(self)
        self.name = name
        self.func = func
        self.result = None
        self.wait_result = threading.Condition()

    def run(self):
        try:
            print("Executor about to run func")
            self.result = self.func()
            print("Executor completed func")
            self.completed = True
        except Exception as e:
            print(f"Executor received abort exception. {e}")
            print(traceback.print_exc(e))

    def stop(self):
        self.raise_exception()

    def wait(self):
        #with self.wait_result:
        self.wait_result.acquire()
        print("Waiting on executor")
        self.wait_result.wait()
        print("Executor received notify")
        self.wait_result.release()

    def get_id(self):
        # returns id of the respective thread
        if hasattr(self, '_thread_id'):
            return self._thread_id
        for id, thread in threading._active.items():
            if thread is self:
                return id

    def raise_exception(self):
        thread_id = self.get_id()
        print(f"About to abort thread {thread_id}")
        res = ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id,
              ctypes.py_object(SystemExit))
        if res > 1:
            ctypes.pythonapi.PyThreadState_SetAsyncExc(thread_id, 0)
            print('Exception raise failure')

@unreal.uclass()
class DiffusersBridge(unreal.StableDiffusionBridge):

    def __init__(self):
        unreal.StableDiffusionBridge.__init__(self)
        self.upsampler = None
        self.pipe = None   
        self.compel = None
        self.executor = None
        self.abort = False
        self.update_frequency = 25
        self.start_timestep = -1

    @unreal.ufunction(override=True)
    def InitModel(self, new_model_options, allow_nsfw, padding_mode):
        self.model_loaded = False
        result = True

        scheduler_module = None
        if new_model_options.scheduler:
            scheduler_cls = getattr(diffusers, new_model_options.scheduler)
            print("Using scheduler class: {scheduler_cls}")

        # Set pipeline
        print(f"Requested pipeline: {new_model_options.diffusion_pipeline}")
        if new_model_options.diffusion_pipeline:
            ActivePipeline = getattr(diffusers, new_model_options.diffusion_pipeline)
        print(f"Loaded pipeline: {ActivePipeline}")

        modelname = new_model_options.model if new_model_options.model else "CompVis/stable-diffusion-v1-5"
        kwargs = {
            "torch_dtype": torch.float32 if new_model_options.precision == "fp32" else torch.float16,
            "use_auth_token": self.get_token(),
            #"safety_checker": None,     # TODO: Init safety checker - this is included to stop models complaining about the missing module
            #"feature_extractor": None,  # TODO: Init feature extractor - this is included to stop models complaining about the missing module
        }

        # Run model init script to generate extra args
        init_script_locals = {}
        exec(new_model_options.python_model_arguments_script, globals(), init_script_locals)
        for key, val in init_script_locals.items():
            if "pipearg_" in key:
                kwargs[key.replace('pipearg_', '')] = val
        
        if new_model_options.revision:
            kwargs["revision"] = new_model_options.revision
        if new_model_options.custom_pipeline:
            kwargs["custom_pipeline"] = new_model_options.custom_pipeline
        
        # Padding mode injection
        patch_conv(padding_mode=padding_mode)

        # Torch performance options
        torch.backends.cudnn.benchmark = True
        torch.backends.cuda.matmul.allow_tf32 = True

        # Load model
        self.pipe = ActivePipeline.from_pretrained(modelname, **kwargs)

        if scheduler_module:
            self.pipe.scheduler = scheduler_cls.from_config(self.pipe.scheduler.config)

        # Compel for weighted prompts
        self.compel = Compel(tokenizer=self.pipe.tokenizer, text_encoder=self.pipe.text_encoder, truncate_long_prompts=False)

        # Performance options for low VRAM gpus
        #self.pipe.enable_sequential_cpu_offload()
        self.pipe.enable_attention_slicing()
        try:
            self.pipe.unet = torch.compile(self.pipe.unet)
        except RuntimeError as e:
            print(f"WARNING: Couldn't compile unet for model. Exception given was '{e}'")
        #self.pipe.enable_xformers_memory_efficient_attention()
        if hasattr(self.pipe, "enable_model_cpu_offload"):
            self.pipe.enable_model_cpu_offload()

        # High resolution support by tiling the VAE
        self.pipe.vae.enable_tiling()

        # NSFW filter
        if allow_nsfw:
            # Backup original NSFW filter
            if not hasattr(self, "orig_NSFW_filter"):
                self.orig_NSFW_filter = self.pipe.safety_checker

            # Dummy passthrough filter
            self.pipe.safety_checker = lambda images, **kwargs: (images, False)
        else:
            if hasattr(self, "orig_NSFW_filter"):
                self.pipe.safety_checker = self.orig_NSFW_filter
        
        self.set_editor_property("ModelOptions", new_model_options)
        self.model_loaded = True
        print("Loaded Stable Diffusion model " + modelname)

        return result

    def InitUpsampler(self):
        upsampler = None
        try:
            upsampler = RealESRGANModel.from_pretrained("nateraw/real-esrgan")
            upsampler = upsampler.to("cuda")
        except Exception as e:
            print("Could not load upsampler. Exception was ".format(e))
        return upsampler

    @unreal.ufunction(override=True)
    def ReleaseModel(self):
        if hasattr(self, "pipe"):
            if self.pipe:
                del self.pipe
                self.pipe = None
        self.model_loaded = False
        torch.cuda.empty_cache()

    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, input):
        self.abort = False
        result = unreal.StableDiffusionImageResult()
        model_options = self.get_editor_property("ModelOptions")
        if not hasattr(self, "pipe"):
            print("Could not find a pipe attribute. Has it been GC'd?")
            return

        layer_img_mappings = {}
        for layer in input.processed_layers:
            layer_img = FColorAsPILImage(layer.layer_pixels, input.options.size_x, input.options.size_y).convert("RGB") if layer.layer_pixels else None
            layer_img = layer_img.resize((input.options.out_size_x, input.options.out_size_y))

            if layer.processor.python_transform_script:
                transform_script_locals = {}
                transform_script_args = {"input_image": layer_img}
                print(f"Running image transform script for layer {layer}")
                exec(layer.processor.python_transform_script, transform_script_args, transform_script_locals)
                layer_img = transform_script_locals["result_image"] if "result_image" in transform_script_locals else layer_img
                

            if layer.role in layer_img_mappings:
                if not hasattr(layer_img_mappings[layer.role], "__len__"):
                    layer_img_mappings[layer.role] = [layer_img_mappings[layer.role]]
                    layer_img_mappings[layer.role].append(layer_img)
            else:
                layer_img_mappings[layer.role] = layer_img


        # Convert unreal pixels to PIL images
        #guide_img = FColorAsPILImage(input.input_image_pixels, input.options.size_x, input.options.size_y).convert("RGB") if input.input_image_pixels else None
        #mask_img = FColorAsPILImage(input.mask_image_pixels, input.options.size_x, input.options.size_y).convert("RGB")  if input.mask_image_pixels else None
        
        # Capability flags
        inpaint_active = (model_options.capabilities & unreal.ModelCapabilities.INPAINT.value) == unreal.ModelCapabilities.INPAINT.value
        depth_active = (model_options.capabilities & unreal.ModelCapabilities.DEPTH.value)  == unreal.ModelCapabilities.DEPTH.value
        strength_active = (model_options.capabilities & unreal.ModelCapabilities.STRENGTH.value)  == unreal.ModelCapabilities.STRENGTH.value
        controlnet_active = (model_options.capabilities & unreal.ModelCapabilities.CONTROL.value)  == unreal.ModelCapabilities.CONTROL.value
        mask_active = depth_active or inpaint_active
        print(f"Capabilities value {model_options.capabilities}, Depth map value: {unreal.ModelCapabilities.DEPTH.value}, Using depthmap? {depth_active}")
        print(f"Capabilities value {model_options.capabilities}, Inpaint value: {unreal.ModelCapabilities.INPAINT.value}, Using inpaint? {inpaint_active}")
        print(f"Capabilities value {model_options.capabilities}, Strength value: {unreal.ModelCapabilities.STRENGTH.value}, Using strength? {strength_active}")
        print(f"Capabilities value {model_options.capabilities}, Strength value: {unreal.ModelCapabilities.CONTROL.value}, Using controlnet? {controlnet_active}")

        # DEBUG: Show input images
        if input.debug_python_images:
            for key, val in layer_img_mappings.items():
                if hasattr(layer_img_mappings[layer.role], "__len__"):
                    for img in layer_img_mappings[layer.role]:
                        img.show()
                else:
                    layer_img_mappings[key].show()

        # Set seed
        max_seed = abs(int((2**31) / 2) - 1)
        seed = random.randrange(0, max_seed) if input.options.seed < 0 else input.options.seed
        
        # Create prompt
        prompt_tensors = self.build_prompt_tensors(positive_prompts=input.options.positive_prompts, negative_prompts=input.options.negative_prompts, compel=self.compel)

        with torch.inference_mode():
            #with autocast("cuda", dtype=torch.float16):
                generator = torch.Generator(device="cpu")
                generator.manual_seed(seed)
                generation_args = {
                    "prompt_embeds": prompt_tensors,
                    "num_inference_steps": input.options.iterations, 
                    "generator": generator, 
                    "guidance_scale": input.options.guidance_scale, 
                    "callback": self.ImageProgressStep,
                    "callback_steps": 1
                }
                self.update_frequency = input.preview_iteration_rate

                # Set the timestep in the scheduler early so we can get the start timestep
                self.pipe.scheduler.set_timesteps(input.options.iterations, device=self.pipe._execution_device)
                print(self.pipe.scheduler.timesteps)
                self.start_timestep = int(self.pipe.scheduler.timesteps.cpu().numpy()[0])
                #self.pipe.scheduler.set_timesteps(input.options.iterations, device=self.pipe._execution_device)
                print(f"Start timestep is {self.start_timestep}")

                # Different capability flags use different keywords in the pipeline
                if strength_active:
                    generation_args["strength"] = input.options.strength

                # Add processed input layers                 
                generation_args.update(layer_img_mappings)
                print(layer_img_mappings)
                
                if input.debug_python_images:
                    print("Generation args:")
                    pprint.pprint(generation_args)
                
                # Create executor to generate the image in its own thread that we can abort if needed
                self.executor = AbortableExecutor("ImageThread", lambda generation_args=generation_args: self.pipe(**generation_args))
                self.executor.start()

                # Block until executor completes
                self.executor.join()
                if not self.executor.result or not self.executor.completed:
                    print(f"Image generation was aborted")
                    self.abort = False

                # Gather result images
                images = self.executor.result.images if self.executor.result else None #self.pipe(**generation_args).images
                image = images[0] if images else None

                if input.debug_python_images and image:
                    image.show()
                
                # Gather result data
                result.input = input
                result.input.options.seed = seed
                print(f"Seed was {seed}. Saved as {result.input.options.seed}")
                result.pixel_data =  PILImageToFColorArray(image.convert("RGBA")) if image else [unreal.Color(0,0,0,255) for i in range(input.options.out_size_x * input.options.out_size_y)]
                result.out_width = image.width if image else input.options.out_size_x
                result.out_height = image.height if image else input.options.out_size_y
                result.completed = True if image else False

                # Cleanup
                self.start_timestep = -1
                #self.executor = None

        return result

    def ImageProgressStep(self, step: int, timestep: int, latents: torch.FloatTensor) -> None:
        pct_complete = (self.start_timestep - timestep) / self.start_timestep

        print(f"Step is {step}. Timestep is {timestep} Frequency is {self.update_frequency}. Modulo is {step % self.update_frequency}")
        if step % self.update_frequency == 0:
            adjusted_latents = 1 / 0.18215 * latents
            image = self.pipe.vae.decode(adjusted_latents).sample
            image = (image / 2 + 0.5).clamp(0, 1)
            image = image.detach().cpu().permute(0, 2, 3, 1).numpy()
            image = self.pipe.numpy_to_pil(image)[0]
            pixels = PILImageToFColorArray(image.convert("RGBA"))
            self.update_image_progress("inprogress", int(step), int(timestep), float(pct_complete), image.width, image.height, pixels)

        # Image finished we can now abort image generation
        if self.abort and self.executor:
            self.executor.stop()

    def build_prompt_tensors(self, positive_prompts: list[unreal.Prompt], negative_prompts: list[unreal.Prompt], compel: Compel):
         # Collate prompts
        positive_prompts = " ".join([f"({split_p.strip()}){prompt.weight}" for prompt in positive_prompts for split_p in prompt.prompt.split(",")])
        negative_prompts = " ".join([f"({split_p.strip()}){prompt.weight}" for prompt in negative_prompts for split_p in prompt.prompt.split(",")])
        print(positive_prompts)
        print(negative_prompts)
        positive_prompt_tensors = compel.build_conditioning_tensor(positive_prompts if positive_prompts else "")
        negative_prompt_tensors = compel.build_conditioning_tensor(negative_prompts if negative_prompts else "")
        return torch.cat(compel.pad_conditioning_tensors_to_same_length([positive_prompt_tensors, negative_prompt_tensors]))

    @unreal.ufunction(override=True)
    def StopImageGeneration(self):
        self.abort = True

    @unreal.ufunction(override=True)
    def StartUpsample(self):
        if not hasattr(self, "upsampler"):
            if not self.upsampler:
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
            del local_upsampler
        torch.cuda.empty_cache()

        # Build result
        result = unreal.StableDiffusionImageResult()
        result.input = image_result.input
        result.pixel_data =  PILImageToFColorArray(upsampled_image.convert("RGBA"))
        result.out_width = upsampled_image.width
        result.out_height = upsampled_image.height
        result.upsampled = True

        return result
