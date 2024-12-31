// Copyright 2024 Team BH.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "RsAttributeSetBase.generated.h"

// Use macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class RS_API URsAttributeSetBase : public UAttributeSet
{
	GENERATED_BODY()

protected:
	void AdjustAttributeForMaxChange(const FGameplayAttribute& AffectedAttribute, float OldMaxValue, float NewMaxValue) const;

	void SendEventIfAttributeOverMax(const FGameplayTag& EventTag, const FGameplayAttributeData& MaxAttribute, const FGameplayAttributeData& CurrentAttribute) const;
	void SendEventIfAttributeBelowZero(const FGameplayTag& EventTag, float OldValue, float NewValue) const;
};
