// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera.generated.h"


class ACharacter;
class UCameraComponent;
class USpringArmComponent;
UCLASS()
class STOLENMATES_API ACamera : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACamera();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float minHeight = 1000;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float maxHeight = 4000;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float cameraMaxHorizontalMoveSpeed = 10;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float cameraMaxVerticalMoveSpeed = 10;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		FRotator CameraRotation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float rotationStartHeight = 4000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float rotationEndHeight = 2250.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float maxAddedRotationAngle = 65.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float heightRotationOffset = 250.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Settings")
		float xAxisRotationOffset = 2500.0f;


	UCameraComponent* CameraComp;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void Focus(TArray<ACharacter*> players);

};
