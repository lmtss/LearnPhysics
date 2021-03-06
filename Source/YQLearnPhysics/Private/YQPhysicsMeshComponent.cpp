// Fill out your copyright notice in the Description page of Project Settings.


#include "YQPhysicsMeshComponent.h"
#include "YQPhysicsWorldSubsystem.h"

#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceConstant.h"


FYQPhysicsMeshSceneProxy::FYQPhysicsMeshSceneProxy(const UYQPhysicsMeshComponent* InComponent, uint32 InBufferIndexOffset)
	: FPrimitiveSceneProxy(InComponent)
	, RenderData(InComponent->GetStaticMesh()->GetRenderData())
	, VertexFactory(InComponent->GetVertexFactory())
	, BufferIndexOffset(InBufferIndexOffset)
{

	UMaterialInterface* MaterialInterface = InComponent->GetMaterial(0);
	MaterialRenderProxy = MaterialInterface->GetRenderProxy();
	MaterialRelevance = MaterialInterface->GetRelevance_Concurrent(GetScene().GetFeatureLevel());

	FYQPhysicsScene* PhysicsScene = InComponent->GetWorld()->GetSubsystem<UYQPhysicsWorldSubsystem>()->GetGPUPhysicsScene();
	GPUPhysicsScene = PhysicsScene;

	bSupportsGPUScene = false;
}

FYQPhysicsMeshSceneProxy::~FYQPhysicsMeshSceneProxy()
{

}

SIZE_T FYQPhysicsMeshSceneProxy::GetTypeHash() const {
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}

bool FYQPhysicsMeshSceneProxy::GetMeshElement(
	int32 LODIndex,
	int32 SectionIndex,
	uint8 InDepthPriorityGroup,
	FMeshBatch& OutMeshBatch) const
{
	return true;
}

void FYQPhysicsMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	// ??ֻ֧??LOD0
	const int LODIndex = 0;

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) 
	{
		if (VisibilityMap & (1 << ViewIndex)) 
		{
			auto& View = Views[ViewIndex];

			/*FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
			DynamicPrimitiveUniformBuffer.Set(GetLocalToWorld(), FMatrix(), GetBounds(), GetLocalBounds(), true, false, DrawsVelocity(), false);*/


			FYQPhysicsScene* PhysicsScene = GPUPhysicsScene;
			const FStaticMeshLODResources& LODModel = RenderData->LODResources[LODIndex];
			for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++) 
			{
				const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];

				FMeshBatch& Mesh = Collector.AllocateMesh();

				Mesh.Type = PT_TriangleList;
				Mesh.bUseForMaterial = true;
				Mesh.MaterialRenderProxy = MaterialRenderProxy;
				Mesh.VertexFactory = VertexFactory;
				Mesh.LCI = nullptr;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.bUseForDepthPass = true;

				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.NumPrimitives = Section.NumTriangles;

				

				//BatchElement.IndexBuffer = &PhysicsScene->GetParticleIndexBufferForRender();
				BatchElement.IndexBuffer = &LODModel.IndexBuffer;
				BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
				BatchElement.BaseVertexIndex = 0;
				BatchElement.FirstIndex = Section.FirstIndex;
				BatchElement.bIsInstanceRuns = false;
				BatchElement.NumInstances = 1;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = 5;


				BatchElement.PrimitiveIdMode = PrimID_FromPrimitiveSceneInfo;

				FYQPhysicsUserData* UserData = &Collector.AllocateOneFrameResource<FYQPhysicsUserData>();
				UserData->PositionBuffer = &PhysicsScene->GetPositionBuffer();
				UserData->NormalBuffer = &PhysicsScene->GetNormalBuffer();
				UserData->BufferIndexOffset = BufferIndexOffset;

				BatchElement.UserData = (void*)UserData;

				Collector.AddMesh(ViewIndex, Mesh);
			}

			
		}
	}
}

FPrimitiveViewRelevance FYQPhysicsMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Relevance;
	Relevance.bDrawRelevance = true;
	Relevance.bDynamicRelevance = true;
	Relevance.bOpaque = true;
	Relevance.bStaticRelevance = false;
	Relevance.bShadowRelevance = true;
	//Relevance.bNormalTranslucency = true;
	Relevance.bRenderInMainPass = true;

	Relevance.bRenderInDepthPass = MaterialRelevance.bOpaque || MaterialRelevance.bMasked;

	Relevance.bVelocityRelevance = false;
	MaterialRelevance.SetPrimitiveViewRelevance(Relevance);



	return Relevance;
}

