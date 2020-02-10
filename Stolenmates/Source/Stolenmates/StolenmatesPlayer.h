#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StolenmatesPlayer.generated.h"


UENUM()
enum class AbilitiesENUM : uint8
{
	ITEM_BOX_ABILITY     UMETA(DisplayName = "ability picked up from item box"),
	ZONE_ABILITY_OVERRIDE    UMETA(DisplayName = "ability override from area"),
};

class AHeart;
class AAbilityBaseClass;
UCLASS()
class STOLENMATES_API AStolenmatesPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	AStolenmatesPlayer();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	UPROPERTY(BlueprintReadOnly, Category = "PlayerInfo")
	bool stunned = false;
	UPROPERTY(BlueprintReadOnly, Category = "PlayerInfo")
	bool invincible = false;
	UPROPERTY(BlueprintReadWrite, Category = "PlayerInfo")
	bool hasHeart = false;
	UPROPERTY(BlueprintReadOnly, Category = "PlayerInfo")
	bool dashReady = true;

	AHeart* heart;
	float timeHoldingHeart = 0;
	bool gameOver = false;
protected:
	virtual void BeginPlay() override;
	virtual void LeftRightAxis(float val);
	virtual void ForwardBackAxis(float val);
	virtual void JumpPressed();
	virtual void JumpReleased();
	virtual void DashPressed();
	virtual void DashReset();
	virtual void EndStun();
	virtual void EndInvincibility();
	virtual void UseAbility();
	virtual void SetInvincibility(float iTime);


	UFUNCTION(BlueprintCallable, Category = "Player")
	virtual void StunPlayer(float StunDuration);
	UFUNCTION(BlueprintCallable, Category = "Player")
	virtual void SetAbility(AbilitiesENUM ability, AAbilityBaseClass* staticAbility);
	UFUNCTION()
	void OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void OnCompOverlap(UPrimitiveComponent* OverlappedComponent,AActor* OtherActor,UPrimitiveComponent* OtherComp,int32 OtherBodyIndex,bool bFromSweep,const FHitResult &SweepResult);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		float  MovementSpeedWithHeart = 1100.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		float  MovementSpeedWithoutHeart = 1200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		float  MaxJumpHoldTime = 0.25f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		float  DashCooldown = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		float  DashForce = 2000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		float HeartLossStunDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		float HeartGainInvincibilityDuration = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		bool holdingHeart = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PlayerProperties")
		FName socketName;

	FTimerHandle dashTimerHandle;
	FTimerHandle stunTimerHandle;
	FTimerHandle invinciblityTimerHandle;
	FVector PlayerMovementDirection = FVector(0,0,0);
	FRotator playerRotationDirection;
	AAbilityBaseClass* overrideAbility = nullptr;
	AAbilityBaseClass* heldAbility = nullptr;
};