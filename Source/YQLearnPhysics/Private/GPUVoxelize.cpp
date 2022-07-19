#include "GPUVoxelize.h"

class FVoxelizeMeshSurfacePS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVoxelizeMeshSurfacePS);
	SHADER_USE_PARAMETER_STRUCT(FVoxelizeMeshSurfacePS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWTexture3D<uint>, VoxelVolume)
		SHADER_PARAMETER(FVector4f, VoxelVolumeSize)
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{

		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("VOXELIZE_MESH_SURFACE_PS"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FVoxelizeMeshSurfacePS, "/YQLearnPhysics/Voxelize.usf", "Main", SF_Pixel);

class FVoxelizeMeshSurfaceVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FVoxelizeMeshSurfaceVS);
	SHADER_USE_PARAMETER_STRUCT(FVoxelizeMeshSurfaceVS, FGlobalShader);
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		END_SHADER_PARAMETER_STRUCT()

		static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{

		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("VOXELIZE_MESH_SURFACE_VS"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FVoxelizeMeshSurfaceVS, "/YQLearnPhysics/Voxelize.usf", "Main", SF_Vertex);

void VoxelizeMeshSurface(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FBufferRHIRef IndexBuffer
	, FUnorderedAccessViewRHIRef VoxelVolume
	, FVector VoxelVolumeSize
	, uint32 NumVertices
	, uint32 NumTriangles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, VoxelizeMeshSurface);

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ERHIFeatureLevel::SM5);

	TShaderMapRef< FVoxelizeMeshSurfaceVS > VertexShader(GlobalShaderMap);
	TShaderMapRef< FVoxelizeMeshSurfacePS > PixelShader(GlobalShaderMap);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BlendState = TStaticBlendState<CW_NONE>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GetVertexDeclarationFVector4();
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit, 0);

	/*FTestViewExtVS::FParameters Parameters;
	Parameters.ViewUniformBuffer = InView.ViewUniformBuffer;
	Parameters.LeafBVBuffer0 = LeafBVBuffer0;
	Parameters.LeafBVBuffer1 = LeafBVBuffer1;
	Parameters.InternalNodeBVBuffer0 = InternalNodeBVBuffer0;
	Parameters.InternalNodeBVBuffer1 = InternalNodeBVBuffer1;
	Parameters.VisualizeNodeIDBuffer = VisualizeNodeIDBuffer.SRV;

	SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), Parameters);*/

	FVoxelizeMeshSurfacePS::FParameters PixelParameters;
	PixelParameters.VoxelVolume = VoxelVolume;
	PixelParameters.VoxelVolumeSize = FVector4f(VoxelVolumeSize.X, VoxelVolumeSize.Y, VoxelVolumeSize.Z, 0.0f);

	SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PixelParameters);

	RHICmdList.SetViewport(
		0, 0, 0, 
		VoxelVolumeSize.X, VoxelVolumeSize.Y, VoxelVolumeSize.Z
	);

	RHICmdList.DrawIndexedPrimitive(IndexBuffer, 0, 0, NumVertices, 0, NumTriangles, 0);
}