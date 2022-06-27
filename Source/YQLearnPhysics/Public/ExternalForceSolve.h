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