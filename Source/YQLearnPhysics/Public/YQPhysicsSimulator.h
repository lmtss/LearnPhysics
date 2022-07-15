#pragma once

#include "RenderResource.h"
#include "CoreMinimal.h"
#include "RHICommandList.h"

#include "Engine/Engine.h"
#include "TickableEditorObject.h"


#include "Engine/StaticMesh.h"

#include "YQPhysicsScene.h"
#include "YQPhysicsClothSim.h"

#include "UObject/ObjectMacros.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Renderer/Private/ScenePrivate.h"


#include "YQPhysicsSimulator.generated.h"

UCLASS(meta = (ScriptName = "YQPhysicsLibrary"))
class UYQPhysicsBlueprintLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = CPUObject)
		static int AddStaticMeshActorToPhysicsScene(UStaticMeshComponent* StaticMeshComponent);

	UFUNCTION(BlueprintCallable, Category = Test, meta = (WorldContext = "WorldContextObject"))
		static void TestGenerateDistanceConstraints(const UObject* WorldContextObject, UStaticMesh* StaticMesh);

	UFUNCTION(BlueprintCallable, Category = CPUObject, meta = (WorldContext = "WorldContextObject"))
		static void AddCPUObjectToPhysicsScene(const UObject* WorldContextObject, USceneComponent* InActor);

	UFUNCTION(BlueprintCallable, Category = CPUObject, meta = (WorldContext = "WorldContextObject"))
		static void UpdateCPUObject(const UObject* WorldContextObject, USceneComponent* InActor);

	UFUNCTION(BlueprintCallable, Category = CPUObject, meta = (WorldContext = "WorldContextObject"))
		static FVector GetCPUObjectFeedBackForce(const UObject* WorldContextObject, USceneComponent* InActor);
};


class FYQPhysicsSimulator
{
public:
	FYQPhysicsSimulator();
	~FYQPhysicsSimulator();

	void SetPhysicsScene(FYQPhysicsScene* InScene);
	void SetScene(FScene* InScene);

	void Tick(float DeltaSeconds);

	void Tick_RenderThread(FRHICommandList& RHICmdList, float DeltaSeconds);

private:

	void Substep(FRHICommandList& RHICmdList, uint32 IterSubStep, uint32 NumSubSteps, float SubstepTime);

	void SolveExternalForce(FRHICommandList& RHICmdList, float DeltaTime);

	FYQPhysicsScene* PhysicsScene = nullptr;
	FScene* Scene;

};