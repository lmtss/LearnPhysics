#include "ConstraintsGenerate.h"

#include "ShaderCore.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "RHI.h"
#include "RenderGraphBuilder.h"
#include "GlobalShader.h"
#include "GPUSort.h"

#include "ShaderCompilerCore.h"	//CFLAG_AllowTypedUAVLoads

#include "YQStreamCompact.h"

class FYQGenerateDistanceConstraintsCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQGenerateDistanceConstraintsCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQGenerateDistanceConstraintsCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, UIntParamBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, FloatParamBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, IDBufferA)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, IDBufferB)
		SHADER_PARAMETER_UAV(RWBuffer<float>, DistanceBuffer)
		SHADER_PARAMETER(uint32, ConstraintOffset)
		SHADER_PARAMETER(uint32, NumConstraints)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("DISTANCE_CONSTRAINT"), 1);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQGenerateDistanceConstraintsCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);



void GenerateDistanceConstraints_RenderThread(
    FRHICommandList& RHICmdList
    , FUnorderedAccessViewRHIRef ParticleA
    , FUnorderedAccessViewRHIRef ParticleB
    , FUnorderedAccessViewRHIRef Distance
    , FShaderResourceViewRHIRef UIntParam
    , FShaderResourceViewRHIRef FloatParam
	, uint32 ConstraintsOffset
    , uint32 NumConstraints
)
{
    check(IsInRenderingThread());


	TShaderMapRef<FYQGenerateDistanceConstraintsCS>ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumConstraints, FYQGenerateDistanceConstraintsCS::ThreadGroupSize);

	FYQGenerateDistanceConstraintsCS::FParameters PassParameters;
    PassParameters.IDBufferA = ParticleA;
    PassParameters.IDBufferB = ParticleB;
    PassParameters.DistanceBuffer = Distance;
    PassParameters.UIntParamBuffer = UIntParam;
    PassParameters.FloatParamBuffer = FloatParam;
	PassParameters.ConstraintOffset = ConstraintsOffset;
	PassParameters.NumConstraints = NumConstraints;

	RHICmdList.Transition(FRHITransitionInfo(ParticleA, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(ParticleA, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}


void GenerateDistanceConstraints(
	FRHICommandList& RHICmdList
    , FUnorderedAccessViewRHIRef ParticleA
    , FUnorderedAccessViewRHIRef ParticleB
    , FUnorderedAccessViewRHIRef Distance
    , FShaderResourceViewRHIRef UIntParam
    , FShaderResourceViewRHIRef FloatParam
	, uint32 ConstraintsOffset
    , uint32 NumConstraints
)
{
	GenerateDistanceConstraints_RenderThread(RHICmdList, ParticleA, ParticleB, Distance, UIntParam, FloatParam, ConstraintsOffset, NumConstraints);
}


class FYQgGenerateSimpleCollisionConstraintsCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQgGenerateSimpleCollisionConstraintsCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQgGenerateSimpleCollisionConstraintsCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, ConstraintsIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, ParticlePositionBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half4>, ParticleVelocityBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ParticleABuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ParticleBBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float>, DistanceBuffer)
		SHADER_PARAMETER(uint32, NumConstraints)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("SIMPLE_COLLISION_CONSTRAINT"), 1);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

//IMPLEMENT_SHADER_TYPE(, FYQgGenerateSimpleCollisionConstraintsCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);

// 通过Mesh的IndexBuffer获得边的信息
class FYQGenerateDistanceConstraintsKeyCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQGenerateDistanceConstraintsKeyCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQGenerateDistanceConstraintsKeyCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, IndexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintKeyBuffer)
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
		OutEnvironment.SetDefine(TEXT("GENERATE_DISTANTCE_CONSTRAINTS_KEY"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQGenerateDistanceConstraintsKeyCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);


// 去掉重复的key
class FYQCompactSortedKeyCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQCompactSortedKeyCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQCompactSortedKeyCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, SortedKeyBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, UniqueKeyBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, GlobalOffsetBuffer)
		SHADER_PARAMETER(uint32, GlobalOffsetIndex)
		SHADER_PARAMETER(uint32, NumKeys)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("COMPACT_SORTED_KEY"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQCompactSortedKeyCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);


class FYQGenerateDistanceConstraintsByCompactKeyCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQGenerateDistanceConstraintsByCompactKeyCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQGenerateDistanceConstraintsByCompactKeyCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, CompactKeyBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, PositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, ColorBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float>, DistanceBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, IDBufferA)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, IDBufferB)
		SHADER_PARAMETER(uint32, NumConstraints)
		SHADER_PARAMETER(uint32, OffsetConstraints)
		SHADER_PARAMETER(uint32, OffsetParticles)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("GENERATE_DISTANCE_CONSTRAINT_BY_COMPACT_KEY"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQGenerateDistanceConstraintsByCompactKeyCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);

