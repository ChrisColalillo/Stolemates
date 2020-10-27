// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/UnrealMathUtility.h"
#include "StolenmatesPlayer.h"
#include "GenericPlatform/GenericPlatformMath.h"

#include "Engine.h"


// Sets default values
ACamera::ACamera()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
}

// Called when the game starts or when spawned
void ACamera::BeginPlay()
{
	Super::BeginPlay();
	//CameraComp->AddLocalRotation(CameraRotation);
	this->SetActorRotation(CameraRotation);
}

// Called every frame
void ACamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACamera::Focus(TArray<ACharacter*> players)
{
//	FVector centerPoint = FVector(0, 0, 0);
//	for (int i = 0; i < players.Num(); i++)
//	{
//		centerPoint.X += players[i]->GetActorLocation().X;
//		centerPoint.Y += players[i]->GetActorLocation().Y;
//	}
//	float farthestDistance = 0;
//	float x, y, distance;
//	for (int i = 0; i < players.Num(); i++)
//	{
//		for (int j = 0; j < players.Num(); j++)
//		{
//			x = FMath::Abs(players[i]->GetActorLocation().X - players[j]->GetActorLocation().X)*1.77;
//			y = FMath::Abs(players[i]->GetActorLocation().Y - players[j]->GetActorLocation().Y);
//			distance = (x > y) ? x : y;
//			if (distance > farthestDistance)
//			{
//				farthestDistance = distance;
//			}
//		}
//	}
//
//	float rotateAmount = 1.0f;
//	if (GetActorLocation().Z <= rotationStartHeight)
//	{
//		rotateAmount = FMath::Clamp((GetActorLocation().Z - rotationEndHeight) / (rotationStartHeight - minHeight), 0.0f, 1.0f);
//		this->SetActorRotation(FRotator((maxAddedRotationAngle * (1.0f - rotateAmount)), 0.0f, 0.0f) + CameraRotation);
//	}
//	else
//	{
//		this->SetActorRotation(CameraRotation);
//	}
//	centerPoint.X -= FMath::Sin(CameraRotation.Yaw - 270)*(GetActorLocation().Z / 2);
//
//	float height = FMath::Clamp(farthestDistance, minHeight, maxHeight);
//	height -= heightRotationOffset * (1.0f - rotateAmount);
//	float sign = FMath::Sign(height - GetActorLocation().Z);
//	height = FMath::Clamp(FMath::Abs(height - GetActorLocation().Z),0.0f, cameraMaxVerticalMoveSpeed);
//
//	centerPoint = centerPoint / players.Num();
//	centerPoint.Z = GetActorLocation().Z;
//	centerPoint.X -= xAxisRotationOffset * (1.0f - rotateAmount);
//	FVector newLocationDirection = centerPoint - GetActorLocation();
//	float magnitude = newLocationDirection.Size();
//	newLocationDirection.Normalize();
//	newLocationDirection *= FMath::Clamp(magnitude, 0.0f, cameraMaxHorizontalMoveSpeed);
//
//
//	this->SetActorLocation(FVector(GetActorLocation().X + newLocationDirection.X, GetActorLocation().Y + newLocationDirection.Y, GetActorLocation().Z + (height * sign)));
//
}

