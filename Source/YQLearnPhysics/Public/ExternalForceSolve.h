#pragma once
#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"
#include "CommonRenderResources.h"


void UpdateExternalForce(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef  InputPosition
	, FShaderResourceViewRHIRef NormalBuffer
	, FUnorderedAccessViewRHIRef OutputPosition
	, FShaderResourceViewRHIRef MaskBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, uint32 Length
	, float DeltaTime
);


void UpdateFixedParticlesTransform(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef MaskBuffer
	, FMatrix44f PreviousWorldToLocal
	, FMatrix44f NewLocalToWorld
	, uint32 NumParticles
	, uint32 BaseParticle
);