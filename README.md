Stable Diffusion Tools for Unreal Engine
========================================

Description
-----------
This plugin feeds your editor viewport through the [Diffusers](https://github.com/huggingface/diffusers) library to create AI synthesized imagery.

![StableDiffusionPluginUnreal](https://user-images.githubusercontent.com/795851/195005569-b7b33432-a981-4f76-81d5-d9948861fd84.png)

Join us on [Discord](https://discord.gg/9m6HxaDB62).

Requirements
------------

* Unreal Engine 5.0
* CUDA compatible GPU (Nvidia)
* Minimum 6GB Vram (to run Unreal and fp16 Stable Diffusion simultaneously)


Installation
------------
Clone this repo and copy the `StableDiffusionTools` folder to `YourProjectFolder/Plugins/` or download the latest release from the [releases page.](https://github.com/Mystfit/Unreal-StableDiffusionTools/releases)

Usage
-----
[Video walkthrough for v0.1](https://youtu.be/dihSydSkd4I)

### Enabling the plugin
After installing the plugin, activate it through `Windows->Plugins`. To open the main tool window for the plugin, go to `Windows->StableDiffusionTools`.

### Dependencies
The first time you use the plugin you will need to install the required python dependencies using the `Update/Install dependencies` button in the Dependencies section. You may need to restart the Unreal Editor the first time you install dependencies.

### Models

![androidquinn](https://user-images.githubusercontent.com/795851/197150314-1b2fee89-3670-47ff-a9ab-473243ba544c.gif)

To generate images using this plugin, you will first need a model. A few model presets are provided with the plugin and you can create your own by creating a `StableDiffusionModelAsset` or you can just enter the model options in the plugin window directly.

To download a model from [huggingface.co](https://huggingface.co), you will need an account and a token with read permissions. The button next to the token input in the `Models` section will open a browser window that will take you to the token creation page if you are logged in to huggingface.co, or you can visit [this link.](https://huggingface.co/settings/tokens)

You can use any diffusers based model from [the diffusers category on huggingface.co.](https://huggingface.co/models?library=diffusers) To choose a model, enter it in the format `Username/Modelname`. To use any of the default models [RunwayML Stable Diffusion 1.5](https://huggingface.co/runwayml/stable-diffusion-v1-5), [Runway inpaiting](https://huggingface.co/runwayml/stable-diffusion-inpainting), or [CompViz Stable Diffusion 1.4](https://huggingface.co/CompVis/stable-diffusion-v1-4), you will need to vist the respective model card page for each of these models and accept their usage agreement before downloading their weights otherwise you will receive an HTTP401 error. 

The `revision` property will allow you to pick a specific branch of the model to download. I recommend the `fp16` branch (if the model provides it) in case you have a limited amount of VRAM (under 6GB). You can also choose the level of precision you want to model to use where 16-bit floating point is the default.

### Seamless textures

If you set the `Covolution padding` model option to `circular`, then generated images will have borders that will wrap around to the opposite side of the image. When combined with prompts such as `a tiling texture of INSERT_SUBJECT_HERE` this will generate textures that are appropriate for mapping onto flat surfaces or landscapes.

### Prompt based image generation

Use the generation section to tweak your prompt, iteration count, viewport influence strength and the seed. If you are new to prompt authoring then I recommend reading [this guide.](https://www.howtogeek.com/833169/how-to-write-an-awesome-stable-diffusion-prompt/)

After you click the `Generate image` button, the in-progress image will display in the plugin window and log its progress in the editor's `Output Log` panel.

### Saving results

The Image outputs section contains options for saving a generated image to either a texture or an external file as well as upsampling options. Enter the destination paths and the name of the image and click the `Save` button for the asset type of your choice, or upsample the output 4x first using Real-ESRGAN.

### Sequencer and exporting animations

https://user-images.githubusercontent.com/795851/198834362-5d6e31eb-9a06-4092-adb0-ee5f8bbf1094.mov

![image](https://user-images.githubusercontent.com/795851/196573891-09b07713-5a29-4bde-8592-f028c28b32f3.png)

The plugin will add two new types of tracks to the Sequencer which can be added using the `+Track` button, a `Stable Diffusion Options` track and `Stable Diffusion Prompt` tracks. 

To create an animation, create a single Options track with a section that will span the length of your animation. You can right-click the section and go to properties to modify parameters that will stay consistent over the course of the animation such as the model options and output frame size. 

The options track has 3 animatable channels, Iterations, Seed and Strength. All of these can be keyframed across the length of your animation.

To add prompts, you can create multiple prompt tracks containing multiple prompt sections. To set the prompt text, right-click on a prompt section, go to properties, and set the prompt text as well as the sentiment. A positive sentiment will tell Stable Diffusion that this prompt is something we would like to see in our generated output, whilst negative sentiments will instruct it to exclude unwanted prompts from the output.

Prompt tracks have a single animatable parameters, `Weight`. Increasing or decreasing this value will increase or decrease the influence of the prompt in your animation.

To render your animation, click the `Render movie` button to open the movie pipeline panel for your sequence. Add a `SDMoviePipelineConfig` config asset to instruct the pipeline to export a Stable Diffusion image sequence. Rendered frames by default will show up in `YourProjectFolder/Saved/MovieRenders`.

**Note:** Currently the Stable Diffusion Movie Pipeline will only export floating point images so I recommend sticking to OpenEXR as the output file type.

Click `Render (local)` to render your stable diffusion animation. If you haven't already initialised a model, then the pipeline will use the model options set in your Options track to initialise a model at the start of the render which may cause a white screen.

### Inpainting

![image](https://user-images.githubusercontent.com/795851/200820616-dd505989-2dc6-475d-9986-61e3abf26dd0.png)

Inpainting lets you fill only a masked portion of your input image whilst keeping areas outside the mask consistent. To use, load the `Runway1-5_Inpaint` model preset or choose any other model and make sure that `Enable inpainting` is set in the model options. To create a mask, add actors to an actor layer and then set the `Inpaint layer` field in the UI to the actor layer you want to use as a mask for inpainting. If using the sequencer, you will need to set the inpaint layer in the properties of an options track and enable `Render CustomDepth Pass` and set `CustomDepth Stencil Value` to **1** for each actor on the layer.

**IMPORTANT:** When generating images from the UI panel, the input image will be too dark. This is due to the plugin now using a SceneCapture2D actor to capture the viewport to enable inpainting stencil support.

To fix this change the following change the following options on a global post processing volume:
- Metering mode = **Manual**
- Exposure Compensation = **15**


## Known issues

* Fix texture memory usage.
* Panning the generated image only works when grabbing it from the upper right quarter.
* You will have to restart the editor after installing dependencies.
* No persistence for fields in the editor. These will eventually be stored in a data asset.

## Roadmap

* Add data assets to hold input information like prompt and seed along with the generated textures.
   
