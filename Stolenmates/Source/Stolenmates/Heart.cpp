// Fill out your copyright notice in the Description page of Project Settings.


#include "Heart.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AHeart::AHeart()
{
	PrimaryActorTick.bCanEverTick = false;

	heartMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("Heart Mesh")
			);

	SetRootComponent(heartMesh);

	heartMesh->AddLocalRotation(
		FRotator(0.0f, 90.0f, 0.0f)
	);

	heartCollider =
		CreateDefaultSubobject<UCapsuleComponent>(
			TEXT("Heart Collider")
			);

	heartCollider->SetupAttachment(heartMesh);
}