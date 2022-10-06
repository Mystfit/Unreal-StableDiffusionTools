SD_dependencies_installed = False

try:
    import diffusers
    import PIL
    import torch
    SD_dependencies_installed = True
    print("Stable Diffusion dependencies are available")
except ImportError as e:
    print("Stable Diffusion plugin python dependencies not installed")

try:
    import load_diffusers_bridge
except ImportError:
    print("Skipping default Diffusers Bridge load until dependencies have been installed")
