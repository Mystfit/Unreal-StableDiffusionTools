import os, pathlib
import unreal

def get_model_name_or_path(model_options: unreal.StableDiffusionModelOptions) -> tuple[str, bool]:
    model_is_file = os.path.exists(model_options.local_file_path.file_path) if model_options.local_file_path.file_path else False
    model_is_folder = os.path.exists(model_options.local_folder_path.path) if model_options.local_folder_path.path else False

    model_name = model_options.model
    model_name = model_options.local_file_path.file_path if model_is_file else model_name
    model_name = model_options.local_folder_path.path if model_is_folder else model_name

    # Single file loading if we provide a URL for diffusers model types
    if model_options.external_url and model_options.model_type == unreal.ModelType.DIFFUSERS:
        model_name = model_options.external_url
        model_is_file = True

    return model_name, model_is_file
