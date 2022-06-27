#include "YQStreamCompact.h"

#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "ShaderCore.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "RHI.h"

class FYQFillUintBufferCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQFillUintBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQFillUintBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputBuffer)
		SHADER_PARAMETER(uint32, Value)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("FILL_UINT_BUFFER"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};


IMPLEMENT_SHADER_TYPE(, FYQFillUintBufferCS, TEXT("/YQLearnPhysics/YQFillBuffer.usf"), TEXT("MainCS"), SF_Compute);

class FYQRandomFillIntBufferCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQRandomFillIntBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQRandomFillIntBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputBuffer)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("RANDOM_FILL_INT_BUFFER"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};


IMPLEMENT_SHADER_TYPE(, FYQRandomFillIntBufferCS, TEXT("/YQLearnPhysics/YQFillBuffer.usf"), TEXT("MainCS"), SF_Compute);


class FYQStreamCompactCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQStreamCompactCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQStreamCompactCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	class FRakingDim : SHADER_PERMUTATION_BOOL("USE_RAKING");
	using FPermutationDomain = TShaderPermutationDomain<FRakingDim>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<int>, IDToIndexTable)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, RWFreeIDList)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, RWFreeIDListSizes)
		SHADER_PARAMETER(uint32, FreeIDListIndex)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		FPermutationDomain PermutationVector(Parameters.PermutationId);
		const bool bUseRaking = PermutationVector.Get<FRakingDim>();
		OutEnvironment.SetDefine(TEXT("USE_RAKING"), bUseRaking ? 1 : 0);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQStreamCompactCS, TEXT("/YQLearnPhysics/YQStreamCompact.usf"), TEXT("MainCS"), SF_Compute);


void YQStreamCompact(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef IDBuffer
	, FUnorderedAccessViewRHIRef OutputBuffer
	, FUnorderedAccessViewRHIRef SizeBuffer
	, uint32 LengthToClear
)
{
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, FYQStreamCompactCS);

	const bool bUseRaking = true;

	FYQStreamCompactCS::FPermutationDomain PermutationVector;
	PermutationVector.Set<FYQStreamCompactCS::FRakingDim>(bUseRaking);

	auto ComputeShaderEntry = GetGlobalShaderMap(GMaxRHIFeatureLevel)->GetShader<FYQStreamCompactCS>(PermutationVector);
	FRHIComputeShader* ComputeShader = ComputeShaderEntry.GetComputeShader();

	//TShaderMapRef<FYQStreamCompactCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader);

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(LengthToClear, FYQStreamCompactCS::ThreadGroupSize * (bUseRaking ? 8 : 1));

	FYQStreamCompactCS::FParameters PassParameters;
	PassParameters.IDToIndexTable = IDBuffer;
	PassParameters.RWFreeIDList = OutputBuffer;
	PassParameters.RWFreeIDListSizes = SizeBuffer;
	PassParameters.FreeIDListIndex = 0;
	
	RHICmdList.Transition(FRHITransitionInfo(OutputBuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShaderEntry, ComputeShader, PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShaderEntry, ComputeShader);
	RHICmdList.Transition(FRHITransitionInfo(OutputBuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}

void YQRandomInitializeIDBuffer(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef IDBuffer
	, uint32 LengthToClear
) 
{
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, YQRandomInitializeIDBuffer);

	TShaderMapRef<FYQRandomFillIntBufferCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(LengthToClear, FYQRandomFillIntBufferCS::ThreadGroupSize);

	FYQRandomFillIntBufferCS::FParameters PassParameters;
	PassParameters.OutputBuffer = IDBuffer;
	PassParameters.Length = LengthToClear;

	RHICmdList.Transition(FRHITransitionInfo(IDBuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(IDBuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}

void YQInitializeSizeBuffer(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef RWFreeIDListSizes
	, uint32 LengthToClear
	, uint32 Value
) {
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, YQInitializeSizeBuffer);

	TShaderMapRef<FYQFillUintBufferCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(LengthToClear, FYQFillUintBufferCS::ThreadGroupSize);

	FYQFillUintBufferCS::FParameters PassParameters;
	PassParameters.OutputBuffer = RWFreeIDListSizes;
	PassParameters.Value = Value;
	PassParameters.Length = LengthToClear;

	RHICmdList.Transition(FRHITransitionInfo(RWFreeIDListSizes, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(RWFreeIDListSizes, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}


FRWBuffer YQProfileIDBuffer;
FRWBuffer YQProfileSizeBuffer;
FRWBuffer YQProfileOutputBuffer;
uint32 YQProfileLength;

void YQTest_InitializeBuffer(FRHICommandList& RHICmdList)
{
	YQProfileLength = 1024 * 1024;

	if (!YQProfileIDBuffer.Buffer) 		
	{
		YQProfileIDBuffer.Initialize(TEXT("YQProfileIDBuffer"), sizeof(uint32), YQProfileLength, EPixelFormat::PF_R32_SINT, ERHIAccess::UAVCompute, BUF_Static);
		YQProfileSizeBuffer.Initialize(TEXT("YQProfileSizeBuffer"), sizeof(uint32), 2, EPixelFormat::PF_R32_UINT, ERHIAccess::UAVCompute, BUF_Static);
		YQProfileOutputBuffer.Initialize(TEXT("YQProfileOutputBuffer"), sizeof(uint32), YQProfileLength, EPixelFormat::PF_R32_UINT, ERHIAccess::UAVCompute, BUF_Static);
	}
}


void YQTest_UpdateBuffer(FRHICommandList& RHICmdList)
{
	YQProfileLength = 1024 * 1024;

	YQInitializeSizeBuffer(RHICmdList, YQProfileSizeBuffer.UAV, 2, 0);
	YQRandomInitializeIDBuffer(RHICmdList, YQProfileIDBuffer.UAV, YQProfileLength);
	YQStreamCompact(RHICmdList, YQProfileIDBuffer.SRV, YQProfileOutputBuffer.UAV, YQProfileSizeBuffer.UAV, YQProfileLength);

	/*YQInitializeSizeBuffer(RHICmdList, YQProfileSizeBuffer.UAV, 2, 0);
	YQRandomInitializeIDBuffer(RHICmdList, YQProfileIDBuffer.UAV, YQProfileLength);
	YQStreamCompact(RHICmdList, YQProfileIDBuffer.SRV, YQProfileOutputBuffer.UAV, YQProfileSizeBuffer.UAV, YQProfileLength);*/
}
