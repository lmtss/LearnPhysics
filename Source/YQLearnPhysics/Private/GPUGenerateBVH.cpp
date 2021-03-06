#include "GPUBasedBVH.h"
#include "GPUSort.h"

#include "ShaderCompilerCore.h"

class FGenerateBVCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FGenerateBVCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FGenerateBVCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<float4>, ParticlePositionBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, TriangleIndexBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, MortonCodeBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, BVHPositionBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<float2>, BVHPositionBuffer1)
		SHADER_PARAMETER(FVector3f, Center)
		SHADER_PARAMETER(FVector3f, Extent)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_SIMPLE_BV"), 1);
	}

};


IMPLEMENT_SHADER_TYPE(, FGenerateBVCS, TEXT("/YQLearnPhysics/GPUBasedBVH.usf"), TEXT("MainCS"), SF_Compute);

class FClearValueForSortCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FClearValueForSortCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FClearValueForSortCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ValueBufferForSort)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_CLEAR_VALUE_BUFFER_FOR_SORT"), 1);
	}

};


IMPLEMENT_SHADER_TYPE(, FClearValueForSortCS, TEXT("/YQLearnPhysics/GPUBasedBVH.usf"), TEXT("MainCS"), SF_Compute);

class FClearAtomicAccessBufferCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FClearAtomicAccessBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FClearAtomicAccessBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(RWBuffer<uint>, AtomicAccessBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, InternalNodeRangeBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, InternalNodeBVBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<float2>, InternalNodeBVBuffer1)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_CLEAR_ATOMIC_ACCESS_BUFFER"), 1);
	}

};


IMPLEMENT_SHADER_TYPE(, FClearAtomicAccessBufferCS, TEXT("/YQLearnPhysics/GPUBasedBVH.usf"), TEXT("MainCS"), SF_Compute);

// ???BV?????????????????????????????????????????????delta???
class FTransferBVBufferCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FTransferBVBufferCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FTransferBVBufferCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, SortedIndexBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, MortonCodeBuffer)
		SHADER_PARAMETER_SRV(Buffer<float4>, BVHPositionBuffer0)
		SHADER_PARAMETER_SRV(Buffer<float2>, BVHPositionBuffer1)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, TransferBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<float2>, TransferBuffer1)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, KeyDeltaBuffer)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_TRANSFER_BV_BUFFER"), 1);
	}

};


IMPLEMENT_SHADER_TYPE(, FTransferBVBufferCS, TEXT("/YQLearnPhysics/GPUBasedBVH.usf"), TEXT("MainCS"), SF_Compute);

class FFSAGenerateBVHTreeCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FFSAGenerateBVHTreeCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FFSAGenerateBVHTreeCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, KeyDeltaBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, AtomicAccessCountBuffer)
		SHADER_PARAMETER_SRV(Buffer<float4>, LeafBVBuffer0)
		SHADER_PARAMETER_SRV(Buffer<float2>, LeafBVBuffer1)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, InternalNodeRangeBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, InternalNodeBVBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<float2>, InternalNodeBVBuffer1)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, BVHChildIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, PhysicsSceneViewBuffer)
		SHADER_PARAMETER(uint32, Length)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_FSA_GENERATE_BVH_TREE"), 1);

		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};


IMPLEMENT_SHADER_TYPE(, FFSAGenerateBVHTreeCS, TEXT("/YQLearnPhysics/GPUBasedBVH.usf"), TEXT("MainCS"), SF_Compute);

// ????????????????????????????????????ID????????????ID
class FGenerateRadixTreeCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FGenerateRadixTreeCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FGenerateRadixTreeCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, MortonCodeBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, BVHParentIDBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, BVHChildIDBuffer)
		SHADER_PARAMETER(uint32, NumInternalNodes)
		SHADER_PARAMETER(uint32, LengthHalfParentIDBuffer)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_GENERATE_RADIX_TREE"), 1);

		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};


