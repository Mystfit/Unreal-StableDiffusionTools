# Original file from https://github.com/nateraw/stable-diffusion-videos licensed under Apache 2.0

from pathlib import Path

import typing
from typing import Optional
import pathlib
from PIL import Image
from huggingface_hub import hf_hub_download
from torch import nn
import numpy as np
from typing import Union

try:
    from realesrgan import RealESRGANer
    from basicsr.archs.rrdbnet_arch import RRDBNet
except ImportError as e:
    raise ImportError(
        "You tried to import realesrgan without having it installed properly. To install Real-ESRGAN, run:\n\n"
        "pip install realesrgan"
    )


class RealESRGANModel(nn.Module):
    def __init__(self, model_path, tile=0, tile_pad=10, pre_pad=0, fp32=False):
        super().__init__()
        try:
            from basicsr.archs.rrdbnet_arch import RRDBNet
            from realesrgan import RealESRGANer
        except ImportError as e:
            raise ImportError(
                "You tried to import realesrgan without having it installed properly. To install Real-ESRGAN, run:\n\n"
                "pip install realesrgan"
            )

        model = RRDBNet(num_in_ch=3, num_out_ch=3, num_feat=64, num_block=23, num_grow_ch=32, scale=4)
        self.upsampler = RealESRGANer(
            scale=4, model_path=model_path, model=model, tile=tile, tile_pad=tile_pad, pre_pad=pre_pad, half=not fp32
        )

    def forward(self, image: Union[Image.Image, np.ndarray], outscale=4, convert_to_pil=True) -> Union[np.ndarray, Image.Image]:
        """Upsample an image array or path.

        Args:
            image (Union[np.ndarray, str]): Either a np array or an image path. np array is assumed to be in RGB format,
                and we convert it to BGR.
            outscale (int, optional): Amount to upscale the image. Defaults to 4.
            convert_to_pil (bool, optional): If True, return PIL image. Otherwise, return numpy array (BGR). Defaults to True.

        Returns:
            Union[np.ndarray, PIL.Image.Image]: An upsampled version of the input image.
        """
        img = np.array(image) if isinstance(image, Image.Image) else image
        #img = img[:, :, ::-1]
        if img.shape[2] == 4:
            # Convert to BGRA
            img = img[:, :, [2, 1, 0, 3]]
        else:
            # Convert to BGR
            img = img[:, :, ::-1]    

        print(f"Input upsample array: {img}")

        image, _ = self.upsampler.enhance(img, outscale=outscale)
        
        print(f"Output upsample array: {image}")

        if convert_to_pil:
            image = Image.fromarray(image[:, :, ::-1])

        return image

    @classmethod
    def from_pretrained(cls, model_name_or_path="nateraw/real-esrgan", cache_dir: Optional[str] = None):
        """Initialize a pretrained Real-ESRGAN upsampler.

        Example:
            ```python
            >>> from stable_diffusion_videos import PipelineRealESRGAN
            >>> pipe = PipelineRealESRGAN.from_pretrained('nateraw/real-esrgan')
            >>> im_out = pipe('input_img.jpg')
            ```

        Args:
            model_name_or_path (str, optional): The Hugging Face repo ID or path to local model. Defaults to 'nateraw/real-esrgan'.

        Returns:
            stable_diffusion_videos.PipelineRealESRGAN: An instance of `PipelineRealESRGAN` instantiated from pretrained model.
        """
        # reuploaded form official ones mentioned here:
        # https://github.com/xinntao/Real-ESRGAN
        if Path(model_name_or_path).exists():
            file = model_name_or_path
        else:
            file = hf_hub_download(model_name_or_path, "RealESRGAN_x4plus.pth", cache_dir=cache_dir)
        return cls(file)

    def upsample_imagefolder(self, in_dir, out_dir, suffix="out", outfile_ext=".png"):
        in_dir, out_dir = Path(in_dir), Path(out_dir)
        if not in_dir.exists():
            raise FileNotFoundError(f"Provided input directory {in_dir} does not exist")

        out_dir.mkdir(exist_ok=True, parents=True)

        image_paths = [x for x in in_dir.glob("*") if x.suffix.lower() in [".png", ".jpg", ".jpeg"]]
        for image in image_paths:
            im = self(str(image))
            out_filepath = out_dir / (image.stem + suffix + outfile_ext)
            im.save(out_filepath)
