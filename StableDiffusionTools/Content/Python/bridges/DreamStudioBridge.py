import unreal
import os, inspect, random, io
import PIL
from PIL import Image
from diffusionconvertors import FColorAsPILImage, PILImageToTexture
from stability_sdk.client import *


@unreal.uclass()
class DreamStudioBridge(unreal.StableDiffusionBridge):
    def __init__(self):
        unreal.StableDiffusionBridge.__init__(self)

    @unreal.ufunction(override=True)
    def InitModel(self, new_model_options, new_pipeline_options, lora_asset, textual_inversion_asset, layers, allow_nsfw, padding_mode):
        result = unreal.StableDiffusionModelInitResult()
        self.set_editor_property("ModelOptions", new_model_options)
        self.set_editor_property("PipelineOptions", new_pipeline_options)
        self.model_loaded = True
        self.stability_api = StabilityInference(
            "grpc.stability.ai:443", self.get_token(), engine=new_model_options.model, verbose=True
        )
        
        print("Loaded DreamStudio model")
        result.model_status = unreal.ModelStatus.LOADED
        self.set_editor_property("ModelStatus", result)

        return result

    @unreal.ufunction(override=True)
    def GetTokenWebsiteHint(self):
        return "https://dreamstudio.ai/account"

    @unreal.ufunction(override=True)
    def GetRequiresToken(self):
        return True

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

        request =  {
            "height": input.options.out_size_y,
            "width": input.options.out_size_x,
            "start_schedule": input.options.strength,
            "end_schedule": 0.01,
            "cfg_scale": 7.0,
            "sampler": get_sampler_from_str("k_lms"),
            "steps": input.options.iterations,
            "seed": seed,
            "samples": 1,
            "init_image": guide_img
            #"mask_image": mask_img,
        }
        answers = self.stability_api.generate(positive_prompts, **request)
        artifacts = process_artifacts_from_answers(
            "generation_", positive_prompts, answers, write=False, verbose=True
        )

        image = None
        for path, artifact in artifacts:
            if artifact.type == generation.ARTIFACT_IMAGE:
                image = Image.open(io.BytesIO(artifact.binary)).convert("RGBA")
               
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
        unreal.log_warn("Upsampler not yet implemented for DreamStudio bridge")

        # Build result
        result = unreal.StableDiffusionImageResult()
        result.input = image_result.input
        result.pixel_data = image_result.pixel_data
        result.out_width = image_result.out_width
        result.out_height = image_result.out_height
        result.generated_texture = None
        result.upsampled = True

        return result