void GenerateDistanceConstraintsKeyFromMesh(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef IndexBuffer
	, FUnorderedAccessViewRHIRef ConstraintKeyBuffer
	, uint32 NumTriangles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, GenerateDistanceConstraintsKey);

	TShaderMapRef<FYQGenerateDistanceConstraintsKeyCS> GenerateKeyCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateKeyCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumTriangles, FYQGenerateDistanceConstraintsKeyCS::ThreadGroupSize);

	FYQGenerateDistanceConstraintsKeyCS::FParameters PassParameters;
	PassParameters.NumTriangles = NumTriangles;
	PassParameters.IndexBuffer = IndexBuffer;
	PassParameters.ConstraintKeyBuffer = ConstraintKeyBuffer;


	SetShaderParameters(RHICmdList, GenerateKeyCS, GenerateKeyCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateKeyCS, GenerateKeyCS.GetComputeShader());
}

void CompactSortedKey(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef SortedKeyBuffer
	, FUnorderedAccessViewRHIRef UniqueKeyBuffer
	, FUnorderedAccessViewRHIRef GlobalOffsetBuffer
	, uint32 GlobalOffsetIndex
	, uint32 NumKeys
)
{
	SCOPED_DRAW_EVENT(RHICmdList, CompactSortedKey);

	TShaderMapRef<FYQCompactSortedKeyCS> CompactKeyCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(CompactKeyCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumKeys, FYQCompactSortedKeyCS::ThreadGroupSize * 4);

	FYQCompactSortedKeyCS::FParameters PassParameters;
	PassParameters.NumKeys = NumKeys;
	PassParameters.GlobalOffsetIndex = GlobalOffsetIndex;
	PassParameters.SortedKeyBuffer = SortedKeyBuffer;
	PassParameters.UniqueKeyBuffer = UniqueKeyBuffer;
	PassParameters.GlobalOffsetBuffer = GlobalOffsetBuffer;
	

	SetShaderParameters(RHICmdList, CompactKeyCS, CompactKeyCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, CompactKeyCS, CompactKeyCS.GetComputeShader());
}


void GenerateDistanceConstraintsByCompactKey(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef CompactKeyBuffer
	, FShaderResourceViewRHIRef PositionBuffer
	, FUnorderedAccessViewRHIRef DistanceBuffer
	, FUnorderedAccessViewRHIRef IDBufferA
	, FUnorderedAccessViewRHIRef IDBufferB
	, FRHIShaderResourceView* ColorBuffer
	, uint32 NumConstraints
	, uint32 OffsetConstraints
	, uint32 OffsetParticles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, CompactSortedKey);

	TShaderMapRef<FYQGenerateDistanceConstraintsByCompactKeyCS> CS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(CS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumConstraints, FYQGenerateDistanceConstraintsByCompactKeyCS::ThreadGroupSize);

	FYQGenerateDistanceConstraintsByCompactKeyCS::FParameters PassParameters;
	PassParameters.NumConstraints = NumConstraints;
	PassParameters.OffsetConstraints = OffsetConstraints;
	PassParameters.IDBufferA = IDBufferA;
	PassParameters.IDBufferB = IDBufferB;
	PassParameters.DistanceBuffer = DistanceBuffer;
	PassParameters.PositionBuffer = PositionBuffer;
	PassParameters.CompactKeyBuffer = CompactKeyBuffer;
	PassParameters.ColorBuffer = ColorBuffer;
	PassParameters.OffsetParticles = OffsetParticles;


	SetShaderParameters(RHICmdList, CS, CS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, CS, CS.GetComputeShader());
}

FRWBuffer ConstraintKeyBuffers[2];
FRWBuffer GlobalOffsetBuffer;

FRWBuffer ParticleBufferAForTest;
FRWBuffer ParticleBufferBForTest;
FRWBuffer DistanceBufferForTest;


