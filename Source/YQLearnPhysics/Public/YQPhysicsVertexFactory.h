#pragma once

#include "CoreMinimal.h"
#include "ShaderParameters.h"
#include "Components.h"
#include "VertexFactory.h"

#include "RHICommandList.h"
#include "RenderResource.h"



class YQLEARNPHYSICS_API FYQPhysicsVertexFactory : public FVertexFactory {
	DECLARE_VERTEX_FACTORY_TYPE(FYQPhysicsVertexFactory);
public:

	FYQPhysicsVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, const char* InDebugName, FStaticMeshRenderData* InRenderData)
		: FVertexFactory(InFeatureLevel)
		, DebugName(InDebugName)
		, RenderData(InRenderData)
	{
	}

	/**
	 * Should we cache the material's shadertype on this platform with this vertex factory?
	 */
	static bool ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters);

	static void ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment);

	static void ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap, TArray<FString>& OutErrors);

	// FRenderResource interface.
	virtual void InitRHI() override;
	virtual void ReleaseRHI() override;


	FUnorderedAccessViewRHIRef GetDynamicNormalBufferUAV();
	FUnorderedAccessViewRHIRef GetDynamicTangentBufferUAV();
	FShaderResourceViewRHIRef GetDynamicNormalBufferSRV();
	FShaderResourceViewRHIRef GetDynamicTangentBufferSRV();

	void SetData(const FStaticMeshDataType& InData);
	
	inline const FUniformBufferRHIRef GetUniformBuffer() const 
	{
		return UniformBuffer;
	}

	inline void SetUniformBuffer();


	FShaderResourceViewRHIRef TexCoordBufferSRV;
	FUniformBufferRHIRef UniformBuffer;
	uint32 BufferIDOffset;

protected:

	FStaticMeshDataType Data;
	FStaticMeshRenderData* RenderData;

	FRWBuffer DynamicNormalBuffer;
	FRWBuffer DynamicTangentBuffer;

	struct FDebugName 
	{
		FDebugName(const char* InDebugName)
#if !UE_BUILD_SHIPPING
			: DebugName(InDebugName)
#endif
		{
		}
	private:
#if !UE_BUILD_SHIPPING
		const char* DebugName;
#endif
	} DebugName;

private:
	
	
};

struct ClothVertex
{
	FVector Position;
};

BEGIN_GLOBAL_SHADER_PARAMETER_STRUCT(FYQPhysicsUniformParameters, YQLEARNPHYSICS_API)
//SHADER_PARAMETER_SRV(StructuredBuffer<ClothVertex>, ClothStructureBuffer)
SHADER_PARAMETER(uint32, YQPhysicsVertexOffset)
END_GLOBAL_SHADER_PARAMETER_STRUCT()

typedef TUniformBufferRef<FYQPhysicsUniformParameters> FYQPhysicsUniformBufferRef;

struct FYQPhysicsUserData : public FOneFrameResource 
{
	FRWBuffer* PositionBuffer;
	FRWBuffer* NormalBuffer;
	uint32 BufferIndexOffset;
};