// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "StolenmatesPlayer.h"

// Sets default values
ACamera::ACamera()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComp->SetupAttachment(RootComponent);
	CameraComp->AddLocalRotation(FRotator(270,0, 0));
}

// Called when the game starts or when spawned
void ACamera::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACamera::Focus(TArray<ACharacter*> players)
{
	FVector centerPoint = FVector(0, 0, 0);
	for (int i = 0; i < players.Num(); i++)
	{
		centerPoint.X += players[i]->GetActorLocation().X;
		centerPoint.Y += players[i]->GetActorLocation().Y;
	}
	centerPoint = centerPoint / players.Num();
	float farthestDistance = 0;
	for (int i = 0; i < players.Num(); i++)
	{
		for (int j = 0; j < players.Num(); j++)
		{
			float distance = FVector::Dist(players[i]->GetActorLocation(), players[j]->GetActorLocation());
			if (distance > farthestDistance)
				farthestDistance = distance;
		}
	}
	centerPoint.Z = FMath::Clamp(farthestDistance*2.0f,minHeight,maxHeight);
	this->SetActorLocation(centerPoint);
}

