#include "YQPhysicsScene.h"

#include "UniformBuffer.h"
#include "SceneView.h"
#include "Components.h"
#include "SceneManagement.h"

#include "StaticMeshCollision.h"
#include "YQPhysicsSceneComputeShader.h"
#include "ExternalForceSolve.h"

#include "ConstraintsGenerate.h"

static TAutoConsoleVariable<int32> CVarNumMaxObjectToGPUPerFrame(
	TEXT("r.YQ.NumMaxObjectToGPUPerFrame"),
	256,
	TEXT("每帧最大添加到GPUScene的物体数量，防止卡顿\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

static TAutoConsoleVariable<int32> CVarNumMaxObject(
	TEXT("r.YQ.NumMaxObject"),
	65536,
	TEXT("场景中最大Object数量\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

#define NUM_INTEGER_ARG_PER_OBJECT 2

void FYQPhysicsScene::InitResource() 
{
	uint32 NumParticleAlloc = YQ_MAX_NUM_PARTICLE;
	uint32 NumMaxOneObjectConstraints = YQ_MAX_NUM_PARTICLE;
	uint32 NumMaxConstraints = YQ_MAX_NUM_PARTICLE * 16;
	uint32 NumMaxCollisionConstraints = YQ_MAX_NUM_PARTICLE;
	//uint32 NumParticleAlloc = CVARPhysicsSceneParticleMaxNum.GetValueOnRenderThread();
	//uint32 NumParticleAlloc = 4194304;
	VertexMaskBuffer.Initialize(TEXT("SimVertexMaskBuffer"), sizeof(uint32), NumParticleAlloc, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	VelocityBuffer.Initialize(TEXT("SimVelocityBuffer"), sizeof(FFloat16Color), NumParticleAlloc, EPixelFormat::PF_FloatRGBA, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	NormalBuffer.Initialize(TEXT("SimNormalBuffer"), sizeof(FFloat16Color), NumParticleAlloc, EPixelFormat::PF_FloatRGBA, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	PositionBufferA.Initialize(TEXT("SimPositionBufferA"), sizeof(FVector4f), NumParticleAlloc, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	PositionBufferB.Initialize(TEXT("SimPositionBufferB"), sizeof(FVector4f), NumParticleAlloc, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	PredictPositionBuffer.Initialize(TEXT("PredictPositionBuffer"), sizeof(FVector4f), NumParticleAlloc, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	OrignalPositionBuffer.Initialize(TEXT("OrignalPositionBuffer"), sizeof(FVector4f), NumParticleAlloc, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	FeedbackBufferSizes.Initialize(TEXT("FeedbackBufferSizes"), sizeof(uint32), 16, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	FeedbackBuffer.Initialize(TEXT("FeedbackBuffer"), sizeof(FFloat16Color), NumParticleAlloc, PF_FloatRGBA, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	// 约束的参数，用于CPU传递参数到GPU
	TempUIntConstraintsParamBuffer.Initialize(TEXT("TempUIntConstraintsParamBuffer"), sizeof(uint32), NumMaxOneObjectConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	TempFloatConstraintsParamBuffer.Initialize(TEXT("TempFloatConstraintsParamBuffer"), sizeof(float), NumMaxOneObjectConstraints, PF_R32_FLOAT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	// Delta Position Buffer
	AccumulateDeltaPositionXBuffer.Initialize(TEXT("AccumulateDeltaPositionXBuffer"), sizeof(int32), NumParticleAlloc, PF_R32_SINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	AccumulateDeltaPositionYBuffer.Initialize(TEXT("AccumulateDeltaPositionYBuffer"), sizeof(int32), NumParticleAlloc, PF_R32_SINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	AccumulateDeltaPositionZBuffer.Initialize(TEXT("AccumulateDeltaPositionZBuffer"), sizeof(int32), NumParticleAlloc, PF_R32_SINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	AccumulateCountBuffer.Initialize(TEXT("AccumulateCountBuffer"), sizeof(uint32), NumParticleAlloc, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	// 约束
	DistanceConstraintsParticleAIDBuffer.Initialize(TEXT("DistanceConstraintsParticleAIDBuffer"), sizeof(uint32), NumMaxConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	DistanceConstraintsParticleBIDBuffer.Initialize(TEXT("DistanceConstraintsParticleBIDBuffer"), sizeof(uint32), NumMaxConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	DistanceConstraintsDistanceBuffer.Initialize(TEXT("DistanceConstraintsDistanceBuffer"), sizeof(float), NumMaxConstraints, PF_R32_FLOAT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	BendingConstraintsParticleAIDBuffer.Initialize(TEXT("BendingConstraintsParticleAIDBuffer"), sizeof(uint32), NumMaxConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	BendingConstraintsParticleBIDBuffer.Initialize(TEXT("BendingConstraintsParticleBIDBuffer"), sizeof(uint32), NumMaxConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	BendingConstraintsParticleCIDBuffer.Initialize(TEXT("BendingConstraintsParticleCIDBuffer"), sizeof(uint32), NumMaxConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	BendingConstraintsParticleDIDBuffer.Initialize(TEXT("BendingConstraintsParticleDIDBuffer"), sizeof(uint32), NumMaxConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	BendingConstraintsAngleBuffer.Initialize(TEXT("BendingConstraintsAngleBuffer"), sizeof(FFloat16), NumMaxConstraints, PF_R16F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	// 碰撞
	CollisionParticleIDBuffer.Initialize(TEXT("CollisionParticleIDBuffer"), sizeof(uint32), NumMaxCollisionConstraints, PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	CollisionErrorDistanceBuffer.Initialize(TEXT("CollisionErrorDistanceBuffer"), sizeof(FFloat16Color), NumMaxCollisionConstraints, PF_FloatRGBA, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	
	// 
	IndexBuffer.Initialize(TEXT("PhysicsParticleIndexBuffer"), sizeof(uint32), NumParticleAlloc * 3, PF_R32_UINT, BUF_UnorderedAccess | BUF_IndexBuffer | BUF_ShaderResource, nullptr);
	//IndexBufferForRender.IndexBufferRHI = IndexBuffer.Buffer;
	//IndexBufferForRender.InitResource();

	VisualizeNodeIDBuffer.Initialize(TEXT("VisualizeNodeIDBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	//PhysicsSceneViewBuffer.Initialize(TEXT("PhysicsSceneViewBuffer"), sizeof(uint32), (sizeof(FPhysicsSceneView)) / (sizeof(uint32)), EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	PhysicsSceneViewBuffers[0].Initialize(TEXT("PhysicsSceneViewBuffer-0"), sizeof(uint32), (sizeof(FPhysicsSceneView)) / (sizeof(uint32)), EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	PhysicsSceneViewBuffers[1].Initialize(TEXT("PhysicsSceneViewBuffer-1"), sizeof(uint32), (sizeof(FPhysicsSceneView)) / (sizeof(uint32)), EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	GPUBvhBuffers.BVPositionBufferA[0].Initialize(TEXT("BVPositionBufferA-0"), sizeof(FVector4f), NumParticleAlloc, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	GPUBvhBuffers.BVPositionBufferB[0].Initialize(TEXT("BVPositionBufferB-0"), sizeof(FVector2f), NumParticleAlloc, EPixelFormat::PF_G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	GPUBvhBuffers.BVPositionBufferA[1].Initialize(TEXT("BVPositionBufferA-1"), sizeof(FVector4f), NumParticleAlloc, EPixelFormat::PF_A32B32G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	GPUBvhBuffers.BVPositionBufferB[1].Initialize(TEXT("BVPositionBufferB-1"), sizeof(FVector2f), NumParticleAlloc, EPixelFormat::PF_G32R32F, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	//GPUBvhBuffers.BVHLeftChildIDBuffer.Initialize(TEXT("BVHLeftChildIDBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	//GPUBvhBuffers.BVHRightChildIDBuffer.Initialize(TEXT("BVHRightChildIDBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	GPUBvhBuffers.BVHChildIDBuffer.Initialize(TEXT("BVHChildIDBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	GPUBvhBuffers.BVHParentIDBuffer.Initialize(TEXT("BVHParentIDBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	GPUBvhBuffers.MortonCodeBuffers[0].Initialize(TEXT("MortonCodeBuffer-0"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	GPUBvhBuffers.MortonCodeBuffers[1].Initialize(TEXT("MortonCodeBuffer-1"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	GPUBvhBuffers.KeyDeltaBuffer.Initialize(TEXT("KeyDeltaBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	
	GPUBvhBuffers.AtomicAccessBuffer.Initialize(TEXT("AtomicAccessBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	GPUBvhBuffers.InternalNodeRangeBuffer.Initialize(TEXT("InternalNodeRangeBuffer"), sizeof(uint32), NumParticleAlloc, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	IndirectArgBuffer.Initialize(TEXT("IndirectArgBuffer"), sizeof(uint32), 6, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_DrawIndirect, nullptr);

	uint32 NumMaxObject = CVarNumMaxObject.GetValueOnRenderThread();
	ObjectEntryOffsetBuffer.Initialize(TEXT("ObjectEntryOffsetBuffer"), sizeof(uint32), NumMaxObject, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	ObjectEntryCountBuffer.Initialize(TEXT("ObjectEntryCountBuffer"), sizeof(uint32), NumMaxObject, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	ObjectEntryBitmaskBuffer.Initialize(TEXT("ObjectEntryBitmaskBuffer"), sizeof(uint32), NumMaxObject, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	ObjectEntryOffsetForRenderBuffer.Initialize(TEXT("ObjectEntryOffsetForRenderBuffer"), sizeof(uint32), NumMaxObject, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	
	uint32 NumMaxObjectToGPUPerFrame = CVarNumMaxObjectToGPUPerFrame.GetValueOnRenderThread();
	uint32 IntegerCountPerObject = NUM_INTEGER_ARG_PER_OBJECT;
	uint32 FloatCountPerObject = 1;
	ObjectIntInfoToGPUBuffer.Initialize(TEXT("ObjectIntInfoToGPUBuffer"), sizeof(uint32), NumMaxObjectToGPUPerFrame * IntegerCountPerObject, EPixelFormat::PF_R32_UINT, BUF_ShaderResource, nullptr);
	ObjectFloatInfoToGPUBuffer.Initialize(TEXT("ObjectFloatInfoToGPUBuffer"), sizeof(float), NumMaxObjectToGPUPerFrame * FloatCountPerObject, EPixelFormat::PF_R32_FLOAT, BUF_ShaderResource, nullptr);

	uint32 SizeToCPUPerFrame = GetSizePerFrameToCPU();
	PerFrameToCPUBuffer.Initialize(TEXT("PerFrameToCPUBuffer"), sizeof(uint32), SizeToCPUPerFrame, EPixelFormat::PF_R32_UINT, BUF_None);
}

void FYQPhysicsScene::Reset() 	
{

}

bool FYQPhysicsScene::AllocImmediate(FRHICommandListImmediate& RHICmdList, uint32 NumParticle, uint32& OutParticleStartIndex)
{
	uint32 NumMaxParticleAlloc = YQ_MAX_NUM_PARTICLE;

	// @todo 
	if(CurrentBufferAllocIndex + NumParticle > NumMaxParticleAlloc)
	{
		return false;
	}

	OutParticleStartIndex = CurrentBufferAllocIndex + NumParticle;
	CurrentBufferAllocIndex += NumParticle;

	return true;
}

FYQPhysicsScene::FYQPhysicsScene()
{
	NumParticlesFromGPU = 0;

	NumConstraintsList.Init(0, (int)EConstraintType::Num);
	ProxyList.Reset(0);

	ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
		[&](FRHICommandListImmediate& RHICmdList) {

		ProxyListRenderThread.Reset(0);
		WaitingReleaseProxyListRenderThread.Reset(0);

		InitResource();
		InitializeBuffer(RHICmdList);
	});
	
}


void FYQPhysicsScene::ReleaseResource()
{


	ENQUEUE_RENDER_COMMAND(ReleaseResource)(
		[&](FRHICommandListImmediate& RHICmdList) {
		//IndexBufferForRender.ReleaseResource();
	});

	

}

FYQPhysicsScene::~FYQPhysicsScene()
{
	ReleaseResource();
	
}

void FYQPhysicsScene::AddCPUActorToScene(USceneComponent* InActor)
{
	
	FCPUObjectProxy** Res = CPUObjectActorMapGameThread.Find(InActor);
	if (Res == nullptr)
	{
		FCPUObjectProxy* Proxy = new FCPUObjectProxy;
		Proxy->WorldPosition = InActor->GetComponentLocation();
		Proxy->Velocity = InActor->ComponentVelocity;
		Proxy->Mass = InActor->IsSimulatingPhysics() ? 10.0 : 10e22;

		FBoxSphereBounds Bound = InActor->GetLocalBounds();
		InActor->GetComponentScale().X;
		Proxy->Size = Bound.SphereRadius * InActor->GetComponentScale().X;

		UE_LOG(LogTemp, Log, TEXT("AddCPUActorToScene Radius %f"), Bound.SphereRadius)

		CPUObjectActorMapGameThread.Add(InActor, Proxy);

		FYQPhysicsScene* Scene = this;

		ENQUEUE_RENDER_COMMAND(AddCPUActorToScene)(
			[Scene, Proxy](FRHICommandListImmediate& RHICmdList)
		{
			Scene->AddCPUActorToScene_RenderThread(Proxy);
		}
		);
	}
}

void FYQPhysicsScene::AddCPUActorToScene_RenderThread(FCPUObjectProxy* Proxy)
{
	CPUObjectProxyListRenderThread.Add(Proxy);
}

void FYQPhysicsScene::UpdateCPUActor(USceneComponent* InActor)
{
	struct FCPUObjectUpdateParams
	{
		FYQPhysicsScene* Scene;
		FVector WorldPosition;
		FVector Velocity;
		FCPUObjectProxy* Proxy;
	};

	

	FCPUObjectProxy** Res = CPUObjectActorMapGameThread.Find(InActor);



	if (Res != nullptr)
	{
		FCPUObjectProxy* Proxy = *Res;

		FCPUObjectUpdateParams UpdateParams;
		UpdateParams.Scene = this;
		UpdateParams.WorldPosition = InActor->GetComponentLocation();
		UpdateParams.Proxy = Proxy;
		UpdateParams.Velocity = InActor->GetComponentVelocity();

		ENQUEUE_RENDER_COMMAND(UpdateCPUActor)(
			[UpdateParams](FRHICommandListImmediate& RHICmdList)
		{
			//FScopeCycleCounter Context(UpdateParams.PrimitiveSceneProxy->GetStatId());
			UpdateParams.Scene->UpdateCPUActor_RenderThread(
				UpdateParams.Proxy
				, UpdateParams.WorldPosition
				, UpdateParams.Velocity
			);
		}
		);
	}
}


void FYQPhysicsScene::UpdateCPUActor_RenderThread(FCPUObjectProxy* Proxy, FVector WorldPosition, FVector Velocity)
{
	FUpdateCPUObjectTransformCommand* Command = UpdatedCPUObjectTransformsRenderThread.Find(Proxy);
	if (Command == nullptr)
	{
		FUpdateCPUObjectTransformCommand NewCommand;
		NewCommand.CenterPosition = WorldPosition;
		NewCommand.Velocity = Velocity;
		UpdatedCPUObjectTransformsRenderThread.Add(Proxy, NewCommand);
	}
	else
	{
		Command->CenterPosition = WorldPosition;
		Command->Velocity = Velocity;
	}
}


void FYQPhysicsScene::UpdateCPUObjectTransform()
{
	for (auto Iter : UpdatedCPUObjectTransformsRenderThread)
	{
		FCPUObjectProxy* Proxy = Iter.Key;
		FUpdateCPUObjectTransformCommand& Command = Iter.Value;
		Proxy->WorldPosition = Command.CenterPosition;
		Proxy->Velocity = Command.Velocity;
	}


	UpdatedCPUObjectTransformsRenderThread.Reset();

}



FVector FYQPhysicsScene::GetCPUObjectFeedBack_GameThread(USceneComponent* InComponent)
{
	FCPUObjectProxy** Res = CPUObjectActorMapGameThread.Find(InComponent);
	
	if (Res != nullptr)
	{
		FCPUObjectProxy* Proxy = *Res;
		return Proxy->FeedBack;
	}

	return FVector();
}

uint32 FYQPhysicsScene::GetNumStaticMeshTriangles()
{
	uint32 NumTriangles = 0;
	int NumProxy = ProxyList.Num();
	for (int i = 0; i < NumProxy; i++) 		
	{
		FYQPhysicsProxy* Proxy = ProxyList[i];
		NumTriangles += Proxy->NumBufferElement;
	}

	return NumTriangles;
}

FBox FYQPhysicsScene::GetBoundingBox() 	
{
	int NumProxy = ProxyList.Num();
	
	FYQPhysicsProxy* Proxy = ProxyList[0];
	FYQCollisionInfo CollisionInfo;
	Proxy->GetCollisionInfo(CollisionInfo);
	return CollisionInfo.BoundingBox;
}

//-------------------------------------------



void FYQPhysicsScene::UpdateGPUObjectTransform_GameThread(FYQPhysicsProxy* Proxy, UPrimitiveComponent* Primitive)
{
	FMatrix44f LocalToWorld = FMatrix44f(Primitive->GetComponentToWorld().ToMatrixWithScale());

	FYQPhysicsScene* PhysicsScene = this;
	FYQPhysicsProxy* PhysicsProxy = Proxy;
	ENQUEUE_RENDER_COMMAND(UpdateCPUActor)(
		[PhysicsScene, PhysicsProxy, LocalToWorld](FRHICommandListImmediate& RHICmdList)
	{
		PhysicsScene->UpdateGPUObjectTransform_RenderThread(
			PhysicsProxy
			, LocalToWorld
		);
	}
	);
}

void FYQPhysicsScene::UpdateGPUObjectTransform_RenderThread(FYQPhysicsProxy* Proxy, FMatrix44f LocalToWorld)
{
	FUpdateGPUObjectTransformCommand* Command = UpdatedGPUObjectTransformsRenderThread.Find(Proxy);
	if (Command == nullptr)
	{
		FUpdateGPUObjectTransformCommand NewCommand;
		NewCommand.LocalToWorld = LocalToWorld;
		UpdatedGPUObjectTransformsRenderThread.Add(Proxy, NewCommand);
	}
	else
	{
		Command->LocalToWorld = LocalToWorld;
	}
}


void FYQPhysicsScene::UpdateGPUObjectTransform(FRHICommandList& RHICmdList)
{
	for (auto Iter : UpdatedGPUObjectTransformsRenderThread)
	{
		FYQPhysicsProxy* Proxy = Iter.Key;
		FUpdateGPUObjectTransformCommand& Command = Iter.Value;
		FMatrix44f NewLocalToWorld = Command.LocalToWorld;
		FMatrix44f PreviousWorldToLocal = Proxy->LocalToWorld.Inverse();

		UpdateFixedParticlesTransform(RHICmdList, GetPositionBuffer().UAV, GetVertexMaskBuffer().SRV, PreviousWorldToLocal, NewLocalToWorld, Proxy->NumVertices, Proxy->BufferIndexOffset);

		Proxy->LocalToWorld = NewLocalToWorld;
	}

	UpdatedCPUObjectTransformsRenderThread.Reset();
}


void FYQPhysicsScene::CopyBufferToPhysicsScene(
	FRHICommandList& RHICmdList
	, FBufferRHIRef IndexBufferRHI
	, FBufferRHIRef VertexBufferRHI
	, FYQPhysicsSceneBufferEntry Entry
	, uint32 NumTriangles
	, uint32 NumVertices
	, bool bIsIndexBuffer32
) {
	FShaderResourceViewRHIRef IndexBufferSRV = RHICreateShaderResourceView(
		IndexBufferRHI
		, bIsIndexBuffer32 ? 4 : 2
		, bIsIndexBuffer32 ? PF_R32_UINT : PF_R16_UINT
	);

	//FShaderResourceViewRHIRef PositionBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(float) * 3, PF_R32G32B32F);
	FShaderResourceViewRHIRef PositionBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(float), PF_R32_FLOAT);

	AppendTriangleToPhysicsScene_RenderThread(
		RHICmdList
		, IndexBufferSRV
		, PositionBufferSRV
		, GetParticleIndexBuffer().UAV
		, GetPositionBuffer().UAV
		, GetPhysicsSceneViewBuffer().UAV
		, nullptr
		, nullptr
		, NumTriangles
		, NumVertices
	);
}

void FYQPhysicsScene::AddStaticMeshToScene(UStaticMeshComponent* StaticMeshComponent)
{
	UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();


	FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

	FStaticMeshLODResources& StaticMeshResourceLOD0 = RenderData->LODResources[0];

	FStaticMeshSectionArray& StaticMeshSectionArray = StaticMeshResourceLOD0.Sections;

	FRawStaticIndexBuffer& MeshIndexBuffer = StaticMeshResourceLOD0.IndexBuffer;
	FBufferRHIRef IndexBufferRHI = MeshIndexBuffer.IndexBufferRHI;
	
	FStaticMeshVertexBuffers& MEshVertexBuffers = StaticMeshResourceLOD0.VertexBuffers;
	FBufferRHIRef VertexBufferRHI = MEshVertexBuffers.PositionVertexBuffer.VertexBufferRHI;

	bool bIsIndexBuffer32 = MeshIndexBuffer.Is32Bit();

	int NumVertices = StaticMeshResourceLOD0.GetNumVertices();
	int NumTriangles = StaticMeshResourceLOD0.GetNumTriangles();
	
	FStaticMeshPhysicsProxy* MeshProxy = new FStaticMeshPhysicsProxy();
	MeshProxy->BufferIndexOffset = 0;
	MeshProxy->IndexBufferRHI = IndexBufferRHI;
	MeshProxy->VertexBufferRHI = VertexBufferRHI;
	MeshProxy->NumBufferElement = NumTriangles;
	MeshProxy->bIsIndexBuffer32 = bIsIndexBuffer32;
	MeshProxy->NumVertices = NumVertices;


	FBox BoundingBox = StaticMesh->GetBoundingBox();
	MeshProxy->BoundingBox = BoundingBox;

	FYQPhysicsProxy* Proxy = MeshProxy;
	FYQPhysicsScene* Scene = this;

	AddPhysicsProxyToScene(MeshProxy);

	/*ENQUEUE_RENDER_COMMAND(AddStaticMeshPhysicsProxy)(
		[Proxy, Scene](FRHICommandListImmediate& RHICmdList) {
		Scene->AddPhysicsProxyToScene(RHICmdList, Proxy);
	});*/
}


void FYQPhysicsScene::InitializeBuffer(FRHICommandList& RHICmdList)
{
	uint32 PhysicsSceneViewBufferLength = sizeof(FPhysicsSceneView) / sizeof(uint32);
	uint32 ObjectEntryBufferLength = CVarNumMaxObject.GetValueOnRenderThread();

	ClearBuffer_RenderThread(
		RHICmdList
		, PhysicsSceneViewBuffers[0].UAV
		, PhysicsSceneViewBufferLength
	);

	ClearBuffer_RenderThread(
		RHICmdList
		, PhysicsSceneViewBuffers[1].UAV
		, PhysicsSceneViewBufferLength
	);

	InitializeObjectEntryBuffer_RenderThread(
		RHICmdList
		, ObjectEntryOffsetBuffer.UAV
		, ObjectEntryCountBuffer.UAV
		, ObjectEntryBitmaskBuffer.UAV
		, ObjectEntryOffsetForRenderBuffer.UAV
		, ObjectEntryBufferLength
	);
}

void FYQPhysicsScene::CopyDataToCPU(FRHICommandList& RHICmdList)
{
	/*CopyDataToCPUBuffer_RenderThread(
		RHICmdList
		, PhysicsSceneViewBuffers[PhysicsSceneViewBufferPingPang].SRV
		, PerFrameToCPUBuffer.UAV
	);

	uint8* Buffer = PerFrameToCPUBuffer.Lock();
	uint32* ReadPtr = (uint32*)Buffer;
	NumParticlesFromGPU = ReadPtr[0];

	PerFrameToCPUBuffer.Unlock();*/
}

void FYQPhysicsScene::AddGPUObject(FGPUPhysicsObjectAllocInfo& Info)
{
	WaitingObjectList.Add(Info);
}


void FYQPhysicsScene::AddGPUObjectToBuffer(FRHICommandList& RHICmdList)
{
	int NumObject = WaitingObjectList.Num();

	if (NumObject == 0)return;

	int NumIntegerPerObject = NUM_INTEGER_ARG_PER_OBJECT;

	// 整型参数
	uint32 NumUIntParam = NumObject * NumIntegerPerObject;
	void* WBuffer = RHILockBuffer(ObjectIntInfoToGPUBuffer.Buffer, 0, NumUIntParam * sizeof(uint32), RLM_WriteOnly);
	uint32* UIntBuffer = static_cast<uint32*>(WBuffer);

	uint32 SumParticles = 0;
	for (int i = 0; i < NumObject; i++)
	{
		uint32 Value = WaitingObjectList[i].Count;
		uint32 Offset = SumParticles;
		UIntBuffer[i * NumIntegerPerObject] = Offset;
		UIntBuffer[i * NumIntegerPerObject + 1] = Value;
		SumParticles += Value;
	}

	

	UE_LOG(LogTemp, Log, TEXT("FYQPhysicsScene::AddGPUObjectToBuffer %d"), SumParticles);


	RHIUnlockBuffer(ObjectIntInfoToGPUBuffer.Buffer);

	AddGPUObjectToBuffer_RenderThread(
		RHICmdList
		, PhysicsSceneViewBuffers[PhysicsSceneViewBufferPingPang].SRV
		, ObjectIntInfoToGPUBuffer.SRV
		, ObjectEntryOffsetBuffer.UAV
		, ObjectEntryCountBuffer.UAV
		, PhysicsSceneViewBuffers[1 - PhysicsSceneViewBufferPingPang].UAV
		, NumObject
	);

	PhysicsSceneViewBufferSwap();

	uint32 BaseParticle = NumParticlesFromGPU;

	// 将位置信息拷贝
	for (int i = 0; i < NumObject; i++)
	{
		FGPUPhysicsObjectAllocInfo& Info = WaitingObjectList[i];

		if (Info.bNeedCopyVertex)
		{
			AppendParticlesToPhysicsScene_RenderThread(
				RHICmdList
				, PhysicsSceneViewBuffers[PhysicsSceneViewBufferPingPang].SRV
				, Info.VertexBuffer
				, GetPositionBuffer().UAV
				, PhysicsSceneViewBuffers[1 - PhysicsSceneViewBufferPingPang].UAV
				, Info.ColorBufferSRV
				, GetVertexMaskBuffer().UAV
				, Info.LocalToWorld
				, Info.Count
				, BaseParticle
			);

			PhysicsSceneViewBufferSwap();

			FYQPhysicsProxy* PhysicsProxy = Info.PhysicsProxy;
			FYQPhysicsPrimitiveSceneInfo* PrimitiveSceneInfo = new FYQPhysicsPrimitiveSceneInfo(PhysicsProxy);
			PhysicsProxy->PhysicsSceneInfo = PrimitiveSceneInfo;

			PrimitiveSceneInfo->GPUPhysicsObjectEntry.ParticlePositionBufferOffset = BaseParticle;
			PrimitiveSceneInfo->GPUPhysicsObjectEntry.NumParticles = Info.Count;

			uint32 ProxyBaseParticle = BaseParticle;

			PhysicsProxy->BufferIndexOffset = ProxyBaseParticle;

			FFunctionGraphTask::CreateAndDispatchWhenReady([PhysicsProxy, ProxyBaseParticle]()
			{
				PhysicsProxy->bIsRegisteredInGPUPhysicsScene = true;
				UE_LOG(LogTemp, Log, TEXT("CreateAndDispatchWhenReady %d"), ProxyBaseParticle)

			}
				, TStatId{}
					, nullptr
					, ENamedThreads::GameThread
					);

			BaseParticle += PhysicsProxy->NumVertices;
		}
		
	}

	NumParticlesFromGPU += SumParticles;

	// 添加约束
	uint32 NumDistanceConstraintsInScene = NumConstraintsList[(int)EConstraintType::Distance];
	uint32 NumDistanceBendingConstraintsInScene = NumConstraintsList[(int)EConstraintType::DistanceBending];
	uint32 NumBendingConstraintsInScene = NumConstraintsList[(int)EConstraintType::Bending];

	for (int i = 0; i < NumObject; i++)
	{
		FGPUPhysicsObjectAllocInfo& Info = WaitingObjectList[i];
		FConstraintsBatch& ConstraintsBatch = Info.ConstraintsBatch;

		FYQPhysicsProxy* PhysicsProxy = Info.PhysicsProxy;

		FYQPhysicsPrimitiveSceneInfo* PrimitiveSceneInfo = PhysicsProxy->PhysicsSceneInfo;

		uint32 NumConstraintTypes = ConstraintsBatch.Elements.Num();

		for (uint32 ElementIndex = 0; ElementIndex < NumConstraintTypes; ElementIndex++)
		{
			FConstraintsBatchElement& BatchElement = ConstraintsBatch.Elements[ElementIndex];

			EConstraintType ConstraintType = BatchElement.Type;
			EConstraintSourceType ConstraintSourceType = BatchElement.ConstraintSourceType;

			FGPUConstraintsEntry& ConstraintsEntry = PrimitiveSceneInfo->GPUPhysicsObjectEntry.Constraints[(int)ConstraintType];

			int NumConstraintsNew = 0;

			if (ConstraintType == EConstraintType::Distance)
			{
				if (ConstraintSourceType == EConstraintSourceType::Mesh)
				{
					NumConstraintsNew = GenerateDistanceConstraintsFromMesh(
						RHICmdList
						, DistanceConstraintsParticleAIDBuffer.UAV
						, DistanceConstraintsParticleBIDBuffer.UAV
						, DistanceConstraintsDistanceBuffer.UAV
						, Info.IndexBufferSRV
						, Info.VertexBuffer
						, Info.ColorBufferSRV
						, NumDistanceConstraintsInScene
						, Info.NumTriangles
						, PhysicsProxy->BufferIndexOffset
					);

					ConstraintsEntry.Offset = NumDistanceConstraintsInScene;
					NumDistanceConstraintsInScene += NumConstraintsNew;
				}
			}
			else if (ConstraintType == EConstraintType::DistanceBending)
			{
				if (ConstraintSourceType == EConstraintSourceType::Mesh)
				{
					NumConstraintsNew = GenerateDistanceBendingConstraintsFromMesh(
						RHICmdList
						, DistanceConstraintsParticleAIDBuffer.UAV
						, DistanceConstraintsParticleBIDBuffer.UAV
						, DistanceConstraintsDistanceBuffer.UAV
						, Info.IndexBufferSRV
						, Info.VertexBuffer
						, Info.ColorBufferSRV
						, NumDistanceConstraintsInScene
						, Info.NumTriangles
						, PhysicsProxy->BufferIndexOffset
					);

					// 因为DistanceBending实际上就是存储在DistanceConstraints的Buffer中
					ConstraintsEntry.Offset = NumDistanceConstraintsInScene;
					
					NumDistanceBendingConstraintsInScene += NumConstraintsNew;
					NumDistanceConstraintsInScene += NumConstraintsNew;
				}
			}
			else if (ConstraintType == EConstraintType::Bending)
			{
				if (ConstraintSourceType == EConstraintSourceType::Mesh)
				{
					NumConstraintsNew = GenerateBendingConstraintsFromMesh(
						RHICmdList
						, BendingConstraintsParticleAIDBuffer.UAV
						, BendingConstraintsParticleBIDBuffer.UAV
						, BendingConstraintsParticleCIDBuffer.UAV
						, BendingConstraintsParticleDIDBuffer.UAV
						, BendingConstraintsAngleBuffer.UAV
						, Info.IndexBufferSRV
						, Info.VertexBuffer
						, Info.ColorBufferSRV
						, NumBendingConstraintsInScene
						, Info.NumTriangles
						, PhysicsProxy->BufferIndexOffset
					);

					ConstraintsEntry.Offset = NumBendingConstraintsInScene;

					NumBendingConstraintsInScene += NumConstraintsNew;
				}
			}
		
			ConstraintsEntry.Num = NumConstraintsNew;
		}
	}

	NumConstraintsList[(int)EConstraintType::Distance] = NumDistanceConstraintsInScene;
	NumConstraintsList[(int)EConstraintType::DistanceBending] = NumDistanceBendingConstraintsInScene;
	NumConstraintsList[(int)EConstraintType::Bending] = NumBendingConstraintsInScene;

	WaitingObjectList.Reset(0);
}


void FYQPhysicsScene::RemoveGPUObjectFromBuffer(FRHICommandList& RHICmdList)
{
	struct FMemoryEmprtyFragment
	{
		uint32 Offset;
		uint32 Num;
	};

	TArray< TArray<FMemoryEmprtyFragment>> FragmentList;
	FragmentList.AddZeroed((int)EConstraintType::Num);

	for (FYQPhysicsProxy* Proxy : WaitingReleaseProxyListRenderThread)
	{
		FYQPhysicsPrimitiveSceneInfo* SceneInfo = Proxy->PhysicsSceneInfo;

		FGPUPhysicsObjectEntry& GPUPhysicsObjectEntry = SceneInfo->GPUPhysicsObjectEntry;

		for (int i = 0; i < (int)EConstraintType::Num; i++)
		{
			FGPUConstraintsEntry& ConstraintsEntry = GPUPhysicsObjectEntry.Constraints[i];
			FMemoryEmprtyFragment Frag;
			Frag.Offset = ConstraintsEntry.Offset;
			Frag.Num = ConstraintsEntry.Num;

			FragmentList[i].Add(Frag);
		}

		delete SceneInfo;
		delete Proxy;
	}

	struct FConstraintsMoveCommand
	{
		uint32 SrcIndex;
		uint32 DestIndex;
		uint32 Num;
	};

	TArray< TArray<FConstraintsMoveCommand>> CommandLists;
	CommandLists.AddZeroed((int)EConstraintType::Num);

	for (int IntConstraintType = 0; IntConstraintType < (int)EConstraintType::Num; IntConstraintType++)
	{
		if (EConstraintType(IntConstraintType) == EConstraintType::DistanceBending)
		{
			// DistanceBending 没有自己的Buffer
			continue;
		}

		TArray<FMemoryEmprtyFragment>& Fragments = FragmentList[IntConstraintType];
		Fragments.Sort([](const FMemoryEmprtyFragment& Frag1, const FMemoryEmprtyFragment& Frag2)
		{
			return  Frag1.Offset < Frag2.Offset;
		});

		TArray<FConstraintsMoveCommand>& CommandList = CommandLists[IntConstraintType];
		uint32 NumConstraints = GetNumConstraints(EConstraintType(IntConstraintType));

		// 求前缀和
		uint32 SumDeleteConstraints = 0;
		uint32 PrevFragEnd = 0;
		for (FMemoryEmprtyFragment& Frag : Fragments)
		{
			if (CommandList.Num() == 0)	// 第一个Frag
			{
				FConstraintsMoveCommand Command;
				Command.SrcIndex = Frag.Offset + Frag.Num;
				Command.DestIndex = Frag.Offset;
				Command.Num = NumConstraints - Command.SrcIndex;

				CommandList.Add(Command);
			}
			else
			{
				FConstraintsMoveCommand& PrevCommand = CommandList[CommandList.Num() - 1];

				if (PrevFragEnd == Frag.Offset)	// 连续的两个Free的空间
				{
					PrevCommand.Num -= Frag.Num;
					PrevCommand.SrcIndex += Frag.Num;
				}
				else
				{
					PrevCommand.Num = Frag.Offset - PrevFragEnd;

					FConstraintsMoveCommand Command;
					Command.SrcIndex = Frag.Offset + Frag.Num;
					Command.DestIndex = Frag.Offset - SumDeleteConstraints;
					Command.Num = NumConstraints - Command.SrcIndex;

					CommandList.Add(Command);
				}
			}

			SumDeleteConstraints += Frag.Num;
			
			PrevFragEnd = Frag.Offset + Frag.Num;
		}
	
		NumConstraintsList[IntConstraintType] -= SumDeleteConstraints;
	}

	// 距离约束
	/*{
		FRHIUnorderedAccessView* BufferList[] = {
		GetDistanceConstraintsParticleAIDBuffer().UAV.GetReference()
		, GetDistanceConstraintsParticleBIDBuffer().UAV.GetReference()
		, GetDistanceConstraintsDistanceBuffer().UAV.GetReference()
		};

		EPixelFormat FormatList[] = { PF_R32_UINT, PF_R32_UINT, PF_R32_FLOAT };

		MoveConstraintsBuffer(RHICmdList, BufferList, FormatList, 3);
	}*/

	{
		TArray<FConstraintsMoveCommand>& CommandList = CommandLists[(int)EConstraintType::Distance];
		for (FConstraintsMoveCommand& Command : CommandList)
		{
			MoveConstraintsBuffer(
				RHICmdList
				, GetDistanceConstraintsParticleAIDBuffer().UAV
				, GetDistanceConstraintsParticleBIDBuffer().UAV
				, GetDistanceConstraintsDistanceBuffer().UAV
				, Command.SrcIndex
				, Command.DestIndex
				, Command.Num
			);
		}
	}

	{
		TArray<FConstraintsMoveCommand>& CommandList = CommandLists[(int)EConstraintType::Bending];
		for (FConstraintsMoveCommand& Command : CommandList)
		{
			MoveConstraintsBuffer(
				RHICmdList
				, GetBendingConstraintsParticleAIDBuffer().UAV
				, GetBendingConstraintsParticleBIDBuffer().UAV
				, GetBendingConstraintsParticleCIDBuffer().UAV
				, GetBendingConstraintsParticleDIDBuffer().UAV
				, GetBendingConstraintsAngleBuffer().UAV
				, Command.SrcIndex
				, Command.DestIndex
				, Command.Num
			);
		}
	}

	
	

	WaitingReleaseProxyListRenderThread.Reset(0);
}


void FYQPhysicsScene::AddPhysicsProxyToScene_RenderThread(FRHICommandList& RHICmdList, FYQPhysicsProxy* Proxy)
{
	check(IsInRenderingThread());

	ProxyListRenderThread.Add(Proxy);

	// -------------- 处理约束 ---------------

	FConstraintsBatch Batch;
	Proxy->GetDynamicPhysicsConstraints(Batch);

	uint32 NumConstraintTypes = Batch.Elements.Num();

	if (NumConstraintTypes > 0)
	{
		FGPUPhysicsObjectAllocInfo Info;
		Info.Bitmask = 0;

		bool bNeedCopyVertex = false;			// 是否将网格中的顶点拷贝到GPU场景
		bool bNeedVertexColorBuffer = false;	// 是否需要顶点色

		for (uint32 ElementIndex = 0; ElementIndex < NumConstraintTypes; ElementIndex++)
		{
			FConstraintsBatchElement& BatchElement = Batch.Elements[ElementIndex];

			EConstraintType ConstraintType = BatchElement.Type;

			// 距离约束这样的情况，需要用网格中的顶点来计算，所以是true
			// 如果是shape match这种，则是用生成的体素来计算，所以是false
			if (ConstraintType == EConstraintType::Distance || ConstraintType == EConstraintType::DistanceBending || ConstraintType == EConstraintType::Bending)
			{
				bNeedCopyVertex = true;
				bNeedVertexColorBuffer = true;
			}
		}

		if (bNeedCopyVertex)
		{
			FShaderResourceViewRHIRef PositionBufferSRV = RHICreateShaderResourceView(Proxy->VertexBufferRHI, sizeof(float), PF_R32_FLOAT);
			FShaderResourceViewRHIRef IndexBufferSRV = RHICreateShaderResourceView(
				Proxy->IndexBufferRHI
				, Proxy->bIsIndexBuffer32 ? 4 : 2
				, Proxy->bIsIndexBuffer32 ? PF_R32_UINT : PF_R16_UINT
			);

			Proxy->IndexBufferSRV = IndexBufferSRV;
			Info.Count = Proxy->NumVertices;
			Info.VertexBuffer = PositionBufferSRV;
			Info.IndexBufferSRV = IndexBufferSRV;
			Info.NumTriangles = Proxy->NumBufferElement;
		}

		Info.bNeedCopyVertex = bNeedCopyVertex;

		if (bNeedVertexColorBuffer)
		{
			// todo: 事实上顶点色的SRV不一定是准备好的，可能需要这时候去创建
			Info.ColorBufferSRV = Proxy->ColorBufferSRV;
		}

		Info.LocalToWorld = Proxy->LocalToWorld;
		Info.PhysicsProxy = Proxy;
		Info.ConstraintsBatch = Batch;

		AddGPUObject(Info);

#if 0
		for (int ElementIndex = 0; ElementIndex < NumConstraintTypes; ElementIndex++)
		{
			FConstraintsBatchElement& BatchElement = Batch.Elements[ElementIndex];

			EConstraintType ConstraintType = BatchElement.Type;

			if (BatchElement.ConstraintSourceType == EConstraintSourceType::Mesh) // 使用Mesh的Buffer创建约束	
			{
				int NumTriangles = Proxy->NumBufferElement;
		
				FShaderResourceViewRHIRef PositionBufferSRV = RHICreateShaderResourceView(Proxy->VertexBufferRHI, sizeof(float), PF_R32_FLOAT);

				FGPUPhysicsObjectAllocInfo Info;
				Info.Count = Proxy->NumVertices;
				Info.Bitmask = 0;
				Info.VertexBuffer = PositionBufferSRV;

				FShaderResourceViewRHIRef IndexBufferSRV = RHICreateShaderResourceView(
					Proxy->IndexBufferRHI
					, Proxy->bIsIndexBuffer32 ? 4 : 2
					, Proxy->bIsIndexBuffer32 ? PF_R32_UINT : PF_R16_UINT
				);

				Proxy->IndexBufferSRV = IndexBufferSRV;

				if (ConstraintType == EConstraintType::Distance)
				{
					Info.ConstraintsBatch = BatchElement;
					Info.IndexBufferRHI = Proxy->IndexBufferRHI;
					Info.VertexBufferRHI = Proxy->VertexBufferRHI;
					Info.ColorBufferSRV = Proxy->ColorBufferSRV;
					Info.NumTriangles = NumTriangles;


				}

				Info.LocalToWorld = Proxy->LocalToWorld;
				Info.PhysicsProxy = Proxy;

				AddGPUObject(Info);
			}
			else
			{
				// 整型参数
				uint32 NumUIntParam = BatchElement.UIntBuffer.Num();
				void* WBuffer = RHILockBuffer(TempUIntConstraintsParamBuffer.Buffer, 0, NumUIntParam * sizeof(uint32), RLM_WriteOnly);
				uint32* UIntBuffer = static_cast<uint32*>(WBuffer);

				for (uint32 i = 0; i < NumUIntParam; i++)
				{
					uint32 Value = BatchElement.UIntBuffer[i];
					UIntBuffer[i] = Value;
				}

				RHIUnlockBuffer(TempUIntConstraintsParamBuffer.Buffer);

				// 浮点型参数
				uint32 NumFloatParam = BatchElement.FloatBuffer.Num();
				WBuffer = RHILockBuffer(TempFloatConstraintsParamBuffer.Buffer, 0, NumFloatParam * sizeof(float), RLM_WriteOnly);
				float* FloatBuffer = static_cast<float*>(WBuffer);

				for (uint32 i = 0; i < NumFloatParam; i++)
				{
					FloatBuffer[i] = BatchElement.FloatBuffer[i];
				}

				RHIUnlockBuffer(TempFloatConstraintsParamBuffer.Buffer);

				GenerateDistanceConstraints(
					RHICmdList
					, DistanceConstraintsParticleAIDBuffer.UAV
					, DistanceConstraintsParticleBIDBuffer.UAV
					, DistanceConstraintsDistanceBuffer.UAV
					, TempUIntConstraintsParamBuffer.SRV
					, TempFloatConstraintsParamBuffer.SRV
					, 0
					, BatchElement.NumConstraints
				);

				NumConstraintsList[(uint32)BatchElement.Type] += BatchElement.NumConstraints;
			}
		}
#endif
		

	}



	// -------------- 处理碰撞 --------------
	FYQCollisionInfo CollisionInfo;
	Proxy->GetCollisionInfo(CollisionInfo);

	if (CollisionInfo.Type == EYQCollisionType::StaticMesh) {
		int NumTriangles = Proxy->NumBufferElement;
		int NumVertices = Proxy->NumVertices;

		FBufferRHIRef IndexBufferRHI = Proxy->IndexBufferRHI;
		FBufferRHIRef VertexBufferRHI = Proxy->VertexBufferRHI;
		bool bIsIndexBuffer32 = Proxy->bIsIndexBuffer32;

		FShaderResourceViewRHIRef IndexBufferSRV = RHICreateShaderResourceView(
			IndexBufferRHI
			, bIsIndexBuffer32 ? 4 : 2
			, bIsIndexBuffer32 ? PF_R32_UINT : PF_R16_UINT
		);

		//FShaderResourceViewRHIRef PositionBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(float) * 3, PF_R32G32B32F);
		FShaderResourceViewRHIRef PositionBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(float), PF_R32_FLOAT);

		AppendTriangleToPhysicsScene_RenderThread(
			RHICmdList
			, IndexBufferSRV
			, PositionBufferSRV
			, GetParticleIndexBuffer().UAV
			, GetPositionBuffer().UAV
			, GetPhysicsSceneViewBuffer().UAV
			, nullptr
			, nullptr
			, NumTriangles
			, NumVertices
		);

		/*IndexBufferSRV->Release();
		PositionBufferSRV->Release();*/
	}
}

void FYQPhysicsScene::AddPhysicsProxyToScene(FYQPhysicsProxy* Proxy)
{
	UE_LOG(LogTemp, Log, TEXT("AddPhysicsProxyToScene %d"), ProxyList.Num());
	
	ProxyList.Add(Proxy);

	FYQPhysicsScene* Scene = this;
	FYQPhysicsProxy* GPUPhysicsProxy = Proxy;

	ENQUEUE_RENDER_COMMAND(UYQPhysicsMeshComponent_PhysicsProxy)(
		[Scene, GPUPhysicsProxy](FRHICommandListImmediate& RHICmdList) {
		Scene->AddPhysicsProxyToScene_RenderThread(RHICmdList, GPUPhysicsProxy);
	});
}

void FYQPhysicsScene::ClearConstraints()
{
	for(int i = 0; i < (int)EConstraintType::Num; i++)
	{
		NumConstraintsList[i] = 0;
	}
}

void FYQPhysicsScene::RemovePhysicsProxyFromScene(FYQPhysicsProxy* Proxy)
{
	ProxyList.Remove(Proxy);

	FYQPhysicsScene* Scene = this;
	FYQPhysicsProxy* GPUPhysicsProxy = Proxy;

	ENQUEUE_RENDER_COMMAND(UYQPhysicsMeshComponent_PhysicsProxy)(
		[Scene, GPUPhysicsProxy](FRHICommandListImmediate& RHICmdList)
	{
		Scene->RemovePhysicsProxyToScene_RenderThread(RHICmdList, GPUPhysicsProxy);
	});
}


void FYQPhysicsScene::RemovePhysicsProxyToScene_RenderThread(FRHICommandList& RHICmdList, FYQPhysicsProxy* Proxy)
{
	check(IsInRenderingThread());

	ProxyListRenderThread.Remove(Proxy);

	WaitingReleaseProxyListRenderThread.AddUnique(Proxy);

	/*FGPUPhysicsObjectEntry ObjectFreeCommand;
	ObjectFreeCommand.ParticlePositionBufferOffset = Proxy->BufferIndexOffset;
	ObjectFreeCommand.NumParticles = Proxy->NumVertices;
	auto& ConstraintsFreeCommands = ObjectFreeCommand.Constraints;*/

	
}

FYQPhysicsPrimitiveSceneInfo::FYQPhysicsPrimitiveSceneInfo(FYQPhysicsProxy* InPhysicsProxy)
{

}