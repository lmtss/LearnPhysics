// Fill out your copyright notice in the Description page of Project Settings.


#include "YQPhysicsComponent.h"
#include "YQPhysicsVertexFactory.h"
#include "YQPhysicsClothSim.h"

#include "YQStreamCompact.h"


class FSimpleClothPhysicsProxy final : public FYQPhysicsProxy
{
public:
	FSimpleClothPhysicsProxy() : FYQPhysicsProxy()
	{

	}

	virtual void GetDynamicPhysicsConstraints(FConstraintsBatch& OutBatch) const 
	{
		UE_LOG(LogTemp, Log, TEXT("FSimpleClothPhysicsProxy::GetDynamicPhysicsConstraints"))

		OutBatch.Type = EConstraintType::Distance;
		OutBatch.bUseMeshInfo = false;

		// 横向的约束，32行，每行31个约束
		for(int Row = 0; Row < 32; Row++)
		{
			for(int Col = 0; Col < 31; Col++)
			{
				uint32 LeftParticleID = Row * 32 + Col;
				uint32 RightParticleID = LeftParticleID + 1;

				if(IsFixed(RightParticleID))
				{
					uint32 t = LeftParticleID;
					LeftParticleID = RightParticleID;
					RightParticleID = t;
				}

				LeftParticleID = HandleParticleID(LeftParticleID);

				OutBatch.UIntBuffer.Add(LeftParticleID);
				OutBatch.UIntBuffer.Add(RightParticleID);
				OutBatch.FloatBuffer.Add(10.0f);
				OutBatch.NumConstraints++;
			}
		}

		// 纵向的约束，31行，每行32个约束
		for(int Row = 0; Row < 31; Row++)
		{
			for(int Col = 0; Col < 32; Col++)
			{
				uint32 LeftParticleID = Row * 32 + Col;
				uint32 RightParticleID = LeftParticleID + 32;

				if(IsFixed(RightParticleID))
				{
					uint32 t = LeftParticleID;
					LeftParticleID = RightParticleID;
					RightParticleID = t;
				}

				LeftParticleID = HandleParticleID(LeftParticleID);

				OutBatch.UIntBuffer.Add(LeftParticleID);
				OutBatch.UIntBuffer.Add(RightParticleID);
				OutBatch.FloatBuffer.Add(10.0f);
				OutBatch.NumConstraints++;
			}
		}

	}
private:
	bool IsFixed(uint32 Val) const
	{
		return Val == 0 || Val == 8 || Val == 16 || Val == 24 || Val == 31;
	}
	uint32 HandleParticleID(uint32 Val) const
	{
		if(Val == 0 || Val == 8 || Val == 16 || Val == 24 || Val == 31)
		{
			return (Val | 0x10000000);
		}

		return Val;
	}
};

class FYQPhysicsSceneProxy final : public FPrimitiveSceneProxy
{
public:
	FYQPhysicsSceneProxy(const UYQPhysicsComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent, "FYQPhysicsSceneProxy")
		, VertexFactory(GetScene().GetFeatureLevel(), "FYQPhysicsVertexFactory")
		, SpherePos(InComponent->SpherePos)
	{
		ERHIFeatureLevel::Type Level = GetScene().GetFeatureLevel();
		//UE_LOG(LogTemp, Log, TEXT("Level %d %d"), (uint32)Level, (uint32)ERHIFeatureLevel::SM6)
		VertexFactory.InitRHI();
		UMaterialInterface* MaterialInterface = InComponent->GetMaterial(0);
		MaterialRenderProxy = MaterialInterface->GetRenderProxy();

		MaterialRelevance = MaterialInterface->GetRelevance_Concurrent(GetScene().GetFeatureLevel());

		bSupportsGPUScene = false;
		bAlwaysHasVelocity = false;

		ENQUEUE_RENDER_COMMAND(YQPhysicsVertexFactoryInit)(
		[this](FRHICommandListImmediate& RHICmdList)
		{
			VertexFactory.InitResource();
		});

	}

	~FYQPhysicsSceneProxy() 
	{
		VertexFactory.ReleaseRHI();
		VertexFactory.ReleaseResource();
	}

	SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
		{
			if (VisibilityMap & (1 << ViewIndex))
			{
				auto& View = Views[ViewIndex];

				//FMeshBatch& Mesh = Collector.AllocateMesh();

				////Mesh.Type = PT_RectList
				//Mesh.Type = PT_TriangleList;
				//Mesh.bUseForMaterial = true;
				//Mesh.MaterialRenderProxy = MaterialRenderProxy;
				//Mesh.VertexFactory = &VertexFactory;
				//Mesh.LCI = nullptr;
				//Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();

				//FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
				//DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), FMatrix(), GetBounds(), GetLocalBounds(), true, false, DrawsVelocity(), false);

				//FYQPhysicsScene* PhysicsScene = FYQPhysicsScene::Get();

				//FMeshBatchElement& BatchElement = Mesh.Elements[0];
				//BatchElement.NumPrimitives = 1922;
				////BatchElement.IndexBuffer = nullptr;
				//BatchElement.IndexBuffer = &PhysicsScene->GetParticleIndexBufferForRender();
				//BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
				//BatchElement.BaseVertexIndex = 0;
				//BatchElement.FirstIndex = 0;
				//BatchElement.bIsInstanceRuns = false;
				//BatchElement.NumInstances = 1;
				//BatchElement.MinVertexIndex = 0;
				//BatchElement.MaxVertexIndex = 5;
				//
				//BatchElement.PrimitiveIdMode = PrimID_FromPrimitiveSceneInfo;

				//FYQPhysicsUserData* UserData = &Collector.AllocateOneFrameResource<FYQPhysicsUserData>();
				//UserData->PositionBuffer = &PhysicsScene->GetPositionBuffer();
				//UserData->NormalBuffer = &PhysicsScene->GetNormalBuffer();

				//BatchElement.UserData = (void*)UserData;

				//Collector.AddMesh(ViewIndex, Mesh);
			}
		}
	}

	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

	void Initialize_RenderThread(FRHICommandListImmediate& RHICmdList)
	{
		//FYQPhysicsScene* PhysicsScene = FYQPhysicsScene::Get();
		//InitializeClothVertex(RHICmdList, PhysicsScene->GetPositionBuffer().UAV, PhysicsScene->GetOutputPositionBuffer().UAV, PhysicsScene->GetVertexMaskBuffer().UAV, VertexBufferLength);
		//CalcNormalByCrossPosition(RHICmdList, PhysicsScene->GetPositionBuffer().SRV, PhysicsScene->GetNormalBuffer().UAV, 0.5);
	}

	void Update_RenderThread(FRHICommandListImmediate& RHICmdList, FVector4* SphereFeedback)
	{

	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override 
	{
		FPrimitiveViewRelevance Relevance;
		Relevance.bDrawRelevance = true;
		Relevance.bDynamicRelevance = true;
		//Relevance.bOpaque = true;
		//Relevance.bNormalTranslucency = true;
		Relevance.bRenderInMainPass = true;
		MaterialRelevance.SetPrimitiveViewRelevance(Relevance);

		return Relevance;
	}


	FYQPhysicsVertexFactory VertexFactory;
	FMaterialRenderProxy* MaterialRenderProxy;
	FMaterialRelevance MaterialRelevance;

	const FVector4& SpherePos;
};


// Sets default values for this component's properties
UYQPhysicsComponent::UYQPhysicsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	//PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	bTickInEditor = true;
}

FBoxSphereBounds UYQPhysicsComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(1000000.0f, 1000000.0f, 1000000.0f), 1000000.0f);
}

FPrimitiveSceneProxy* UYQPhysicsComponent::CreateSceneProxy()
{
	

	

	return Material ? new FYQPhysicsSceneProxy(this) : nullptr;
}

void UYQPhysicsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	/*ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
		[](FRHICommandListImmediate& RHICmdList) {
		YQTest_InitializeBuffer(RHICmdList);
		YQTest_UpdateBuffer(RHICmdList);
	});*/


	//FYQPhysicsScene* Scene = FYQPhysicsScene::Get();

	//if (SceneProxy != nullptr)
	//{
	//	FYQPhysicsSceneProxy* InSceneProxy = (FYQPhysicsSceneProxy*)SceneProxy;
	//	FVector4* Feedback = &SphereFeedback;

	//	if (!bIsInitialized) 	
	//	{
	//		ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
	//			[InSceneProxy, Scene](FRHICommandListImmediate& RHICmdList) {
	//			//Scene->Initialize(RHICmdList, true, false);
	//			InSceneProxy->Initialize_RenderThread(RHICmdList);
	//		});
	//		
	//		bIsInitialized = true;
	//	}
	//	else 			
	//	{
	//		ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
	//			[InSceneProxy, Feedback](FRHICommandListImmediate& RHICmdList) {
	//			InSceneProxy->Update_RenderThread(RHICmdList, Feedback);
	//		});
	//	}

	//	
	//	
	//}
}

void UYQPhysicsComponent::BeginPlay()
{
	Super::BeginPlay();
	bIsInitialized = false;

	/*if(Material && PhysicsProxy == nullptr)
	{
		PhysicsProxy = new FSimpleClothPhysicsProxy();

		UE_LOG(LogTemp, Log, TEXT("UYQPhysicsComponent::BeginPlay"))

		ENQUEUE_RENDER_COMMAND(UYQPhysicsComponent_PhysicsProxy)(
			[&](FRHICommandListImmediate& RHICmdList) {
			FYQPhysicsScene* Scene = FYQPhysicsScene::Get();
			Scene->AddPhysicsProxyToScene(RHICmdList, PhysicsProxy);
		});

		
	}*/


}

void UYQPhysicsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);


	/*if(PhysicsProxy != nullptr)
	{
		ENQUEUE_RENDER_COMMAND(YQPhysicsVertexFactoryInit)(
		[&](FRHICommandListImmediate& RHICmdList)
		{
			FYQPhysicsScene* Scene = FYQPhysicsScene::Get();
			Scene->RemovePhysicsProxyFromScene(PhysicsProxy);
		});
	}*/
	
}

UMaterialInterface* UYQPhysicsComponent::GetMaterial(int32 MaterialIndex) const {
	

	return Material;
}

int32 UYQPhysicsComponent::GetNumMaterials() const 
{
	return 1;
}

void UYQPhysicsComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const {
	if (Material != nullptr) 		
	{
		OutMaterials.Add(Material);
	}

}

void UYQPhysicsComponent::SetSpherePosition(FVector4 PosAndRadius)
{
	SpherePos = PosAndRadius;
}

FVector4 UYQPhysicsComponent::GetFeedback()
{
	return SphereFeedback;
}