IMPLEMENT_SHADER_TYPE(, FGenerateRadixTreeCS, TEXT("/YQLearnPhysics/GPUBasedBVH.usf"), TEXT("MainCS"), SF_Compute);


// ????????????????????????????????????BVH
class FGenerateBVHByRadixTreeCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FGenerateBVHByRadixTreeCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FGenerateBVHByRadixTreeCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 64;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_SRV(Buffer<uint>, BVHChildIDBuffer)
		SHADER_PARAMETER_SRV(Buffer<uint>, BVHParentIDBuffer)
		SHADER_PARAMETER_SRV(Buffer<float4>, LeafBVBuffer0)
		SHADER_PARAMETER_SRV(Buffer<float2>, LeafBVBuffer1)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, AtomicAccessCountBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<float4>, InternalNodeBVBuffer0)
		SHADER_PARAMETER_UAV(RWBuffer<float2>, InternalNodeBVBuffer1)
		SHADER_PARAMETER(uint32, NumLeafNodes)
		SHADER_PARAMETER(uint32, LeafNodeParentOffset)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::ES3_1);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREAD_COUNT"), ThreadGroupSize);
		OutEnvironment.SetDefine(TEXT("COMPILE_GENERATE_BVH_BY_TREE"), 1);

		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};


IMPLEMENT_SHADER_TYPE(, FGenerateBVHByRadixTreeCS, TEXT("/YQLearnPhysics/GPUBasedBVH.usf"), TEXT("MainCS"), SF_Compute);

FRWBuffer TempSortValueBuffers[2];

// ?????????????????????BV
void ReduceMeshMaxBV_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FShaderResourceViewRHIRef TriangleIndexBuffer
	, FUnorderedAccessViewRHIRef BVHPositionBuffer0
	, FUnorderedAccessViewRHIRef BVHPositionBuffer1
	, FUnorderedAccessViewRHIRef MortonCodeBufferUAV
	, uint32 Length
) 
{
	
}

