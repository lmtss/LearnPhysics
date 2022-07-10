#include "YQPhysicsVertexFactory.h"

#include "MeshMaterialShader.h"

//#include "EmptyRHIPrivate.h"
#include "UniformBuffer.h"


#include "CoreMinimal.h"
#include "RenderResource.h"
#include "SceneView.h"
#include "Components.h"
#include "SceneManagement.h"
#include "VertexFactory.h"


#include "YQPhysicsSimulator.h"

IMPLEMENT_GLOBAL_SHADER_PARAMETER_STRUCT(FYQPhysicsUniformParameters, "YQPhysicsVF");

class FYQPhysicsVertexFactoryShaderParametersVS : public FVertexFactoryShaderParameters
{
	//DECLARE_INLINE_TYPE_LAYOUT(FYQPhysicsVertexFactoryShaderParametersVS, NonVirtual);
	DECLARE_TYPE_LAYOUT(FYQPhysicsVertexFactoryShaderParametersVS, NonVirtual);
public:
	void Bind(const FShaderParameterMap& ParameterMap)
	{
		PositionBufferParameter.Bind(ParameterMap, TEXT("PositionBuffer"));
		NormalBufferParameter.Bind(ParameterMap, TEXT("NormalBuffer"));
		TangentBufferParameter.Bind(ParameterMap, TEXT("TangentBuffer"));
	}

	void GetElementShaderBindings(
		const FSceneInterface* Scene,
		const FSceneView* View,
		const FMeshMaterialShader* Shader,
		const EVertexInputStreamType InputStreamType,
		ERHIFeatureLevel::Type FeatureLevel,
		const FVertexFactory* VertexFactory,
		const FMeshBatchElement& BatchElement,
		class FMeshDrawSingleShaderBindings& ShaderBindings,
		FVertexInputStreamArray& VertexStreams) const
	{
		FYQPhysicsVertexFactory* VF = (FYQPhysicsVertexFactory*)VertexFactory;
		ShaderBindings.Add(Shader->GetUniformBufferParameter<FYQPhysicsUniformParameters>(), VF->GetUniformBuffer());

		FYQPhysicsUserData* UserData = (FYQPhysicsUserData*)BatchElement.UserData;
		ShaderBindings.Add(PositionBufferParameter, UserData->PositionBuffer->SRV);
		//ShaderBindings.Add(NormalBufferParameter, UserData->NormalBuffer->SRV);
		ShaderBindings.Add(NormalBufferParameter, VF->GetDynamicNormalBufferSRV());
		ShaderBindings.Add(TangentBufferParameter, VF->GetDynamicTangentBufferSRV());

		//VF->GetDynamicTangentBufferUAV();
	}

	LAYOUT_FIELD(FShaderResourceParameter, PositionBufferParameter);
	LAYOUT_FIELD(FShaderResourceParameter, NormalBufferParameter);
	LAYOUT_FIELD(FShaderResourceParameter, TangentBufferParameter);
};

IMPLEMENT_TYPE_LAYOUT(FYQPhysicsVertexFactoryShaderParametersVS);

bool FYQPhysicsVertexFactory::ShouldCompilePermutation(const FVertexFactoryShaderPermutationParameters& Parameters) 
{
	if (
		 Parameters.MaterialParameters.MaterialDomain == MD_Surface 
		 //&& Parameters.MaterialParameters.ShadingModels == EMaterialShadingModel::MSM_Unlit 
		 && !Parameters.MaterialParameters.bIsUsedWithMeshParticles
		 && !Parameters.MaterialParameters.bIsUsedWithNiagaraMeshParticles
		 && !Parameters.MaterialParameters.bIsUsedWithNiagaraRibbons
		 && !Parameters.MaterialParameters.bIsUsedWithNiagaraSprites
		 && !Parameters.MaterialParameters.bIsUsedWithLandscape
		 || Parameters.MaterialParameters.bIsDefaultMaterial
		)
	{
		return true;
	}

	return false;
}

void FYQPhysicsVertexFactory::ModifyCompilationEnvironment(const FVertexFactoryShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) 
{
	OutEnvironment.SetDefine(TEXT("VF_YQ_PHYSICS"), 1);
}

void FYQPhysicsVertexFactory::ValidateCompiledResult(const FVertexFactoryType* Type, EShaderPlatform Platform, const FShaderParameterMap& ParameterMap, TArray<FString>& OutErrors)
{
	
}


