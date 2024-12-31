// Copyright 2024 Team BH.


#include "RsAbilitySystemComponent.h"

#include "Rs/AbilitySystem/Data/RsAbilitySet.h"
#include "Rs/AbilitySystem/Abilities/RsGameplayAbility.h"
#include "Rs/AbilitySystem/Attributes/RsAttributeSetBase.h"

URsAbilitySystemComponent::URsAbilitySystemComponent()
{
	// Sets the Ability System Component to use "Mixed" replication mode. This will replicate minimal Gameplay Effects to Simulated Proxies and full info to everyone else.
	ReplicationMode = EGameplayEffectReplicationMode::Mixed;

	// Explicitly set the Ability System Component to replicate.
	SetIsReplicatedByDefault(true);
}

void URsAbilitySystemComponent::InitializeAbilitySystem(URsAbilitySet* AbilitySet, AActor* InOwningActor, AActor* InAvatarActor)
{
	if (AbilitySystemDataInitialized)
	{
		return;
	}
	AbilitySystemDataInitialized = true;

	// Set the Owning Actor and Avatar Actor. (Used throughout the Gameplay Ability System to get references etc.)
	InitAbilityActorInfo(InOwningActor, InAvatarActor);

	// Apply the Gameplay Tag container as loose Gameplay Tags. (These are not replicated by default and should be applied on both server and client respectively.)
	if (!AbilitySet->GrantedTags.IsEmpty())
	{
		AddLooseGameplayTags(AbilitySet->GrantedTags);
	}
	
	// Check to see if we have authority. (Attribute Sets / Attribute Base Values / Gameplay Abilities / Gameplay Effects should only be added -or- set on authority and will be replicated to the client automatically.)
	if (!GetOwnerActor()->HasAuthority())
	{
		return;
	}
	
	// Grant Attribute Sets if the array isn't empty.
	if (!AbilitySet->GrantedAttributes.IsEmpty())
	{
		for (const TSubclassOf<UAttributeSet> AttributeSetClass : AbilitySet->GrantedAttributes)
		{
			GetOrCreateAttributeSubobject(AttributeSetClass);
		}
	}

	// Set base attribute values if the map isn't empty.
	if (!AbilitySet->GrantedAttributeValues.IsEmpty())
	{
		for (const TTuple<FGameplayAttribute, FScalableFloat>& AttributeBaseValue : AbilitySet->GrantedAttributeValues)
		{
			if (HasAttributeSetForAttribute(AttributeBaseValue.Key))
			{
				SetNumericAttributeBase(AttributeBaseValue.Key, AttributeBaseValue.Value.GetValueAtLevel(0.f));
			}
		}
	}

	// Grant Gameplay Abilities if the array isn't empty.
	if (!AbilitySet->GrantedAbilities.IsEmpty())
	{
		for (const TSubclassOf<URsGameplayAbility> GameplayAbility : AbilitySet->GrantedAbilities)
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(GameplayAbility, 0, INDEX_NONE, this);
			GiveAbility(AbilitySpec);
		}
	}

	// Apply Gameplay Effects if the array isn't empty.
	if (!AbilitySet->GrantedEffects.IsEmpty())
	{
		for (const TSubclassOf<UGameplayEffect>& GameplayEffect : AbilitySet->GrantedEffects)
		{
			if (!IsValid(GameplayEffect))
			{
				continue;
			}
			
			FGameplayEffectContextHandle EffectContextHandle = MakeEffectContext();
			EffectContextHandle.AddSourceObject(this);

			if (FGameplayEffectSpecHandle GameplayEffectSpecHandle = MakeOutgoingSpec(GameplayEffect, 1, EffectContextHandle); GameplayEffectSpecHandle.IsValid())
			{
				ApplyGameplayEffectSpecToTarget(*GameplayEffectSpecHandle.Data.Get(), this);
			}
		}
	}
}

