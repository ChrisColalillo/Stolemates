// Fill out your copyright notice in the Description page of Project Settings.


#include "Heart.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AHeart::AHeart()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	heartMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Heart Mesh"));
	heartMesh->AddLocalRotation(FRotator(0, 90, 0));
	heartMesh->SetupAttachment(RootComponent);
	heartCollider = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Heart Collider"));
	heartCollider->SetupAttachment(heartMesh);
}

// Called when the game starts or when spawned
void AHeart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHeart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