void FYQPhysicsVertexFactory::SetData(const FStaticMeshDataType& InData) {
	check(IsInRenderingThread());

	// The shader code makes assumptions that the color component is a FColor, performing swizzles on ES3 and Metal platforms as necessary
	// If the color is sent down as anything other than VET_Color then you'll get an undesired swizzle on those platforms
	check((InData.ColorComponent.Type == VET_None) || (InData.ColorComponent.Type == VET_Color));

	Data = InData;
	UpdateRHI();
}

void FYQPhysicsVertexFactory::InitRHI()
{

	FYQPhysicsUniformParameters Parameters;
	//Parameters.ClothStructureBuffer = StructuredVertexSRV;
	Parameters.YQPhysicsVertexOffset = BufferIDOffset;
	UniformBuffer = FYQPhysicsUniformBufferRef::CreateUniformBufferImmediate(Parameters, UniformBuffer_MultiFrame);


	// ----------------- LocalVertexFactory -------------------
	if (Data.PositionComponent.VertexBuffer != Data.TangentBasisComponents[0].VertexBuffer) {
		auto AddDeclaration = [this](EVertexInputStreamType InputStreamType, bool bAddNormal) {
			FVertexDeclarationElementList StreamElements;
			StreamElements.Add(AccessStreamComponent(Data.PositionComponent, 0, InputStreamType));

			bAddNormal = bAddNormal && Data.TangentBasisComponents[1].VertexBuffer != NULL;
			if (bAddNormal) {
				StreamElements.Add(AccessStreamComponent(Data.TangentBasisComponents[1], 1, InputStreamType));
			}

			AddPrimitiveIdStreamElement(InputStreamType, StreamElements, 1, 8);

			InitDeclaration(StreamElements, InputStreamType);
		};

		AddDeclaration(EVertexInputStreamType::PositionOnly, false);
		AddDeclaration(EVertexInputStreamType::PositionAndNormalOnly, true);
	}

	FVertexDeclarationElementList Elements;
	if (Data.PositionComponent.VertexBuffer != nullptr) {
		Elements.Add(AccessStreamComponent(Data.PositionComponent, 0));
	}

	AddPrimitiveIdStreamElement(EVertexInputStreamType::Default, Elements, 13, 8);

	// Only the tangent and normal are used by the stream; the bitangent is derived in the shader.
	uint8 TangentBasisAttributes[2] = { 1, 2 };
	for (int32 AxisIndex = 0; AxisIndex < 2; AxisIndex++) {
		if (Data.TangentBasisComponents[AxisIndex].VertexBuffer != nullptr) {
			Elements.Add(AccessStreamComponent(Data.TangentBasisComponents[AxisIndex], TangentBasisAttributes[AxisIndex]));
		}
	}

	if (Data.ColorComponentsSRV == nullptr) {
		Data.ColorComponentsSRV = GNullColorVertexBuffer.VertexBufferSRV;
		Data.ColorIndexMask = 0;
	}

	//ColorStreamIndex = -1;
	//if (Data.ColorComponent.VertexBuffer) {
	//	Elements.Add(AccessStreamComponent(Data.ColorComponent, 3));
	//	ColorStreamIndex = Elements.Last().StreamIndex;
	//}
	//else {
	//	// If the mesh has no color component, set the null color buffer on a new stream with a stride of 0.
	//	// This wastes 4 bytes per vertex, but prevents having to compile out twice the number of vertex factories.
	//	FVertexStreamComponent NullColorComponent(&GNullColorVertexBuffer, 0, 0, VET_Color, EVertexStreamUsage::ManualFetch);
	//	Elements.Add(AccessStreamComponent(NullColorComponent, 3));
	//	ColorStreamIndex = Elements.Last().StreamIndex;
	//}
	UE_LOG(LogTemp, Log, TEXT("Data.TextureCoordinates.Num %d"), Data.TextureCoordinates.Num())
	if (Data.TextureCoordinates.Num()) {
		const int32 BaseTexCoordAttribute = 3;
		for (int32 CoordinateIndex = 0; CoordinateIndex < Data.TextureCoordinates.Num(); ++CoordinateIndex) {
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[CoordinateIndex],
				BaseTexCoordAttribute + CoordinateIndex
			));
		}

		for (int32 CoordinateIndex = Data.TextureCoordinates.Num(); CoordinateIndex < MAX_STATIC_TEXCOORDS / 2; ++CoordinateIndex) {
			Elements.Add(AccessStreamComponent(
				Data.TextureCoordinates[Data.TextureCoordinates.Num() - 1],
				BaseTexCoordAttribute + CoordinateIndex
			));
		}
	}

	/*if (Data.LightMapCoordinateComponent.VertexBuffer) {
		Elements.Add(AccessStreamComponent(Data.LightMapCoordinateComponent, 15));
	}
	else if (Data.TextureCoordinates.Num()) {
		Elements.Add(AccessStreamComponent(Data.TextureCoordinates[0], 15));
	}*/

	check(Streams.Num() > 0);

	InitDeclaration(Elements);


	// ---------- ¶¯Ì¬Buffer -------------
	FStaticMeshLODResources& StaticMeshResourceLOD0 = RenderData->LODResources[0];

	FStaticMeshSectionArray& StaticMeshSectionArray = StaticMeshResourceLOD0.Sections;

	FRawStaticIndexBuffer& MeshIndexBuffer = StaticMeshResourceLOD0.IndexBuffer;

	FStaticMeshVertexBuffers& MeshVertexBuffers = StaticMeshResourceLOD0.VertexBuffers;

	int32 NumVertices = StaticMeshResourceLOD0.GetNumVertices();

	DynamicNormalBuffer.Initialize(TEXT("DynamicNormalBuffer"), sizeof(uint32), NumVertices, PF_R8G8B8A8, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);
	DynamicTangentBuffer.Initialize(TEXT("DynamicTangentBuffer"), sizeof(uint32), NumVertices, PF_R8G8B8A8, BUF_UnorderedAccess | BUF_ShaderResource, nullptr);

	TexCoordBufferSRV = MeshVertexBuffers.StaticMeshVertexBuffer.GetTexCoordsSRV();
	if (TexCoordBufferSRV == nullptr)
	{

	}
}

