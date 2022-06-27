// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "Components/MeshComponent.h"

#include "YQPhysicsScene.h"

#include "YQPhysicsComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class YQLEARNPHYSICS_API UYQPhysicsComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UYQPhysicsComponent();
	
	
	UPROPERTY(EditAnywhere)
		UMaterialInterface* Material;
	
	
	
	
	
	
	
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;

	virtual int32 GetNumMaterials() const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
protected:

public:	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable)
	void SetSpherePosition(FVector4 PosAndRadius);

	UFUNCTION(BlueprintCallable)
		FVector4 GetFeedback();


	FVector4 SpherePos = FVector4(150.0, 0.0, -220.0, 30.0);

private:
	bool bIsInitialized = false;
		

	FVector4 SphereFeedback = FVector4(0.0, 0.0, 0.0, 0.0);

	FYQPhysicsProxy* PhysicsProxy = nullptr;
};
