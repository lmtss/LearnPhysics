#pragma once
#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"
#include "UniformBuffer.h"
#include "ShaderCompilerCore.h"

namespace AppendStaticMeshPermutation
{
	// Shared permutation dimensions between deferred and mobile renderer.
	class FFixedByColorDim : SHADER_PERMUTATION_BOOL("USE_FIXED_BY_COLOR");

}

class FYQAppendStaticMeshToPhysicsSceneCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQAppendStaticMeshToPhysicsSceneCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQAppendStaticMeshToPhysicsSceneCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	using FPermutationDomain = TShaderPermutationDomain<AppendStaticMeshPermutation::FFixedByColorDim>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float>, InputVertexBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, InputIndexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputVertexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputIndexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, PhysicsSceneViewBuffer)
		SHADER_PARAMETER_SRV(Buffer<half4>, InputColorBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputMaskBuffer)
		SHADER_PARAMETER(uint32, NumTriangles)
		SHADER_PARAMETER(uint32, NumVertices)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_APPEND_STATIC_TRIANGLE"), 1);

	}

};


class FYQInitializeObjectEntryCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQInitializeObjectEntryCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQInitializeObjectEntryCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ObjectEntryOffsetBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ObjectEntryCountBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ObjectEntryBitmaskBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ObjectEntryOffsetForRenderBuffer)
		SHADER_PARAMETER(uint32, BufferLength)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_INITIALIZE_OBJECT_ENTRY"), 1);

	}
};

class FYQClearBufferCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQClearBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQClearBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutBuffer)
		SHADER_PARAMETER(uint32, BufferLength)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_CLEAR_BUFFER"), 1);
	}
};


class FYQAddGPUObjectToBufferCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQAddGPUObjectToBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQAddGPUObjectToBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, InPhysicsSceneViewBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, ObjectEntryArgBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ObjectEntryOffsetBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ObjectEntryCountBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutPhysicsSceneViewBuffer)
		SHADER_PARAMETER(uint32, NumObjects)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_ADD_OBJECT_TO_BUFFER"), 1);
	}
};

namespace AppendParticlesPermutation
{
	// Shared permutation dimensions between deferred and mobile renderer.
	class FFixedByColorDim : SHADER_PERMUTATION_BOOL("USE_FIXED_BY_COLOR");

}

class FYQAppendParticlesToPhysicsSceneCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQAppendParticlesToPhysicsSceneCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQAppendParticlesToPhysicsSceneCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	using FPermutationDomain = TShaderPermutationDomain<AppendParticlesPermutation::FFixedByColorDim>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, InPhysicsSceneViewBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, InputVertexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputVertexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutPhysicsSceneViewBuffer)
		SHADER_PARAMETER_SRV(Buffer<half4>, InputColorBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputMaskBuffer)
		SHADER_PARAMETER(uint32, NumParticles)
		SHADER_PARAMETER(uint32, BaseParticle)
		SHADER_PARAMETER(FMatrix44f, LocalToWorld)
		END_SHADER_PARAMETER_STRUCT()


		

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_APPEND_PARTICLES_TO_SCENE"), 1);

	}

};

class FYQCopyDataToCPUBufferCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQCopyDataToCPUBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQCopyDataToCPUBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 8;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, PhysicsSceneViewBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, PerFrameToCPUBuffer)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_COPY_DATA_TO_CPU_BUFFER"), 1);

	}

};



void AppendTriangleToPhysicsScene_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InputIndexBuffer
	, FShaderResourceViewRHIRef InputVertexBuffer
	, FUnorderedAccessViewRHIRef OutputIndexBuffer
	, FUnorderedAccessViewRHIRef OutputVertexBuffer
	, FUnorderedAccessViewRHIRef PhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef InputColorBuffer
	, FUnorderedAccessViewRHIRef OutputMaskBuffer
	, uint32 NumTriangles
	, uint32 NumVertices
);

void InitializeObjectEntryBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef ObjectEntryOffsetBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryCountBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryBitmaskBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryOffsetForRenderBuffer
	, uint32 BufferLength
);

void ClearBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef Buffer
	, uint32 BufferLength
);

void AddGPUObjectToBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InPhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef ObjectEntryArgBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryOffsetBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryCountBuffer
	, FUnorderedAccessViewRHIRef OutPhysicsSceneViewBuffer
	, uint32 NumObjects
);

void AppendParticlesToPhysicsScene_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InPhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef InputVertexBuffer
	, FUnorderedAccessViewRHIRef OutputVertexBuffer
	, FUnorderedAccessViewRHIRef OutPhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef InputColorBuffer
	, FUnorderedAccessViewRHIRef OutputMaskBuffer
	, FMatrix44f LocalToWorld
	, uint32 NumParticles
	, uint32 BaseParticle
);

void CopyDataToCPUBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PhysicsSceneViewBuffer
	, FUnorderedAccessViewRHIRef PerFrameToCPUBuffer
);