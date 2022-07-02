#include "YQPhysicsSceneComputeShader.h"

IMPLEMENT_SHADER_TYPE(, FYQAppendStaticMeshToPhysicsSceneCS, TEXT("/YQLearnPhysics/YQPhysicsScene.usf"), TEXT("MainCS"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FYQInitializeObjectEntryCS, TEXT("/YQLearnPhysics/YQPhysicsScene.usf"), TEXT("MainCS"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FYQClearBufferCS, TEXT("/YQLearnPhysics/YQPhysicsScene.usf"), TEXT("MainCS"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FYQAddGPUObjectToBufferCS, TEXT("/YQLearnPhysics/YQPhysicsScene.usf"), TEXT("MainCS"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FYQAppendParticlesToPhysicsSceneCS, TEXT("/YQLearnPhysics/YQPhysicsScene.usf"), TEXT("MainCS"), SF_Compute);

IMPLEMENT_SHADER_TYPE(, FYQCopyDataToCPUBufferCS, TEXT("/YQLearnPhysics/YQPhysicsScene.usf"), TEXT("MainCS"), SF_Compute);


void AppendTriangleToPhysicsScene_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InputIndexBuffer
	, FShaderResourceViewRHIRef InputVertexBuffer
	, FUnorderedAccessViewRHIRef OutputIndexBuffer
	, FUnorderedAccessViewRHIRef OutputVertexBuffer
	, FUnorderedAccessViewRHIRef PhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef InputColorBuffer
	, FUnorderedAccessViewRHIRef OutputMaskBuffer
	, uint32 NumTriangles
	, uint32 NumVertices
)
{
	check(IsInRenderingThread());

	uint32 NumThread = NumTriangles > NumVertices ? NumTriangles : NumVertices;

	FYQAppendStaticMeshToPhysicsSceneCS::FPermutationDomain AppendPermutationVector;
	AppendPermutationVector.Set<AppendStaticMeshPermutation::FFixedByColorDim>(InputColorBuffer != nullptr);

	TShaderMapRef<FYQAppendStaticMeshToPhysicsSceneCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), AppendPermutationVector);
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumThread, FYQAppendStaticMeshToPhysicsSceneCS::ThreadGroupSize);

	FYQAppendStaticMeshToPhysicsSceneCS::FParameters PassParameters;
	PassParameters.InputIndexBuffer = InputIndexBuffer;
	PassParameters.InputVertexBuffer = InputVertexBuffer;
	PassParameters.OutputIndexBuffer = OutputIndexBuffer;
	PassParameters.OutputVertexBuffer = OutputVertexBuffer;
	PassParameters.PhysicsSceneViewBuffer = PhysicsSceneViewBuffer;
	PassParameters.NumTriangles = NumTriangles;
	PassParameters.NumVertices = NumVertices;
	PassParameters.InputColorBuffer = InputColorBuffer;
	PassParameters.OutputMaskBuffer = OutputMaskBuffer;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}


void InitializeObjectEntryBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef ObjectEntryOffsetBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryCountBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryBitmaskBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryOffsetForRenderBuffer
	, uint32 BufferLength
)
{
	TShaderMapRef<FYQInitializeObjectEntryCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(BufferLength, FYQInitializeObjectEntryCS::ThreadGroupSize);

	FYQInitializeObjectEntryCS::FParameters PassParameters;
	PassParameters.ObjectEntryOffsetBuffer = ObjectEntryOffsetBuffer;
	PassParameters.ObjectEntryCountBuffer = ObjectEntryCountBuffer;
	PassParameters.ObjectEntryBitmaskBuffer = ObjectEntryBitmaskBuffer;
	PassParameters.ObjectEntryOffsetForRenderBuffer = ObjectEntryOffsetForRenderBuffer;
	PassParameters.BufferLength = BufferLength;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}

void ClearBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FUnorderedAccessViewRHIRef Buffer
	, uint32 BufferLength
)
{
	TShaderMapRef<FYQClearBufferCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(BufferLength, FYQClearBufferCS::ThreadGroupSize);

	FYQClearBufferCS::FParameters PassParameters;
	PassParameters.OutBuffer = Buffer;
	PassParameters.BufferLength = BufferLength;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}


void AddGPUObjectToBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InPhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef ObjectEntryArgBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryOffsetBuffer
	, FUnorderedAccessViewRHIRef ObjectEntryCountBuffer
	, FUnorderedAccessViewRHIRef OutPhysicsSceneViewBuffer
	, uint32 NumObjects
)
{
	TShaderMapRef<FYQAddGPUObjectToBufferCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumObjects, FYQAddGPUObjectToBufferCS::ThreadGroupSize);

	FYQAddGPUObjectToBufferCS::FParameters PassParameters;
	PassParameters.InPhysicsSceneViewBuffer = InPhysicsSceneViewBuffer;
	PassParameters.ObjectEntryArgBuffer = ObjectEntryArgBuffer;
	PassParameters.ObjectEntryOffsetBuffer = ObjectEntryOffsetBuffer;
	PassParameters.ObjectEntryCountBuffer = ObjectEntryCountBuffer;
	PassParameters.OutPhysicsSceneViewBuffer = OutPhysicsSceneViewBuffer;
	PassParameters.NumObjects = NumObjects;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}

void AppendParticlesToPhysicsScene_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef InPhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef InputVertexBuffer
	, FUnorderedAccessViewRHIRef OutputVertexBuffer
	, FUnorderedAccessViewRHIRef OutPhysicsSceneViewBuffer
	, FShaderResourceViewRHIRef InputColorBuffer
	, FUnorderedAccessViewRHIRef OutputMaskBuffer
	, FMatrix44f LocalToWorld
	, uint32 NumParticles
)
{
	FYQAppendParticlesToPhysicsSceneCS::FPermutationDomain AppendPermutationVector;
	AppendPermutationVector.Set<AppendParticlesPermutation::FFixedByColorDim>(InputColorBuffer != nullptr);

	TShaderMapRef<FYQAppendParticlesToPhysicsSceneCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), AppendPermutationVector);
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = FMath::DivideAndRoundUp(NumParticles, FYQAppendParticlesToPhysicsSceneCS::ThreadGroupSize);

	FYQAppendParticlesToPhysicsSceneCS::FParameters PassParameters;
	PassParameters.InPhysicsSceneViewBuffer = InPhysicsSceneViewBuffer;
	PassParameters.InputVertexBuffer = InputVertexBuffer;
	PassParameters.OutputVertexBuffer = OutputVertexBuffer;
	PassParameters.OutPhysicsSceneViewBuffer = OutPhysicsSceneViewBuffer;
	PassParameters.NumParticles = NumParticles;
	PassParameters.InputColorBuffer = InputColorBuffer;
	PassParameters.OutputMaskBuffer = OutputMaskBuffer;
	PassParameters.LocalToWorld = LocalToWorld;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}


void CopyDataToCPUBuffer_RenderThread(
	FRHICommandList& RHICmdList
	, FShaderResourceViewRHIRef PhysicsSceneViewBuffer
	, FUnorderedAccessViewRHIRef PerFrameToCPUBuffer
)
{
	TShaderMapRef<FYQCopyDataToCPUBufferCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	RHICmdList.SetComputeShader(ComputeShader.GetComputeShader());

	const uint32 NumThreadGroups = 1;

	FYQCopyDataToCPUBufferCS::FParameters PassParameters;
	PassParameters.PhysicsSceneViewBuffer = PhysicsSceneViewBuffer;
	PassParameters.PerFrameToCPUBuffer = PerFrameToCPUBuffer;

	SetShaderParameters(RHICmdList, ComputeShader, ComputeShader.GetComputeShader(), PassParameters);
	RHICmdList.DispatchComputeShader(NumThreadGroups, 1, 1);
	UnsetShaderUAVs(RHICmdList, ComputeShader, ComputeShader.GetComputeShader());
}