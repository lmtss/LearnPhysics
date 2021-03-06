#include "YQPhysicsSimulator.h"
#include "YQStreamCompact.h"
#include "PBDConstraintSolve.h"
#include "GPUBasedBVH.h"
#include "ExternalForceSolve.h"
#include "MeshModify.h"
#include "CollisionResponse.h"

#include "YQStiffness.h"


int UYQPhysicsBlueprintLibrary::AddStaticMeshActorToPhysicsScene(UStaticMeshComponent* StaticMeshComponent)
{
	return 0;
}

void FYQPhysicsSimulator::SetPhysicsScene(FYQPhysicsScene* InScene)
{
	PhysicsScene = InScene;
}


void FYQPhysicsSimulator::SetScene(FScene* InScene)
{
	Scene = InScene;
}

FYQPhysicsSimulator::FYQPhysicsSimulator()
	: Scene(nullptr)
{

}



FYQPhysicsSimulator::~FYQPhysicsSimulator()
{

}


//void Update_RenderThread(FRHICommandList& RHICmdList, FYQPhysicsScene* PhysicsScene)
//{
//
//	FGPUBvhBuffers& GPUBvhBuffers = PhysicsScene->GetGPUBvhBuffers();
//
//	GPUGenerateBVH_RenderThread(
//		RHICmdList
//		, PhysicsScene->GetPositionBuffer().SRV
//		, TriangleIndexBuffer.SRV
//		, GPUBvhBuffers
//		, PhysicsScene->GetPhysicsSceneViewBuffer()
//		, PhysicsScene->GetBoundingBox()
//		, PhysicsScene->GetNumStaticMeshTriangles()
//	);
//}

void FYQPhysicsSimulator::Tick(float DeltaSeconds)
{
	//UE_LOG(LogTemp, Log, TEXT("FYQPhysicsSimulator::Tick %f"), DeltaSeconds)

	DeltaSeconds = FMath::Clamp(DeltaSeconds, 0.0, 0.05);

	/*if(Scene != nullptr)
	{
		FYQPhysicsScene* PhysicsScene = PhysicsScene;
		FYQPhysicsSimulator* Simulator = this;
		ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
			[=](FRHICommandListImmediate& RHICmdList) 
		{
			PhysicsScene->Tick(RHICmdList);
			Simulator->Tick_RenderThread(RHICmdList, 0.03);
		});
		
	}*/

}

void FYQPhysicsSimulator::Substep(FRHICommandList& RHICmdList, uint32 IterSubStep, uint32 NumSubSteps, float SubstepTime)
{
	SCOPED_DRAW_EVENT(RHICmdList, YQPhysicsSimulator_Substep);


}

void FYQPhysicsSimulator::SolveExternalForce(FRHICommandList& RHICmdList, float DeltaTime)
{
	SCOPED_DRAW_EVENT(RHICmdList, SolveExternalForce);

	bool bEnableWind = false;
	FVector DirectionalWindDirection;
	float DirectionalWindSpeed;
	float DirectionalWindMinGustAmt;
	float DirectionalWindMaxGustAmt;

	if (Scene)
	{
		bEnableWind = true;
		Scene->GetDirectionalWindParameters(DirectionalWindDirection, DirectionalWindSpeed, DirectionalWindMinGustAmt, DirectionalWindMaxGustAmt);
	}

	FRWBuffer& NormalBuffer = PhysicsScene->GetNormalBuffer();
	FRWBuffer& VertexMaskBuffer = PhysicsScene->GetVertexMaskBuffer();
	FRWBuffer& VelocityBuffer = PhysicsScene->GetVelocityBuffer();

	FRWBuffer& PredictPositionBuffer = PhysicsScene->GetPredictPositionBuffer();
	FRWBuffer& OrignalPositionBuffer = PhysicsScene->GetOrignalPositionBuffer();

	TArray<FYQPhysicsProxy*>& ProxyList = PhysicsScene->GetProxyListRenderThread();

	for (FYQPhysicsProxy* Proxy : ProxyList)
	{
		if (bEnableWind)
		{
			UpdateExternalForce(
				RHICmdList
				, PhysicsScene->GetPositionBuffer().SRV
				, NormalBuffer.SRV
				, PhysicsScene->GetOutputPositionBuffer().UAV
				, VertexMaskBuffer.SRV
				, VelocityBuffer.UAV
				, FVector3f(DirectionalWindDirection)
				, DirectionalWindSpeed
				, Proxy->bEnableGravity ? 1.0 : 0.0
				, Proxy->BufferIndexOffset
				, Proxy->NumVertices
				, DeltaTime
			);
		}
		else
		{
			UpdateExternalForce(
				RHICmdList
				, PhysicsScene->GetPositionBuffer().SRV
				, NormalBuffer.SRV
				, PhysicsScene->GetOutputPositionBuffer().UAV
				, VertexMaskBuffer.SRV
				, VelocityBuffer.UAV
				, Proxy->bEnableGravity ? 1.0 : 0.0
				, Proxy->BufferIndexOffset
				, Proxy->NumVertices
				, DeltaTime
			);
		}
		
	}
}

