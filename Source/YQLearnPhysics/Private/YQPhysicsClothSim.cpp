#include "YQPhysicsClothSim.h"

#include "ShaderCore.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "RHI.h"


#include "ShaderCompilerCore.h"	//CFLAG_AllowTypedUAVLoads
#include "GlobalShader.h"
#include "HAL/PlatformApplicationMisc.h"
#include <limits>

class FYQInitializeClothIndexBufferCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQInitializeClothIndexBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQInitializeClothIndexBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, IndexBuffer)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("INITIALIZE_CLOTH_INDEX_BUFFER"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQInitializeClothIndexBufferCS, TEXT("/YQLearnPhysics/YQPhysicsInitializeShader.usf"), TEXT("MainCS"), SF_Compute);

class FYQInitializeVertexSampleHorzCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQInitializeVertexSampleHorzCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQInitializeVertexSampleHorzCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<float4>, PositionBufferA)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, PositionBufferB)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, MaskBuffer)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("INITIALIZE_POSITION_SAMPLE_HORIZ"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQInitializeVertexSampleHorzCS, TEXT("/YQLearnPhysics/YQPhysicsInitializeShader.usf"), TEXT("MainCS"), SF_Compute);


void InitializeClothVertex(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef& BufferA
	, FUnorderedAccessViewRHIRef& BufferB
	, FUnorderedAccessViewRHIRef& MaskBuffer
	, uint32 LengthToClear
)
{
	check(IsInRenderingThread());


	TShaderMapRef<FYQInitializeVertexSampleHorzCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(LengthToClear, FYQInitializeVertexSampleHorzCS::ThreadGroupSize);

	FYQInitializeVertexSampleHorzCS::FParameters PassParameters;
	PassParameters.PositionBufferA = BufferA;
	PassParameters.PositionBufferB = BufferB;
	PassParameters.MaskBuffer = MaskBuffer;

	RHICmdList.Transition(FRHITransitionInfo(BufferA, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(BufferA, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));


}

void InitializeClothIndexBuffer(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef& IndexBuffer
	, uint32 LengthToClear
)
{
	check(IsInRenderingThread());


	TShaderMapRef<FYQInitializeClothIndexBufferCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(LengthToClear, FYQInitializeClothIndexBufferCS::ThreadGroupSize);

	FYQInitializeClothIndexBufferCS::FParameters PassParameters;
	PassParameters.IndexBuffer = IndexBuffer;

	RHICmdList.Transition(FRHITransitionInfo(IndexBuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(IndexBuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));


}



class FYQUpdateExternalForceCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQUpdateExternalForceCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQUpdateExternalForceCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InputPositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<float4>, NormalBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, VelocityBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER(FVector4f, ExternalForceParams)
		SHADER_PARAMETER(FVector4f, WindParams)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("UPDATE_EXTERNAL_FORCE"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQUpdateExternalForceCS, TEXT("/YQLearnPhysics/YQPhysicsInitializeShader.usf"), TEXT("MainCS"), SF_Compute);

void UpdateSimpleExternalForce(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InputPosition
	, FShaderResourceViewRHIRef NormalBuffer
	, FUnorderedAccessViewRHIRef OutputPosition
	, FShaderResourceViewRHIRef MaskBuffer
	, FUnorderedAccessViewRHIRef VelocityBuffer
	, uint32 Length
	, float DeltaTime
)
{
	check(IsInRenderingThread());


	SCOPED_DRAW_EVENT(RHICmdList, UpdateSimpleExternalForce);

	TShaderMapRef<FYQUpdateExternalForceCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FYQUpdateExternalForceCS::ThreadGroupSize);

	FYQUpdateExternalForceCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.NormalBuffer = NormalBuffer;
	PassParameters.OutputPositionBuffer = OutputPosition;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.ExternalForceParams = FVector4f(9.8, DeltaTime, 0.0, 0.0);
	PassParameters.WindParams = FVector4f(0.0, 1.0, 0.0, 0.0);

	RHICmdList.Transition(FRHITransitionInfo(OutputPosition, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(OutputPosition, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}

class FYQJacobiMassSpringProjectionCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQJacobiMassSpringProjectionCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQJacobiMassSpringProjectionCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InputPositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, VelocityBuffer)
		SHADER_PARAMETER(FVector4f, SpringParams)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("JACOBI_MASS_SPRING"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQJacobiMassSpringProjectionCS, TEXT("/YQLearnPhysics/YQPhysicsInitializeShader.usf"), TEXT("MainCS"), SF_Compute);

void JacobiMassSpringProjection(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FShaderResourceViewRHIRef& MaskBuffer
	, FUnorderedAccessViewRHIRef& PositionBuffer
	, FUnorderedAccessViewRHIRef& VelocityBuffer
	, float DeltaTime
) 	
{
	check(IsInRenderingThread());


	SCOPED_DRAW_EVENT(RHICmdList, JacobiMassSpringProjection);

	TShaderMapRef<FYQJacobiMassSpringProjectionCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	uint32 Length = 1024;
	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FYQJacobiMassSpringProjectionCS::ThreadGroupSize);

	FYQJacobiMassSpringProjectionCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.OutputPositionBuffer = PositionBuffer;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.SpringParams = FVector4f(10.0, 1.0 / DeltaTime, 0.0, 0.0);

	RHICmdList.Transition(FRHITransitionInfo(PositionBuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(PositionBuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}



class FYQCalcNormalCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQCalcNormalCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQCalcNormalCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, OutputNormalBuffer)
		SHADER_PARAMETER(FVector4f, NormalParams)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("CALC_NORMAL_CROSS_POSITION"), 1);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQCalcNormalCS, TEXT("/YQLearnPhysics/YQPhysicsInitializeShader.usf"), TEXT("MainCS"), SF_Compute);

void CalcNormalByCrossPosition(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FUnorderedAccessViewRHIRef& NormalBuffer
	, float Intensity
) {
	check(IsInRenderingThread());

	SCOPED_DRAW_EVENT(RHICmdList, CalcNormalByCrossPosition);

	TShaderMapRef<FYQCalcNormalCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	uint32 Length = 1024;
	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FYQCalcNormalCS::ThreadGroupSize);

	FYQCalcNormalCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.OutputNormalBuffer = NormalBuffer;
	PassParameters.NormalParams = FVector4f(Intensity, 0.0, 0.0, 0.0);

	RHICmdList.Transition(FRHITransitionInfo(NormalBuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(NormalBuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}


class FYQJacobiSimpleCollisionCS: public FGlobalShader{
public:
	DECLARE_SHADER_TYPE(FYQJacobiSimpleCollisionCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQJacobiSimpleCollisionCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InputPositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, VelocityBuffer)
		SHADER_PARAMETER(FVector4f, SpringParams)
		SHADER_PARAMETER(FVector4f, SphereCollisionParams)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("JACOBI_SIMPLE_COLLISION"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQJacobiSimpleCollisionCS, TEXT("/YQLearnPhysics/YQPhysicsInitializeShader.usf"), TEXT("MainCS"), SF_Compute);

void JacobiSimpleCollisionProjection(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FShaderResourceViewRHIRef& MaskBuffer
	, FUnorderedAccessViewRHIRef& PositionBuffer
	, FUnorderedAccessViewRHIRef& VelocityBuffer
	, FVector4f SphereCenterAndRadius
	, float DeltaTime
) {
	check(IsInRenderingThread());


	SCOPED_DRAW_EVENT(RHICmdList, JacobiSimpleCollisionProjection);

	TShaderMapRef<FYQJacobiSimpleCollisionCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	uint32 Length = 1024;
	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FYQJacobiSimpleCollisionCS::ThreadGroupSize);

	FYQJacobiSimpleCollisionCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.OutputPositionBuffer = PositionBuffer;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.SpringParams = FVector4f(10.0, 1.0 / DeltaTime, 0.0, 0.0);
	PassParameters.SphereCollisionParams = SphereCenterAndRadius;

	RHICmdList.Transition(FRHITransitionInfo(PositionBuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(PositionBuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}


class FYQJacobiSimpleCollisionFeedbackCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQJacobiSimpleCollisionFeedbackCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQJacobiSimpleCollisionFeedbackCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, InputPositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MaskBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, OutputPositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, VelocityBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, FeedbackBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, FeedbackBufferSizes)
		SHADER_PARAMETER(FVector4f, SpringParams)
		SHADER_PARAMETER(FVector4f, SphereCollisionParams)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("JACOBI_SIMPLE_COLLISION_FEEDBACK"), 1);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQJacobiSimpleCollisionFeedbackCS, TEXT("/YQLearnPhysics/YQPhysicsInitializeShader.usf"), TEXT("MainCS"), SF_Compute);

void JacobiSimpleCollisionProjectionFeedback(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef& InputPosition
	, FShaderResourceViewRHIRef& MaskBuffer
	, FUnorderedAccessViewRHIRef& PositionBuffer
	, FUnorderedAccessViewRHIRef& VelocityBuffer
	, FUnorderedAccessViewRHIRef& FeedbackBuffer
	, FUnorderedAccessViewRHIRef& FeedbackBufferSizes
	, FVector4f SphereCenterAndRadius
	, float DeltaTime
) {
	check(IsInRenderingThread());


	SCOPED_DRAW_EVENT(RHICmdList, FYQJacobiSimpleCollisionFeedbackCS);

	TShaderMapRef<FYQJacobiSimpleCollisionFeedbackCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	uint32 Length = 1024;
	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FYQJacobiSimpleCollisionFeedbackCS::ThreadGroupSize);

	FYQJacobiSimpleCollisionFeedbackCS::FParameters PassParameters;
	PassParameters.InputPositionBuffer = InputPosition;
	PassParameters.OutputPositionBuffer = PositionBuffer;
	PassParameters.MaskBuffer = MaskBuffer;
	PassParameters.VelocityBuffer = VelocityBuffer;
	PassParameters.SpringParams = FVector4f(10.0, 1.0 / DeltaTime, 0.0, 0.0);
	PassParameters.SphereCollisionParams = SphereCenterAndRadius;
	PassParameters.FeedbackBuffer = FeedbackBuffer;
	PassParameters.FeedbackBufferSizes = FeedbackBufferSizes;

	RHICmdList.Transition(FRHITransitionInfo(PositionBuffer, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(PositionBuffer, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}