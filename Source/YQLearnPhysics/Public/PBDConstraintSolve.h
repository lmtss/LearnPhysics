#pragma once
#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "CommonRenderResources.h"

void SolvePBDDistanceConstraint_RenderThread(
    FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticleABuffer
	, FShaderResourceViewRHIRef ParticleBBuffer
	, FShaderResourceViewRHIRef DistanceBuffer
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, FUnorderedAccessViewRHIRef AccumulateCountBuffer
	, uint32 NumConstraints
	, float DeltaTime
	, float Stiffness
);

void ResolveDeltaPosition_RenderThread(
    FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InParticlePositionBuffer
	, FUnorderedAccessViewRHIRef OutParticlePositionBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, FUnorderedAccessViewRHIRef AccumulateCountBuffer
	, uint32 NumParticle
	, float DeltaTime
);

void InitializeDeltaBuffer_RenderThread(
    FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, FUnorderedAccessViewRHIRef AccumulateCountBuffer
	, uint32 NumParticle
);

void SolvePBDBendingConstraint_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticleABuffer
	, FShaderResourceViewRHIRef ParticleBBuffer
	, FShaderResourceViewRHIRef ParticleCBuffer
	, FShaderResourceViewRHIRef ParticleDBuffer
	, FShaderResourceViewRHIRef AngleBuffer
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, FUnorderedAccessViewRHIRef AccumulateCountBuffer
	, uint32 NumConstraints
	, float DeltaTime
	, float Stiffness
);


void SimpleSphereCollisionDetect_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FUnorderedAccessViewRHIRef CollisionParticleIDBuffer
	, FUnorderedAccessViewRHIRef CollisionErrorDistanceBuffer
	, FVector4f SphereInfo
	, uint32 NumParticle
);

void SimpleSphereCollisionHandle_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FUnorderedAccessViewRHIRef CollisionParticleIDBuffer
	, FUnorderedAccessViewRHIRef CollisionErrorDistanceBuffer
	, FVector4f SphereInfo
	, uint32 NumParticle
);