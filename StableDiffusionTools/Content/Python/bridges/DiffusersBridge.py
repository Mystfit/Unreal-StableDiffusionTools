from asyncio.windows_utils import pipe
import io
import pipes
from tqdm.auto import tqdm
import functools
from urllib.parse import urlparse
from turtle import update
import unreal
import os, inspect, importlib, random, threading, ctypes, time, traceback, pprint, gc, shutil, re
from pathlib import Path
from contextlib import nullcontext
import numpy as np
import requests
from diffusers.pipelines.stable_diffusion.convert_from_ckpt import download_from_original_stable_diffusion_ckpt


import safetensors
import torch
from torch import autocast
#from torchvision.transforms.functional import rgb_to_grayscale
import PIL
from PIL import Image
from transformers import CLIPFeatureExtractor
from compel import Compel, DiffusersTextualInversionManager, ReturnedEmbeddingsType
import diffusers
from diffusers import StableDiffusionImg2ImgPipeline, StableDiffusionPipeline, StableDiffusionInpaintPipeline, StableDiffusionDepth2ImgPipeline, StableDiffusionUpscalePipeline
from diffusers.pipelines.stable_diffusion.safety_checker import StableDiffusionSafetyChecker
from diffusers.schedulers.scheduling_utils import SchedulerMixin
from diffusionconvertors import FColorAsPILImage, PILImageToFColorArray, PILImageToTexture
from huggingface_hub.utils import HfFolder, scan_cache_dir
from huggingface_hub.utils._errors import LocalEntryNotFoundError

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


def layer_type_name(layer_type: unreal.LayerImageType):
    layer_type_map = {
        unreal.LayerImageType.IMAGE: "image",
        unreal.LayerImageType.CONTROL_IMAGE: "control_image",
        unreal.LayerImageType.CUSTOM: "custom",
    }
    return layer_type_map[layer_type]


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

#def preprocess_image_depth(image):
#    w, h = image.size
#    w, h = map(lambda x: x - x % 32, (w, h))  # resize to integer multiple of 32
#    image = image.convert("RGB")
#    image = image.resize((w, h), resample=PIL.Image.LANCZOS)
#    image = np.array(image).astype(np.float32) / 255.0
#    image = image[None].transpose(0, 3, 1, 2)
#    #image = 1 - image
#    image = torch.from_numpy(image)
#    image = rgb_to_grayscale(image, 1)
#    print(f"Presqueeze: Depthmap has shape {image.shape}")
#    image = image.squeeze(1)
#    print(f"Postqueeze: Depthmap has shape {image.shape}")
#    return image


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


def download_file(url: str, destination_dir: Path):
    response = requests.get(url, stream=True, allow_redirects=True)
    if response.status_code != 200:
        response.raise_for_status()  # Will only raise for 4xx codes, so...
        raise RuntimeError(f"Request to {url} returned status code {r.status_code}")
    file_size = int(response.headers.get('Content-Length', 0))
    desc = "(Unknown total file size)" if file_size == 0 else ""

    url_filename = response.headers.get('content-disposition')
    filenames = re.findall('filename=(.+)', url_filename) 
    if filenames:
        filename = filenames[0].replace('\"', '')
        filepath = destination_dir / filename
        print(f"Downloading file to {filepath}")
        response.raw.read = functools.partial(response.raw.read, decode_content=True)  # Decompress if needed
        with tqdm.wrapattr(response.raw, "read", total=file_size, desc=desc) as r_raw:
            with filepath.open("wb") as f:
                shutil.copyfileobj(r_raw, f)
        return filepath