void FYQPhysicsSimulator::Tick_RenderThread(FRHICommandList& RHICmdList, float DeltaSeconds)
{
	SCOPED_DRAW_EVENT(RHICmdList, YQPhysicsSimulator_Tick);
	//UE_LOG(LogTemp, Log, TEXT("FYQPhysicsSimulator::Tick_RenderThread %f"), DeltaSeconds)

	int NumSubSteps = 3;
	float StepTime = DeltaSeconds;
	float SubstepTime = StepTime / NumSubSteps;

	

	
	

	//FYQPhysicsScene* PhysicsScene = Scene;

	FRWBuffer& NormalBuffer = PhysicsScene->GetNormalBuffer();
	FRWBuffer& VertexMaskBuffer = PhysicsScene->GetVertexMaskBuffer();
	FRWBuffer& VelocityBuffer = PhysicsScene->GetVelocityBuffer();

	FRWBuffer& FeedbackBuffer = PhysicsScene->GetFeedbackBuffer();
	FRWBuffer& FeedbackBufferSizes = PhysicsScene->GetFeedbackBufferSizes();

	FRWBuffer& AccumulateDeltaPositionXBuffer = PhysicsScene->GetAccumulateDeltaPositionXBuffer();
	FRWBuffer& AccumulateDeltaPositionYBuffer = PhysicsScene->GetAccumulateDeltaPositionYBuffer();
	FRWBuffer& AccumulateDeltaPositionZBuffer = PhysicsScene->GetAccumulateDeltaPositionZBuffer();
	FRWBuffer& AccumulateCountBuffer = PhysicsScene->GetAccumulateCountBuffer();

	FRWBuffer& DistanceConstraintsParticleAIDBuffer = PhysicsScene->GetDistanceConstraintsParticleAIDBuffer();
	FRWBuffer& DistanceConstraintsParticleBIDBuffer = PhysicsScene->GetDistanceConstraintsParticleBIDBuffer();
	FRWBuffer& DistanceConstraintsDistanceBuffer = PhysicsScene->GetDistanceConstraintsDistanceBuffer();

	FRWBuffer& BendingConstraintsParticleAIDBuffer = PhysicsScene->GetBendingConstraintsParticleAIDBuffer();
	FRWBuffer& BendingConstraintsParticleBIDBuffer = PhysicsScene->GetBendingConstraintsParticleBIDBuffer();
	FRWBuffer& BendingConstraintsParticleCIDBuffer = PhysicsScene->GetBendingConstraintsParticleCIDBuffer();
	FRWBuffer& BendingConstraintsParticleDIDBuffer = PhysicsScene->GetBendingConstraintsParticleDIDBuffer();
	FRWBuffer& BendingConstraintsAngleBuffer = PhysicsScene->GetBendingConstraintsAngleBuffer();

	FRWBuffer& PredictPositionBuffer = PhysicsScene->GetPredictPositionBuffer();
	FRWBuffer& OrignalPositionBuffer = PhysicsScene->GetOrignalPositionBuffer();

	uint32 NumParticles = PhysicsScene->GetNumParticlesFromGPU();

	InitializeDeltaBuffer_RenderThread(
		RHICmdList
		, PhysicsScene->GetAccumulateDeltaPositionXBuffer().UAV
		, PhysicsScene->GetAccumulateDeltaPositionYBuffer().UAV
		, PhysicsScene->GetAccumulateDeltaPositionZBuffer().UAV
		, PhysicsScene->GetAccumulateCountBuffer().UAV
		, NumParticles
	);

	uint32 NumDistanceConstraints = PhysicsScene->GetNumConstraints(EConstraintType::Distance);
	uint32 NumBendingConstraints = PhysicsScene->GetNumConstraints(EConstraintType::Bending);
	
	TArray< FCPUObjectProxy*>& CPUObjectProxyList = PhysicsScene->GetCPUObjectProxyList();
	TArray< FVector> AccFeedBacks;
	AccFeedBacks.AddZeroed(CPUObjectProxyList.Num());

	TArray<FYQPhysicsProxy*>& ProxyList = PhysicsScene->GetProxyListRenderThread();

	int NumIters = 20;


	float Stiffness = FYQStiffness::CalcPBDStiffness(SubstepTime, NumIters, 0.9);

	for (int IterSubStep = 0; IterSubStep < NumSubSteps; IterSubStep++)
	{
		CopyPositionForCollision(RHICmdList, PhysicsScene->GetPositionBuffer().SRV, OrignalPositionBuffer.UAV, NumParticles);

		for (int i = 0; i < ProxyList.Num(); i++)
		{
			FYQPhysicsProxy* Proxy = ProxyList[i];

			CalcNormalByCross(
				RHICmdList
				, PhysicsScene->GetPositionBuffer().SRV
				, Proxy->IndexBufferSRV
				, NormalBuffer.UAV
				, AccumulateDeltaPositionXBuffer.UAV
				, AccumulateDeltaPositionYBuffer.UAV
				, AccumulateDeltaPositionZBuffer.UAV
				, Proxy->NumVertices
				, Proxy->NumBufferElement
				, Proxy->BufferIndexOffset
			);
		}

		SolveExternalForce(RHICmdList, SubstepTime);

		PhysicsScene->SwapBuffer();

		CopyPositionForCollision(RHICmdList, PhysicsScene->GetPositionBuffer().SRV, PredictPositionBuffer.UAV, NumParticles);

		for (int Iter = 0; Iter < NumIters; Iter++)
		{
			SolvePBDDistanceConstraint_RenderThread(
				RHICmdList
				, DistanceConstraintsParticleAIDBuffer.SRV
				, DistanceConstraintsParticleBIDBuffer.SRV
				, DistanceConstraintsDistanceBuffer.SRV
				, PhysicsScene->GetPositionBuffer().SRV
				, AccumulateDeltaPositionXBuffer.UAV
				, AccumulateDeltaPositionYBuffer.UAV
				, AccumulateDeltaPositionZBuffer.UAV
				, AccumulateCountBuffer.UAV
				, NumDistanceConstraints
				, SubstepTime
				, Stiffness		//1.0 / (float)NumIters
			);

			SolvePBDBendingConstraint_RenderThread(
				RHICmdList
				, BendingConstraintsParticleAIDBuffer.SRV
				, BendingConstraintsParticleBIDBuffer.SRV
				, BendingConstraintsParticleCIDBuffer.SRV
				, BendingConstraintsParticleDIDBuffer.SRV
				, BendingConstraintsAngleBuffer.SRV
				, PhysicsScene->GetPositionBuffer().SRV
				, AccumulateDeltaPositionXBuffer.UAV
				, AccumulateDeltaPositionYBuffer.UAV
				, AccumulateDeltaPositionZBuffer.UAV
				, AccumulateCountBuffer.UAV
				, NumBendingConstraints
				, SubstepTime
				, Stiffness			//1.0 / (float)NumIters
			);

			ResolveDeltaPosition_RenderThread(
				RHICmdList
				, PhysicsScene->GetPositionBuffer().SRV
				, PhysicsScene->GetOutputPositionBuffer().UAV
				, VelocityBuffer.UAV
				, AccumulateDeltaPositionXBuffer.UAV
				, AccumulateDeltaPositionYBuffer.UAV
				, AccumulateDeltaPositionZBuffer.UAV
				, AccumulateCountBuffer.UAV
				, NumParticles
				, SubstepTime
			);

			PhysicsScene->SwapBuffer();
		}
	

		for (int CollisIter = 0; CollisIter < NumIters; CollisIter++)
		{
			
			for (int i = 0; i < CPUObjectProxyList.Num(); i++)
			{
				FCPUObjectProxy* CPUObjectProxy = CPUObjectProxyList[i];

				FVector3f CenterPosition = FVector3f(CPUObjectProxy->WorldPosition);
				FVector3f Velocity = FVector3f(CPUObjectProxy->Velocity);
				float ColliderMass = CPUObjectProxy->Mass;
				float Radius = CPUObjectProxy->Size;
				//Velocity *= 100;

				YQInitializeSizeBuffer(RHICmdList, FeedbackBufferSizes.UAV, 1, 0);

				NaiveCollisionWithCollider(
					RHICmdList
					, PredictPositionBuffer.SRV
					, OrignalPositionBuffer.SRV
					, NormalBuffer.SRV
					, VertexMaskBuffer.SRV
					, PhysicsScene->GetPositionBuffer().UAV
					, VelocityBuffer.UAV
					, FeedbackBuffer.UAV
					, FeedbackBufferSizes.UAV
					, CenterPosition
					, Radius
					, Velocity
					, ColliderMass
					, NumParticles
					, SubstepTime
					, CollisIter
					, NumIters
				);

				// todo????????GPU????????????????cpu????????
				// ????????????
				if (CollisIter == (NumIters - 1))// && IterSubStep == (NumSubSteps - 1))
				{
					FVector SumFeedBack = FVector();
					void* SizePtr = RHILockBuffer(FeedbackBufferSizes.Buffer, 0, 4, EResourceLockMode::RLM_ReadOnly);
					uint32* SizePtr32 = (uint32*)SizePtr;
					uint32 NumFeedBack = SizePtr32[0];
					RHIUnlockBuffer(FeedbackBufferSizes.Buffer);

					if (NumFeedBack != 0)
					{
						void* ReadPtr = RHILockBuffer(FeedbackBuffer.Buffer, 0, NumFeedBack * sizeof(FFloat16Color), EResourceLockMode::RLM_ReadOnly);
						FFloat16Color* ReadPtr32 = (FFloat16Color*)ReadPtr;
						FVector FeedBack = FVector();
						for (uint32 IterFeedBack = 0; IterFeedBack < NumFeedBack; IterFeedBack++)
						{
							FFloat16Color FeedBack16 = ReadPtr32[IterFeedBack];
							FeedBack += FVector(FeedBack16.R, FeedBack16.G, FeedBack16.B);

							//SumFeedBack += FeedBack;
						}

						RHIUnlockBuffer(FeedbackBuffer.Buffer);

						// ??????????NumFeedBack??????????
						//SumFeedBack = ColliderMass* (FeedBack / NumFeedBack - CPUObjectProxy->Velocity) / StepTime;

						SumFeedBack = FeedBack;

						// ??shader??????????????????????????????????????
						//SumFeedBack -= CPUObjectProxy->Velocity;
						
						// 
						//SumFeedBack += FVector(0, 0, 980 * DeltaSeconds);

						/*UE_LOG(LogTemp, Log, TEXT("NaiveCollisionWithCollider (%f, %f, %f) (%f, %f, %f) NumFeedBack: %d")
							, FeedBack.X, FeedBack.Y, FeedBack.Z
							, SumFeedBack.X, SumFeedBack.Y, SumFeedBack.Z
							, NumFeedBack);*/
					}

					AccFeedBacks[i] += SumFeedBack;
					
				}

				//PhysicsScene->SwapBuffer();
			}
		}


	}

	// todo????????gamethread????
	for (int i = 0; i < CPUObjectProxyList.Num(); i++)
	{
		FCPUObjectProxy* CPUObjectProxy = CPUObjectProxyList[i];
		FVector AccFeedBack = AccFeedBacks[i];
		CPUObjectProxy->FeedBack = AccFeedBack / NumSubSteps;
	}

	//CPUObjectProxy->FeedBack = SumFeedBack;

	for (FYQPhysicsProxy* Proxy : ProxyList)
	{
		if (Proxy->bHasRenderSceneProxy)
		{
			FRenderDataRequestBatch RenderDataRequestBatch;
			Proxy->RenderSceneProxy->GetRenderDataRequest(RenderDataRequestBatch);

			for (FRenderDataRequest& RenderDataRequest : RenderDataRequestBatch.Elements)
			{
				if (RenderDataRequest.bNeedDynamicNormal && RenderDataRequest.bNeedDynamicTangent)
				{
					if (RenderDataRequest.NormalBufferUAV && RenderDataRequest.TangentBufferUAV && RenderDataRequest.TexCoordBufferSRV)
					{
						CalcNormalAndTangent(
							RHICmdList
							, PhysicsScene->GetPositionBuffer().SRV
							, Proxy->IndexBufferSRV
							, RenderDataRequest.TexCoordBufferSRV
							, RenderDataRequest.NormalBufferUAV
							, RenderDataRequest.TangentBufferUAV
							, AccumulateDeltaPositionXBuffer.UAV
							, AccumulateDeltaPositionYBuffer.UAV
							, AccumulateDeltaPositionZBuffer.UAV
							, 0
							, Proxy->BufferIndexOffset
							, Proxy->NumVertices
							, Proxy->NumBufferElement
						);
					}
				}
			}
		}
	}
}