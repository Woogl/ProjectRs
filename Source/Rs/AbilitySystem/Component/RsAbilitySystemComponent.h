// Copyright 2024 Team BH.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "RsAbilitySystemComponent.generated.h"

struct FAbilitySystemData;
/**
 * 
 */
UCLASS()
class RS_API URsAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	URsAbilitySystemComponent();
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

protected:
	bool AbilitySystemDataInitialized = false;

private:
	// Called to initialize an Ability System Component with the supplied data. (Can be found in "AbilitySystemData.h")
	// Call this on the Server and Client to properly init references / values.
	UFUNCTION(BlueprintCallable)
	void InitializeAbilitySystemData(const FAbilitySystemInitializationData& InitializationData, AActor* InOwningActor, AActor* InAvatarActor);
};