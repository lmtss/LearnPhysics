#pragma once

#include "YQRHIUtilities.h"
#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"

#include "Engine/Engine.h"

#include "Experimental/Containers/RobinHoodHashTable.h"

#include "YQPhysicsProxy.h"
#include "GPUBasedBVH.h"
#include "PrimitiveInstanceUpdateCommand.h"

// static TAutoConsoleVariable<uint32> CVARPhysicsSceneParticleMaxNum(
// 	TEXT("r.YQPhysics.MaxNumParticles"),
// 	4194304,
// 	TEXT(""),
// 	ECVF_RenderThreadSafe
// );

#define YQ_MAX_NUM_PARTICLE 4194304

class FCPUObjectProxy
{
public:
	FVector WorldPosition;
	FVector Velocity;

	FVector FeedBack;
	float Mass;
	float Size;
};

struct FGPUPhysicsObjectAllocInfo
{
	uint32 Count;
	uint32 Bitmask;

	FShaderResourceViewRHIRef VertexBuffer;

	FConstraintsBatch ConstraintsBatch;
	FBufferRHIRef IndexBufferRHI;
	FBufferRHIRef VertexBufferRHI;
	FRHIShaderResourceView* ColorBufferSRV;

	uint32 NumTriangles;

	FMatrix44f LocalToWorld;
};

class FYQPhysicsScene
{


public:
	FYQPhysicsScene();
	~FYQPhysicsScene();

	//static FYQPhysicsScene* Get();
	bool AllocImmediate(FRHICommandListImmediate& RHICmdList, uint32 NumParticle, uint32& OutParticleStartIndex);

	// 清空内容
	// 清空Proxy
	// 清空Buffer
	void Reset();

#if 1

	FRWBuffer& GetPositionBufferA();
	FRWBuffer& GetPositionBufferB();
	FRWBuffer& GetPredictPositionBuffer();
	FRWBuffer& GetOrignalPositionBuffer();

	FRWBuffer& GetParticleIndexBuffer();
	FIndexBuffer& GetParticleIndexBufferForRender();
	
	FRWBuffer& GetVelocityBuffer();
	FRWBuffer& GetNormalBuffer();
	FRWBuffer& GetVertexMaskBuffer();
	FRWBuffer& GetFeedbackBuffer();
	FRWBuffer& GetFeedbackBufferSizes();

	
	FRWBuffer& GetAccumulateDeltaPositionXBuffer();
	FRWBuffer& GetAccumulateDeltaPositionYBuffer();
	FRWBuffer& GetAccumulateDeltaPositionZBuffer();
	FRWBuffer& GetAccumulateCountBuffer();

	
	FRWBuffer& GetDistanceConstraintsParticleAIDBuffer();
	FRWBuffer& GetDistanceConstraintsParticleBIDBuffer();
	FRWBuffer& GetDistanceConstraintsDistanceBuffer();

	FRWBuffer& GetVisualizeBVHNodeBuffer();
	FRWBuffer& GetIndirectArgBuffer();

	FRWBuffer& GetPhysicsSceneViewBuffer();
	
	FRWBuffer& GetBVPositionBufferA();
	FRWBuffer& GetBVPositionBufferB();
	FRWBuffer& GetBVHLeftChildIDBuffer();
	
	FRWBuffer& GetMortonCodeBufferA();
	FRWBuffer& GetMortonCodeBufferB();

	FGPUBvhBuffers& GetGPUBvhBuffers();

	
	FRWBuffer& GetPositionBuffer()
	{
		return bIsPingpangA ? PositionBufferA : PositionBufferB;
	}

	FRWBuffer& GetOutputPositionBuffer() 
	{
		return bIsPingpangA ? PositionBufferB : PositionBufferA;
	}

#endif

	void SwapBuffer() 
	{
		bIsPingpangA = !bIsPingpangA;
	}

	void Initialize(FRHICommandListImmediate& RHICmdList, bool bIsBeginPlay, bool bForce);


