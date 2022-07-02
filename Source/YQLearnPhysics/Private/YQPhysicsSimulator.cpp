#include "YQPhysicsSimulator.h"
#include "YQStreamCompact.h"
#include "PBDConstraintSolve.h"
#include "GPUBasedBVH.h"
#include "ExternalForceSolve.h"
#include "MeshModify.h"
#include "CollisionResponse.h"

FYQPhysicsSimulator* FYQPhysicsSimulator::Instance = nullptr;

void Initialize_RenderThread(FRHICommandListImmediate& RHICmdList)
{
	//FYQPhysicsScene* PhysicsScene = FYQPhysicsScene::Get();
	////InitializeClothVertex(RHICmdList, PhysicsScene->GetPositionBuffer().UAV, PhysicsScene->GetOutputPositionBuffer().UAV, PhysicsScene->GetVertexMaskBuffer().UAV, 1024);
	//CalcNormalByCrossPosition(RHICmdList, PhysicsScene->GetPositionBuffer().SRV, PhysicsScene->GetNormalBuffer().UAV, 0.5);
	////InitializeClothIndexBuffer(RHICmdList, PhysicsScene->GetParticleIndexBuffer().UAV, 1922 * 3);
	//
	//InitializeDeltaBuffer_RenderThread(
	//	RHICmdList
	//	, PhysicsScene->GetAccumulateDeltaPositionXBuffer().UAV
	//	, PhysicsScene->GetAccumulateDeltaPositionYBuffer().UAV
	//	, PhysicsScene->GetAccumulateDeltaPositionZBuffer().UAV
	//	, PhysicsScene->GetAccumulateCountBuffer().UAV
	//	, 1024 * 1024
	//);

}

void UYQPhysicsBlueprintLibrary::InitializeParticle() 
{
	/*FYQPhysicsScene* PhysicsScene = FYQPhysicsScene::Get();
	PhysicsScene->Reset();

	*/

	/*ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
		[](FRHICommandListImmediate& RHICmdList) {
		Initialize_RenderThread(RHICmdList);
	});*/
}

void UYQPhysicsBlueprintLibrary::OnEndPlay() 
{
	//FYQPhysicsScene* PhysicsScene = FYQPhysicsScene::Get();
	//PhysicsScene->ClearConstraints();
}

int UYQPhysicsBlueprintLibrary::AddObjectToPhysicsScene()
{
	//return FYQPhysicsScene::Get()->AddCPUObject();
	return 0;
}

FVector4 UYQPhysicsBlueprintLibrary::GetCPUObjectFeedback(int ObjectID)
{
	/*FYQPhysicsScene* Scene = FYQPhysicsScene::Get();
	return Scene->GetCPUObjectFeedback(ObjectID);*/
	return FVector4();
}

void UYQPhysicsBlueprintLibrary::SetCPUObjectPositionAndVelocity(int ObjectID, FVector4 PosAndRadius, FVector Velocity)
{
	/*FYQPhysicsScene* Scene = FYQPhysicsScene::Get();
	Scene->SetCPUObjectPosition(ObjectID, PosAndRadius);*/
}





int UYQPhysicsBlueprintLibrary::AddStaticMeshActorToPhysicsScene(UStaticMeshComponent* StaticMeshComponent)
{
	//FYQPhysicsScene::Get()->AddStaticMeshToScene(StaticMeshComponent);
	return 0;
}

void FYQPhysicsSimulator::SetPhysicsScene(FYQPhysicsScene* InScene)
{
	Scene = InScene;
}

FYQPhysicsSimulator::FYQPhysicsSimulator()
{
	ViewExtension = FSceneViewExtensions::NewExtension<FYQPhysicsViewExtension>();
}

FYQPhysicsSimulator::~FYQPhysicsSimulator()
{

}


void Update_RenderThread(FRHICommandList& RHICmdList, FYQPhysicsScene* PhysicsScene)
{


	FGPUBvhBuffers& GPUBvhBuffers = PhysicsScene->GetGPUBvhBuffers();

	/*GPUGenerateBVH_RenderThread(
		RHICmdList
		, PhysicsScene->GetPositionBuffer().SRV
		, TriangleIndexBuffer.SRV
		, GPUBvhBuffers
		, PhysicsScene->GetPhysicsSceneViewBuffer()
		, PhysicsScene->GetBoundingBox()
		, PhysicsScene->GetNumStaticMeshTriangles()
	);*/


}

void FYQPhysicsSimulator::Tick(float DeltaSeconds)
{
	//UE_LOG(LogTemp, Log, TEXT("FYQPhysicsSimulator::Tick %f"), DeltaSeconds)

	DeltaSeconds = FMath::Clamp(DeltaSeconds, 0.0, 0.05);

	if(Scene != nullptr)
	{
		FYQPhysicsScene* PhysicsScene = Scene;
		FYQPhysicsSimulator* Simulator = this;
		ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
			[=](FRHICommandListImmediate& RHICmdList) 
		{
			PhysicsScene->Tick(RHICmdList);
			Simulator->Tick_RenderThread(RHICmdList, 0.03);
		});
		
	}

}

void FYQPhysicsSimulator::Tick_RenderThread(FRHICommandList& RHICmdList, float DeltaSeconds)
{
	//UE_LOG(LogTemp, Log, TEXT("FYQPhysicsSimulator::Tick_RenderThread %f"), DeltaSeconds)

	int NumSubSteps = 3;
	float StepTime = DeltaSeconds;
	float SubstepTime = StepTime / NumSubSteps;

	FYQPhysicsScene* PhysicsScene = Scene;

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
	
	TArray< FCPUObjectProxy*>& CPUObjectProxyList = PhysicsScene->GetCPUObjectProxyList();
	TArray< FVector> AccFeedBacks;
	AccFeedBacks.AddZeroed(CPUObjectProxyList.Num());

	int NumIters = 20;
	for (int IterSubStep = 0; IterSubStep < NumSubSteps; IterSubStep++)
	{
		CopyPositionForCollision(RHICmdList, PhysicsScene->GetPositionBuffer().SRV, OrignalPositionBuffer.UAV, NumParticles);

		UpdateExternalForce(
			RHICmdList
			, PhysicsScene->GetPositionBuffer().SRV
			, NormalBuffer.SRV
			, PhysicsScene->GetOutputPositionBuffer().UAV
			, VertexMaskBuffer.SRV
			, VelocityBuffer.UAV
			, NumParticles
			, SubstepTime
		);

		PhysicsScene->SwapBuffer();

		TArray<FYQPhysicsProxy*>& ProxyList = PhysicsScene->GetProxyListRenderThread();
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
			);
		}

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
				, 1.0 / (float)NumIters
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

				// 只在最后一次
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

						//SumFeedBack = ColliderMass* (FeedBack / NumFeedBack - CPUObjectProxy->Velocity) / StepTime;

						//SumFeedBack = FeedBack / NumFeedBack * 0.005 - CPUObjectProxy->Velocity;
						SumFeedBack = FeedBack / NumFeedBack;// -CPUObjectProxy->Velocity;
						SumFeedBack = FeedBack;
						//SumFeedBack -= CPUObjectProxy->Velocity;
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

	for (int i = 0; i < CPUObjectProxyList.Num(); i++)
	{
		FCPUObjectProxy* CPUObjectProxy = CPUObjectProxyList[i];
		FVector AccFeedBack = AccFeedBacks[i];
		CPUObjectProxy->FeedBack = AccFeedBack / NumSubSteps;
	}

	//CPUObjectProxy->FeedBack = SumFeedBack;

}