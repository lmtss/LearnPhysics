#include "PBDConstraintSolve.h"

#include "ShaderCore.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "RHI.h"

#include "GlobalShader.h"
#include "ShaderCompilerCore.h"

class FYQProjectPBDDistanceConstraintCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQProjectPBDDistanceConstraintCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQProjectPBDDistanceConstraintCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, ParticleABuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, ParticleBBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, DistanceBuffer)
		SHADER_PARAMETER_SRV(Buffer<float4>, ParticlePositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionXBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionYBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionZBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, AccumulateCountBuffer)
		SHADER_PARAMETER(uint32, ConstraintOffset)
		SHADER_PARAMETER(uint32, NumConstraints)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(float, InvIterCount)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DISTANCE_CONSTRAINT"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQProjectPBDDistanceConstraintCS, TEXT("/YQLearnPhysics/PBDSimCS.usf"), TEXT("MainCS"), SF_Compute);

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
	, float InvIterCount
)
{
	SCOPED_DRAW_EVENT(RHICmdList, SolvePBDDistanceConstraint);

    TShaderMapRef<FYQProjectPBDDistanceConstraintCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumConstraints, FYQProjectPBDDistanceConstraintCS::ThreadGroupSize);

	FYQProjectPBDDistanceConstraintCS::FParameters PassParameters;
    PassParameters.ParticleABuffer = ParticleABuffer;
    PassParameters.ParticleBBuffer = ParticleBBuffer;
    PassParameters.DistanceBuffer = DistanceBuffer;
    PassParameters.ParticlePositionBuffer = ParticlePositionBuffer;
    PassParameters.AccumulateDeltaPositionXBuffer = AccumulateDeltaPositionXBuffer;
    PassParameters.AccumulateDeltaPositionYBuffer = AccumulateDeltaPositionYBuffer;
    PassParameters.AccumulateDeltaPositionZBuffer = AccumulateDeltaPositionZBuffer;
	PassParameters.AccumulateCountBuffer = AccumulateCountBuffer;
	PassParameters.ConstraintOffset = 0;
	PassParameters.NumConstraints = NumConstraints;
    PassParameters.DeltaTime = DeltaTime;
	PassParameters.InvIterCount = InvIterCount;

	//RHICmdList.Transition(FRHITransitionInfo(ParticleABuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	//RHICmdList.Transition(FRHITransitionInfo(ParticleABuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}


class FYQResolveDeltaPositionCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQResolveDeltaPositionCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQResolveDeltaPositionCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InParticlePositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutParticlePositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, VelocityBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionXBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionYBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionZBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, AccumulateCountBuffer)
		SHADER_PARAMETER(uint32, NumParticle)
		SHADER_PARAMETER(float, DeltaTime)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RESOLVE_DELTA_POSITION_BUFFER"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQResolveDeltaPositionCS, TEXT("/YQLearnPhysics/PBDSimCS.usf"), TEXT("MainCS"), SF_Compute);

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
)
{
	SCOPED_DRAW_EVENT(RHICmdList, ResolveDeltaPosition);

	TShaderMapRef<FYQResolveDeltaPositionCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticle, FYQResolveDeltaPositionCS::ThreadGroupSize);

	FYQResolveDeltaPositionCS::FParameters PassParameters;
    PassParameters.OutParticlePositionBuffer = OutParticlePositionBuffer;
	PassParameters.InParticlePositionBuffer = InParticlePositionBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
    PassParameters.AccumulateDeltaPositionXBuffer = AccumulateDeltaPositionXBuffer;
    PassParameters.AccumulateDeltaPositionYBuffer = AccumulateDeltaPositionYBuffer;
    PassParameters.AccumulateDeltaPositionZBuffer = AccumulateDeltaPositionZBuffer;
	PassParameters.AccumulateCountBuffer = AccumulateCountBuffer;
	PassParameters.NumParticle = NumParticle;
    PassParameters.DeltaTime = DeltaTime;

	//RHICmdList.Transition(FRHITransitionInfo(ParticleABuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}

class FYQInitializeDeltaBufferCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQInitializeDeltaBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQInitializeDeltaBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionXBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionYBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<int>, AccumulateDeltaPositionZBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, AccumulateCountBuffer)
		SHADER_PARAMETER(uint32, NumParticle)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("INITIALIZE_DELTA_BUFFER"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQInitializeDeltaBufferCS, TEXT("/YQLearnPhysics/PBDSimCS.usf"), TEXT("MainCS"), SF_Compute);

void InitializeDeltaBuffer_RenderThread(
    FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionXBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionYBuffer
	, FUnorderedAccessViewRHIRef AccumulateDeltaPositionZBuffer
	, FUnorderedAccessViewRHIRef AccumulateCountBuffer
	, uint32 NumParticle
)
{
	TShaderMapRef<FYQInitializeDeltaBufferCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticle, FYQInitializeDeltaBufferCS::ThreadGroupSize);

	FYQInitializeDeltaBufferCS::FParameters PassParameters;
    PassParameters.AccumulateDeltaPositionXBuffer = AccumulateDeltaPositionXBuffer;
    PassParameters.AccumulateDeltaPositionYBuffer = AccumulateDeltaPositionYBuffer;
    PassParameters.AccumulateDeltaPositionZBuffer = AccumulateDeltaPositionZBuffer;
	PassParameters.AccumulateCountBuffer = AccumulateCountBuffer;
	PassParameters.NumParticle = NumParticle;

	//RHICmdList.Transition(FRHITransitionInfo(ParticleABuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}