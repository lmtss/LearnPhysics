#pragma once
#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"
#include "CommonRenderResources.h"


void CalcNormalByCross(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FUnorderedAccessViewRHIRef OutputNormalBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, uint32 NumVertices
	, uint32 NumTriangles
	, uint32 OffsetParticles
);

void CalcNormalAndTangent(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef TexCoordBuffer
	, FUnorderedAccessViewRHIRef OutputNormalBuffer
	, FUnorderedAccessViewRHIRef OutputTangentBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, uint32 WriteOffset
	, uint32 PositionBufferOffset
	, uint32 NumVertices
	, uint32 NumTriangles
);