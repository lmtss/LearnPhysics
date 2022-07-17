#pragma once
#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"
#include "CommonRenderResources.h"


void VoxelizeMeshSurface(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, uint32 NumVertices
	, uint32 NumTriangles
);