// ?????????????????????BV
void GPUGenerateBV_RenderThread(
    FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FShaderResourceViewRHIRef TriangleIndexBuffer
    , FUnorderedAccessViewRHIRef BVHPositionBuffer0
    , FUnorderedAccessViewRHIRef BVHPositionBuffer1
    , FUnorderedAccessViewRHIRef MortonCodeBufferUAV
	, FBox BoundingBox
    , uint32 Length
)
{
	SCOPED_DRAW_EVENT(RHICmdList, GPUGenerateBV);

	TShaderMapRef<FGenerateBVCS> GenerateBVCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateBVCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FGenerateBVCS::ThreadGroupSize);

	FGenerateBVCS::FParameters GenerateBVParameters;
	GenerateBVParameters.Length = Length;
    GenerateBVParameters.MortonCodeBuffer = MortonCodeBufferUAV;
    GenerateBVParameters.TriangleIndexBuffer = TriangleIndexBuffer;
    GenerateBVParameters.ParticlePositionBuffer = ParticlePositionBuffer;
    GenerateBVParameters.BVHPositionBuffer0 = BVHPositionBuffer0;
    GenerateBVParameters.BVHPositionBuffer1 = BVHPositionBuffer1;

	FVector3d BoxCenterD = BoundingBox.GetCenter();
	FVector3d BoxExtentD = BoundingBox.GetExtent();

	GenerateBVParameters.Center = FVector3f(BoxCenterD.X, BoxCenterD.Y, BoxCenterD.Z);
	GenerateBVParameters.Extent = FVector3f(BoxExtentD.X, BoxExtentD.Y, BoxExtentD.Z);

	RHICmdList.Transition(FRHITransitionInfo(MortonCodeBufferUAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
	SetShaderParameters(RHICmdList, GenerateBVCS, GenerateBVCS.GetComputeShader(), GenerateBVParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateBVCS, GenerateBVCS.GetComputeShader());
	RHICmdList.Transition(FRHITransitionInfo(MortonCodeBufferUAV, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute));
}

void ClearSortValueBuffer_RenderThread(
    FRHICommandList& RHICmdList
    , uint32 Length
)
{
	SCOPED_DRAW_EVENT(RHICmdList, ClearValueBuffer);

	if(TempSortValueBuffers[0].SRV == nullptr)
	{
		TempSortValueBuffers[0].Initialize(TEXT("TempSortValueBuffers-0"), sizeof(uint32), 1024 * 1024, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
		TempSortValueBuffers[1].Initialize(TEXT("TempSortValueBuffers-1"), sizeof(uint32), 1024 * 1024, EPixelFormat::PF_R32_UINT, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	}

	TShaderMapRef<FClearValueForSortCS> ClearValueForSortCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ClearValueForSortCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FClearValueForSortCS::ThreadGroupSize);

	FClearValueForSortCS::FParameters PassParameters;
	PassParameters.Length = Length;
    PassParameters.ValueBufferForSort = TempSortValueBuffers[0].UAV;


	SetShaderParameters(RHICmdList, ClearValueForSortCS, ClearValueForSortCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ClearValueForSortCS, ClearValueForSortCS.GetComputeShader());
}



void TransferBVBySortedIndex_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef SortedIndexBuffer
	, FShaderResourceViewRHIRef MortonCodeBuffer
	, FShaderResourceViewRHIRef BVHPositionBuffer0
	, FShaderResourceViewRHIRef BVHPositionBuffer1
	, FUnorderedAccessViewRHIRef TransferBuffer0
	, FUnorderedAccessViewRHIRef TransferBuffer1
	, FUnorderedAccessViewRHIRef KeyDeltaBuffer
	, uint32 Length
)
{
	SCOPED_DRAW_EVENT(RHICmdList, TransferBVBySortedIndex);

	TShaderMapRef<FTransferBVBufferCS> TransferBVBufferCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(TransferBVBufferCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FTransferBVBufferCS::ThreadGroupSize);

	FTransferBVBufferCS::FParameters PassParameters;
	PassParameters.Length = Length;
    PassParameters.SortedIndexBuffer = SortedIndexBuffer;
	PassParameters.MortonCodeBuffer = MortonCodeBuffer;
	PassParameters.BVHPositionBuffer0 = BVHPositionBuffer0;
	PassParameters.BVHPositionBuffer1 = BVHPositionBuffer1;
	PassParameters.TransferBuffer0 = TransferBuffer0;
	PassParameters.TransferBuffer1 = TransferBuffer1;
	PassParameters.KeyDeltaBuffer = KeyDeltaBuffer;


	SetShaderParameters(RHICmdList, TransferBVBufferCS, TransferBVBufferCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, TransferBVBufferCS, TransferBVBufferCS.GetComputeShader());
}

void ClearAtomicAccessBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef AtomicAccessBuffer
	, FUnorderedAccessViewRHIRef InternalNodeRangeBuffer
	, FUnorderedAccessViewRHIRef InternalNodeBVBuffer0
	, FUnorderedAccessViewRHIRef InternalNodeBVBuffer1
	, uint32 Length
) 	
{
	SCOPED_DRAW_EVENT(RHICmdList, ClearAtomicAccessBuffer);

	TShaderMapRef<FClearAtomicAccessBufferCS> ClearValueForSortCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ClearValueForSortCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FClearAtomicAccessBufferCS::ThreadGroupSize);

	FClearAtomicAccessBufferCS::FParameters PassParameters;
	PassParameters.Length = Length;
	PassParameters.AtomicAccessBuffer = AtomicAccessBuffer;
	PassParameters.InternalNodeRangeBuffer = InternalNodeRangeBuffer;
	PassParameters.InternalNodeBVBuffer0 = InternalNodeBVBuffer0;
	PassParameters.InternalNodeBVBuffer1 = InternalNodeBVBuffer1;

	SetShaderParameters(RHICmdList, ClearValueForSortCS, ClearValueForSortCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ClearValueForSortCS, ClearValueForSortCS.GetComputeShader());
}

