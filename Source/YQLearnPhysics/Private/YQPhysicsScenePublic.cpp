#include "YQPhysicsScene.h"


FRWBuffer& FYQPhysicsScene::GetIndirectArgBuffer()
{
	return IndirectArgBuffer;
}

FRWBuffer& FYQPhysicsScene::GetVisualizeBVHNodeBuffer()
{
	return VisualizeNodeIDBuffer;
}

FRWBuffer& FYQPhysicsScene::GetPhysicsSceneViewBuffer()
{
	return PhysicsSceneViewBuffers[PhysicsSceneViewBufferPingPang];
}

//FIndexBuffer& FYQPhysicsScene::GetParticleIndexBufferForRender()
//{
//	return IndexBufferForRender;
//}

FRWBuffer& FYQPhysicsScene::GetParticleIndexBuffer()
{
	return IndexBuffer;
}

FRWBuffer& FYQPhysicsScene::GetPositionBufferA()
{
	return PositionBufferA;
}

FRWBuffer& FYQPhysicsScene::GetPositionBufferB()
{
	return PositionBufferB;
}

FRWBuffer& FYQPhysicsScene::GetVelocityBuffer()
{
	return VelocityBuffer;
}

FRWBuffer& FYQPhysicsScene::GetNormalBuffer()
{
	return NormalBuffer;
}

FRWBuffer& FYQPhysicsScene::GetVertexMaskBuffer()
{
	return VertexMaskBuffer;
}

FRWBuffer& FYQPhysicsScene::GetFeedbackBuffer()
{
	return FeedbackBuffer;
}

FRWBuffer& FYQPhysicsScene::GetFeedbackBufferSizes()
{
	return FeedbackBufferSizes;
}

FRWBuffer& FYQPhysicsScene::GetAccumulateDeltaPositionXBuffer()
{
	return AccumulateDeltaPositionXBuffer;
}

FRWBuffer& FYQPhysicsScene::GetAccumulateDeltaPositionYBuffer()
{
	return AccumulateDeltaPositionYBuffer;
}

FRWBuffer& FYQPhysicsScene::GetAccumulateDeltaPositionZBuffer()
{
	return AccumulateDeltaPositionZBuffer;
}

FRWBuffer& FYQPhysicsScene::GetAccumulateCountBuffer()
{
	return AccumulateCountBuffer;
}

FRWBuffer& FYQPhysicsScene::GetDistanceConstraintsParticleAIDBuffer()
{
	return DistanceConstraintsParticleAIDBuffer;
}

FRWBuffer& FYQPhysicsScene::GetDistanceConstraintsParticleBIDBuffer()
{
	return DistanceConstraintsParticleBIDBuffer;
}

FRWBuffer& FYQPhysicsScene::GetDistanceConstraintsDistanceBuffer()
{
	return DistanceConstraintsDistanceBuffer;
}

FGPUBvhBuffers& FYQPhysicsScene::GetGPUBvhBuffers()
{
	return GPUBvhBuffers;
}

uint32 FYQPhysicsScene::GetNumParticlesFromGPU()
{
	return NumParticlesFromGPU;
}

FRWBuffer& FYQPhysicsScene::GetPredictPositionBuffer()
{
	return PredictPositionBuffer;
}

FRWBuffer& FYQPhysicsScene::GetOrignalPositionBuffer()
{
	return OrignalPositionBuffer;
}

FRWBuffer& FYQPhysicsScene::GetBendingConstraintsParticleAIDBuffer()
{
	return BendingConstraintsParticleAIDBuffer;
}

FRWBuffer& FYQPhysicsScene::GetBendingConstraintsParticleBIDBuffer()
{
	return BendingConstraintsParticleBIDBuffer;
}

FRWBuffer& FYQPhysicsScene::GetBendingConstraintsParticleCIDBuffer()
{
	return BendingConstraintsParticleCIDBuffer;
}

FRWBuffer& FYQPhysicsScene::GetBendingConstraintsParticleDIDBuffer()
{
	return BendingConstraintsParticleDIDBuffer;
}

FRWBuffer& FYQPhysicsScene::GetBendingConstraintsAngleBuffer()
{
	return BendingConstraintsAngleBuffer;
}

void FYQPhysicsScene::Tick(FRHICommandList& RHICmdList)
{
	check(IsInRenderingThread());

	//IsBVHInit = true;
	// GenerateDistanceConstraints(
	// 	DistanceConstraintsParticleAIDBuffer.UAV
	// 	, DistanceConstraintsParticleBIDBuffer.UAV
	// 	, DistanceConstraintsDistanceBuffer.UAV
	// 	, TempUIntConstraintsParamBuffer.SRV
	// 	, TempFloatConstraintsParamBuffer.SRV
	// 	, 0
	// 	, 32
	// );
	
	// 流压缩

	

	// append 新的物体
	AddGPUObjectToBuffer(RHICmdList);
	CopyDataToCPU(RHICmdList);
	UpdateCPUObjectTransform();
	UpdateGPUObjectTransform(RHICmdList);
}