Stable Diffusion Tools for Unreal Engine
========================================

Description
-----------
This plugin generates AI synthesized images in Unreal Editor.

![image](https://github.com/Mystfit/Unreal-StableDiffusionTools/assets/795851/35bc1146-0a2b-4259-a10d-314a89c10a8b)
![image](https://github.com/Mystfit/Unreal-StableDiffusionTools/assets/795851/565313ed-6cbb-4311-93af-54f11c76e859)

Join us on [Discord](https://discord.gg/9m6HxaDB62).

Requirements
------------

* Unreal Engine 5.1+
* CUDA compatible GPU (Nvidia)
* Minimum 6GB Vram to run Unreal and half-precision floating point (fp16) Stable Diffusion simultaneously
* [Git](https://git-scm.com/)


Installation
------------

Setup instructions are available in the [wiki](https://github.com/Mystfit/Unreal-StableDiffusionTools/wiki/Installation-and-setup).


Usage
-----

[Video walkthrough for v0.4](https://youtu.be/JR1s2AhejvA)


Compatible generators
-----

This plugin provides three different AI image generator systems (bridges) out of the box. Local generation using the [Diffusers library](https://github.com/huggingface/diffusers), remote generation using the Stability.AI SDK and [Dream Studio](https://beta.dreamstudio.ai/dream) and remote generation using [Stable Horde](https://stablehorde.net/).


FAQ
------

* Help, I don't know how any of this works.
  * Take a look at the [wiki](https://github.com/Mystfit/Unreal-StableDiffusionTools/wiki).


Known issues
-----

* Sequencer generation options section isn't serialising pipeline assets and stages.
* Cancelling a model load can occasionally disable the generation button. Relaunch the editor to fix.


Roadmap
-----

* Serialisable fields for the plugin UI.
* Queue multiple image generations.
* Paint tools for mask editing (inpaint).
* Better documentation.
