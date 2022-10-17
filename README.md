Stable Diffusion Tools for Unreal Engine
========================================

Description
-----------
This plugin feeds your editor viewport through the [Diffusers](https://github.com/huggingface/diffusers) library to create AI synthesized imagery.

![StableDIffusionPluginUnreal](https://user-images.githubusercontent.com/795851/195005569-b7b33432-a981-4f76-81d5-d9948861fd84.png)


Installation
------------
Clone this repo and copy the `StableDiffusionTools` folder to `YourProjectFolder/Plugins/` or download the latest release from the [releases page.](https://github.com/Mystfit/Unreal-StableDiffusionTools/releases)

Usage
-----
[Video walkthrough for v0.1](https://youtu.be/dihSydSkd4I)

### Enabling the plugin
After installing the plugin, access the Stable Diffusion Tools window in your editor via `Windows->StableDiffusionTools`.

### Dependencies
The first time you use the plugin you will need to install the required python dependencies using the `Update/Install dependencies` button in the Dependencies section. You may need to restart the Unreal Editor the first time you install dependencies.

### Models
To download a model from [huggingface.co](https://huggingface.co), you will need an account and a token with read permissions. The button next to the token input in the `Models` section will open a browser window that will take you to the token creation page if you are logged in to huggingface.co, or you can visit [this link.](https://huggingface.co/settings/tokens)

You can use any diffusers based model from [the diffusers category on huggingface.co.](https://huggingface.co/models?library=diffusers) To choose a model, enter it in the format `Username/Modelname`. To use the default model, you need to vist the [model card page](https://huggingface.co/CompVis/stable-diffusion-v1-4) and accept the usage agreement in order to download the weights. 

The `revision` property will allow you to pick a specific branch of the model to download. I recommend the `fp16` branch of the default `CompVis/stable-diffusion-v1-4` in case you don't have a limited amount of VRAM (under 6GB). You can also choose the level of precision you want to model to use where 16-bit floating point is the default.

**Note:** For the first release, there may be an issue with downloading git lfs files hosted in models.

### Prompt based image generation

Use the generation section to tweak your prompt, iteration count, viewport influence strength and the seed. If you are new to prompt authoring then I recommend treading [this guide.](https://www.howtogeek.com/833169/how-to-write-an-awesome-stable-diffusion-prompt/)

After you click the `Generate image` button, the in-progress image will display in the plugin window and log its progress in the editor's `Output Log` panel.

### Saving results

The Image outputs section contains options for saving a generated image to either a texture or an external file. Check the boxes for the assets you would like to export, enter the destination paths and the name of the image and click the `Save` button.

## Known issues

* Fix texture memory usage.
* Panning the generated image only works when grabbing it from the upper right quarter.
* Downloaded models are not pulling git lfs files for some reason.
* You may have to restart the editor after installing dependencies.
* No persistence for fields in the editor. These will eventually be stored in a data asset.

## Roadmap

* Add sequencer support.
* Add data assets to hold input information like prompt and seed along with the generated textures.
* Add image upscaling.
* Add input image masking using layers and stencil buffers.
   