	// ----------- 与CPU对象交互 -------------
	void AddCPUActorToScene(USceneComponent* InActor);
	void AddCPUActorToScene_RenderThread(FCPUObjectProxy* Proxy);
	void UpdateCPUActor(USceneComponent* InActor);
	void UpdateCPUActor_RenderThread(FCPUObjectProxy* Proxy, FVector WorldPosition, FVector Velocity);

	TArray< FCPUObjectProxy*>& GetCPUObjectProxyList(){
		return CPUObjectProxyListRenderThread;
	}

	FVector GetCPUObjectFeedBack_GameThread(USceneComponent* InComponent);

	//------------ 添加GPU物体 ----------------
	
	// 将静态网格添加到物理场景
	// 目前只是生成BVH
	void AddStaticMeshToScene(UStaticMeshComponent* StaticMeshComponent);

	uint32 GetNumStaticMeshTriangles();
	uint32 GetNumParticlesFromGPU();
	FBox GetBoundingBox();

	void AddPhysicsProxyToScene(FYQPhysicsProxy* Proxy);
	void RemovePhysicsProxyFromScene(FYQPhysicsProxy* Proxy);
	void UpdateGPUObjectTransform_GameThread(FYQPhysicsProxy* Proxy, UPrimitiveComponent* Primitive);
	void UpdateGPUObjectTransform_RenderThread(FYQPhysicsProxy* Proxy, FMatrix44f LocalToWorld);

	void ClearConstraints();

	void Tick(FRHICommandList& RHICmdList);

	bool IsBVHInit = false;

	uint32 GetNumConstraints(EConstraintType Type)
	{
		return NumConstraintsList[(int)Type];
	}

	TArray<FYQPhysicsProxy*>& GetProxyListRenderThread()
	{
		return ProxyListRenderThread;
	}

private:

	struct FUpdateCPUObjectTransformCommand
	{
		FVector CenterPosition;
		FVector Velocity;
	};

	struct FUpdateGPUObjectTransformCommand
	{
		FMatrix44f LocalToWorld;
	};


	void UpdateCPUObjectTransform();
	void UpdateGPUObjectTransform(FRHICommandList& RHICmdList);

	void CopyDataToCPU(FRHICommandList& RHICmdList);

	// 每帧固定从GPU拷贝到CPU的buffer的大小, 4B为单位
	uint32 GetSizePerFrameToCPU()
	{
		return 1;
	}

	// 初始化各种Buffer
	// 
	void InitializeBuffer(FRHICommandList& RHICmdList);

	void AddPhysicsProxyToScene_RenderThread(FRHICommandList& RHICmdList, FYQPhysicsProxy* Proxy);

	// 申请一段空间
	void AllocateBufferMemory(uint32 IndexBufferLength, uint32 VertexBufferLength, FYQPhysicsSceneBufferEntry& OutEntry);

	//void AllocGPUObjectMemory
	
	void CopyBufferToPhysicsScene(
		FRHICommandList& RHICmdList
		, FBufferRHIRef IndexBufferRHI, FBufferRHIRef VertexBufferRHI, FYQPhysicsSceneBufferEntry Entry
		, uint32 NumTriangles
		, uint32 NumVertices
		, bool bIsIndexBuffer32
	);
	
	void AddGPUObject(FGPUPhysicsObjectAllocInfo& Info);

	void AddGPUObjectToBuffer(FRHICommandList& RHICmdList);

	void InitResource();
	void ReleaseResource();

	bool bIsInitByBeginPlay = false;	
	bool bIsPingpangA = true;

	// 存储粒子的Offset+Count
	// Object的详细属性
	FRWBuffer ObjectEntryOffsetBuffer;
	FRWBuffer ObjectEntryCountBuffer;
	FRWBuffer ObjectEntryBitmaskBuffer;
	FRWBuffer ObjectEntryOffsetForRenderBuffer;

	// 等待放入GPU的物体
	TArray< FGPUPhysicsObjectAllocInfo> WaitingObjectList;