int GenerateDistanceConstraintsFromMesh(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef ParticleA
	, FUnorderedAccessViewRHIRef ParticleB
	, FUnorderedAccessViewRHIRef DistanceBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef PositionBuffer
	, FRHIShaderResourceView* ColorBuffer
	, uint32 ConstraintsOffset
	, uint32 NumTriangles
	, uint32 OffsetParticles
	, bool bTest
)
{
	
	uint32 MaxKeyBufferLength = 1 << 20;
	if (ConstraintKeyBuffers[0].SRV == nullptr)
	{
		ConstraintKeyBuffers[0].Initialize(TEXT("ConstraintKeyBuffer-0"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		ConstraintKeyBuffers[1].Initialize(TEXT("ConstraintKeyBuffer-1"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		GlobalOffsetBuffer.Initialize(TEXT("GlobalOffsetBuffer"), sizeof(uint32), 1, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	}

	YQInitializeSizeBuffer(RHICmdList, GlobalOffsetBuffer.UAV, 1, 0);
	
	GenerateDistanceConstraintsKeyFromMesh(RHICmdList, IndexBuffer, ConstraintKeyBuffers[0].UAV, NumTriangles);

	FGPUSortBuffers GPUSortBuffers;
	GPUSortBuffers.RemoteKeySRVs[0] = ConstraintKeyBuffers[0].SRV;
	GPUSortBuffers.RemoteKeySRVs[1] = ConstraintKeyBuffers[1].SRV;
	GPUSortBuffers.RemoteKeyUAVs[0] = ConstraintKeyBuffers[0].UAV;
	GPUSortBuffers.RemoteKeyUAVs[1] = ConstraintKeyBuffers[1].UAV;
	GPUSortBuffers.RemoteValueSRVs[0] = nullptr;
	GPUSortBuffers.RemoteValueSRVs[1] = nullptr;
	GPUSortBuffers.RemoteValueUAVs[0] = nullptr;
	GPUSortBuffers.RemoteValueUAVs[1] = nullptr;

	int32 BufferIndex = SortGPUBuffers(
		RHICmdList
		, GPUSortBuffers
		, 0
		, 0xFFFFFFFF
		, NumTriangles * 3
		, ERHIFeatureLevel::SM5
	);

	YQInitializeSizeBuffer(RHICmdList, ConstraintKeyBuffers[1 - BufferIndex].UAV, NumTriangles * 4, 0);

	CompactSortedKey(
		RHICmdList
		, ConstraintKeyBuffers[BufferIndex].SRV
		, ConstraintKeyBuffers[1 - BufferIndex].UAV
		, GlobalOffsetBuffer.UAV
		, 0
		, NumTriangles * 3
	);

	void* ReadPtr = RHILockBuffer(GlobalOffsetBuffer.Buffer, 0, sizeof(uint32), EResourceLockMode::RLM_ReadOnly);

	uint32* ReadPtr32 = (uint32*)ReadPtr;
	uint32 NumUniqueConstraints = ReadPtr32[0];

	RHIUnlockBuffer(GlobalOffsetBuffer.Buffer);

	// ----- Log
	if (bTest)
	{
		YQInitializeSizeBuffer(RHICmdList, ConstraintKeyBuffers[BufferIndex].UAV, MaxKeyBufferLength, 0);

		GPUSortBuffers.RemoteKeySRVs[0] = ConstraintKeyBuffers[1 - BufferIndex].SRV;
		GPUSortBuffers.RemoteKeySRVs[1] = ConstraintKeyBuffers[BufferIndex].SRV;
		GPUSortBuffers.RemoteKeyUAVs[0] = ConstraintKeyBuffers[1 - BufferIndex].UAV;
		GPUSortBuffers.RemoteKeyUAVs[1] = ConstraintKeyBuffers[BufferIndex].UAV;

		BufferIndex = SortGPUBuffers(
			RHICmdList
			, GPUSortBuffers
			, 0
			, 0xFFFFFFFF
			, NumUniqueConstraints
			, ERHIFeatureLevel::SM5
		);
	}
	else
	{
		BufferIndex = 1 - BufferIndex;
	}

	if (ParticleA == nullptr && bTest)
	{
		if (ParticleBufferAForTest.SRV == nullptr)
		{
			ParticleBufferAForTest.Initialize(TEXT("ParticleBufferAForTest"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);;
			ParticleBufferBForTest.Initialize(TEXT("ParticleBufferBForTest"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);;
			DistanceBufferForTest.Initialize(TEXT("DistanceBufferForTest"), sizeof(float), MaxKeyBufferLength, EPixelFormat::PF_R32_FLOAT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);;
		}
		
		ParticleA = ParticleBufferAForTest.UAV;
		ParticleB = ParticleBufferBForTest.UAV;
		DistanceBuffer = DistanceBufferForTest.UAV;
	}

	if (ParticleA != nullptr && PositionBuffer != nullptr)
	{
		GenerateDistanceConstraintsByCompactKey(
			RHICmdList
			, ConstraintKeyBuffers[BufferIndex].SRV
			, PositionBuffer
			, DistanceBuffer
			, ParticleA
			, ParticleB
			, ColorBuffer
			, NumUniqueConstraints
			, ConstraintsOffset
			, OffsetParticles
		);
	}

	return NumUniqueConstraints;
}




// 根据IndexBuffer输出所有边的key，以及对应的三角形的id
class FYQGenerateBendingConstraintsKeyCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQGenerateBendingConstraintsKeyCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQGenerateBendingConstraintsKeyCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, IndexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintKeyBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, TriangleIndexBuffer)
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
		OutEnvironment.SetDefine(TEXT("GENERATE_BENDING_CONSTRAINTS_KEY"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};


IMPLEMENT_SHADER_TYPE(, FYQGenerateBendingConstraintsKeyCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);


// 选出重复的边，并输出对应的两个三角形的ID
class FYQCalcBendingConstraintsTriangleIndexCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQCalcBendingConstraintsTriangleIndexCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQCalcBendingConstraintsTriangleIndexCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, SortedEdgeKeyBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, InputTriangleIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputTriangleIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, GlobalOffsetBuffer)
		SHADER_PARAMETER(uint32, GlobalOffsetIndex)
		SHADER_PARAMETER(uint32, NumKeys)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("CALC_BENDING_CONSTRAINTS_TRIANGLE_INDEX"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQCalcBendingConstraintsTriangleIndexCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);


// 根据两个有共享边的三角形，产生距离约束版本的弯曲约束
// 参考UE的chaos
class FYQGenerateDistanceBendingConstraintsCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQGenerateDistanceBendingConstraintsCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQGenerateDistanceBendingConstraintsCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, IndexBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, CombinedTriangleIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintsParticleABuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintsParticleBBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float>, DistanceBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, PositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, ColorBuffer)
		SHADER_PARAMETER(uint32, NumConstraints)
		SHADER_PARAMETER(uint32, OffsetConstraints)
		SHADER_PARAMETER(uint32, OffsetParticles)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("GENERATE_DISTANCE_BENDING_CONSTRAINTS"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQGenerateDistanceBendingConstraintsCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);



void GenerateBendingConstraintsKeyAndTriangleIndexFromMesh(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef IndexBuffer
	, FUnorderedAccessViewRHIRef ConstraintKeyBuffer
	, FUnorderedAccessViewRHIRef TriangleIndexBuffer
	, uint32 NumTriangles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, GenerateBendingConstraintsKeyAndTriangleIndexFromMesh);

	TShaderMapRef<FYQGenerateBendingConstraintsKeyCS> GenerateKeyCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateKeyCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumTriangles, FYQGenerateBendingConstraintsKeyCS::ThreadGroupSize);

	FYQGenerateBendingConstraintsKeyCS::FParameters PassParameters;
	PassParameters.NumTriangles = NumTriangles;
	PassParameters.IndexBuffer = IndexBuffer;
	PassParameters.ConstraintKeyBuffer = ConstraintKeyBuffer;
	PassParameters.TriangleIndexBuffer = TriangleIndexBuffer;

	SetShaderParameters(RHICmdList, GenerateKeyCS, GenerateKeyCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateKeyCS, GenerateKeyCS.GetComputeShader());
}



void CalcBendingConstraintsTriangleIndex(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef SortedEdgeKeyBuffer
	, FShaderResourceViewRHIRef InputTriangleIDBuffer
	, FUnorderedAccessViewRHIRef OutputTriangleIDBuffer
	, FUnorderedAccessViewRHIRef InGlobalOffsetBuffer
	, uint32 GlobalOffsetIndex
	, uint32 NumKeys
)
{
	SCOPED_DRAW_EVENT(RHICmdList, CalcBendingConstraintsTriangleIndex);

	TShaderMapRef<FYQCalcBendingConstraintsTriangleIndexCS> CompactKeyCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(CompactKeyCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumKeys, FYQCalcBendingConstraintsTriangleIndexCS::ThreadGroupSize * 4);

	FYQCalcBendingConstraintsTriangleIndexCS::FParameters PassParameters;
	PassParameters.NumKeys = NumKeys;
	PassParameters.GlobalOffsetIndex = GlobalOffsetIndex;
	PassParameters.SortedEdgeKeyBuffer = SortedEdgeKeyBuffer;
	PassParameters.OutputTriangleIDBuffer = OutputTriangleIDBuffer;
	PassParameters.GlobalOffsetBuffer = InGlobalOffsetBuffer;
	PassParameters.InputTriangleIDBuffer = InputTriangleIDBuffer;


	SetShaderParameters(RHICmdList, CompactKeyCS, CompactKeyCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, CompactKeyCS, CompactKeyCS.GetComputeShader());
}

void GenerateDistanceBendingConstraints(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef CombinedTriangleIDBuffer
	, FShaderResourceViewRHIRef PositionBuffer
	, FUnorderedAccessViewRHIRef DistanceBuffer
	, FUnorderedAccessViewRHIRef IDBufferA
	, FUnorderedAccessViewRHIRef IDBufferB
	, FRHIShaderResourceView* ColorBuffer
	, uint32 NumConstraints
	, uint32 OffsetConstraints
	, uint32 OffsetParticles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, GenerateDistanceBendingConstraints);

	TShaderMapRef<FYQGenerateDistanceBendingConstraintsCS> CS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(CS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumConstraints, FYQGenerateDistanceBendingConstraintsCS::ThreadGroupSize);

	FYQGenerateDistanceBendingConstraintsCS::FParameters PassParameters;
	PassParameters.ConstraintsParticleABuffer = IDBufferA;
	PassParameters.ConstraintsParticleBBuffer = IDBufferB;
	PassParameters.DistanceBuffer = DistanceBuffer;
	PassParameters.PositionBuffer = PositionBuffer;
	PassParameters.IndexBuffer = IndexBuffer;
	PassParameters.ColorBuffer = ColorBuffer;
	PassParameters.CombinedTriangleIDBuffer = CombinedTriangleIDBuffer;
	PassParameters.NumConstraints = NumConstraints;
	PassParameters.OffsetConstraints = OffsetConstraints;
	PassParameters.OffsetParticles = OffsetParticles;

	SetShaderParameters(RHICmdList, CS, CS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, CS, CS.GetComputeShader());
}


FRWBuffer ConstraintTriangleIndexBuffer[2];


int GenerateDistanceBendingConstraintsFromMesh(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef ParticleA
	, FUnorderedAccessViewRHIRef ParticleB
	, FUnorderedAccessViewRHIRef DistanceBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef PositionBuffer
	, FRHIShaderResourceView* ColorBuffer
	, uint32 ConstraintsOffset
	, uint32 NumTriangles
	, uint32 OffsetParticles
	, bool bTest
)
{

	uint32 MaxKeyBufferLength = 1 << 20;
	if (ConstraintKeyBuffers[0].SRV == nullptr)
	{
		ConstraintKeyBuffers[0].Initialize(TEXT("ConstraintKeyBuffer-0"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		ConstraintKeyBuffers[1].Initialize(TEXT("ConstraintKeyBuffer-1"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		GlobalOffsetBuffer.Initialize(TEXT("GlobalOffsetBuffer"), sizeof(uint32), 1, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	}

	if (ConstraintTriangleIndexBuffer[0].SRV == nullptr)
	{
		ConstraintTriangleIndexBuffer[0].Initialize(TEXT("TriangleIndexBuffer-0"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		ConstraintTriangleIndexBuffer[1].Initialize(TEXT("TriangleIndexBuffer-1"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	}

	YQInitializeSizeBuffer(RHICmdList, GlobalOffsetBuffer.UAV, 1, 0);

	GenerateBendingConstraintsKeyAndTriangleIndexFromMesh(RHICmdList, IndexBuffer, ConstraintKeyBuffers[0].UAV, ConstraintTriangleIndexBuffer[0].UAV,  NumTriangles);

	FGPUSortBuffers GPUSortBuffers;
	GPUSortBuffers.RemoteKeySRVs[0] = ConstraintKeyBuffers[0].SRV;
	GPUSortBuffers.RemoteKeySRVs[1] = ConstraintKeyBuffers[1].SRV;
	GPUSortBuffers.RemoteKeyUAVs[0] = ConstraintKeyBuffers[0].UAV;
	GPUSortBuffers.RemoteKeyUAVs[1] = ConstraintKeyBuffers[1].UAV;
	GPUSortBuffers.RemoteValueSRVs[0] = ConstraintTriangleIndexBuffer[0].SRV;
	GPUSortBuffers.RemoteValueSRVs[1] = ConstraintTriangleIndexBuffer[1].SRV;
	GPUSortBuffers.RemoteValueUAVs[0] = ConstraintTriangleIndexBuffer[0].UAV;
	GPUSortBuffers.RemoteValueUAVs[1] = ConstraintTriangleIndexBuffer[1].UAV;

	int32 BufferIndex = SortGPUBuffers(
		RHICmdList
		, GPUSortBuffers
		, 0
		, 0xFFFFFFFF
		, NumTriangles * 3
		, ERHIFeatureLevel::SM5
	);

	YQInitializeSizeBuffer(RHICmdList, ConstraintKeyBuffers[1 - BufferIndex].UAV, NumTriangles * 4, 0);

	CalcBendingConstraintsTriangleIndex(
		RHICmdList
		, ConstraintKeyBuffers[BufferIndex].SRV
		, ConstraintTriangleIndexBuffer[BufferIndex].SRV
		, ConstraintTriangleIndexBuffer[1 - BufferIndex].UAV
		, GlobalOffsetBuffer.UAV
		, 0
		, NumTriangles * 3
	);

	void* ReadPtr = RHILockBuffer(GlobalOffsetBuffer.Buffer, 0, sizeof(uint32), EResourceLockMode::RLM_ReadOnly);

	uint32* ReadPtr32 = (uint32*)ReadPtr;
	uint32 NumUniqueConstraints = ReadPtr32[0];

	RHIUnlockBuffer(GlobalOffsetBuffer.Buffer);

	// ----- Log
	{
		BufferIndex = 1 - BufferIndex;
	}

	if (ParticleA == nullptr && bTest)
	{
		if (ParticleBufferAForTest.SRV == nullptr)
		{
			ParticleBufferAForTest.Initialize(TEXT("ParticleBufferAForTest"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);;
			ParticleBufferBForTest.Initialize(TEXT("ParticleBufferBForTest"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);;
			DistanceBufferForTest.Initialize(TEXT("DistanceBufferForTest"), sizeof(float), MaxKeyBufferLength, EPixelFormat::PF_R32_FLOAT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);;
		}

		ParticleA = ParticleBufferAForTest.UAV;
		ParticleB = ParticleBufferBForTest.UAV;
		DistanceBuffer = DistanceBufferForTest.UAV;
	}

	if (ParticleA != nullptr && PositionBuffer != nullptr)
	{
		GenerateDistanceBendingConstraints(
			RHICmdList
			, IndexBuffer
			, ConstraintTriangleIndexBuffer[BufferIndex].SRV
			, PositionBuffer
			, DistanceBuffer
			, ParticleA
			, ParticleB
			, ColorBuffer
			, NumUniqueConstraints
			, ConstraintsOffset
			, OffsetParticles
		);
	}

	return NumUniqueConstraints;
}



// 根据两个有共享边的三角形，产生弯曲约束
// 参考UE的chaos
class FYQGenerateBendingConstraintsCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQGenerateBendingConstraintsCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQGenerateBendingConstraintsCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, IndexBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, CombinedTriangleIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintsParticleABuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintsParticleBBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintsParticleCBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ConstraintsParticleDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<half>, AngleBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, PositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<float>, ColorBuffer)
		SHADER_PARAMETER(uint32, NumConstraints)
		SHADER_PARAMETER(uint32, OffsetConstraints)
		SHADER_PARAMETER(uint32, OffsetParticles)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("GENERATE_BENDING_CONSTRAINTS"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQGenerateBendingConstraintsCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);


void GenerateBendingConstraints(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef CombinedTriangleIDBuffer
	, FShaderResourceViewRHIRef PositionBuffer
	, FUnorderedAccessViewRHIRef AngleBuffer
	, FUnorderedAccessViewRHIRef IDBufferA
	, FUnorderedAccessViewRHIRef IDBufferB
	, FUnorderedAccessViewRHIRef IDBufferC
	, FUnorderedAccessViewRHIRef IDBufferD
	, FRHIShaderResourceView* ColorBuffer
	, uint32 NumConstraints
	, uint32 OffsetConstraints
	, uint32 OffsetParticles
)
{
	SCOPED_DRAW_EVENT(RHICmdList, GenerateDistanceBendingConstraints);

	TShaderMapRef<FYQGenerateBendingConstraintsCS> CS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(CS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumConstraints, FYQGenerateBendingConstraintsCS::ThreadGroupSize);

	FYQGenerateBendingConstraintsCS::FParameters PassParameters;
	PassParameters.ConstraintsParticleABuffer = IDBufferA;
	PassParameters.ConstraintsParticleBBuffer = IDBufferB;
	PassParameters.ConstraintsParticleCBuffer = IDBufferC;
	PassParameters.ConstraintsParticleDBuffer = IDBufferD;
	PassParameters.AngleBuffer = AngleBuffer;
	PassParameters.PositionBuffer = PositionBuffer;
	PassParameters.IndexBuffer = IndexBuffer;
	PassParameters.ColorBuffer = ColorBuffer;
	PassParameters.CombinedTriangleIDBuffer = CombinedTriangleIDBuffer;
	PassParameters.NumConstraints = NumConstraints;
	PassParameters.OffsetConstraints = OffsetConstraints;
	PassParameters.OffsetParticles = OffsetParticles;

	SetShaderParameters(RHICmdList, CS, CS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, CS, CS.GetComputeShader());
}

int GenerateBendingConstraintsFromMesh(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef ParticleA
	, FUnorderedAccessViewRHIRef ParticleB
	, FUnorderedAccessViewRHIRef ParticleC
	, FUnorderedAccessViewRHIRef ParticleD
	, FUnorderedAccessViewRHIRef AngleBuffer
	, FShaderResourceViewRHIRef IndexBuffer
	, FShaderResourceViewRHIRef PositionBuffer
	, FRHIShaderResourceView* ColorBuffer
	, uint32 ConstraintsOffset
	, uint32 NumTriangles
	, uint32 OffsetParticles
)
{
	uint32 MaxKeyBufferLength = 1 << 20;
	if (ConstraintKeyBuffers[0].SRV == nullptr)
	{
		ConstraintKeyBuffers[0].Initialize(TEXT("ConstraintKeyBuffer-0"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		ConstraintKeyBuffers[1].Initialize(TEXT("ConstraintKeyBuffer-1"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		GlobalOffsetBuffer.Initialize(TEXT("GlobalOffsetBuffer"), sizeof(uint32), 1, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	}

	if (ConstraintTriangleIndexBuffer[0].SRV == nullptr)
	{
		ConstraintTriangleIndexBuffer[0].Initialize(TEXT("TriangleIndexBuffer-0"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		ConstraintTriangleIndexBuffer[1].Initialize(TEXT("TriangleIndexBuffer-1"), sizeof(uint32), MaxKeyBufferLength, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	}

	YQInitializeSizeBuffer(RHICmdList, GlobalOffsetBuffer.UAV, 1, 0);

	GenerateBendingConstraintsKeyAndTriangleIndexFromMesh(RHICmdList, IndexBuffer, ConstraintKeyBuffers[0].UAV, ConstraintTriangleIndexBuffer[0].UAV, NumTriangles);

	FGPUSortBuffers GPUSortBuffers;
	GPUSortBuffers.RemoteKeySRVs[0] = ConstraintKeyBuffers[0].SRV;
	GPUSortBuffers.RemoteKeySRVs[1] = ConstraintKeyBuffers[1].SRV;
	GPUSortBuffers.RemoteKeyUAVs[0] = ConstraintKeyBuffers[0].UAV;
	GPUSortBuffers.RemoteKeyUAVs[1] = ConstraintKeyBuffers[1].UAV;
	GPUSortBuffers.RemoteValueSRVs[0] = ConstraintTriangleIndexBuffer[0].SRV;
	GPUSortBuffers.RemoteValueSRVs[1] = ConstraintTriangleIndexBuffer[1].SRV;
	GPUSortBuffers.RemoteValueUAVs[0] = ConstraintTriangleIndexBuffer[0].UAV;
	GPUSortBuffers.RemoteValueUAVs[1] = ConstraintTriangleIndexBuffer[1].UAV;

	int32 BufferIndex = SortGPUBuffers(
		RHICmdList
		, GPUSortBuffers
		, 0
		, 0xFFFFFFFF
		, NumTriangles * 3
		, ERHIFeatureLevel::SM5
	);

	YQInitializeSizeBuffer(RHICmdList, ConstraintKeyBuffers[1 - BufferIndex].UAV, NumTriangles * 4, 0);

	CalcBendingConstraintsTriangleIndex(
		RHICmdList
		, ConstraintKeyBuffers[BufferIndex].SRV
		, ConstraintTriangleIndexBuffer[BufferIndex].SRV
		, ConstraintTriangleIndexBuffer[1 - BufferIndex].UAV
		, GlobalOffsetBuffer.UAV
		, 0
		, NumTriangles * 3
	);

	void* ReadPtr = RHILockBuffer(GlobalOffsetBuffer.Buffer, 0, sizeof(uint32), EResourceLockMode::RLM_ReadOnly);

	uint32* ReadPtr32 = (uint32*)ReadPtr;
	uint32 NumUniqueConstraints = ReadPtr32[0];

	RHIUnlockBuffer(GlobalOffsetBuffer.Buffer);

	{
		BufferIndex = 1 - BufferIndex;
	}

	GenerateBendingConstraints(
		RHICmdList
		, IndexBuffer
		, ConstraintTriangleIndexBuffer[BufferIndex].SRV
		, PositionBuffer
		, AngleBuffer
		, ParticleA
		, ParticleB
		, ParticleC
		, ParticleD
		, ColorBuffer
		, NumUniqueConstraints
		, ConstraintsOffset
		, OffsetParticles
	);

	return NumUniqueConstraints;
}


namespace MoveConstraintsPermutation
{
	class FNumUintBufferDim : SHADER_PERMUTATION_INT("NUM_UINT_BUFFER", 5);
	class FNumHalfBufferDim : SHADER_PERMUTATION_INT("NUM_HALF_BUFFER", 3);
	class FNumFloatBufferDim : SHADER_PERMUTATION_INT("NUM_FLOAT_BUFFER", 3);
}

class FYQMoveConstraintsBufferCS : public FGlobalShader
{
public:
	DECLARE_SHADER_TYPE(FYQMoveConstraintsBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQMoveConstraintsBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, UintParamBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, UintParamBuffer1)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, UintParamBuffer2)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, UintParamBuffer3)
		SHADER_PARAMETER_UAV(RWBuffer<half>, HalfParamBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<half>, HalfParamBuffer1)
		SHADER_PARAMETER_UAV(RWBuffer<float>, FloatParamBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<float>, FloatParamBuffer1)
		SHADER_PARAMETER(uint32, SrcIndex)
		SHADER_PARAMETER(uint32, DestIndex)
		SHADER_PARAMETER(uint32, NumConstraints)
		END_SHADER_PARAMETER_STRUCT()


		using FPermutationDomain = TShaderPermutationDomain<
				MoveConstraintsPermutation::FNumUintBufferDim
				, MoveConstraintsPermutation::FNumHalfBufferDim
				, MoveConstraintsPermutation::FNumFloatBufferDim
			>;

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("MOVE_CONSTRAINTS_BUFFER"), 1);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

	}

};

IMPLEMENT_SHADER_TYPE(, FYQMoveConstraintsBufferCS, TEXT("/YQLearnPhysics/ConstraintsGenerate.usf"), TEXT("MainCS"), SF_Compute);

void MoveConstraintsBuffer(
	FRHICommandList& RHICmdList
	, FRHIUnorderedAccessView** BufferList
	, EPixelFormat* FormatList
	, uint32 NumBuffer
)
{
	SCOPED_DRAW_EVENT(RHICmdList, MoveConstraintsBuffer);


}


void MoveConstraintsBuffer(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef UintParamBuffer0
	, FUnorderedAccessViewRHIRef UintParamBuffer1
	, FUnorderedAccessViewRHIRef FloatParamBuffer
	, uint32 SrcIndex
	, uint32 DestIndex
	, uint32 NumConstraints
)
{
	SCOPED_DRAW_EVENT(RHICmdList, MoveConstraintsBuffer);

	FYQMoveConstraintsBufferCS::FPermutationDomain PermutationVector;
	PermutationVector.Set<MoveConstraintsPermutation::FNumUintBufferDim>(2);
	PermutationVector.Set<MoveConstraintsPermutation::FNumFloatBufferDim>(1);
	PermutationVector.Set<MoveConstraintsPermutation::FNumHalfBufferDim>(0);

	TShaderMapRef<FYQMoveConstraintsBufferCS> CS(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
	RHICmdList.SetComputeShader(CS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumConstraints, FYQMoveConstraintsBufferCS::ThreadGroupSize);

	FYQMoveConstraintsBufferCS::FParameters PassParameters;
	PassParameters.UintParamBuffer0 = UintParamBuffer0;
	PassParameters.UintParamBuffer1 = UintParamBuffer1;
	PassParameters.FloatParamBuffer0 = FloatParamBuffer;
	PassParameters.DestIndex = DestIndex;
	PassParameters.SrcIndex = SrcIndex;
	PassParameters.NumConstraints = NumConstraints;

	SetShaderParameters(RHICmdList, CS, CS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, CS, CS.GetComputeShader());
}

void MoveConstraintsBuffer(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef UintParamBuffer0
	, FUnorderedAccessViewRHIRef UintParamBuffer1
	, FUnorderedAccessViewRHIRef UintParamBuffer2
	, FUnorderedAccessViewRHIRef UintParamBuffer3
	, FUnorderedAccessViewRHIRef HalfParamBuffer
	, uint32 SrcIndex
	, uint32 DestIndex
	, uint32 NumConstraints
)
{
	SCOPED_DRAW_EVENT(RHICmdList, MoveConstraintsBuffer);

	FYQMoveConstraintsBufferCS::FPermutationDomain PermutationVector;
	PermutationVector.Set<MoveConstraintsPermutation::FNumUintBufferDim>(4);
	PermutationVector.Set<MoveConstraintsPermutation::FNumFloatBufferDim>(0);
	PermutationVector.Set<MoveConstraintsPermutation::FNumHalfBufferDim>(1);

	TShaderMapRef<FYQMoveConstraintsBufferCS> CS(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
	RHICmdList.SetComputeShader(CS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumConstraints, FYQMoveConstraintsBufferCS::ThreadGroupSize);

	FYQMoveConstraintsBufferCS::FParameters PassParameters;
	PassParameters.UintParamBuffer0 = UintParamBuffer0;
	PassParameters.UintParamBuffer1 = UintParamBuffer1;
	PassParameters.UintParamBuffer2 = UintParamBuffer2;
	PassParameters.UintParamBuffer3 = UintParamBuffer3;
	PassParameters.HalfParamBuffer0 = HalfParamBuffer;
	PassParameters.DestIndex = DestIndex;
	PassParameters.SrcIndex = SrcIndex;
	PassParameters.NumConstraints = NumConstraints;

	SetShaderParameters(RHICmdList, CS, CS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, CS, CS.GetComputeShader());
}