FYQPhysicsMeshRenderDataProxy::FYQPhysicsMeshRenderDataProxy(const UYQPhysicsMeshComponent* InComponent) 
	: IYQRenderSceneProxy()
	, VertexFactory(InComponent->GetVertexFactory())
{

}

void FYQPhysicsMeshRenderDataProxy::GetRenderDataRequest(FRenderDataRequestBatch& OutBatch) const
{
	if (VertexFactory != nullptr)
	{
		FUnorderedAccessViewRHIRef DynamicNormalBufferUAV = VertexFactory->GetDynamicNormalBufferUAV();
		FUnorderedAccessViewRHIRef DynamicTangentBufferUAV = VertexFactory->GetDynamicTangentBufferUAV();
		FShaderResourceViewRHIRef TexCoordBufferSRV = VertexFactory->TexCoordBufferSRV;

		if (DynamicNormalBufferUAV && DynamicTangentBufferUAV && TexCoordBufferSRV)
		{
			OutBatch.Elements.AddZeroed(1);

			FRenderDataRequest& Request = OutBatch.Elements[0];
			Request.bNeedDynamicNormal = true;
			Request.bNeedDynamicTangent = true;
			Request.NormalBufferUAV = DynamicNormalBufferUAV;
			Request.TangentBufferUAV = DynamicTangentBufferUAV;
			Request.TexCoordBufferSRV = TexCoordBufferSRV;
		}
	}

}

FYQMeshPhysicsProxy::FYQMeshPhysicsProxy(const UYQPhysicsMeshComponent* InComponent) 
	: FYQPhysicsProxy()
	, bUseBendingConstraints(InComponent->bUseBendingConstraints)
	, bUseDistanceBendingConstraints(InComponent->bUseDistanceBendingConstraints)
{
	FStaticMeshRenderData* RenderData = InComponent->GetStaticMesh()->GetRenderData();
	FStaticMeshLODResources& StaticMeshResourceLOD0 = RenderData->LODResources[0];

	FStaticMeshSectionArray& StaticMeshSectionArray = StaticMeshResourceLOD0.Sections;

	FRawStaticIndexBuffer& MeshIndexBuffer = StaticMeshResourceLOD0.IndexBuffer;
	IndexBufferRHI = MeshIndexBuffer.IndexBufferRHI;

	FStaticMeshVertexBuffers& MeshVertexBuffers = StaticMeshResourceLOD0.VertexBuffers;
	VertexBufferRHI = MeshVertexBuffers.PositionVertexBuffer.VertexBufferRHI;

	if (StaticMeshResourceLOD0.bHasColorVertexData)
	{
		ColorBufferSRV = MeshVertexBuffers.ColorVertexBuffer.GetColorComponentsSRV();
	}
	else
	{
		ColorBufferSRV = nullptr;
	}


	bIsIndexBuffer32 = MeshIndexBuffer.Is32Bit();

	NumVertices = StaticMeshResourceLOD0.GetNumVertices();
	int NumTriangles = StaticMeshResourceLOD0.GetNumTriangles();

	//BufferIndexOffset = 0;
	NumBufferElement = NumTriangles;

	LocalToWorld = FMatrix44f(InComponent->GetComponentTransform().ToMatrixWithScale());

	//bIsRegisteredInGPUPhysicsScene = false;

	bEnableGravity = InComponent->IsGravityEnabled();
}

void FYQMeshPhysicsProxy::GetDynamicPhysicsConstraints(FConstraintsBatch& OutBatch) const 
{
	TArray<FConstraintsBatchElement>& BatchElements = OutBatch.Elements;

	FConstraintsBatchElement DistanceConstraints;

	DistanceConstraints.Type = EConstraintType::Distance;
	DistanceConstraints.bUseMeshInfo = true;
	DistanceConstraints.ConstraintSourceType = EConstraintSourceType::Mesh;
	DistanceConstraints.NumConstraints = 1;

	BatchElements.Add(DistanceConstraints);

	if (bUseBendingConstraints)
	{
		if (bUseDistanceBendingConstraints)
		{
			FConstraintsBatchElement DistanceBendingConstraints;

			DistanceBendingConstraints.Type = EConstraintType::DistanceBending;
			DistanceBendingConstraints.bUseMeshInfo = true;
			DistanceBendingConstraints.ConstraintSourceType = EConstraintSourceType::Mesh;
			DistanceBendingConstraints.NumConstraints = 1;

			BatchElements.Add(DistanceBendingConstraints);
		}
		else
		{
			FConstraintsBatchElement DistanceBendingConstraints;

			DistanceBendingConstraints.Type = EConstraintType::Bending;
			DistanceBendingConstraints.bUseMeshInfo = true;
			DistanceBendingConstraints.ConstraintSourceType = EConstraintSourceType::Mesh;
			DistanceBendingConstraints.NumConstraints = 1;

			BatchElements.Add(DistanceBendingConstraints);
		}

		
	}
}