// Fast and Simple Agglomerative LBVH Construction ??????pass??????BVH???????????????
void GenerateTree_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef KeyDeltaBuffer
	, FUnorderedAccessViewRHIRef AtomicAccessCountBuffer
	, FShaderResourceViewRHIRef LeafBVBuffer0
	, FShaderResourceViewRHIRef LeafBVBuffer1
	, FUnorderedAccessViewRHIRef InternalNodeRangeBuffer
	, FUnorderedAccessViewRHIRef InternalNodeBVBuffer0
	, FUnorderedAccessViewRHIRef InternalNodeBVBuffer1
	, FUnorderedAccessViewRHIRef BVHChildIDBuffer
	, FUnorderedAccessViewRHIRef PhysicsSceneViewBuffer
	, uint32 Length
) 	
{
	SCOPED_DRAW_EVENT(RHICmdList, GenerateTree);

	TShaderMapRef<FFSAGenerateBVHTreeCS> GenerateTreeCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateTreeCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(Length, FFSAGenerateBVHTreeCS::ThreadGroupSize);

	FFSAGenerateBVHTreeCS::FParameters PassParameters;
	PassParameters.Length = Length;
	PassParameters.KeyDeltaBuffer = KeyDeltaBuffer;
	PassParameters.AtomicAccessCountBuffer = AtomicAccessCountBuffer;
	PassParameters.LeafBVBuffer0 = LeafBVBuffer0;
	PassParameters.LeafBVBuffer1 = LeafBVBuffer1;
	PassParameters.InternalNodeRangeBuffer = InternalNodeRangeBuffer;
	PassParameters.InternalNodeBVBuffer0 = InternalNodeBVBuffer0;
	PassParameters.InternalNodeBVBuffer1 = InternalNodeBVBuffer1;
	PassParameters.BVHChildIDBuffer = BVHChildIDBuffer;
	PassParameters.PhysicsSceneViewBuffer = PhysicsSceneViewBuffer;

	SetShaderParameters(RHICmdList, GenerateTreeCS, GenerateTreeCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateTreeCS, GenerateTreeCS.GetComputeShader());
}

// Maximizing Parallelism in the Construction of BVHs ?????????????????????
// ????????????????????????????????????????????????????????????????????????
void GenerateRadixTree_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef MortonCodeBuffer
	, FUnorderedAccessViewRHIRef BVHChildIDBuffer
	, FUnorderedAccessViewRHIRef BVHParentIDBuffer
	, uint32 NumInternalNodes
	, uint32 LengthHalfParentIDBuffer
)
{
	SCOPED_DRAW_EVENT(RHICmdList, GenerateRadixTree);

	TShaderMapRef<FGenerateRadixTreeCS> GenerateTreeCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateTreeCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumInternalNodes, FGenerateRadixTreeCS::ThreadGroupSize);

	FGenerateRadixTreeCS::FParameters PassParameters;
	PassParameters.BVHChildIDBuffer = BVHChildIDBuffer;
	PassParameters.BVHParentIDBuffer = BVHParentIDBuffer;
	PassParameters.MortonCodeBuffer = MortonCodeBuffer;
	PassParameters.NumInternalNodes = NumInternalNodes;
	PassParameters.LengthHalfParentIDBuffer = LengthHalfParentIDBuffer;

	SetShaderParameters(RHICmdList, GenerateTreeCS, GenerateTreeCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateTreeCS, GenerateTreeCS.GetComputeShader());
}

