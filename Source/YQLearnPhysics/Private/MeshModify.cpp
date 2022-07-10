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


class FYQCalcNormalAndTangentCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQCalcNormalAndTangentCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQCalcNormalAndTangentCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, PositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, IndexBuffer)
		SHADER_PARAMETER_SRV(Buffer<half2>, TexCoordBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaXBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaYBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaZBuffer)
		SHADER_PARAMETER(uint32, NumTriangles)
		SHADER_PARAMETER(uint32, NumVertices)
		SHADER_PARAMETER(uint32, PositionBufferOffset)
		END_SHADER_PARAMETER_STRUCT()


public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("CALC_NORMAL_AND_TANGENT"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQCalcNormalAndTangentCS, TEXT("/YQLearnPhysics/MeshModify.usf"), TEXT("MainCS"), SF_Compute);


class FYQResolveDeltaNormalAndTangentCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQResolveDeltaNormalAndTangentCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQResolveDeltaNormalAndTangentCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<half4>, OutputNormalBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, OutputTangentBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaXBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaYBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaZBuffer)
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
		OutEnvironment.SetDefine(TEXT("RESOLVE_DELTA_NORMAL_AND_TANGENT"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQResolveDeltaNormalAndTangentCS, TEXT("/YQLearnPhysics/MeshModify.usf"), TEXT("MainCS"), SF_Compute);

void CalcNormalAndTangentByAtomic(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef TexCoordBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaZBuffer
	, uint32 PositionBufferOffset
	, uint32 NumTriangles
	, uint32 NumVertices
)
{
	SCOPED_DRAW_EVENT(RHICmdList, CalcNormalAndTangentByAtomic);

	TShaderMapRef<FYQCalcNormalAndTangentCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumTriangles, FYQCalcNormalAndTangentCS::ThreadGroupSize);

	FYQCalcNormalAndTangentCS::FParameters PassParameters;
	PassParameters.PositionBuffer = PositionBuffer;
	PassParameters.IndexBuffer = IndexBuffer;
	PassParameters.AccumulateDeltaXBuffer = AccumulateDeltaXBuffer;
	PassParameters.AccumulateDeltaYBuffer = AccumulateDeltaYBuffer;
	PassParameters.AccumulateDeltaZBuffer = AccumulateDeltaZBuffer;
	PassParameters.TexCoordBuffer = TexCoordBuffer;
	PassParameters.NumTriangles = NumTriangles;
	PassParameters.PositionBufferOffset = PositionBufferOffset;
	PassParameters.NumVertices = NumVertices;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}


void ResolveDeltaNormalAndTangent(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef OutputNormalBuffer
	, FUnorderedAccessViewRHIRef OutputTangentBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaZBuffer
	, uint32 NumVertices
)
{
	SCOPED_DRAW_EVENT(RHICmdList, FYQResolveDeltaNormalAndTangentCS);

	TShaderMapRef<FYQResolveDeltaNormalAndTangentCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumVertices, FYQResolveDeltaNormalAndTangentCS::ThreadGroupSize);

	FYQResolveDeltaNormalAndTangentCS::FParameters PassParameters;
	PassParameters.OutputNormalBuffer = OutputNormalBuffer; 
	PassParameters.OutputTangentBuffer = OutputTangentBuffer;
	PassParameters.AccumulateDeltaXBuffer = AccumulateDeltaXBuffer;
	PassParameters.AccumulateDeltaYBuffer = AccumulateDeltaYBuffer;
	PassParameters.AccumulateDeltaZBuffer = AccumulateDeltaZBuffer;
	PassParameters.NumVertices = NumVertices;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}

void CalcNormalAndTangent(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef TexCoordBuffer
	, FUnorderedAccessViewRHIRef OutputNormalBuffer
	, FUnorderedAccessViewRHIRef OutputTangentBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaZBuffer
	, uint32 WriteOffset
	, uint32 PositionBufferOffset
	, uint32 NumVertices
	, uint32 NumTriangles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, CalcNormalAndTangent);

	CalcNormalAndTangentByAtomic(
		RHICmdList
		, PositionBuffer
		, IndexBuffer
		, TexCoordBuffer
		, AccumulateDeltaXBuffer
		, AccumulateDeltaYBuffer
		, AccumulateDeltaZBuffer
		, PositionBufferOffset
		, NumTriangles
		, NumVertices
	);

	ResolveDeltaNormalAndTangent(
		RHICmdList
		, OutputNormalBuffer
		, OutputTangentBuffer
		, AccumulateDeltaXBuffer
		, AccumulateDeltaYBuffer
		, AccumulateDeltaZBuffer
		, NumVertices
	);
}