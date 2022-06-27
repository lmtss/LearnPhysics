#pragma once
#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"
#include "CommonRenderResources.h"
#include "ShaderParameterUtils.h"


void CopyPositionForCollision(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FUnorderedAccessViewRHIRef OutputPositionBuffer
	, uint32 NumParticles
);

void NaiveCollisionWithCollider(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBufferCopy
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FShaderResourceViewRHIRef NormalBuffer
	, FShaderResourceViewRHIRef MaskBuffer
	, FUnorderedAccessViewRHIRef OutputPositionBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, FUnorderedAccessViewRHIRef FeedbackBuffer
	, FUnorderedAccessViewRHIRef FeedbackBufferSizes
	, FVector3f ColliderCenterPosition
	, float SphereColliderRadius
	, FVector3f ColliderVelocity
	, float SphereColliderMass
	, uint32 NumParticles
	, float DeltaTime
	, uint32 IndexIter
	, uint32 NumIters
);