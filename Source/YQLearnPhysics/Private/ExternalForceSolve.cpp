#include "ExternalForceSolve.h"
#include "ShaderCompilerCore.h"	//CFLAG_AllowTypedUAVLoads

namespace ExternalForcePermutation
{
	// Shared permutation dimensions between deferred and mobile renderer.
	class FFixedByColorDim : SHADER_PERMUTATION_BOOL("USE_FIXED_BY_COLOR");

}

class FYQSolveExternalForceCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQSolveExternalForceCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQSolveExternalForceCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, VelocityBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER(FVector4f, ExternalForceParams)
		SHADER_PARAMETER(FVector4f, WindParams)
		SHADER_PARAMETER(uint32, NumParticles)
		END_SHADER_PARAMETER_STRUCT()

		using FPermutationDomain = TShaderPermutationDomain<ExternalForcePermutation::FFixedByColorDim>;

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
	, uint32 NumParticles
	, float DeltaTime
)
{
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, UpdateExternalForce);

	FYQSolveExternalForceCS::FPermutationDomain ExternalForcePermutationVector;
	ExternalForcePermutationVector.Set<ExternalForcePermutation::FFixedByColorDim>(false);

	TShaderMapRef<FYQSolveExternalForceCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), ExternalForcePermutationVector);
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticles, FYQSolveExternalForceCS::ThreadGroupSize);

	FYQSolveExternalForceCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.OutputPositionBuffer = OutputPosition;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.ExternalForceParams = FVector4f(9.8 * 100, DeltaTime, 0.0, 0.0);
	PassParameters.WindParams = FVector4f(0.0, 1.0, 0.0, 0.0);
	PassParameters.NumParticles = NumParticles;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}