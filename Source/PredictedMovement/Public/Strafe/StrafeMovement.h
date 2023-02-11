// Copyright (c) 2023 Jared Taylor. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Prone/ProneMovement.h"
#include "StrafeMovement.generated.h"

class AStrafeCharacter;
UCLASS()
class PREDICTEDMOVEMENT_API UStrafeMovement : public UProneMovement
{
	GENERATED_BODY()
	
private:
	/** Character movement component belongs to */
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<AStrafeCharacter> StrafeCharacterOwner;

public:
	/** Max Acceleration (rate of change of velocity) */
	UPROPERTY(Category="Character Movement (General Settings)", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float MaxAccelerationStrafing;
	
	/** The maximum ground speed when Strafing. */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxWalkSpeedStrafing;

	/**
	 * Deceleration when walking and not applying acceleration. This is a constant opposing force that directly lowers velocity by a constant value.
	 * @see GroundFriction, MaxAcceleration
	 */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationStrafing;

	/**
	 * Setting that affects movement control. Higher values allow faster changes in direction.
	 * If bUseSeparateBrakingFriction is false, also affects the ability to stop more quickly when braking (whenever Acceleration is zero), where it is multiplied by BrakingFrictionFactor.
	 * When braking, this property allows you to control how much friction is applied when moving across the ground, applying an opposing force that scales with current velocity.
	 * This can be used to simulate slippery surfaces such as ice or oil by changing the value (possibly based on the material pawn is standing on).
	 * @see BrakingDecelerationWalking, BrakingFriction, bUseSeparateBrakingFriction, BrakingFrictionFactor
	 */
	UPROPERTY(Category="Character Movement: Walking", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float GroundFrictionStrafing;

	/**
	 * Friction (drag) coefficient applied when braking (whenever Acceleration = 0, or if character is exceeding max speed); actual value used is this multiplied by BrakingFrictionFactor.
	 * When braking, this property allows you to control how much friction is applied when moving across the ground, applying an opposing force that scales with current velocity.
	 * Braking is composed of friction (velocity-dependent drag) and constant deceleration.
	 * This is the current value, used in all movement modes; if this is not desired, override it or bUseSeparateBrakingFriction when movement mode changes.
	 * @note Only used if bUseSeparateBrakingFriction setting is true, otherwise current friction such as GroundFriction is used.
	 * @see bUseSeparateBrakingFriction, BrakingFrictionFactor, GroundFriction, BrakingDecelerationWalking
	 */
	UPROPERTY(Category="Character Movement (General Settings)", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", EditCondition="bUseSeparateBrakingFriction"))
	float BrakingFrictionStrafing;
	
public:
	/** If true, try to Strafe (or keep Strafing) on next update. If false, try to stop Strafing on next update. */
	UPROPERTY(Category="Character Movement (General Settings)", VisibleInstanceOnly, BlueprintReadOnly)
	uint8 bWantsToStrafe:1;

public:
	UStrafeMovement(const FObjectInitializer& ObjectInitializer);

	virtual bool HasValidData() const override;
	virtual void PostLoad() override;
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;

public:
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxBrakingDeceleration() const override;
	
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override
	{
		if (IsStrafing() && IsMovingOnGround())
		{
			Friction = GroundFrictionStrafing;
		}
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	}
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration) override
	{
		if (IsStrafing() && IsMovingOnGround())
		{
			Friction = (bUseSeparateBrakingFriction ? BrakingFrictionStrafing : GroundFrictionStrafing);
		}
		Super::ApplyVelocityBraking(DeltaTime, Friction, BrakingDeceleration);
	}
	
public:
	virtual bool IsStrafing() const;

	/**
	 * Call CharacterOwner->OnStartStrafe() if successful.
	 * In general you should set bWantsToStrafe instead to have the Strafe persist during movement, or just use the Strafe functions on the owning Character.
	 * @param	bClientSimulation	true when called when bIsStrafeed is replicated to non owned clients.
	 */
	virtual void Strafe(bool bClientSimulation = false);
	
	/**
	 * Checks if default capsule size fits (no encroachment), and trigger OnEndStrafe() on the owner if successful.
	 * @param	bClientSimulation	true when called when bIsStrafeed is replicated to non owned clients.
	 */
	virtual void UnStrafe(bool bClientSimulation = false);

	/** Returns true if the character is allowed to Strafe in the current state. By default it is allowed when walking or falling. */
	virtual bool CanStrafeInCurrentState() const;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

protected:
	virtual bool ClientUpdatePositionAfterServerUpdate() override;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	
public:
	/** Get prediction data for a client game. Should not be used if not running as a client. Allocates the data on demand and can be overridden to allocate a custom override if desired. Result must be a FNetworkPredictionData_Client_Character. */
	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
};

class PREDICTEDMOVEMENT_API FSavedMove_Character_Strafe : public FSavedMove_Character_Prone
{
	using Super = FSavedMove_Character_Prone;

public:
	FSavedMove_Character_Strafe()
		: bWantsToStrafe(0)
	{
	}

	virtual ~FSavedMove_Character_Strafe() override
	{}

	uint32 bWantsToStrafe:1;
		
	/** Clear saved move properties, so it can be re-used. */
	virtual void Clear() override;

	/** Called to set up this saved move (when initially created) to make a predictive correction. */
	virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;

	/** Returns a byte containing encoded special movement information (jumping, crouching, etc.)	 */
	virtual uint8 GetCompressedFlags() const override;
};

class PREDICTEDMOVEMENT_API FNetworkPredictionData_Client_Character_Strafe : public FNetworkPredictionData_Client_Character_Prone
{
	using Super = FNetworkPredictionData_Client_Character_Prone;

public:
	FNetworkPredictionData_Client_Character_Strafe(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
	{}

	virtual FSavedMovePtr AllocateNewMove() override;
};
