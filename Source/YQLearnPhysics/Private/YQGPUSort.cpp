#include "YQGPUSort.h"

#include "GlobalShader.h"
#include "ShaderParameterUtils.h"
#include "ShaderCore.h"
#include "PipelineStateCache.h"
#include "RHIStaticStates.h"
#include "SceneUtils.h"
#include "SceneInterface.h"
#include "RHI.h"

class FYQSimpleRadixSortCS : public FGlobalShader {
public:
	DECLARE_SHADER_TYPE(FYQSimpleRadixSortCS, Global);
	SHADER_USE_PARAMETER_STRUCT(FYQSimpleRadixSortCS, FGlobalShader);

	static constexpr uint32 ThreadGroupSize = 32;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_UAV(Buffer<uint>, KeyBuffer)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputBuffer)
		END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
		UE_LOG(LogTemp, Log, TEXT("FYQSimpleRadixSortCS %d %d"), (int)Parameters.Platform, IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::Num))
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM6);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment) {
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		//OutEnvironment.SetDefine(TEXT("INITIALIZE_POSITION_SAMPLE_HORIZ"), 1);
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}

};

IMPLEMENT_SHADER_TYPE(, FYQSimpleRadixSortCS, TEXT("/YQLearnPhysics/YQRadixSort.usf"), TEXT("MainCS"), SF_Compute);