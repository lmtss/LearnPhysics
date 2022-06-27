#include "GPUBasedBVH.h"

class FTestViewExtPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTestViewExtPS);
	SHADER_USE_PARAMETER_STRUCT(FTestViewExtPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{

		return true;
	}
};

IMPLEMENT_GLOBAL_SHADER(FTestViewExtPS, "/YQLearnPhysics/VisualizeBVH.usf", "TestViewExtPS", SF_Pixel);

class FTestViewExtVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTestViewExtVS);
	SHADER_USE_PARAMETER_STRUCT(FTestViewExtVS, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, ViewUniformBuffer)
		SHADER_PARAMETER_SRV(Buffer<float4>, LeafBVBuffer0)
		SHADER_PARAMETER_SRV(Buffer<float2>, LeafBVBuffer1)
		SHADER_PARAMETER_SRV(Buffer<float4>, InternalNodeBVBuffer0)
		SHADER_PARAMETER_SRV(Buffer<float2>, InternalNodeBVBuffer1)
		SHADER_PARAMETER_SRV(Buffer<uint>, VisualizeNodeIDBuffer)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{

		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) 
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("COMPILE_VISUALIZE_BVH_VS"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FTestViewExtVS, "/YQLearnPhysics/VisualizeBVH.usf", "TestViewExtVS", SF_Vertex);

class FGenerateSingleLayerBVCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FGenerateSingleLayerBVCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FGenerateSingleLayerBVCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, BVHChildIDBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, PhysicsSceneViewBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, VisualizeNodeIDBuffer)
		SHADER_PARAMETER(uint32, LayerIndex)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_SINGLE_LAYER_BV"), 1);
	}

};

IMPLEMENT_SHADER_TYPE(, FGenerateSingleLayerBVCS, TEXT("/YQLearnPhysics/VisualizeBVH.usf"), TEXT("MainCS"), SF_Compute);

class FGenerateVisualizeBVHIndirectArgCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FGenerateVisualizeBVHIndirectArgCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FGenerateVisualizeBVHIndirectArgCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 16;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, VisualizeNodeIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, IndirectArgBuffer)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_VISUALIZE_INDIRECT_ARG"), 1);
	}

};

IMPLEMENT_SHADER_TYPE(, FGenerateVisualizeBVHIndirectArgCS, TEXT("/YQLearnPhysics/VisualizeBVH.usf"), TEXT("MainCS"), SF_Compute);

class FClearVisualBVHBufferCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FClearVisualBVHBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FClearVisualBVHBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 16;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, VisualizeNodeIDBuffer)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_CLEAR_VISUALIZE_BVH_BUFFER"), 1);
	}

};

IMPLEMENT_SHADER_TYPE(, FClearVisualBVHBufferCS, TEXT("/YQLearnPhysics/VisualizeBVH.usf"), TEXT("MainCS"), SF_Compute);

void VisualizeBVH_RenderThread(
	FRHICommandList& RHICmdList
	, FSceneView& InView
	, FShaderResourceViewRHIRef LeafBVBuffer0
	, FShaderResourceViewRHIRef LeafBVBuffer1
	, FShaderResourceViewRHIRef InternalNodeBVBuffer0
	, FShaderResourceViewRHIRef InternalNodeBVBuffer1
	, FShaderResourceViewRHIRef BVHChildIDBuffer
	, FShaderResourceViewRHIRef PhysicsSceneViewBuffer
	, FRWBuffer& VisualizeNodeIDBuffer
	, FRWBuffer& IndirectArgBuffer
	, uint32 LayerIndex
)
{
	SCOPED_DRAW_EVENT(RHICmdList, VisualizeBVH);

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ERHIFeatureLevel::SM5);
    
	// --------- 清空buffer，只需要清空第一个，也就是表示数量的就好 ------------
	TShaderMapRef<FClearVisualBVHBufferCS> ClearVisualBVHBufferCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ClearVisualBVHBufferCS.GetComputeShader());

	FClearVisualBVHBufferCS::FParameters ClearPassParameters;
	ClearPassParameters.VisualizeNodeIDBuffer = VisualizeNodeIDBuffer.UAV;

	SetShaderParameters(RHICmdList, ClearVisualBVHBufferCS, ClearVisualBVHBufferCS.GetComputeShader(), ClearPassParameters);
	RHICmdList.DispatchComputeShader(1, 1, 1);
	UnsetShaderUAVs(RHICmdList, ClearVisualBVHBufferCS, ClearVisualBVHBufferCS.GetComputeShader());

	// --------- 计算某一层的节点 ----------
	TShaderMapRef<FGenerateSingleLayerBVCS> GenerateLayerNodeCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateLayerNodeCS.GetComputeShader());

	

	uint32 Length = 1 << LayerIndex;

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FGenerateSingleLayerBVCS::ThreadGroupSize);

	FGenerateSingleLayerBVCS::FParameters PassParameters;
	PassParameters.PhysicsSceneViewBuffer = PhysicsSceneViewBuffer;
	PassParameters.BVHChildIDBuffer = BVHChildIDBuffer;
	PassParameters.VisualizeNodeIDBuffer = VisualizeNodeIDBuffer.UAV;
	PassParameters.LayerIndex = LayerIndex;
	PassParameters.Length = Length;

	SetShaderParameters(RHICmdList, GenerateLayerNodeCS, GenerateLayerNodeCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateLayerNodeCS, GenerateLayerNodeCS.GetComputeShader());

	// --------- 计算IndirectArg ---------
	TShaderMapRef<FGenerateVisualizeBVHIndirectArgCS> GenerateDrawIndirectCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateDrawIndirectCS.GetComputeShader());

	FGenerateVisualizeBVHIndirectArgCS::FParameters GenerateIndirectParameters;
	GenerateIndirectParameters.VisualizeNodeIDBuffer = VisualizeNodeIDBuffer.SRV;
	GenerateIndirectParameters.IndirectArgBuffer = IndirectArgBuffer.UAV;

	SetShaderParameters(RHICmdList, GenerateDrawIndirectCS, GenerateDrawIndirectCS.GetComputeShader(), GenerateIndirectParameters);
	RHICmdList.DispatchComputeShader(1, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateDrawIndirectCS, GenerateDrawIndirectCS.GetComputeShader());

	// --------- 绘制 --------------

	TShaderMapRef< FTestViewExtVS > VertexShader(GlobalShaderMap);
    TShaderMapRef< FTestViewExtPS > PixelShader(GlobalShaderMap);
    
    FGraphicsPipelineStateInitializer GraphicsPSOInit;
    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
    GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
    GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
    GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
    GraphicsPSOInit.PrimitiveType = PT_LineList;
    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
    GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
    GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
    SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

    FTestViewExtVS::FParameters Parameters;
    Parameters.ViewUniformBuffer = InView.ViewUniformBuffer;
	Parameters.LeafBVBuffer0 = LeafBVBuffer0;
	Parameters.LeafBVBuffer1 = LeafBVBuffer1;
	Parameters.InternalNodeBVBuffer0 = InternalNodeBVBuffer0;
	Parameters.InternalNodeBVBuffer1 = InternalNodeBVBuffer1;
	Parameters.VisualizeNodeIDBuffer = VisualizeNodeIDBuffer.SRV;

	SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), Parameters);

    //RHICmdList.DrawPrimitive(0, 12, 1922);
	RHICmdList.DrawPrimitiveIndirect(IndirectArgBuffer.Buffer, 0);

}



