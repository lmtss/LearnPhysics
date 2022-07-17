// Fill out your copyright notice in the Description page of Project Settings.


#include "YQPhysicsWorldSubsystem.h"
#include "ConstraintsGenerate.h"

FShaderResourceViewRHIRef IndexBufferSRV;

void UYQPhysicsBlueprintLibrary::TestGenerateDistanceConstraints(const UObject* WorldContextObject, UStaticMesh* StaticMesh)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return;
	if (World->HasSubsystem < UYQPhysicsWorldSubsystem>())
	{
		UYQPhysicsWorldSubsystem* System = World->GetSubsystem < UYQPhysicsWorldSubsystem>();

		System->GetGPUPhysicsScene();

		FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();

		FStaticMeshLODResources& StaticMeshResourceLOD0 = RenderData->LODResources[0];

		FStaticMeshSectionArray& StaticMeshSectionArray = StaticMeshResourceLOD0.Sections;

		FRawStaticIndexBuffer& MeshIndexBuffer = StaticMeshResourceLOD0.IndexBuffer;
		FBufferRHIRef IndexBufferRHI = MeshIndexBuffer.IndexBufferRHI;

		FStaticMeshVertexBuffers& MEshVertexBuffers = StaticMeshResourceLOD0.VertexBuffers;
		FBufferRHIRef VertexBufferRHI = MEshVertexBuffers.PositionVertexBuffer.VertexBufferRHI;

		bool bIsIndexBuffer32 = MeshIndexBuffer.Is32Bit();

		int NumVertices = StaticMeshResourceLOD0.GetNumVertices();
		int NumTriangles = StaticMeshResourceLOD0.GetNumTriangles();

		FRHIShaderResourceView* ColorBufferSRV = MEshVertexBuffers.ColorVertexBuffer.GetColorComponentsSRV();

		if (IndexBufferRHI == nullptr || IndexBufferRHI.IsValid() == false)return;

		//UE_LOG(LogTemp, Log, TEXT("GenerateDistanceConstraintsFromMesh"))

		ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
			[=](FRHICommandListImmediate& RHICmdList)
		{
			if (!IndexBufferSRV.IsValid())
			{
				/*IndexBufferSRV = RHICreateShaderResourceView(
					IndexBufferRHI
					, bIsIndexBuffer32 ? 4 : 2
					, bIsIndexBuffer32 ? PF_R32_UINT : PF_R16_UINT
				);*/
			}
			
			FShaderResourceViewRHIRef IndexBuffer = RHICreateShaderResourceView(
				IndexBufferRHI
				, bIsIndexBuffer32 ? 4 : 2
				, bIsIndexBuffer32 ? PF_R32_UINT : PF_R16_UINT
			);

			FShaderResourceViewRHIRef PositionBuffer = RHICreateShaderResourceView(
				VertexBufferRHI
				, 4
				, PF_R32_FLOAT
			);

			GenerateDistanceConstraintsFromMesh(
				RHICmdList
				, nullptr, nullptr, nullptr
				, IndexBuffer
				, PositionBuffer
				, ColorBufferSRV
				, 0
				, NumTriangles
				, true
				);

			IndexBuffer.SafeRelease();
			PositionBuffer.SafeRelease();
		});
	}
	
}

void UYQPhysicsBlueprintLibrary::AddCPUObjectToPhysicsScene(const UObject* WorldContextObject, USceneComponent* InActor)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (World->HasSubsystem < UYQPhysicsWorldSubsystem>())
	{
		UYQPhysicsWorldSubsystem* System = World->GetSubsystem < UYQPhysicsWorldSubsystem>();

		FYQPhysicsScene* Scene = System->GetGPUPhysicsScene();
		Scene->AddCPUActorToScene(InActor);
	}
}

void UYQPhysicsBlueprintLibrary::UpdateCPUObject(const UObject* WorldContextObject, USceneComponent* InActor)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (World->HasSubsystem < UYQPhysicsWorldSubsystem>())
	{
		UYQPhysicsWorldSubsystem* System = World->GetSubsystem < UYQPhysicsWorldSubsystem>();

		FYQPhysicsScene* Scene = System->GetGPUPhysicsScene();
		Scene->UpdateCPUActor(InActor);
	}
}