def refresh_supplementary_model(model_asset: unreal.StableDiffusionModelAsset, download_path: str):
    model_id = None

    # Get supplementary model from local filepath first
    if os.path.exists(model_asset.options.local_file_path.file_path):
        model_id = model_asset.options.local_file_path.file_path
    elif model_asset.options.external_url and os.path.exists(os.path.join(download_path, os.path.basename(urlparse(model_asset.options.external_url).path))):
         model_id = os.path.join(download_path, os.path.basename(urlparse(download_path).path))
    elif model_asset.options.local_file_path:
        if model_asset.options.external_url:
            # Download file
            print(f"Downloading supplementary model from {model_asset.options.external_url}")
            local_file = download_file(model_asset.options.external_url, Path(download_path))
            model_asset.options.local_file_path.file_path = str(local_file) if str(local_file) else ""
        model_id = model_asset.options.local_file_path.file_path
    else:
        # Use model ID to load from huggingface cache or hub
        model_id = model_asset.options.model

    return model_id

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
    def convert_raw_model(self, model_asset: unreal.StableDiffusionModelAsset, delete_original: bool):
        success = False

        # Get paths and make sure they exist
        if not model_asset.options.model_type == unreal.ModelType.CHECKPOINT:
            print("Can't convert model to diffusers format: Model is not a checkpoint")
            return False
            
        in_path = Path(model_asset.options.local_file_path.file_path)
        out_path = in_path.parent / f"{in_path.stem}"

        if not os.path.exists(in_path):
            print(f"Can't convert model to diffusers format: Source model does not exist at path {in_path.resolve()}")
            return False

        if not os.path.exists(out_path):
            os.makedirs(out_path)

        print(f"Converting {in_path} model to diffusers format")

        # Set up args
        args = {
           "checkpoint_path": str(in_path.resolve()), 
           "from_safetensors": True if in_path.suffix == ".safetensors" else False 
        }
        if model_asset.options.base_resolution.x < 0:
            args["image_size"] = model_asset.options.base_model.options.base_resolution.x if model_asset.options.base_model else 512
        else:
            args["image_size"] = model_asset.options.base_resolution.x
        
        # Load pipeline weights from the checkpoint and save converted model
        with unreal.ScopedSlowTask(2, 'Converting model') as slow_task:
            slow_task.make_dialog(True)
            try:
                slow_task.enter_progress_frame(1, "Loading diffusers pipeline")
                pipe = download_from_original_stable_diffusion_ckpt(**args)
                pipe.to(torch_dtype=torch.float32 if model_asset.options.precision == "fp32" else torch.float16)
                slow_task.enter_progress_frame(1, "Saving model")
                pipe.save_pretrained(str(out_path.resolve()), safe_serialization=True)
                
                # Unload pipeline
                del pipe
                pipe = None

                success = True
            except Exception as e:
                print(f"Could not convert model to diffusers format. Exception was {e}")
        
        if success:
            # Update model asset
            model_asset.options.model_type = unreal.ModelType.DIFFUSERS
            model_asset.options.local_folder_path.path = str(out_path.resolve())

            # Cleanup original file
            if delete_original:
                os.remove(in_path)

        return success 


    @unreal.ufunction(override=True)
    def InitModel(self, new_model_options, new_pipeline_asset, lora_asset, textual_inversion_asset, layers, allow_nsfw, padding_mode):
        # Reset states
        self.abort = False

        # Free up any previously loaded mnodels
        self.ReleaseModel()

        scheduler_cls = None
        if new_pipeline_asset.options.scheduler:
            scheduler_cls = getattr(diffusers, new_pipeline_asset.options.scheduler)
            print(f"Using scheduler class: {scheduler_cls}")

        # Set pipeline
        if new_pipeline_asset.options.diffusion_pipeline:
            ActivePipeline = getattr(diffusers, new_pipeline_asset.options.diffusion_pipeline)
        print(f"Using pipeline class: {ActivePipeline}")

        # Use local model path if it is set, otherwise use the model name
        model_is_file = os.path.exists(new_model_options.local_file_path.file_path) if new_model_options.local_file_path.file_path else False
        model_is_folder = os.path.exists(new_model_options.local_folder_path.path) if new_model_options.local_folder_path.path else False

        modelname = new_model_options.model
        modelname = new_model_options.local_file_path.file_path if model_is_file else modelname
        modelname = new_model_options.local_folder_path.path if model_is_folder else modelname

        # Update model status to let UI know the model is downloading or available
        result = unreal.StableDiffusionModelInitResult()
        result.model_status = unreal.ModelStatus.LOADING if self.ModelExists(modelname) else unreal.ModelStatus.DOWNLOADING
        result.model_name = new_model_options.model
        self.set_editor_property("ModelStatus", result)

        kwargs = {
            "torch_dtype": torch.float32 if new_model_options.precision == "fp32" else torch.float16,
            "use_auth_token": self.get_token(),
            "cache_dir": self.get_settings_model_save_path().path
        }

        # Single file loading if we provide a URL for diffusers model types
        if new_model_options.external_url and new_model_options.model_type == unreal.ModelType.DIFFUSERS:
            modelname = new_model_options.external_url
            model_is_file = True
            kwargs["use_safetensors"] = True

        # Run model init script to generate extra pipeline args
        init_script_locals = {}
        exec(new_pipeline_asset.options.python_model_arguments_script, globals(), init_script_locals)
        for key, val in init_script_locals.items():
            if "pipearg_" in key:
                new_key = key.replace('pipearg_', '')
                if new_key in kwargs:
                    try:
                        if not hasattr(kwargs[new_key], "__len__"):
                            kwargs[new_key] = [kwargs[new_key]]
                    except KeyError:
                        kwargs[new_key] = [kwargs[new_key]]
                    kwargs[new_key].append(val)
                else:
                    kwargs[new_key] = val

        # Run layer processor init scripts to generate extra pipeline args
        for layer in layers:
            layer_init_script_locals = {}
            exec(layer.processor.python_model_init_script, globals(), layer_init_script_locals)
            for key, val in layer_init_script_locals.items():
                if "pipearg_" in key:
                    new_key = key.replace('pipearg_', '')
                    if new_key in kwargs:
                        try:
                            if not hasattr(kwargs[new_key], "__len__"):
                                kwargs[new_key] = [kwargs[new_key]]
                        except KeyError:
                            kwargs[new_key] = [kwargs[new_key]]
                        kwargs[new_key].append(val)
                    else:
                        kwargs[new_key] = val
        
        # Set pipe optional args
        if new_model_options.revision:
            kwargs["revision"] = new_model_options.revision
        if new_pipeline_asset.options.custom_pipeline:
            kwargs["custom_pipeline"] = new_pipeline_asset.options.custom_pipeline
        if allow_nsfw:
            kwargs["safety_checker"] = None
        
        # Padding mode injection
        patch_conv(padding_mode=padding_mode)

        # Torch performance options
        torch.backends.cudnn.benchmark = True
        torch.backends.cuda.matmul.allow_tf32 = True

        # Load model
        try:
            if model_is_file:
                print(f"Requested single-file pipeline is: {ActivePipeline}. Model is {modelname}. Args are {kwargs}")
                self.pipe = ActivePipeline.from_single_file(modelname, **kwargs)
                print(f"Loaded pipeline is: {self.pipe}")
            else:
                self.pipe = ActivePipeline.from_pretrained(modelname, **kwargs)
        except LocalEntryNotFoundError as e:
            result.model_status = unreal.ModelStatus.ERROR
            result.error_msg = f"Failed to load the model due to a missing local model or a download error. Full exception: {e}"
            print(result.error_msg)
            return result
        except ValueError as e:
            result.model_status = unreal.ModelStatus.ERROR
            result.error_msg = f"Incorrect values passed to the model init function. Full exception: {e}"
            print(result.error_msg)
            return result

        # Cache model and pipeline
        self.set_editor_property("ModelOptions", new_model_options)
        self.set_editor_property("PipelineAsset", new_pipeline_asset)

         # Run model post-init scripts
        post_init_script_locals = {}
        post_init_script_args = {
            "pipeline": self.pipe,
            "pipeline_asset": new_pipeline_asset,
            "model_asset": new_model_options
        }
        exec(new_pipeline_asset.options.python_pipeline_post_init_script, post_init_script_args, post_init_script_locals)
        self.pipe = post_init_script_locals["pipeline"] if "pipeline" in post_init_script_locals else self.pipe
        
        if scheduler_cls:
            self.pipe.scheduler = scheduler_cls.from_config(self.pipe.scheduler.config)

        # Performance options for low VRAM gpus
        self.pipe.enable_attention_slicing(1)
        try:
            self.pipe.unet = torch.compile(self.pipe.unet)
        except RuntimeError as e:
            print(f"WARNING: Couldn't compile unet for model. Exception given was '{e}'")
        #self.pipe.enable_xformers_memory_efficient_attention()
        if hasattr(self.pipe, "enable_model_cpu_offload"):# and not lora_asset:
            self.pipe.enable_model_cpu_offload()

        # High resolution support by tiling the VAE
        self.pipe.vae.enable_tiling()

        # LoRA setup
        lora_id = None
        if lora_asset:
            if lora_asset.options.model or lora_asset.options.external_url:
                # Load LoRA weights into pipeline
                lora_id = refresh_supplementary_model(lora_asset, self.get_settings_model_save_path().path)
                if lora_id:
                    # Force pipeline to CUDA until support is added for pipe.enable_model_cpu_offload()
                    self.pipe.to("cuda")
                    self.pipe.load_lora_weights(".", weight_name=lora_id)
                    # Move model back to CPU so model offloading works
                    self.pipe.to("cpu")

        # Textual inversion setup
        textual_inversion_id = None
        if textual_inversion_asset:
            if textual_inversion_asset.options.model or textual_inversion_asset.options.external_url:
                # Load textual inversion weights into pipeline
                textual_inversion_id = refresh_supplementary_model(textual_inversion_asset, self.get_settings_model_save_path().path)
                if textual_inversion_id:
                    # Force pipeline to CUDA until support is added for pipe.enable_model_cpu_offload()
                    self.pipe.to("cuda")
                    self.pipe.load_textual_inversion(textual_inversion_id)
                    # Move model back to CPU so model offloading works
                    self.pipe.to("cpu")
        
        # Cache assets
        self.set_editor_property("LORAAsset", lora_asset)
        self.set_editor_property("CachedTextualInversionAsset", textual_inversion_asset)

        print("Loaded Stable Diffusion model " + modelname)

        # Cache status
        result.model_name = new_model_options.model
        result.model_status = unreal.ModelStatus.LOADED
        self.set_editor_property("ModelStatus", result)

        # TODO: Implement multithreaded load with abortable model load
        if self.abort:
            self.ReleaseModel()

        return result

    @unreal.ufunction(override=True)
    def AvailableSchedulers(self):
        if not self.pipe:
            raise("No pipeline loaded to get compatible schedulers from")
        return [cls.__name__ for cls in self.pipe.scheduler.compatibles]

    @unreal.ufunction(override=True)
    def GetScheduler(self):
        if not self.pipe:
            raise("No pipeline loaded to get the current scheduler from")
        return self.pipe.scheduler.__class__.__name__

    @unreal.ufunction(override=True)
    def GetTokenWebsiteHint(self):
        return "https://huggingface.co/settings/tokens"

    @unreal.ufunction(override=True)
    def GetRequiresToken(self):
        return True

    def InitUpsampler(self):
        upsampler = None
        try:
            upsampler = RealESRGANModel.from_pretrained("nateraw/real-esrgan", cache_dir=self.get_settings_model_save_path().path)
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
        gc.collect()
        torch.cuda.empty_cache()

        result = unreal.StableDiffusionModelInitResult()
        result.model_status = unreal.ModelStatus.UNLOADED
        self.set_editor_property("ModelStatus", result)

    @unreal.ufunction(override=True)
    def ModelExists(self, model_name):
        # Check for filenames
        if os.path.exists(model_name):
            return True

        # Check huggingface cached repos
        cache = scan_cache_dir(cache_dir=self.get_settings_model_save_path().path)
        return bool(next((repo for repo in cache.repos if repo.repo_id == model_name), False))

    @unreal.ufunction(override=True)
    def GenerateImageFromStartImage(self, input, out_texture, preview_texture):
        self.abort = False
        result = unreal.StableDiffusionImageResult()
        model_options = self.get_editor_property("ModelOptions")
        pipeline_asset = self.get_editor_property("PipelineAsset")
        lora_asset = self.get_editor_property("LORAAsset")
        textual_inversion_asset = self.get_editor_property("CachedTextualInversionAsset")

        if not hasattr(self, "pipe"):
            print("Could not find a pipe attribute. Has it been GC'd?")
            return

        layer_img_mappings = {}
        controlnet_scales = []
        for layer in input.processed_layers:
            layer_img = None
            if layer.output_type == unreal.ImageType.LATENT:
                print("Loading latent data from layer")
                layer_img = torch.load(io.BytesIO(bytearray(layer.latent_data)))
            else:
                layer_img = FColorAsPILImage(layer.layer_pixels, input.options.size_x, input.options.size_y).convert("RGB") if layer.layer_pixels else None
                layer_img = layer_img.resize((input.options.out_size_x, input.options.out_size_y))

            if layer.processor.python_transform_script and not layer.output_type == unreal.ImageType.LATENT:
                transform_script_locals = {}
                transform_script_args = {"input_image": layer_img}
                print(f"Running image transform script for layer {layer}")
                exec(layer.processor.python_transform_script, transform_script_args, transform_script_locals)
                layer_img = transform_script_locals["result_image"] if "result_image" in transform_script_locals else layer_img
            
            # Set controlnet conditioning scales
            if layer.layer_type == unreal.LayerImageType.CONTROL_IMAGE and layer.processor_options:
                controlnet_scales.append(layer.processor_options.strength)

            role = layer.role if layer.layer_type == unreal.LayerImageType.CUSTOM else layer_type_name(layer.layer_type)
            if role in layer_img_mappings:
                if not hasattr(layer_img_mappings[role], "__len__"):
                    layer_img_mappings[role] = [layer_img_mappings[role]]
                layer_img_mappings[role].append(layer_img)
            else:
                layer_img_mappings[role] = layer_img

        # Convert unreal pixels to PIL images
        #guide_img = FColorAsPILImage(input.input_image_pixels, input.options.size_x, input.options.size_y).convert("RGB") if input.input_image_pixels else None
        #mask_img = FColorAsPILImage(input.mask_image_pixels, input.options.size_x, input.options.size_y).convert("RGB")  if input.mask_image_pixels else None
        
        # Capability flags
        no_prompt_weights_active = (pipeline_asset.options.capabilities & unreal.PipelineCapabilities.NO_PROMPT_WEIGHTS.value) == unreal.PipelineCapabilities.NO_PROMPT_WEIGHTS.value
        requires_pooled_active = (pipeline_asset.options.capabilities & unreal.PipelineCapabilities.POOLED_EMBEDDINGS.value) == unreal.PipelineCapabilities.POOLED_EMBEDDINGS.value
        inpaint_active = (pipeline_asset.options.capabilities & unreal.PipelineCapabilities.INPAINT.value) == unreal.PipelineCapabilities.INPAINT.value
        depth_active = (pipeline_asset.options.capabilities & unreal.PipelineCapabilities.DEPTH.value)  == unreal.PipelineCapabilities.DEPTH.value
        strength_active = (pipeline_asset.options.capabilities & unreal.PipelineCapabilities.STRENGTH.value)  == unreal.PipelineCapabilities.STRENGTH.value
        controlnet_active = (pipeline_asset.options.capabilities & unreal.PipelineCapabilities.CONTROL.value)  == unreal.PipelineCapabilities.CONTROL.value
        mask_active = depth_active or inpaint_active
        print(f"Capabilities value {pipeline_asset.options.capabilities}, Depth map value: {unreal.PipelineCapabilities.DEPTH.value}, Using depthmap? {depth_active}")
        print(f"Capabilities value {pipeline_asset.options.capabilities}, Inpaint value: {unreal.PipelineCapabilities.INPAINT.value}, Using inpaint? {inpaint_active}")
        print(f"Capabilities value {pipeline_asset.options.capabilities}, Strength value: {unreal.PipelineCapabilities.STRENGTH.value}, Using strength? {strength_active}")
        print(f"Capabilities value {pipeline_asset.options.capabilities}, ControlNet value: {unreal.PipelineCapabilities.CONTROL.value}, Using controlnet? {controlnet_active}")

        # DEBUG: Show input images
        if input.debug_python_images:
            for key, val in layer_img_mappings.items():
                role = layer.role if layer.layer_type == unreal.LayerImageType.CUSTOM else layer_type_name(layer.layer_type)
                if hasattr(layer_img_mappings[role], "__len__"):
                    for img in layer_img_mappings[role]:
                        img.show()
                else:
                    layer_img_mappings[key].show()

        # Set seed
        max_seed = abs(int((2**31) / 2) - 1)
        seed = random.randrange(0, max_seed) if input.options.seed < 0 else input.options.seed
        
        # Create prompt using the compel library
        text_encoders = None 
        tokenizers = None
        if hasattr(self.pipe, "text_encoder") and hasattr(self.pipe, "text_encoder_2"):
            if self.pipe.text_encoder and self.pipe.text_encoder_2:
                text_encoders = [self.pipe.text_encoder, self.pipe.text_encoder_2]
            else:
                text_encoders = self.pipe.text_encoder if self.pipe.text_encoder else self.pipe.text_encoder_2
        else:
            if hasattr(self.pipe, "text_encoder"):
                text_encoders = self.pipe.text_encoder if self.pipe.text_encoder else None
            elif hasattr(self.pipe, "text_encoder_2"):
                text_encoders = self.pipe.text_encoder_2 if self.pipe.text_encoder_2 else None

        if hasattr(self.pipe, "tokenizer") and hasattr(self.pipe, "tokenizer_2"):
            if self.pipe.tokenizer and self.pipe.tokenizer_2:
                tokenizers = [self.pipe.tokenizer, self.pipe.tokenizer_2]
            else:
                tokenizers = self.pipe.tokenizer if self.pipe.tokenizer else self.pipe.tokenizer_2
        else:
            if hasattr(self.pipe, "tokenizer"):
                tokenizers = self.pipe.tokenizer if self.pipe.tokenizer else None
            elif hasattr(self.pipe, "tokenizer_2"):
                tokenizers = self.pipe.tokenizer2 if self.pipe.tokenizer_2 else None
        
        if not tokenizers or not text_encoders:
            print(f"Missing either the text encoder or tokenizer for compel. Tokenizer: {tokenizers}, Text Encoder: {text_encoders}")
        #[self.pipe.text_encoder, self.pipe.text_encoder_2] if hasattr(self.pipe, "text_encoder_2") else self.pipe.text_encoder
        #[self.pipe.tokenizer, self.pipe.tokenizer_2] if hasattr(self.pipe, "tokenizer_2") else self.pipe.tokenizer

        requires_pooled = [False, requires_pooled_active] if isinstance(tokenizers, list) or isinstance(text_encoders, list) else requires_pooled_active
        compel_args = {
            "tokenizer": tokenizers,
            "text_encoder": text_encoders,
            "truncate_long_prompts": False
        }

        if requires_pooled:
            compel_args["requires_pooled"] = requires_pooled
            compel_args["returned_embeddings_type"] = ReturnedEmbeddingsType.PENULTIMATE_HIDDEN_STATES_NON_NORMALIZED

        # Textual inversions require a manager to be set in compel
        if textual_inversion_asset:
            compel_args["textual_inversion_manager"] = DiffusersTextualInversionManager(self.pipe)

        print(f"Compel args: {compel_args}")
        compel = Compel(**compel_args)

        # Create prompt tensors
        prompt_tensors = None
        negative_prompt_tensors = None
        pooled_prompt_tensors = None   
        negative_pooled_prompt_tensors = None
        prompt_tensors_group = compel(" ".join([f"({split_p.strip()}){prompt.weight}" for prompt in input.options.positive_prompts for split_p in prompt.prompt.split(",")]))
        #negative_prompt_tensors_group = compel(" ".join([f"({split_p.strip()}){prompt.weight}" for prompt in input.options.positive_prompts for split_p in prompt.prompt.split(",")]))

        #self.build_prompt_tensors(positive_prompts=input.options.positive_prompts, negative_prompts=input.options.negative_prompts, compel=compel)
        if requires_pooled_active and isinstance(prompt_tensors_group, tuple):
           prompt_tensors, pooled_prompt_tensors = (prompt_tensors_group[0], prompt_tensors_group[1]) 
           #negative_prompt_tensors, negative_pooled_prompt_tensors = compel.pad_conditioning_tensors_to_same_length([negative_prompt_tensors_group[0], negative_prompt_tensors_group[1]]) 
           #pooled_prompt_tensors, negative_pooled_prompt_tensors = compel.pad_conditioning_tensors_to_same_length([prompt_tensors_group[1], negative_prompt_tensors_group[1]])
        else:
            prompt_tensors = prompt_tensors_group
            #negative_prompt_tensors = negative_prompt_tensors_group

        # Cache preview texture
        self.preview_texture = preview_texture

        with torch.inference_mode():
            with torch.autocast("cuda", dtype=torch.float32 if model_options.precision == "fp32" else torch.float16) if lora_asset else nullcontext():
                generator = torch.Generator(device="cpu")
                generator.manual_seed(seed)
                generation_args = {
                    #"prompt": " ".join([f"{split_p.strip()}" for prompt in input.options.positive_prompts for split_p in prompt.prompt.split(",")]),
                    #"negative_prompt" : " ".join([f"{split_p.strip()}" for prompt in input.options.negative_prompts for split_p in prompt.prompt.split(",")]),
                    #"prompt_embeds": prompt_tensors,
                    #"negative_prompt_embeds": negative_prompt_tensors,
                    "num_inference_steps": input.options.iterations, 
                    "generator": generator, 
                    "guidance_scale": input.options.guidance_scale, 
                    "callback": self.ImageProgressStep,
                    "callback_steps": 1
                }

                # Set LoRA weights
                if lora_asset:
                    print(f"Using LoRA asset {lora_asset.options.model}")
                    generation_args["cross_attention_kwargs"] = {"scale":input.options.lora_weight};

                # How frequent we want the image preview to be updated during generation
                self.update_frequency = input.preview_iteration_rate

                # Set the timestep in the scheduler early so we can get the start timestep
                self.pipe.scheduler.set_timesteps(input.options.iterations, device=self.pipe._execution_device)
                print(self.pipe.scheduler.timesteps)
                self.start_timestep = int(self.pipe.scheduler.timesteps.cpu().numpy()[0])
                print(f"Start timestep is {self.start_timestep}")

                # Fallback to prompts without compel weights if the pipeline doesn't support prompt_embeds
                if no_prompt_weights_active:
                    generation_args["prompt"] = " ".join([f"{split_p.strip()}" for prompt in input.options.positive_prompts for split_p in prompt.prompt.split(",")])
                    generation_args["negative_prompt"] = " ".join([f"{split_p.strip()}" for prompt in input.options.negative_prompts for split_p in prompt.prompt.split(",")])
                else:
                    generation_args["prompt_embeds"] = prompt_tensors

                # Different capability flags use different keywords in the pipeline
                if strength_active:
                    generation_args["strength"] = input.options.strength

                # SDXL requires pooled prompt embeddings along with the regular prompt embeddings
                if requires_pooled_active:
                    generation_args["pooled_prompt_embeds"] = pooled_prompt_tensors
                    #generation_args["negative_pooled_prompt_embeds"] = negative_pooled_prompt_tensors

                # Add processed input layers                 
                generation_args.update(layer_img_mappings)

                # Set controlnet scales if available
                if len(controlnet_scales):
                    generation_args["controlnet_conditioning_scale"] = controlnet_scales if len(controlnet_scales) > 1 else controlnet_scales[0]

                # Set whether we want to return an image or just latent data
                if input.output_type == unreal.ImageType.LATENT:
                    generation_args["output_type"] = "latent"

                if pipeline_asset.options.python_pre_render_script:
                    pre_render_script_locals = {}
                    pre_render_script_args = {
                        "input": input, 
                        "pipeline_asset": pipeline_asset,
                        "model_options": model_options
                    }
                    print(f"Running pre-render script")
                    exec(pipeline_asset.options.python_pre_render_script, pre_render_script_args, pre_render_script_locals)
                    if "generation_args" in pre_render_script_locals:
                        print("Found updated generation_args in pre render script")
                        generation_args.update(pre_render_script_locals["generation_args"])

                if input.debug_python_images:
                    print("Generation args:")
                    pprint.pprint(generation_args)

                # Reset progress bar
                self.update_image_progress("inprogress", int(0), int(1), float(0.0), input.options.out_size_x, input.options.out_size_y, None)
            
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
                image = images[0] if not images is None else None

                if input.debug_python_images and not image is None:
                    image.show()

                if image is None:
                    print("No image was generated")
                else:
                    if pipeline_asset.options.python_post_render_script:
                        post_render_script_locals = {}
                        post_render_script_args = {"input_image": image, "generation_args": generation_args }
                        print(f"Running post-render script")
                        exec(pipeline_asset.options.python_post_render_script, post_render_script_args, post_render_script_locals)
                        image = post_render_script_locals["result_image"]# if "result_image" in post_render_script_locals else image
                
                # Gather result data
                print(f"Result model options: {model_options}")
                print(f"Result pipeline options: {pipeline_asset.options}")
                result.model = model_options
                result.pipeline = pipeline_asset.options
                result.lora = lora_asset.options if lora_asset else unreal.StableDiffusionModelOptions()

                # Save latent if required
                if input.output_type == unreal.ImageType.LATENT:
                    buffer = io.BytesIO()
                    torch.save(image, buffer)
                    result.out_latent = buffer.getvalue()
                    result.out_width = input.options.out_size_x
                    result.out_height = input.options.out_size_y
                else:
                    # Save texture
                    result.out_texture = PILImageToTexture(image.convert("RGBA"), out_texture, True) if not image is None else None
                    result.out_width = image.width if not image is None else input.options.out_size_x
                    result.out_height = image.height if not image is None else input.options.out_size_y
                
                result.input = input
                result.input.options.seed = seed
                print(f"Seed was {seed}. Saved as {result.input.options.seed}")
                result.completed = image is not None

                # Cleanup
                self.start_timestep = -1
                del self.executor 
                self.executor = None
                prompt_tensors = None
                pooled_prompt_tensors = None
                gc.collect()
                torch.cuda.empty_cache()
                torch.clear_autocast_cache()

        return result

    def ImageProgressStep(self, step: int, timestep: int, latents: torch.FloatTensor) -> None:
        pct_complete = (self.start_timestep - timestep) / self.start_timestep

        print(f"Step is {step}. Timestep is {timestep} Frequency is {self.update_frequency}. Modulo is {step % self.update_frequency}")
        texture = None
        image_size = (0,0)
        if (step % self.update_frequency == 0 or step == 1) and self.preview_texture:
            adjusted_latents = 1 / 0.18215 * latents
            image = self.pipe.vae.decode(adjusted_latents).sample
            image = (image / 2 + 0.5).clamp(0, 1)
            image = image.detach().cpu().permute(0, 2, 3, 1).numpy()
            image = self.pipe.numpy_to_pil(image)[0]
            texture = PILImageToTexture(image.convert("RGBA"), self.preview_texture, True)
            image_size = (image.width, image.height)
        
        self.update_image_progress("inprogress", int(step), int(timestep), float(pct_complete), image_size[0], image_size[1], texture)

        # Image finished we can now abort image generation
        if self.abort and self.executor:
            self.executor.stop()

    def build_prompt_tensors(self, positive_prompts: list[unreal.Prompt], negative_prompts: list[unreal.Prompt], compel: Compel):
         # Collate prompts
        positive_prompts = " ".join([f"({split_p.strip()}){prompt.weight}" for prompt in positive_prompts for split_p in prompt.prompt.split(",")])
        negative_prompts = " ".join([f"({split_p.strip()}){prompt.weight}" for prompt in negative_prompts for split_p in prompt.prompt.split(",")])
        print(positive_prompts)
        print(negative_prompts)
        positive_prompt_tensors, positive_pooled_tensors = compel.build_conditioning_tensor(positive_prompts if positive_prompts else "")
        negative_prompt_tensors, negative_pooled_tensors = compel.build_conditioning_tensor(negative_prompts if negative_prompts else "")
        return (torch.cat(compel.pad_conditioning_tensors_to_same_length([positive_prompt_tensors, negative_prompt_tensors])), torch.cat(compel.pad_conditioning_tensors_to_same_length([positive_pooled_tensors, negative_pooled_tensors])))

    @unreal.ufunction(override=True)
    def StopImageGeneration(self):
        self.abort = True
        # Update model status to let UI know the model is downloading or available
        current_model_status = self.get_editor_property("ModelStatus")

        # Let UI know that we're cancelling the model load - but we'll still have to wait
        # TODO: Multithreaded model loading?
        result = unreal.StableDiffusionModelInitResult()
        result.model_status = unreal.ModelStatus.CANCELLING if current_model_status.model_status == unreal.ModelStatus.LOADING else current_model_status.model_status
        result.model_name = current_model_status.model_name
        self.set_editor_property("ModelStatus", result)

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
            gc.collect()
            torch.cuda.empty_cache()

    @unreal.ufunction(override=True)
    def UpsampleImage(self, image_result: unreal.StableDiffusionImageResult, out_texture):
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
            
            input_pixels = unreal.StableDiffusionBlueprintLibrary.read_pixels(image_result.out_texture)
            image = FColorAsPILImage(input_pixels, image_result.out_texture.blueprint_get_size_x(), image_result.out_texture.blueprint_get_size_y()).convert("RGB")
            print(f"Upscaling image result from {image_result.out_width}:{image_result.out_height} to {image_result.out_width * 4}:{image_result.out_height * 4}")
            upsampled_image = active_upsampler(image)
            print("Upsample complete")
        
        # Free local upsampler to restore VRAM
        if local_upsampler:
            del local_upsampler
       

        # Build result
        result = unreal.StableDiffusionImageResult()
        result.model = image_result.model
        result.pipeline = image_result.pipeline
        result.lora = image_result.lora
        result.input = image_result.input
        result.out_texture =  PILImageToTexture(upsampled_image.convert("RGBA"), out_texture, True) if upsampled_image else None
        result.out_width = upsampled_image.width
        result.out_height = upsampled_image.height
        result.upsampled = True
        result.completed = True

        # Cleanup
        #gc.collect()
        #torch.cuda.empty_cache()

        return result