void UYQPhysicsMeshComponent::OnRegister()
{
	Super::OnRegister();

	if (IsRegistered() || !IsRenderStateCreated())
	{
		IsCreateRenderStatePending = true;
	}
}

void UYQPhysicsMeshComponent::PostInitProperties() {
	

	Super::PostInitProperties();

	/*if (GetStaticMesh() != nullptr) 		
	{
		SetStaticMesh(StaticMesh);
	}*/
}

void UYQPhysicsMeshComponent::PostLoad()
{
	//NotifyIfStaticMeshChanged();

	// need to postload the StaticMesh because super initializes variables based on GetStaticLightingType() which we override and use from the StaticMesh
	/*if (GetStaticMesh()) {
		GetStaticMesh()->ConditionalPostLoad();

		SetStaticMesh(GetStaticMesh());
	}*/

	Super::PostLoad();
}

void UYQPhysicsMeshComponent::SendRenderTransform_Concurrent()
{
	UWorld* World = GetWorld();
	if (World && World->HasSubsystem<UYQPhysicsWorldSubsystem>() && GPUPhysicsProxy != nullptr)
	{
		UYQPhysicsWorldSubsystem* System = World->GetSubsystem<UYQPhysicsWorldSubsystem>();
		System->GetGPUPhysicsScene()->UpdateGPUObjectTransform_GameThread(GPUPhysicsProxy, (UPrimitiveComponent*)this);
	}

	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::SendRenderTransform_Concurrent"));
	
	Super::SendRenderTransform_Concurrent();
}

void UYQPhysicsMeshComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UStaticMeshComponent, SourceFlipbook);
}


UYQPhysicsMeshComponent::UYQPhysicsMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
	, bUseBendingConstraints(false)
	, bUseDistanceBendingConstraints(false)
	, VertexFactory(nullptr)
	, GPUPhysicsProxy(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;

	bTickInEditor = true;
	IsCreateRenderStatePending = false;

}




bool UYQPhysicsMeshComponent::ShouldCreateRenderState() const 
{
	if (!Super::ShouldCreateRenderState()) {
		UE_LOG(LogStaticMesh, Verbose, TEXT("ShouldCreateRenderState returned false for %s (Base class was false)"), *GetFullName());
		return false;
	}

	

	if (GPUPhysicsProxy == nullptr)return false;


	if (GPUPhysicsProxy != nullptr && GPUPhysicsProxy->bIsRegisteredInGPUPhysicsScene == false)return false;

	if (IsRenderStateCreated())return false;

	return true;
}

bool UYQPhysicsMeshComponent::SetStaticMesh(UStaticMesh* NewMesh) 
{
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::SetStaticMesh"))

	StaticMesh = NewMesh;

	/*if (StaticMesh != nullptr && !GetStaticMesh()->IsCompiling() && StaticMesh->GetRenderData() != nullptr && FApp::CanEverRender() && !StaticMesh->HasAnyFlags(RF_ClassDefaultObject)) {
		checkf(StaticMesh->GetRenderData()->IsInitialized(), TEXT("Uninitialized Renderdata for Mesh: %s, Mesh NeedsLoad: %i, Mesh NeedsPostLoad: %i, Mesh Loaded: %i, Mesh NeedInit: %i, Mesh IsDefault: %i")
			, *StaticMesh->GetFName().ToString()
			, StaticMesh->HasAnyFlags(RF_NeedLoad)
			, StaticMesh->HasAnyFlags(RF_NeedPostLoad)
			, StaticMesh->HasAnyFlags(RF_LoadCompleted)
			, StaticMesh->HasAnyFlags(RF_NeedInitialization)
			, StaticMesh->HasAnyFlags(RF_ClassDefaultObject)
		);
	}*/

	// Need to send this to render thread at some point
	if (IsRenderStateCreated()) {
		MarkRenderStateDirty();
	}
	// If we didn't have a valid StaticMesh assigned before
	// our render state might not have been created so
	// do it now.
	else 		
	{
		if (ShouldCreateRenderState()) {

			UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::ShouldCreateRenderState"))
			RecreateRenderState_Concurrent();
		}
		else 			
		{
			UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::IsCreateRenderStatePending"))
			IsCreateRenderStatePending = true;
		}
	}

	return true;
}

void UYQPhysicsMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) 	
{
	if (IsCreateRenderStatePending) 		
	{
		if (IsGPUPhysicsStateCreated() && IsRenderStateCreated())
		{
			IsCreateRenderStatePending = false;
		}
		else if (ShouldCreateGPUPhysicsState())
		{
			CreateGPUPhysicsState();
		}
		else if (ShouldCreateRenderState())
		{
			CreateVertexFactory();
			CreateRenderState_Concurrent(nullptr);
		}
	}
}

void UYQPhysicsMeshComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::CreateRenderState_Concurrent"));
	IsCreateRenderStatePending = false;
	
	// ??Ϊ??Ⱦ??Դ??ǰ??ʹ??ͳһbuffer????????ע?ᵽPhysicsScene֮???????ӵ???Ⱦ????
	//LLM_SCOPE(ELLMTag::StaticMesh);
	Super::CreateRenderState_Concurrent(Context);
}


void UYQPhysicsMeshComponent::CreateVertexFactory()
{
	FStaticMeshVertexBuffers* StaticMeshVertexBuffers = &GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers;
	uint32 InBufferIDOffset = GPUPhysicsProxy->BufferIndexOffset;


	VertexFactory = new FYQPhysicsVertexFactory(GetScene()->GetFeatureLevel(), "FYQPhysicsVertexFactory", GetStaticMesh()->GetRenderData());

	FYQPhysicsVertexFactory* InVertexFactory = VertexFactory;
	ENQUEUE_RENDER_COMMAND(UYQPhysicsMeshComponent_CreateVertexFactory)(
		[InVertexFactory, StaticMeshVertexBuffers, InBufferIDOffset](FRHICommandListImmediate& RHICmdList)
	{
		check(StaticMeshVertexBuffers->PositionVertexBuffer.IsInitialized());
		check(StaticMeshVertexBuffers->StaticMeshVertexBuffer.IsInitialized());

		FLocalVertexFactory::FDataType Data;
		StaticMeshVertexBuffers->PositionVertexBuffer.BindPositionVertexBuffer(InVertexFactory, Data);
		StaticMeshVertexBuffers->StaticMeshVertexBuffer.BindTangentVertexBuffer(InVertexFactory, Data);
		StaticMeshVertexBuffers->StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(InVertexFactory, Data);

		InVertexFactory->BufferIDOffset = InBufferIDOffset;

		InVertexFactory->SetData(Data);

		InVertexFactory->InitResource();
	});
}


void UYQPhysicsMeshComponent::DestroyVertexFactory()
{
	if (VertexFactory)
	{
		FYQPhysicsVertexFactory* VF = VertexFactory;
		ENQUEUE_RENDER_COMMAND(UYQPhysicsMeshComponent_OnComponentDestroyed)(
			[VF](FRHICommandListImmediate& RHICmdList)
		{
			VF->ReleaseResource();
			delete VF;
		});

		VertexFactory = nullptr;
	}
}

bool UYQPhysicsMeshComponent::IsGPUPhysicsStateCreated() const
{
	return GPUPhysicsProxy != nullptr;
}

