// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StolenMatesPlayer.h"
#include "AbilityBaseClass.generated.h"

UCLASS()
class STOLENMATES_API AAbilityBaseClass : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAbilityBaseClass();
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
		void fireAbility(AStolenmatesPlayer* abilityUser);
protected:
};
