// @lint-ignore-every LICENSELINT
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OculusXRHMD_SpectatorScreenController.h"

#if OCULUS_HMD_SUPPORTED_PLATFORMS
#include "OculusXRHMD.h"
#include "TextureResource.h"
#include "Engine/TextureRenderTarget2D.h"

namespace OculusXRHMD
{

	//-------------------------------------------------------------------------------------------------
	// FSpectatorScreenController
	//-------------------------------------------------------------------------------------------------

	FSpectatorScreenController::FSpectatorScreenController(FOculusXRHMD* InOculusXRHMD)
		: FDefaultSpectatorScreenController(InOculusXRHMD)
		, OculusXRHMD(InOculusXRHMD)
		, SpectatorMode(EMRSpectatorScreenMode::Default)
		, ForegroundRenderTexture(nullptr)
		, BackgroundRenderTexture(nullptr)
	{
	}

	void FSpectatorScreenController::RenderSpectatorScreen_RenderThread(FRHICommandListImmediate& RHICmdList, FRHITexture* BackBuffer, FTextureRHIRef RenderTexture, FVector2D WindowSize)
	{
		CheckInRenderThread();
		if (OculusXRHMD->GetCustomPresent_Internal())
		{
			if (SpectatorMode == EMRSpectatorScreenMode::ExternalComposition)
			{
				auto ForegroundResource = ForegroundRenderTexture->GetRenderTargetResource();
				auto BackgroundResource = BackgroundRenderTexture->GetRenderTargetResource();
				if (ForegroundResource && BackgroundResource)
				{
					RenderSpectatorModeExternalComposition(
						RHICmdList,
						FTextureRHIRef(BackBuffer),
						ForegroundResource->GetRenderTargetTexture(),
						BackgroundResource->GetRenderTargetTexture());
					return;
				}
			}
			else if (SpectatorMode == EMRSpectatorScreenMode::DirectComposition)
			{
				auto BackgroundResource = BackgroundRenderTexture->GetRenderTargetResource();
				if (BackgroundResource)
				{
					RenderSpectatorModeDirectComposition(
						RHICmdList,
						FTextureRHIRef(BackBuffer),
						BackgroundRenderTexture->GetRenderTargetResource()->GetRenderTargetTexture());
					return;
				}
			}
			FDefaultSpectatorScreenController::RenderSpectatorScreen_RenderThread(RHICmdList, BackBuffer, RenderTexture, WindowSize);
		}
	}

	void FSpectatorScreenController::RenderSpectatorModeUndistorted(FRHICommandListImmediate& RHICmdList, FTextureRHIRef TargetTexture, FTextureRHIRef EyeTexture, FTextureRHIRef OtherTexture, FVector2D WindowSize)
	{
		CheckInRenderThread();
		FSettings* Settings = OculusXRHMD->GetSettings_RenderThread();
		FIntRect DestRect(0, 0, TargetTexture->GetSizeX() / 2, TargetTexture->GetSizeY());
		for (int i = 0; i < 2; ++i)
		{
			OculusXRHMD->CopyTexture_RenderThread(RHICmdList, EyeTexture, Settings->EyeRenderViewport[i], TargetTexture, DestRect, false, true);
			DestRect.Min.X += TargetTexture->GetSizeX() / 2;
			DestRect.Max.X += TargetTexture->GetSizeX() / 2;
		}
	}

	void FSpectatorScreenController::RenderSpectatorModeDistorted(FRHICommandListImmediate& RHICmdList, FTextureRHIRef TargetTexture, FTextureRHIRef EyeTexture, FTextureRHIRef OtherTexture, FVector2D WindowSize)
	{
		CheckInRenderThread();
		FCustomPresent* CustomPresent = OculusXRHMD->GetCustomPresent_Internal();
		FTextureRHIRef MirrorTexture = CustomPresent->GetMirrorTexture();
		if (MirrorTexture)
		{
			FIntRect SrcRect(0, 0, MirrorTexture->GetSizeX(), MirrorTexture->GetSizeY());
			FIntRect DestRect(0, 0, TargetTexture->GetSizeX(), TargetTexture->GetSizeY());
			OculusXRHMD->CopyTexture_RenderThread(RHICmdList, MirrorTexture, SrcRect, TargetTexture, DestRect, false, true);
		}
	}

	void FSpectatorScreenController::RenderSpectatorModeSingleEye(FRHICommandListImmediate& RHICmdList, FTextureRHIRef TargetTexture, FTextureRHIRef EyeTexture, FTextureRHIRef OtherTexture, FVector2D WindowSize)
	{
		CheckInRenderThread();
		FSettings* Settings = OculusXRHMD->GetSettings_RenderThread();
		const FIntRect SrcRect = Settings->EyeRenderViewport[0];
		const FIntRect DstRect(0, 0, TargetTexture->GetSizeX(), TargetTexture->GetSizeY());

		OculusXRHMD->CopyTexture_RenderThread(RHICmdList, EyeTexture, SrcRect, TargetTexture, DstRect, false, true);
	}

	void FSpectatorScreenController::RenderSpectatorModeDirectComposition(FRHICommandListImmediate& RHICmdList, FTextureRHIRef TargetTexture, const FTextureRHIRef SrcTexture) const
	{
		CheckInRenderThread();
		const FIntRect SrcRect(0, 0, SrcTexture->GetSizeX(), SrcTexture->GetSizeY());
		const FIntRect DstRect(0, 0, TargetTexture->GetSizeX(), TargetTexture->GetSizeY());

		OculusXRHMD->CopyTexture_RenderThread(RHICmdList, SrcTexture, SrcRect, TargetTexture, DstRect, false, true);
	}

	void FSpectatorScreenController::RenderSpectatorModeExternalComposition(FRHICommandListImmediate& RHICmdList, FTextureRHIRef TargetTexture, const FTextureRHIRef FrontTexture, const FTextureRHIRef BackTexture) const
	{
		CheckInRenderThread();
		const FIntRect FrontSrcRect(0, 0, FrontTexture->GetSizeX(), FrontTexture->GetSizeY());
		const FIntRect FrontDstRect(0, 0, TargetTexture->GetSizeX() / 2, TargetTexture->GetSizeY());
		const FIntRect BackSrcRect(0, 0, BackTexture->GetSizeX(), BackTexture->GetSizeY());
		const FIntRect BackDstRect(TargetTexture->GetSizeX() / 2, 0, TargetTexture->GetSizeX(), TargetTexture->GetSizeY());

		OculusXRHMD->CopyTexture_RenderThread(RHICmdList, FrontTexture, FrontSrcRect, TargetTexture, FrontDstRect, false, true);
		OculusXRHMD->CopyTexture_RenderThread(RHICmdList, BackTexture, BackSrcRect, TargetTexture, BackDstRect, false, true);
	}

} // namespace OculusXRHMD

#endif // OCULUS_HMD_SUPPORTED_PLATFORMS
