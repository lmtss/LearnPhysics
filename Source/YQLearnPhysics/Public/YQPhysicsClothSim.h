#pragma once
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "CommonRenderResources.h"


void UpdateSimpleExternalForce(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef  InputPosition
	, FShaderResourceViewRHIRef NormalBuffer
	, FUnorderedAccessViewRHIRef OutputPosition
	, FShaderResourceViewRHIRef MaskBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, uint32 Length
	, float DeltaTime
);

void JacobiMassSpringProjection(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FShaderResourceViewRHIRef& MaskBuffer
	, FUnorderedAccessViewRHIRef& PositionBuffer
	, FUnorderedAccessViewRHIRef& VelocityBuffer
	, float DeltaTime
);


void InitializeClothVertex(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef& BufferA
	, FUnorderedAccessViewRHIRef& BufferB
	, FUnorderedAccessViewRHIRef& MaskBuffer
	, uint32 LengthToClear
);

void InitializeClothIndexBuffer(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef& IndexBuffer
	, uint32 LengthToClear
);


void CalcNormalByCrossPosition(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FUnorderedAccessViewRHIRef& NormalBuffer
	, float Intensity
);


void JacobiSimpleCollisionProjection(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FShaderResourceViewRHIRef& MaskBuffer
	, FUnorderedAccessViewRHIRef& PositionBuffer
	, FUnorderedAccessViewRHIRef& VelocityBuffer
	, FVector4f SphereCenterAndRadius
	, float DeltaTime
);


void JacobiSimpleCollisionProjectionFeedback(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FShaderResourceViewRHIRef& MaskBuffer
	, FUnorderedAccessViewRHIRef& PositionBuffer
	, FUnorderedAccessViewRHIRef& VelocityBuffer
	, FUnorderedAccessViewRHIRef& FeedbackBuffer
	, FUnorderedAccessViewRHIRef& FeedbackBufferSizes
	, FVector4f SphereCenterAndRadius
	, float DeltaTime
);