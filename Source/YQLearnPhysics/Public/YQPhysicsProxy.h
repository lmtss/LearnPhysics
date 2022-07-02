#pragma once

#include "CoreMinimal.h"
#include "RenderResource.h"
#include "Constraints.h"
#include "Collision.h"

// 
struct FYQPhysicsSceneBufferEntry 
{
    uint32 IndexBufferOffset;
    uint32 VertexBufferOffset;
};

struct FConstraintsBatch
{
    EConstraintType Type = EConstraintType::Num;
    EConstraintSourceType ConstraintSourceType = EConstraintSourceType::Num;
    uint32 NumConstraints = 0;

    TArray<uint32> UIntBuffer;
    TArray<float> FloatBuffer;

    union {
        uint64 PackedFlags;
        struct {
            uint64 bUseMeshInfo : 1;
        };
    };
};

struct FYQCollisionInfo
{
    EYQCollisionType Type = EYQCollisionType::Num;
    FBox BoundingBox;
    
};

// 
// 尽可能不存储图形资源
class FYQPhysicsProxy
{
public:
    FYQPhysicsProxy();

	union {
		uint64 PackedFlags;
		struct {
            uint64 bIsIndexBuffer32 : 1;
		};
	};

    // 静态网格
    uint32 NumVertices;
    FBufferRHIRef IndexBufferRHI;
    FBufferRHIRef VertexBufferRHI;
    FRHIShaderResourceView* ColorBufferSRV;
    FShaderResourceViewRHIRef IndexBufferSRV;

    FYQPhysicsSceneBufferEntry BufferEntry;

    // 对应在PhysicsScene的Buffer中的起始位置
    uint32 BufferIndexOffset;

    // 共有多少个元素，通常等同于多少个粒子    
    uint32 NumBufferElement;

    //
    FMatrix44f LocalToWorld;

    // 用来收集这个约束的信息来产生最终ComputeShader的Dispatch信息
    // 当前无论是`静态`还是`动态产生约束`都要经过这个函数
    virtual void GetDynamicPhysicsConstraints(FConstraintsBatch& OutBatch) const {};
    
    // 获取碰撞相关信息
    // 目前用来给静态网格生成BVH
    virtual void GetCollisionInfo(FYQCollisionInfo& OutInfo) const {};
private:

};