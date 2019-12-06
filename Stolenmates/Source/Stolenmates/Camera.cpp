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
}

// Called when the game starts or when spawned
void ACamera::BeginPlay()
{
	Super::BeginPlay();
	CameraComp->AddLocalRotation(CameraRotation);
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
	float x, y, distance;
	for (int i = 0; i < players.Num(); i++)
	{
		for (int j = 0; j < players.Num(); j++)
		{
			x = FMath::Abs(players[i]->GetActorLocation().X - players[j]->GetActorLocation().X)*1.77;
			y = FMath::Abs(players[i]->GetActorLocation().Y - players[j]->GetActorLocation().Y);
			distance = (x > y) ? x : y;
			if (distance > farthestDistance)
			{
				farthestDistance = distance;
				//centerPoint = (players[i]->GetActorLocation()+ players[j]->GetActorLocation())/2;
			}
		}
	}
	centerPoint.Z += FMath::Clamp(farthestDistance,minHeight,maxHeight);
	centerPoint.X -= FMath::Sin(CameraRotation.Yaw - 270)*(centerPoint.Z/2);
	FVector newLocationDirection = centerPoint- GetActorLocation();
	float magnitude = newLocationDirection.Size();
	newLocationDirection.Normalize();
	newLocationDirection *= FMath::Clamp(magnitude,0.0f,cameraMaxMoveSpeed);
	this->SetActorLocation(GetActorLocation()+newLocationDirection);
}

