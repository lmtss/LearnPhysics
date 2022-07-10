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

	uint8 bUseBendingConstraints : 1;
	uint8 bUseDistanceBendingConstraints : 1;
};

/**
 * 
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class YQLEARNPHYSICS_API UYQPhysicsMeshComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = StaticMesh, meta = (AllowPrivateAccess = "true"))
		TObjectPtr<class UStaticMesh> StaticMesh;

protected:

	

public:
	UFUNCTION(BlueprintCallable, Category = "Components|StaticMesh")
		virtual bool SetStaticMesh(class UStaticMesh* NewMesh);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cloth)
		uint8 bUseBendingConstraints : 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Cloth)
		uint8 bUseDistanceBendingConstraints : 1;


	//~ Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	//~ End UPrimitiveComponent Interface

	//~ Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	//~ End USceneComponent Interface


	//~ Begin UActorComponent Interface
	virtual void OnRegister() override;
	virtual void OnCreatePhysicsState() override;
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void SendRenderTransform_Concurrent() override;
	virtual bool ShouldCreateRenderState() const override;
	virtual bool ShouldCreatePhysicsState() const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ End UActorComponent Interface

	//~ Begin UObject Interface.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;
	//~ End UObject Interface.


	

	FYQPhysicsVertexFactory* GetVertexFactory() const;

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
	FYQPhysicsVertexFactory* VertexFactory;



	bool ShouldCreateGPUPhysicsState() const;
	bool IsGPUPhysicsStateCreated() const;

	void CreateGPUPhysicsState();
	void CreateVertexFactory();
	void DestroyVertexFactory();

};


class FYQPhysicsMeshSceneProxy final : public FPrimitiveSceneProxy//, public IYQRenderSceneProxy
{
public:
	FYQPhysicsMeshSceneProxy(const UYQPhysicsMeshComponent* InComponent, uint32 InBufferIndexOffset);

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

	//virtual void GetRenderDataRequest(FRenderDataRequestBatch& OutBatch) const override;

	FYQPhysicsVertexFactory* VertexFactory;
	FMaterialRenderProxy* MaterialRenderProxy;
	FMaterialRelevance MaterialRelevance;

	FStaticMeshRenderData* RenderData;

	// 在物理场景的IndexBuffer中的偏移, Cache在这里
	uint32 BufferIndexOffset;


	FYQPhysicsScene* GPUPhysicsScene;

};

class FYQPhysicsMeshRenderDataProxy : public IYQRenderSceneProxy
{
public:

	FYQPhysicsMeshRenderDataProxy(const UYQPhysicsMeshComponent* InComponent);
	virtual void GetRenderDataRequest(FRenderDataRequestBatch& OutBatch) const override;


	FYQPhysicsVertexFactory* VertexFactory;
};