FVector UYQPhysicsBlueprintLibrary::GetCPUObjectFeedBackForce(const UObject* WorldContextObject, USceneComponent* InActor)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (World->HasSubsystem < UYQPhysicsWorldSubsystem>())
	{
		UYQPhysicsWorldSubsystem* System = World->GetSubsystem < UYQPhysicsWorldSubsystem>();

		FYQPhysicsScene* Scene = System->GetGPUPhysicsScene();
		return Scene->GetCPUObjectFeedBack_GameThread(InActor);
	}

	return FVector();
}


UYQPhysicsWorldSubsystem::UYQPhysicsWorldSubsystem() 
{
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsWorldSubsystem::UYQPhysicsWorldSubsystem"));

	GPUPhysicsScene = nullptr;
}

bool UYQPhysicsWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	if (UWorld* WorldOuter = Cast<UWorld>(Outer)) 
	{
		return true;
	}

	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsWorldSubsystem::ShouldCreateSubsystem False"))

	return false;
}

void UYQPhysicsWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection) 
{
	Super::Initialize(Collection);
	
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsWorldSubsystem::Initialize"));

	bPause = false;

	GPUPhysicsScene = new FYQPhysicsScene();
	
	GPUPhysicsSimulator = new FYQPhysicsSimulator();
	GPUPhysicsSimulator->SetPhysicsScene(GPUPhysicsScene);

	//FWorldDelegates::OnWorldCleanup.AddUObject(this, &UYQPhysicsWorldSubsystem::OnWorldEndPlay);
#if WITH_EDITORONLY_DATA
	FEditorDelegates::EndPIE.AddUObject(this, &UYQPhysicsWorldSubsystem::OnEndPIE);
#endif

	ViewExtension = FSceneViewExtensions::NewExtension<FYQPhysicsViewExtension>(GetWorld(), GPUPhysicsSimulator, GPUPhysicsScene);
}

void UYQPhysicsWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();

	delete GPUPhysicsScene;
}

FYQPhysicsScene* UYQPhysicsWorldSubsystem::GetGPUPhysicsScene()
{
	return GPUPhysicsScene;
}

bool UYQPhysicsWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const 
{
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsWorldSubsystem::DoesSupportWorldType %d"), WorldType)
		return WorldType == EWorldType::Game || WorldType == EWorldType::Editor || WorldType == EWorldType::PIE;// || WorldType == EWorldType::EditorPreview;
}

void UYQPhysicsWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsWorldSubsystem::OnWorldBeginPlay %d"), InWorld.WorldType);

	if (InWorld.WorldType == EWorldType::Editor)
	{
		bPause = true;
	}
#if WITH_EDITOR
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::Editor)
		{
			UWorld* World = Context.World();
			if (World)
			{
				if (World->HasSubsystem< UYQPhysicsWorldSubsystem>())
				{
					UYQPhysicsWorldSubsystem* System = World->GetSubsystem<UYQPhysicsWorldSubsystem>();
					System->Pause();
				}
			}
		}
	}
#endif
}


void UYQPhysicsWorldSubsystem::OnEndPIE(const bool bIsSimulating)
{
	Continue();
}

TStatId UYQPhysicsWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYQPhysicsWorldSubsystem, STATGROUP_Tickables);
}

void UYQPhysicsWorldSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	/*if(!bPause)
		GPUPhysicsSimulator->Tick(DeltaTime);*/

	if (!bPause)
	{
		//FScene* Scene = static_cast<FScene*>(GetWorld()->Scene);
		FScene* Scene = nullptr;
		if (GetWorld()->Scene)
		{
			Scene = GetWorld()->Scene->GetRenderScene();
		}
		
		FYQPhysicsScene* PhysicsScene = GPUPhysicsScene;
		FYQPhysicsSimulator* Simulator = GPUPhysicsSimulator;
		ENQUEUE_RENDER_COMMAND(FYQPhysicsSceneProxy_Initialize)(
			[=](FRHICommandListImmediate& RHICmdList)
		{
			PhysicsScene->Tick(RHICmdList);
			Simulator->SetScene(Scene);
			Simulator->Tick_RenderThread(RHICmdList, 0.03);
		});
	}
}

void UYQPhysicsWorldSubsystem::BeginDestroy() 
{
	UE_LOG(LogTemp, Log, TEXT("UYQPhysicsWorldSubsystem::BeginDestroy"))

	Super::BeginDestroy();
}

void UYQPhysicsWorldSubsystem::Pause()
{
	bPause = true;

	ViewExtension->DisableSimulate();
}

void UYQPhysicsWorldSubsystem::Continue()
{
	bPause = false;

	ViewExtension->EnableSimulate();
}