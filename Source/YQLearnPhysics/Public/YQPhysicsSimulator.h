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

#include "YQPhysicsViewExtension.h"

#include "YQPhysicsSimulator.generated.h"

UCLASS(meta = (ScriptName = "YQPhysicsLibrary"))
class UYQPhysicsBlueprintLibrary : public UBlueprintFunctionLibrary {
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Particle)
		static void InitializeParticle();

	UFUNCTION(BlueprintCallable, Category = Particle)
		static void OnEndPlay();

	UFUNCTION(BlueprintCallable, Category = CPUObject)
		static int AddObjectToPhysicsScene();

	UFUNCTION(BlueprintCallable, Category = CPUObject)
		static FVector4 GetCPUObjectFeedback(int ObjectID);

	UFUNCTION(BlueprintCallable, Category = CPUObject)
		static void SetCPUObjectPositionAndVelocity(int ObjectID, FVector4 PosAndRadius, FVector Velocity);

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

	void Tick(float DeltaSeconds);

private:

	void Tick_RenderThread(FRHICommandList& RHICmdList, float DeltaSeconds);
	

	FYQPhysicsScene* Scene = nullptr;
	static FYQPhysicsSimulator* Instance;

	TSharedPtr<FYQPhysicsViewExtension, ESPMode::ThreadSafe> ViewExtension;
};