	// 每Tick从CPU创建的物体，在CPU端写入
	FRWBuffer ObjectIntInfoToGPUBuffer;
	FRWBuffer ObjectFloatInfoToGPUBuffer;

	// 每帧从GPU拷贝的数据，比如粒子数量
	FWriteToCPUBuffer PerFrameToCPUBuffer;
	uint32 NumParticlesFromGPU;
	
	FRWBuffer VertexMaskBuffer;

	FRWBuffer PositionBufferA;
	FRWBuffer PositionBufferB;
	FRWBuffer PredictPositionBuffer;
	FRWBuffer OrignalPositionBuffer;

	FRWBuffer IndexBuffer;
	FIndexBuffer IndexBufferForRender;

	FRWBuffer VelocityBuffer;

	FRWBuffer NormalBuffer;

	FRWBuffer FeedbackBuffer;
	FRWBuffer FeedbackBufferSizes;

	uint32 CurrentBufferAllocIndex = 0;

	// 在GameThread的物体进行物理运算的物体
	TArray< FCPUObjectProxy*> CPUObjectProxyListRenderThread;
	TMap<USceneComponent*, FCPUObjectProxy*> CPUObjectActorMapGameThread;
	TMap<FCPUObjectProxy*, FUpdateCPUObjectTransformCommand> UpdatedCPUObjectTransformsRenderThread;

	TArray<FVector4> CPUObjectPositions;
	TArray<FVector4> CPUObjectFeedback;

	// 物理代理
	TArray<FYQPhysicsProxy*> ProxyList;
	TArray<FYQPhysicsProxy*> ProxyListRenderThread;

	TMap<FYQPhysicsProxy*, FUpdateGPUObjectTransformCommand> UpdatedGPUObjectTransformsRenderThread;

	// 约束数量
	TArray<uint32> NumConstraintsList;

	// 用来从CPU构建约束的Buffer
	FRWBuffer TempUIntConstraintsParamBuffer;
	FRWBuffer TempFloatConstraintsParamBuffer;

	// Delta Position Buffer
	FRWBuffer AccumulateDeltaPositionXBuffer;
	FRWBuffer AccumulateDeltaPositionYBuffer;
	FRWBuffer AccumulateDeltaPositionZBuffer;
	// 
	FRWBuffer AccumulateCountBuffer;

	// ID == [MAX粒子数] 的粒子为无效
	// ID 中 后28位表示ID

	// 距离约束的Buffer
	// 粒子A的ID
	FRWBuffer DistanceConstraintsParticleAIDBuffer;
	// 粒子B的ID
	FRWBuffer DistanceConstraintsParticleBIDBuffer;
	// 距离Buffer
	FRWBuffer DistanceConstraintsDistanceBuffer;

	// 碰撞约束的Buffer
	// 
	FRWBuffer CollisionParticleIDBuffer;
	FRWBuffer CollisionErrorDistanceBuffer;

	// 各种参数buffer
	FRWBuffer PhysicsSceneViewBuffers[2];
	int PhysicsSceneViewBufferPingPang = 0;
	void PhysicsSceneViewBufferSwap()
	{
		PhysicsSceneViewBufferPingPang = 1 - PhysicsSceneViewBufferPingPang;
	}

	// -------------------- BVH --------------------------
	FGPUBvhBuffers GPUBvhBuffers;

	// 可视化BVH用的
	FRWBuffer VisualizeNodeIDBuffer;
	FRWBuffer IndirectArgBuffer;
};

struct FPhysicsSceneView 	
{
	// BVH根节点的ID
	uint32 BVHRootID;

	// 有多少个图元
	uint32 NumPrimitives;

	// PositionBuffer 中有多少有效粒子
	// 每一tick会通过归约来计算，并更新这个值
	uint32 NumParticles;

	// ObjectEntryBuffer 中有多少个Object
	// 可能不是有效的
	uint32 NumObject;


};

