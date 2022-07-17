#include "GPUVoxelize.h"



void VoxelizeMeshSurface(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, uint32 NumVertices
	, uint32 NumTriangles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, VoxelizeMeshSurface);


}