// Maximizing Parallelism in the Construction of BVHs ???????????????pass???????????????????????????BVH
void CombineBVHByReduce_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef BVHChildIDBuffer
	, FShaderResourceViewRHIRef BVHParentIDBuffer
	, FShaderResourceViewRHIRef LeafBVBuffer0
	, FShaderResourceViewRHIRef LeafBVBuffer1
	, FUnorderedAccessViewRHIRef InternalNodeBVBuffer0
	, FUnorderedAccessViewRHIRef InternalNodeBVBuffer1
	, FUnorderedAccessViewRHIRef AtomicAccessCountBuffer
	, uint32 NumLeafNodes
	, uint32 LeafNodeParentOffset
) {

	SCOPED_DRAW_EVENT(RHICmdList, CombineBVHByReduce);

	TShaderMapRef<FGenerateBVHByRadixTreeCS> GenerateTreeCS(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(GenerateTreeCS.GetComputeShader());

	uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumLeafNodes, FGenerateBVHByRadixTreeCS::ThreadGroupSize);

	FGenerateBVHByRadixTreeCS::FParameters PassParameters;

	PassParameters.BVHChildIDBuffer = BVHChildIDBuffer;
	PassParameters.BVHParentIDBuffer = BVHParentIDBuffer;
	PassParameters.LeafBVBuffer0 = LeafBVBuffer0;
	PassParameters.LeafBVBuffer1 = LeafBVBuffer1;

	PassParameters.AtomicAccessCountBuffer = AtomicAccessCountBuffer;
	PassParameters.InternalNodeBVBuffer0 = InternalNodeBVBuffer0;
	PassParameters.InternalNodeBVBuffer1 = InternalNodeBVBuffer1;

	PassParameters.NumLeafNodes = NumLeafNodes;
	PassParameters.LeafNodeParentOffset = LeafNodeParentOffset;

	SetShaderParameters(RHICmdList, GenerateTreeCS, GenerateTreeCS.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, GenerateTreeCS, GenerateTreeCS.GetComputeShader());
}

void GPUGenerateBVHTwoPass_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FShaderResourceViewRHIRef TriangleIndexBuffer
	, FGPUBvhBuffers& GPUBvhBuffers
	, FRWBuffer& PhysicsSceneViewBuffer
	, uint32 Length
)
{
	
}

