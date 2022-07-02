#pragma once

#include "Constraints.h"

#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"
#include "CommonRenderResources.h"


// 使用计算着色器将临时参数Buffer的数据传入物理运算参数Buffer
void GenerateDistanceConstraints(
    FRHICommandList& RHICmdList
    , FUnorderedAccessViewRHIRef ParticleA
    , FUnorderedAccessViewRHIRef ParticleB
    , FUnorderedAccessViewRHIRef Distance
    , FShaderResourceViewRHIRef UIntParam
    , FShaderResourceViewRHIRef FloatParam
    , uint32 ConstraintsOffset
    , uint32 NumConstraints
);

// 通过Mesh信息创建距离约束
int GenerateDistanceConstraintsFromMesh(
    FRHICommandList& RHICmdList
    , FUnorderedAccessViewRHIRef ParticleA
    , FUnorderedAccessViewRHIRef ParticleB
    , FUnorderedAccessViewRHIRef Distance
    , FShaderResourceViewRHIRef IndexBuffer
    , FShaderResourceViewRHIRef PositionBuffer
    , FRHIShaderResourceView* ColorBuffer
    , uint32 ConstraintsOffset
    , uint32 NumTriangles
    , uint32 OffsetParticles
    , bool bTest = false
);