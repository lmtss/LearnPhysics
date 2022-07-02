#include "CollisionResponse.h"

#include "ShaderCompilerCore.h"	//CFLAG_AllowTypedUAVLoads



class FYQNaiveCollisionWithColliderCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQNaiveCollisionWithColliderCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQNaiveCollisionWithColliderCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, ParticlePositionBufferCopy)
		SHADER_PARAMETER_SRV(Buffer<float4>, ParticlePositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<half4>, NormalBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, VelocityBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, FeedbackBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, FeedbackSizesBuffer)
		SHADER_PARAMETER(FVector4f, SphereCollisionParams)
		SHADER_PARAMETER(uint32, NumParticles)
		SHADER_PARAMETER(float, DeltaTime)
		SHADER_PARAMETER(FVector4f, SphereVelocityAndMass)
		SHADER_PARAMETER(FVector4f, IterParams)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("NAIVE_COLLISION_WITH_COLLIDER"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQNaiveCollisionWithColliderCS, TEXT("/YQLearnPhysics/CollisionResponse.usf"), TEXT("MainCS"), SF_Compute);


void NaiveCollisionWithCollider(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBufferCopy
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FShaderResourceViewRHIRef NormalBuffer
	, FShaderResourceViewRHIRef MaskBuffer
	, FUnorderedAccessViewRHIRef OutputPositionBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, FUnorderedAccessViewRHIRef FeedbackBuffer
	, FUnorderedAccessViewRHIRef FeedbackSizesBuffer
	, FVector3f ColliderCenterPosition
	, float SphereColliderRadius
	, FVector3f ColliderVelocity
	, float SphereColliderMass
	, uint32 NumParticles
	, float DeltaTime
	, uint32 IndexIter
	, uint32 NumIters
)
{
	SCOPED_DRAW_EVENT(RHICmdList, NaiveCollisionWithCollider);

	TShaderMapRef<FYQNaiveCollisionWithColliderCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticles, FYQNaiveCollisionWithColliderCS::ThreadGroupSize);

	FYQNaiveCollisionWithColliderCS::FParameters PassParameters;
	PassParameters.ParticlePositionBuffer = ParticlePositionBuffer;
	PassParameters.NormalBuffer = NormalBuffer;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.OutputPositionBuffer = OutputPositionBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.FeedbackBuffer = FeedbackBuffer;
	PassParameters.FeedbackSizesBuffer = FeedbackSizesBuffer;
	PassParameters.SphereCollisionParams = FVector4f(ColliderCenterPosition, SphereColliderRadius);
	PassParameters.NumParticles = NumParticles;
	PassParameters.DeltaTime = DeltaTime;
	PassParameters.SphereVelocityAndMass = FVector4f(ColliderVelocity, SphereColliderMass);
	PassParameters.ParticlePositionBufferCopy = ParticlePositionBufferCopy;
	PassParameters.IterParams = FVector4f(IndexIter, NumIters, 0, 0);


	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}






class FYQCopyPositionBufferCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQCopyPositionBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQCopyPositionBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, PositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER(uint32, NumParticles)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("COPY_POSITION_BUFFER"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQCopyPositionBufferCS, TEXT("/YQLearnPhysics/CollisionResponse.usf"), TEXT("MainCS"), SF_Compute);




void CopyPositionForCollision(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FUnorderedAccessViewRHIRef OutputPositionBuffer
	, uint32 NumParticles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, CopyPositionForCollision);

	TShaderMapRef<FYQCopyPositionBufferCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticles, FYQCopyPositionBufferCS::ThreadGroupSize);

	FYQCopyPositionBufferCS::FParameters PassParameters;
	PassParameters.PositionBuffer = ParticlePositionBuffer;
	PassParameters.OutputPositionBuffer = OutputPositionBuffer;
	PassParameters.NumParticles = NumParticles;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}
