// Fill out your copyright notice in the Description page of Project Settings.


#include "YQPhysicsMeshComponent.h"
#include "YQPhysicsWorldSubsystem.h"


FYQPhysicsMeshSceneProxy::FYQPhysicsMeshSceneProxy(const UYQPhysicsMeshComponent* InComponent, FYQPhysicsSceneBufferEntry& InPhysicsSceneBufferEntry)
	: FPrimitiveSceneProxy(InComponent)
	, RenderData(InComponent->GetStaticMesh()->GetRenderData())
	, VertexFactory(GetScene().GetFeatureLevel(), "FYQPhysicsVertexFactory")
	, PhysicsSceneBufferEntry(InPhysicsSceneBufferEntry)
{
	ERHIFeatureLevel::Type Level = GetScene().GetFeatureLevel();
	//UE_LOG(LogTemp, Log, TEXT("Level %d %d"), (uint32)Level, (uint32)ERHIFeatureLevel::SM6)
	FStaticMeshVertexBuffers* StaticMeshVertexBuffers = &RenderData->LODResources[0].VertexBuffers;
	FYQPhysicsVertexFactory* InVertexFactory = &VertexFactory;
	ENQUEUE_RENDER_COMMAND(StaticMeshVertexBuffersLegacyBspInit)(
		[InVertexFactory, StaticMeshVertexBuffers](FRHICommandListImmediate& RHICmdList) {
		check(StaticMeshVertexBuffers->PositionVertexBuffer.IsInitialized());
		check(StaticMeshVertexBuffers->StaticMeshVertexBuffer.IsInitialized());

		FLocalVertexFactory::FDataType Data;
		StaticMeshVertexBuffers->PositionVertexBuffer.BindPositionVertexBuffer(InVertexFactory, Data);
		StaticMeshVertexBuffers->StaticMeshVertexBuffer.BindTangentVertexBuffer(InVertexFactory, Data);
		StaticMeshVertexBuffers->StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(InVertexFactory, Data);
		//StaticMeshVertexBuffers->StaticMeshVertexBuffer.BindLightMapVertexBuffer(InVertexFactory, Data, 1);
		//FColorVertexBuffer::BindDefaultColorVertexBuffer(InVertexFactory, Data, FColorVertexBuffer::NullBindStride::ZeroForDefaultBufferBind);
		InVertexFactory->SetData(Data);

		//InitOrUpdateResource(InVertexFactory);
		InVertexFactory->InitResource();
	});
	


	
	UMaterialInterface* MaterialInterface = InComponent->GetMaterial(0);
	MaterialRenderProxy = MaterialInterface->GetRenderProxy();

	MaterialRelevance = MaterialInterface->GetRelevance_Concurrent(GetScene().GetFeatureLevel());

	bSupportsGPUScene = false;

	ENQUEUE_RENDER_COMMAND(YQPhysicsVertexFactoryInit)(
		[this](FRHICommandListImmediate& RHICmdList) {
		//VertexFactory.InitResource();
	});

	FYQPhysicsScene* PhysicsScene = InComponent->GetWorld()->GetSubsystem<UYQPhysicsWorldSubsystem>()->GetGPUPhysicsScene();
	GPUPhysicsScene = PhysicsScene;
}

FYQPhysicsMeshSceneProxy::~FYQPhysicsMeshSceneProxy()
{
	VertexFactory.ReleaseRHI();
	VertexFactory.ReleaseResource();
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
	// 先只支持LOD0
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
				Mesh.VertexFactory = &VertexFactory;
				Mesh.LCI = nullptr;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.bUseForDepthPass = true;

				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.NumPrimitives = Section.NumTriangles;

				

				//BatchElement.IndexBuffer = &PhysicsScene->GetParticleIndexBufferForRender();
				BatchElement.IndexBuffer = &LODModel.IndexBuffer;
				BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
				BatchElement.BaseVertexIndex = 0;
				BatchElement.FirstIndex = PhysicsSceneBufferEntry.IndexBufferOffset + Section.FirstIndex;	// 假设不同的Section被紧密的放置在一起
				BatchElement.bIsInstanceRuns = false;
				BatchElement.NumInstances = 1;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = 5;


				BatchElement.PrimitiveIdMode = PrimID_FromPrimitiveSceneInfo;

				FYQPhysicsUserData* UserData = &Collector.AllocateOneFrameResource<FYQPhysicsUserData>();
				UserData->PositionBuffer = &PhysicsScene->GetPositionBuffer();
				UserData->NormalBuffer = &PhysicsScene->GetNormalBuffer();

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

FYQMeshPhysicsProxy::FYQMeshPhysicsProxy(const UYQPhysicsMeshComponent* InComponent) : FYQPhysicsProxy()
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

	BufferIndexOffset = 0;
	NumBufferElement = NumTriangles;

}

