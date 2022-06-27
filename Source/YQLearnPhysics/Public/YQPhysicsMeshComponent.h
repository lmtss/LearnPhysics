// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "YQPhysicsVertexFactory.h"
#include "YQPhysicsScene.h"
#include "YQPhysicsMeshComponent.generated.h"

class FYQMeshPhysicsProxy final : public FYQPhysicsProxy
{
public:
	FYQMeshPhysicsProxy(const UYQPhysicsMeshComponent* InComponent);

	virtual void GetDynamicPhysicsConstraints(FConstraintsBatch& OutBatch) const;
private:

};

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class YQLEARNPHYSICS_API UYQPhysicsMeshComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = StaticMesh, ReplicatedUsing = OnRep_StaticMesh, meta = (AllowPrivateAccess = "true"))
		TObjectPtr<class UStaticMesh> StaticMesh;

	UFUNCTION()
		void OnRep_StaticMesh(class UStaticMesh* OldStaticMesh);

protected:

	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;

	virtual bool ShouldCreateRenderState() const override;

public:
	UFUNCTION(BlueprintCallable, Category = "Components|StaticMesh")
		virtual bool SetStaticMesh(class UStaticMesh* NewMesh);

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

	TObjectPtr<UStaticMesh> GetStaticMesh() const
	{
#if WITH_EDITOR
		// This should never happen and is a last resort, we should have catched the property overwrite well before we reach this code
		/*if (KnownStaticMesh != StaticMesh) {
			OutdatedKnownStaticMeshDetected();
		}*/
#endif
		return StaticMesh;
	}

private:
	FYQMeshPhysicsProxy* GPUPhysicsProxy;
	bool IsCreateRenderStatePending;
};


class FYQPhysicsMeshSceneProxy final : public FPrimitiveSceneProxy {
public:
	FYQPhysicsMeshSceneProxy(const UYQPhysicsMeshComponent* InComponent, FYQPhysicsSceneBufferEntry& InPhysicsSceneBufferEntry);

	~FYQPhysicsMeshSceneProxy();

	SIZE_T GetTypeHash() const override;

	virtual bool GetMeshElement(
		int32 LODIndex,
		int32 ElementIndex,
		uint8 InDepthPriorityGroup,
		FMeshBatch& OutMeshBatch) const;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const;

	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;


	FYQPhysicsVertexFactory VertexFactory;
	FMaterialRenderProxy* MaterialRenderProxy;
	FMaterialRelevance MaterialRelevance;

	FStaticMeshRenderData* RenderData;

	// 在物理场景的IndexBuffer中的偏移, Cache在这里
	FYQPhysicsSceneBufferEntry& PhysicsSceneBufferEntry;


	FYQPhysicsScene* GPUPhysicsScene;
};