void FYQPhysicsVertexFactory::ReleaseRHI()
{
	UniformBuffer.SafeRelease();
	FVertexFactory::ReleaseRHI();
}


FShaderResourceViewRHIRef FYQPhysicsVertexFactory::GetDynamicNormalBufferSRV()
{
	if (DynamicNormalBuffer.Buffer != nullptr)
	{
		return DynamicNormalBuffer.SRV;
	}

	return nullptr;
}

FShaderResourceViewRHIRef FYQPhysicsVertexFactory::GetDynamicTangentBufferSRV()
{
	if (DynamicTangentBuffer.Buffer != nullptr)
	{
		return DynamicTangentBuffer.SRV;
	}

	return nullptr;
}

FUnorderedAccessViewRHIRef FYQPhysicsVertexFactory::GetDynamicNormalBufferUAV()
{
	if (DynamicNormalBuffer.Buffer != nullptr)
	{
		return DynamicNormalBuffer.UAV;
	}

	return nullptr;
}

FUnorderedAccessViewRHIRef FYQPhysicsVertexFactory::GetDynamicTangentBufferUAV()
{
	if (DynamicTangentBuffer.Buffer != nullptr)
	{
		return DynamicTangentBuffer.UAV;
	}

	return nullptr;
}

IMPLEMENT_VERTEX_FACTORY_PARAMETER_TYPE(FYQPhysicsVertexFactory, SF_Vertex, FYQPhysicsVertexFactoryShaderParametersVS);
//IMPLEMENT_VERTEX_FACTORY_TYPE_EX(FYQPhysicsVertexFactory, TEXT("/YQLearnPhysics/YQPhysicsVertexFactory.ush"), true, false, true, false, true, true, true);

IMPLEMENT_VERTEX_FACTORY_TYPE(FYQPhysicsVertexFactory, "/YQLearnPhysics/YQPhysicsVertexFactory.ush",
	EVertexFactoryFlags::UsedWithMaterials
	//| EVertexFactoryFlags::SupportsStaticLighting
	| EVertexFactoryFlags::SupportsDynamicLighting
	//| EVertexFactoryFlags::SupportsPrecisePrevWorldPos
	| EVertexFactoryFlags::SupportsPositionOnly
	| EVertexFactoryFlags::SupportsCachingMeshDrawCommands
	//| EVertexFactoryFlags::SupportsPrimitiveIdStream
	//| EVertexFactoryFlags::SupportsRayTracing
	//| EVertexFactoryFlags::SupportsRayTracingDynamicGeometry
);
