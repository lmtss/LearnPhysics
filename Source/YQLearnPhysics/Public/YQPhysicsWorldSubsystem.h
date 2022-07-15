// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YQPhysicsScene.h"
#include "YQPhysicsSimulator.h"
#include "YQPhysicsViewExtension.h"

#if WITH_EDITORONLY_DATA
#include "Editor.h"
#include "Settings/LevelEditorPlaySettings.h"
#endif // WITH_EDITORONLY_DATA

#include "YQPhysicsWorldSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class YQLEARNPHYSICS_API UYQPhysicsWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	UYQPhysicsWorldSubsystem();

	// FTickableGameObject implementation Begin
	virtual bool IsTickable() const override { return !bPause; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	ETickableTickType GetTickableTickType() const override {
		return ETickableTickType::Always;
	}
	// FTickableGameObject implementation End

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// UWorldSubsystem implementation Begin
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;


	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	// UWorldSubsystem implementation End

	void OnEndPIE(const bool bIsSimulating);

	// UObject implementation Begin
	virtual void BeginDestroy() override;
	// UObject implementation End

	FYQPhysicsScene* GetGPUPhysicsScene();
	FYQPhysicsSimulator* GetGPUSimulator()
	{
		return GPUPhysicsSimulator;
	}

	void Pause();

	void Continue();

	bool IsPause()
	{
		return bPause;
	}

private:
	FYQPhysicsScene* GPUPhysicsScene;
	FYQPhysicsSimulator* GPUPhysicsSimulator;

	bool bPause;

	TSharedPtr<FYQPhysicsViewExtension, ESPMode::ThreadSafe> ViewExtension;
};
