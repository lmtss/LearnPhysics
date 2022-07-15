#include "ExternalForceSolve.h"
#include "ShaderCompilerCore.h"	//CFLAG_AllowTypedUAVLoads

namespace ExternalForcePermutation
{
	class FUseSimpleDirectionalWindDim : SHADER_PERMUTATION_BOOL("USE_SIMPLE_DIRECTIONAL_WIND");
}

class FYQSolveExternalForceCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQSolveExternalForceCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQSolveExternalForceCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InputPositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<half4>, NormalBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, VelocityBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER(FVector4f, ExternalForceParams)
		SHADER_PARAMETER(FVector4f, WindParams)
		SHADER_PARAMETER(uint32, OffsetParticles)
		SHADER_PARAMETER(uint32, NumParticles)
		END_SHADER_PARAMETER_STRUCT()

		using FPermutationDomain = TShaderPermutationDomain<ExternalForcePermutation::FUseSimpleDirectionalWindDim>;

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SOLVE_EXTERNAL_FORCE"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQSolveExternalForceCS, TEXT("/YQLearnPhysics/YQPhysicsExternalForce.usf"), TEXT("MainCS"), SF_Compute);

void UpdateExternalForce(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef  InputPosition
	, FShaderResourceViewRHIRef NormalBuffer
	, FUnorderedAccessViewRHIRef OutputPosition
	, FShaderResourceViewRHIRef MaskBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, float GravityScale
	, uint32 OffsetParticles
	, uint32 NumParticles
	, float DeltaTime
)
{
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, UpdateExternalForce);

	FYQSolveExternalForceCS::FPermutationDomain ExternalForcePermutationVector;
	ExternalForcePermutationVector.Set<ExternalForcePermutation::FUseSimpleDirectionalWindDim>(false);

	TShaderMapRef<FYQSolveExternalForceCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), ExternalForcePermutationVector);
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticles, FYQSolveExternalForceCS::ThreadGroupSize);

	FYQSolveExternalForceCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.OutputPositionBuffer = OutputPosition;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.ExternalForceParams = FVector4f(9.8 * 100 * GravityScale, DeltaTime, 0.0, 0.0);
	PassParameters.WindParams = FVector4f(0.0, 1.0, 0.0, 0.0);
	PassParameters.NumParticles = NumParticles;
	PassParameters.OffsetParticles = OffsetParticles;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}

void UpdateExternalForce(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef  InputPosition
	, FShaderResourceViewRHIRef NormalBuffer
	, FUnorderedAccessViewRHIRef OutputPosition
	, FShaderResourceViewRHIRef MaskBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, FVector3f DirectionalWindDirection
	, float DirectionalWindSpeed
	, float GravityScale
	, uint32 OffsetParticles
	, uint32 NumParticles
	, float DeltaTime
)
{
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, UpdateExternalForceWithDirectionalWind);

	FYQSolveExternalForceCS::FPermutationDomain ExternalForcePermutationVector;
	ExternalForcePermutationVector.Set<ExternalForcePermutation::FUseSimpleDirectionalWindDim>(true);

	TShaderMapRef<FYQSolveExternalForceCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), ExternalForcePermutationVector);
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticles, FYQSolveExternalForceCS::ThreadGroupSize);

	FYQSolveExternalForceCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.OutputPositionBuffer = OutputPosition;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.NormalBuffer = NormalBuffer;
	PassParameters.ExternalForceParams = FVector4f(9.8 * 100 * GravityScale, DeltaTime, 0.0, 0.0);
	PassParameters.WindParams = FVector4f(DirectionalWindDirection, DirectionalWindSpeed);
	PassParameters.NumParticles = NumParticles;
	PassParameters.OffsetParticles = OffsetParticles;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}


class FYQUpdateFixedParticlesTransformCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQUpdateFixedParticlesTransformCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQUpdateFixedParticlesTransformCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, PositionBuffer)
		SHADER_PARAMETER(FMatrix44f, PreviousWorldToLocal)
		SHADER_PARAMETER(FMatrix44f, NewLocalToWorld)
		SHADER_PARAMETER(uint32, NumParticles)
		SHADER_PARAMETER(uint32, BaseParticle)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("UPDATE_FIXED_PARTICLES_TRANSFORM"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQUpdateFixedParticlesTransformCS, TEXT("/YQLearnPhysics/YQPhysicsExternalForce.usf"), TEXT("MainCS"), SF_Compute);


void UpdateFixedParticlesTransform(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef PositionBuffer
	, FShaderResourceViewRHIRef MaskBuffer
	, FMatrix44f PreviousWorldToLocal
	, FMatrix44f NewLocalToWorld
	, uint32 NumParticles
	, uint32 BaseParticle
)
{
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, UpdateFixedParticlesTransform);

	TShaderMapRef<FYQUpdateFixedParticlesTransformCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticles, FYQUpdateFixedParticlesTransformCS::ThreadGroupSize);

	FYQUpdateFixedParticlesTransformCS::FParameters PassParameters;
	PassParameters.PositionBuffer = PositionBuffer;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.BaseParticle = BaseParticle;
	PassParameters.NumParticles = NumParticles;
	PassParameters.PreviousWorldToLocal = PreviousWorldToLocal;
	PassParameters.NewLocalToWorld = NewLocalToWorld;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}