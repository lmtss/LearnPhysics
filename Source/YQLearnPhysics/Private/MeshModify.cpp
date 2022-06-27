#include "MeshModify.h"
#include "ShaderCompilerCore.h"	//CFLAG_AllowTypedUAVLoads


class FYQCalcNormalByCrossAndAtomicCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQCalcNormalByCrossAndAtomicCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQCalcNormalByCrossAndAtomicCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, PositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, IndexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionXBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionYBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionZBuffer)
		SHADER_PARAMETER(uint32, NumTriangles)
		END_SHADER_PARAMETER_STRUCT()


public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("CALC_NORMAL_BY_CROSS_ATOMIC"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQCalcNormalByCrossAndAtomicCS, TEXT("/YQLearnPhysics/MeshModify.usf"), TEXT("MainCS"), SF_Compute);


class FYQResolveDeltaNormalCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQResolveDeltaNormalCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQResolveDeltaNormalCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputNormalBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionXBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionYBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionZBuffer)
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
		OutEnvironment.SetDefine(TEXT("RESOLVE_DELTA_NORMAL"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQResolveDeltaNormalCS, TEXT("/YQLearnPhysics/MeshModify.usf"), TEXT("MainCS"), SF_Compute);

void CalcNormalByCrossAndAtomic(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, uint32 NumTriangles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, CalcNormalByCrossAndAtomic);

	TShaderMapRef<FYQCalcNormalByCrossAndAtomicCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumTriangles, FYQCalcNormalByCrossAndAtomicCS::ThreadGroupSize);

	FYQCalcNormalByCrossAndAtomicCS::FParameters PassParameters;
	PassParameters.PositionBuffer = PositionBuffer;
	PassParameters.IndexBuffer = IndexBuffer;
	PassParameters.AccumulateDeltaPositionXBuffer = AccumulateDeltaPositionXBuffer;
	PassParameters.AccumulateDeltaPositionYBuffer = AccumulateDeltaPositionYBuffer;
	PassParameters.AccumulateDeltaPositionZBuffer = AccumulateDeltaPositionZBuffer;
	PassParameters.NumTriangles = NumTriangles;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}

void ResolveDeltaNormal(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef OutputNormalBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, uint32 NumVertices
)
{
	SCOPED_DRAW_EVENT(RHICmdList, ResolveDeltaNormal);

	TShaderMapRef<FYQResolveDeltaNormalCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumVertices, FYQResolveDeltaNormalCS::ThreadGroupSize);

	FYQResolveDeltaNormalCS::FParameters PassParameters;
	PassParameters.OutputNormalBuffer = OutputNormalBuffer;
	PassParameters.AccumulateDeltaPositionXBuffer = AccumulateDeltaPositionXBuffer;
	PassParameters.AccumulateDeltaPositionYBuffer = AccumulateDeltaPositionYBuffer;
	PassParameters.AccumulateDeltaPositionZBuffer = AccumulateDeltaPositionZBuffer;
	PassParameters.NumVertices = NumVertices;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}

void CalcNormalByCross(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FUnorderedAccessViewRHIRef OutputNormalBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, uint32 NumVertices
	, uint32 NumTriangles
)
{
	CalcNormalByCrossAndAtomic(RHICmdList, PositionBuffer, IndexBuffer, AccumulateDeltaPositionXBuffer, AccumulateDeltaPositionYBuffer, AccumulateDeltaPositionZBuffer, NumTriangles);

	ResolveDeltaNormal(RHICmdList, OutputNormalBuffer, AccumulateDeltaPositionXBuffer, AccumulateDeltaPositionYBuffer, AccumulateDeltaPositionZBuffer, NumVertices);
}