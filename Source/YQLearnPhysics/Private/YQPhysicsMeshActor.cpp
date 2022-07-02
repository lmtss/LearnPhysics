// Fill out your copyright notice in the Description page of Project Settings.


#include "YQPhysicsMeshActor.h"

// Sets default values
AYQPhysicsMeshActor::AYQPhysicsMeshActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SetCanBeDamaged(false);

	PhysicsMeshComponent = CreateDefaultSubobject<UYQPhysicsMeshComponent>(TEXT("YQPhysicsMeshComponent0"));
	PhysicsMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	PhysicsMeshComponent->Mobility = EComponentMobility::Static;
	PhysicsMeshComponent->SetGenerateOverlapEvents(false);
	//PhysicsMeshComponent->bUseDefaultCollision = true;

	RootComponent = PhysicsMeshComponent;
}

// Called when the game starts or when spawned
void AYQPhysicsMeshActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AYQPhysicsMeshActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