bool UYQPhysicsMeshComponent::ShouldCreateGPUPhysicsState() const
{
	if (GPUPhysicsProxy != nullptr)return false;

	if (GetStaticMesh() == nullptr)
	{
		UE_LOG(LogStaticMesh, Verbose, TEXT("ShouldCreateGPUPhysicsState returned false for %s (StaticMesh is null)"), *GetFullName());
		return false;
	}

	// The render state will be recreated after compilation finishes in case it is skipped here.
	if (GetStaticMesh()->IsCompiling())
	{
		UE_LOG(LogStaticMesh, Verbose, TEXT("ShouldCreateGPUPhysicsState returned false for %s (StaticMesh is not ready)"), *GetFullName());
		return false;
	}

	if (GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer.VertexBufferRHI == nullptr)
	{
		return false;
	}

	if (GetWorld() == nullptr)return false;

	if (GetWorld()->HasSubsystem<UYQPhysicsWorldSubsystem>() == false)return false;

	if (!GetWorld()->GetSubsystem<UYQPhysicsWorldSubsystem>()->IsInitialized())return false;

	if (GetWorld()->GetSubsystem<UYQPhysicsWorldSubsystem>()->GetGPUPhysicsScene() == nullptr)return false;

	return true;
}

bool UYQPhysicsMeshComponent::ShouldCreatePhysicsState() const
{
	return false;
}

void UYQPhysicsMeshComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);

	DestroyVertexFactory();
}

void UYQPhysicsMeshComponent::CreateGPUPhysicsState()
{
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::OnCreatePhysicsState"));
	UWorld* World = GetWorld();
	FYQPhysicsScene* PhysicsScene = World->GetSubsystem<UYQPhysicsWorldSubsystem>()->GetGPUPhysicsScene();

	// ע?ᵽPhysicsScene
	GPUPhysicsProxy = new FYQMeshPhysicsProxy(this);
	PhysicsScene->AddPhysicsProxyToScene(GPUPhysicsProxy);
}

void UYQPhysicsMeshComponent::DestroyGPUPhysicsState()
{
	FYQPhysicsScene* PhysicsScene = GetWorld()->GetSubsystem<UYQPhysicsWorldSubsystem>()->GetGPUPhysicsScene();
	PhysicsScene->RemovePhysicsProxyFromScene(GPUPhysicsProxy);
	GPUPhysicsProxy = nullptr;
}

void UYQPhysicsMeshComponent::OnCreatePhysicsState()
{
	


}

UMaterialInterface* UYQPhysicsMeshComponent::GetMaterial(int32 MaterialIndex) const
{
	if (OverrideMaterials.IsValidIndex(MaterialIndex) && OverrideMaterials[MaterialIndex]) {
		return OverrideMaterials[MaterialIndex];
	}
	// Otherwise get from static mesh
	else {
		return GetStaticMesh() ? GetStaticMesh()->GetMaterial(MaterialIndex) : nullptr;
	}
}

void UYQPhysicsMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	for (int32 ElementIndex = 0; ElementIndex < GetNumMaterials(); ElementIndex++)
	{
		if (UMaterialInterface* MaterialInterface = GetMaterial(ElementIndex)) {
			OutMaterials.Add(MaterialInterface);
		}
	}

}
//todo
int32 UYQPhysicsMeshComponent::GetNumMaterials() const 
{
	// @note : you don't have to consider Materials.Num()
	// that only counts if overridden and it can't be more than GetStaticMesh()->Materials. 
	if (GetStaticMesh()) 
	{
		return GetStaticMesh()->GetStaticMaterials().Num();
	}
	else {
		return 0;
	}
}

FYQPhysicsVertexFactory* UYQPhysicsMeshComponent::GetVertexFactory() const
{
	return VertexFactory;
}

FBoxSphereBounds UYQPhysicsMeshComponent::CalcBounds(const FTransform& LocalToWorld) const 
{
	return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(1000000.0f, 1000000.0f, 1000000.0f), 1000000.0f);
}

FPrimitiveSceneProxy* UYQPhysicsMeshComponent::CreateSceneProxy()
{
	if (GetMaterial(0) == nullptr)return nullptr;

	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::CreateSceneProxy"))

	FPrimitiveSceneProxy* Proxy = new FYQPhysicsMeshSceneProxy(this, GPUPhysicsProxy->BufferIndexOffset);

	FYQPhysicsMeshRenderDataProxy* RenderDataProxy = new FYQPhysicsMeshRenderDataProxy(this);

	GPUPhysicsProxy->RenderSceneProxy = (IYQRenderSceneProxy*)RenderDataProxy;

	ENQUEUE_RENDER_COMMAND(UYQPhysicsMeshComponent_SetRenderSceneProxy)(
		[=](FRHICommandListImmediate& RHICmdList)
	{
		GPUPhysicsProxy->bHasRenderSceneProxy = true;
	});

	return Proxy;
}