void GPUGenerateBVH_RenderThread(
    FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef ParticlePositionBuffer
	, FShaderResourceViewRHIRef TriangleIndexBuffer
    , FGPUBvhBuffers& GPUBvhBuffers
	, FRWBuffer& PhysicsSceneViewBuffer
	, FBox BoundingBox
    , uint32 Length
)
{
	check(IsInRenderingThread());

	// ---------------- ?????????????????????BV ??? Morton??? ------------------------
	FVector3d Extent = BoundingBox.GetExtent();
	UE_LOG(LogTemp, Log, TEXT("BoudingBox (%f, %f, %f)"), Extent.X, Extent.Y, Extent.Z)

	GPUGenerateBV_RenderThread(
		RHICmdList
		, ParticlePositionBuffer
		, TriangleIndexBuffer
		, GPUBvhBuffers.BVPositionBufferA[0].UAV
		, GPUBvhBuffers.BVPositionBufferB[0].UAV
		, GPUBvhBuffers.MortonCodeBuffers[0].UAV
		, BoundingBox
		, Length
	);

	// ------------------- ????????????Buffer??????????????? -------------

	ClearSortValueBuffer_RenderThread(RHICmdList, Length);

	// --------------- ??????????????? --------------------------

	FGPUSortBuffers GPUSortBuffers;
	GPUSortBuffers.RemoteKeySRVs[0] = GPUBvhBuffers.MortonCodeBuffers[0].SRV;
	GPUSortBuffers.RemoteKeySRVs[1] = GPUBvhBuffers.MortonCodeBuffers[1].SRV;
	GPUSortBuffers.RemoteKeyUAVs[0] = GPUBvhBuffers.MortonCodeBuffers[0].UAV;
	GPUSortBuffers.RemoteKeyUAVs[1] = GPUBvhBuffers.MortonCodeBuffers[1].UAV;
	GPUSortBuffers.RemoteValueSRVs[0] = TempSortValueBuffers[0].SRV;
	GPUSortBuffers.RemoteValueSRVs[1] = TempSortValueBuffers[1].SRV;
	GPUSortBuffers.RemoteValueUAVs[0] = TempSortValueBuffers[0].UAV;
	GPUSortBuffers.RemoteValueUAVs[1] = TempSortValueBuffers[1].UAV;

	int32 BufferIndex = SortGPUBuffers(
		RHICmdList
		, GPUSortBuffers
		, 0
		, 0xFFFFFFFF
		, Length
		, ERHIFeatureLevel::SM5
		);

	// ---------------- ????????????????????????BV??????????????????i???i+1??????????? -----------------------
	TransferBVBySortedIndex_RenderThread(
		RHICmdList
		, TempSortValueBuffers[BufferIndex].SRV
		, GPUBvhBuffers.MortonCodeBuffers[BufferIndex].SRV
		, GPUBvhBuffers.BVPositionBufferA[0].SRV
		, GPUBvhBuffers.BVPositionBufferB[0].SRV
		, GPUBvhBuffers.BVPositionBufferA[1].UAV	//????????????????????????BVH????????????1????????????????????????BV???0???????????????BV
		, GPUBvhBuffers.BVPositionBufferB[1].UAV
		, GPUBvhBuffers.KeyDeltaBuffer.UAV
		, Length
	);

	// ---------------- ??????AtomicAccessBuffer ----------------------
	// todo: ???????????????????????????????????????0???1???????????????????????????2???3??????
	ClearAtomicAccessBuffer_RenderThread(
		RHICmdList
		, GPUBvhBuffers.AtomicAccessBuffer.UAV
		, GPUBvhBuffers.InternalNodeRangeBuffer.UAV
		, GPUBvhBuffers.BVPositionBufferA[0].UAV
		, GPUBvhBuffers.BVPositionBufferB[0].UAV
		, Length * 2
	);
#if 0
	// ---------------- ??????BVH ---------------------
	GenerateTree_RenderThread(
		RHICmdList
		, GPUBvhBuffers.KeyDeltaBuffer.SRV
		, GPUBvhBuffers.AtomicAccessBuffer.UAV
		, GPUBvhBuffers.BVPositionBufferA[1].SRV
		, GPUBvhBuffers.BVPositionBufferB[1].SRV
		, GPUBvhBuffers.InternalNodeRangeBuffer.UAV
		, GPUBvhBuffers.BVPositionBufferA[0].UAV
		, GPUBvhBuffers.BVPositionBufferB[0].UAV
		, GPUBvhBuffers.BVHChildIDBuffer.UAV
		, PhysicsSceneViewBuffer.UAV
		, Length
	);
#else
	GenerateRadixTree_RenderThread(
		RHICmdList
		, GPUBvhBuffers.MortonCodeBuffers[BufferIndex].SRV
		, GPUBvhBuffers.BVHChildIDBuffer.UAV
		, GPUBvhBuffers.BVHParentIDBuffer.UAV
		, Length - 1
		, Length
	);

	CombineBVHByReduce_RenderThread(
		RHICmdList
		, GPUBvhBuffers.BVHChildIDBuffer.SRV
		, GPUBvhBuffers.BVHParentIDBuffer.SRV
		, GPUBvhBuffers.BVPositionBufferA[1].SRV
		, GPUBvhBuffers.BVPositionBufferB[1].SRV
		, GPUBvhBuffers.BVPositionBufferA[0].UAV
		, GPUBvhBuffers.BVPositionBufferB[0].UAV
		, GPUBvhBuffers.AtomicAccessBuffer.UAV
		, Length
		, Length
	);
#endif
}