#pragma once

#include "RenderGraphUtils.h"

struct FGPUBvhBuffers
{
	// BV位置信息
	FRWBuffer BVPositionBufferA[2];
	FRWBuffer BVPositionBufferB[2];

	// 树结构
	FRWBuffer BVHChildIDBuffer;
	FRWBuffer BVHParentIDBuffer;
	
	// 莫顿码
	FRWBuffer MortonCodeBuffers[2];

	// Delta，i和i+1之间最长共同前缀长度
	FRWBuffer KeyDeltaBuffer;

	// 用于归约时判断是否是第二个访问节点的线程
	FRWBuffer AtomicAccessBuffer;

	// 构造时使用，标记内节点的范围
	FRWBuffer InternalNodeRangeBuffer;
};

void VisualizeBVH_RenderThread(
	FRHICommandList& RHICmdList
	, FSceneView& InView
	, FShaderResourceViewRHIRef LeafBVBuffer0
	, FShaderResourceViewRHIRef LeafBVBuffer1
	, FShaderResourceViewRHIRef InternalNodeBVBuffer0
	, FShaderResourceViewRHIRef InternalNodeBVBuffer1
	, FShaderResourceViewRHIRef BVHChildIDBuffer
	, FShaderResourceViewRHIRef PhysicsSceneViewBuffer
	, FRWBuffer& VisualizeNodeIDBuffer
	, FRWBuffer& IndirectArgBuffer
	, uint32 LayerIndex
);


void GPUGenerateBVH_RenderThread(
    FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FShaderResourceViewRHIRef TriangleIndexBuffer
    , FGPUBvhBuffers& GPUBvhBuffers
	, FRWBuffer& PhysicsSceneViewBuffer
	, FBox BoundingBox
    , uint32 Length
);
