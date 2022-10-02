// Fill out your copyright notice in the Description page of Project Settings.
#include "StableDiffusionBridge.h"
#include "Math/Color.h"

UStableDiffusionBridge* UStableDiffusionBridge::Get()
{
    TArray<UClass*> PythonBridgeClasses;
    GetDerivedClasses(UStableDiffusionBridge::StaticClass(), PythonBridgeClasses);
    int32 NumClasses = PythonBridgeClasses.Num();
    if (NumClasses > 0)
    {
        return Cast<UStableDiffusionBridge>(PythonBridgeClasses[NumClasses - 1]->GetDefaultObject());
    }
    return nullptr;
};

UTexture2D* UStableDiffusionBridge::ColorBufferToTexture(const FString& FrameName, const TArray<FColor>& FrameColors, const FIntPoint& FrameSize)
{
	if (!FrameColors.Num())
		return nullptr;

	TObjectPtr<UTexture2D> OutTex = NewObject<UTexture2D>();//UTexture2D::CreateTransient(FrameSize.X, FrameSize.Y, EPixelFormat::PF_B8G8R8A8, FName(FrameName));
	OutTex->MipGenSettings = TMGS_NoMipmaps;
	OutTex->PlatformData = new FTexturePlatformData();
	OutTex->PlatformData->SizeX = FrameSize.X;
	OutTex->PlatformData->SizeY = FrameSize.Y;
	OutTex->PlatformData->SetNumSlices(1);
	OutTex->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	// Allocate first mipmap.
	FTexture2DMipMap* Mip = new(OutTex->PlatformData->Mips) FTexture2DMipMap();
	Mip->SizeX = FrameSize.X;
	Mip->SizeY = FrameSize.Y;

	// Lock the texture so it can be modified
	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(FrameSize.X * FrameSize.Y * 4);
	FMemory::Memcpy(TextureData, FrameColors.GetData(), sizeof(uint8) * FrameSize.X * FrameSize.Y * 4);
	Mip->BulkData.Unlock();

	FTaskTagScope TaskTagScope(ETaskTag::EParallelRenderingThread);
	OutTex->UpdateResource();

	OutTex->Source.Init(FrameSize.X, FrameSize.Y, 1, 1, ETextureSourceFormat::TSF_RGBA8, reinterpret_cast<const uint8*>(FrameColors.GetData()));
	OutTex->UpdateResource();

	return OutTex;
}
