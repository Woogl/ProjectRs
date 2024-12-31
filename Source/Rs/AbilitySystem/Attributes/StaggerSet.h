// Copyright 2024 Team BH.

#pragma once

#include "CoreMinimal.h"
#include "RsAttributeSetBase.h"
#include "StaggerSet.generated.h"

/**
 * 
 */
UCLASS()
class RS_API UStaggerSet : public URsAttributeSetBase
{
	GENERATED_BODY()

public:
	UStaggerSet();

	// Attribute Set Overrides.
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	// Set Attributes to replicate.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Used to create a local copy of Loss which is then subtracted from Current Stagger.
	UPROPERTY(BlueprintReadOnly, meta = (HideFromLevelInfos))
	FGameplayAttributeData StaggerGain;
	ATTRIBUTE_ACCESSORS(UStaggerSet, StaggerGain)
	
	// Holds the current value for Stagger.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentStagger)
	FGameplayAttributeData CurrentStagger;
	ATTRIBUTE_ACCESSORS(UStaggerSet, CurrentStagger)

	// Holds the value for Maximum Stagger.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxStagger)
	FGameplayAttributeData MaxStagger;
	ATTRIBUTE_ACCESSORS(UStaggerSet, MaxStagger)

	// Holds the current value for Stagger Regeneration.
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_StaggerRegen)
	FGameplayAttributeData StaggerRegen;
	ATTRIBUTE_ACCESSORS(UStaggerSet, StaggerRegen)

protected:
	UFUNCTION()
	virtual void OnRep_CurrentStagger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxStagger(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_StaggerRegen(const FGameplayAttributeData& OldValue);
};