void FYQMeshPhysicsProxy::GetDynamicPhysicsConstraints(FConstraintsBatch& OutBatch) const 
{
	OutBatch.Type = EConstraintType::Distance;
	OutBatch.bUseMeshInfo = true;
	OutBatch.ConstraintSourceType = EConstraintSourceType::Mesh;
	OutBatch.NumConstraints = 1;
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
	if (GetStaticMesh()) {
		GetStaticMesh()->ConditionalPostLoad();

		SetStaticMesh(GetStaticMesh());
	}

	Super::PostLoad();
}

void UYQPhysicsMeshComponent::OnRep_StaticMesh(class UStaticMesh* OldStaticMesh)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		// Only do stuff if this actually changed from the last local value
		if (OldStaticMesh != StaticMesh) 
		{
			// We have to force a call to SetStaticMesh with a new StaticMesh
			UStaticMesh* NewStaticMesh = StaticMesh;
			StaticMesh = OldStaticMesh;

			SetStaticMesh(NewStaticMesh);
		}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

		
}
void UYQPhysicsMeshComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(UStaticMeshComponent, SourceFlipbook);
}


UYQPhysicsMeshComponent::UYQPhysicsMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
{
	PrimaryComponentTick.bCanEverTick = true;

	bTickInEditor = true;
	IsCreateRenderStatePending = false;

	GPUPhysicsProxy = nullptr;
}

bool UYQPhysicsMeshComponent::ShouldCreateRenderState() const 
{
	if (!Super::ShouldCreateRenderState()) {
		UE_LOG(LogStaticMesh, Verbose, TEXT("ShouldCreateRenderState returned false for %s (Base class was false)"), *GetFullName());
		return false;
	}

	// It is especially important to avoid creating a render state for an invalid or compiling static mesh.
	// The shader compiler might try to replace materials on a component that has a render state but doesn't 
	// even have a render proxy which would cause huge game-thread stalls in render state recreation code that 
	// doesn't have to be run in the first place.
	if (GetStaticMesh() == nullptr) {
		UE_LOG(LogStaticMesh, Verbose, TEXT("ShouldCreateRenderState returned false for %s (StaticMesh is null)"), *GetFullName());
		return false;
	}

	// The render state will be recreated after compilation finishes in case it is skipped here.
	if (GetStaticMesh()->IsCompiling()) {
		UE_LOG(LogStaticMesh, Verbose, TEXT("ShouldCreateRenderState returned false for %s (StaticMesh is not ready)"), *GetFullName());
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

	if (GPUPhysicsProxy != nullptr)return false;

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
	//UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::TickComponent"))
	if (IsCreateRenderStatePending) 		
	{
		if (ShouldCreateRenderState()) 
		{
			RecreateRenderState_Concurrent();
		}
	}
}

void UYQPhysicsMeshComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	IsCreateRenderStatePending = false;
	UWorld* World = GetWorld();
	FYQPhysicsScene* Scene = World->GetSubsystem<UYQPhysicsWorldSubsystem>()->GetGPUPhysicsScene();

	// 注册到PhysicsScene
	GPUPhysicsProxy = new FYQMeshPhysicsProxy(this);
	Scene->AddPhysicsProxyToScene(GPUPhysicsProxy);

	/*ENQUEUE_RENDER_COMMAND(UYQPhysicsMeshComponent_PhysicsProxy)(
		[&](FRHICommandListImmediate& RHICmdList) {
		Scene->AddPhysicsProxyToScene(RHICmdList, GPUPhysicsProxy);
	});*/

	// 因为渲染资源当前是使用统一buffer，所以在注册到PhysicsScene之后再添加到渲染场景
	//LLM_SCOPE(ELLMTag::StaticMesh);
	Super::CreateRenderState_Concurrent(Context);
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


FBoxSphereBounds UYQPhysicsMeshComponent::CalcBounds(const FTransform& LocalToWorld) const 
{
	return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(1000000.0f, 1000000.0f, 1000000.0f), 1000000.0f);
}

FPrimitiveSceneProxy* UYQPhysicsMeshComponent::CreateSceneProxy()
{
	if (GetMaterial(0) == nullptr)return nullptr;

	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsMeshComponent::CreateSceneProxy"))

	FPrimitiveSceneProxy* Proxy = new FYQPhysicsMeshSceneProxy(this, GPUPhysicsProxy->BufferEntry);

	return Proxy;
}