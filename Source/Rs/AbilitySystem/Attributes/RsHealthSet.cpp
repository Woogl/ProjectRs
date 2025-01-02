// Copyright 2024 Team BH.


#include "RsHealthSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

URsHealthSet::URsHealthSet()
{
	MaxHealth = 0.0f;
	CurrentHealth = 0.0f;
	HealthRegen = 0.0f;
}

void URsHealthSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams Params{};
	Params.bIsPushBased = true;
	Params.Condition = COND_None;
	
	// Replicated to all
	DOREPLIFETIME_WITH_PARAMS_FAST(URsHealthSet, CurrentHealth, Params);
	DOREPLIFETIME_WITH_PARAMS_FAST(URsHealthSet, MaxHealth, Params);

	// Only Owner
	Params.Condition = COND_OwnerOnly;
	DOREPLIFETIME_WITH_PARAMS_FAST(URsHealthSet, HealthRegen, Params);
}

void URsHealthSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	
	else if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void URsHealthSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Store a local copy of the amount of Damage done and clear the Damage attribute.
		const float LocalDamageDone = GetDamage();

		SetDamage(0.f);
	
		if (LocalDamageDone > 0.0f)
		{
			// Apply the Health change and then clamp it.
			const float NewHealth = GetCurrentHealth() - LocalDamageDone;
			SetCurrentHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
		}
	}

	else if (Data.EvaluatedData.Attribute == GetHealingAttribute())
	{
		// Store a local copy of the amount of Healing done and clear the Healing attribute.
		const float LocalHealingDone = GetHealing();

		SetHealing(0.f);
	
		if (LocalHealingDone > 0.0f)
		{
			// Apply the Health change and then clamp it.
			const float NewHealth = GetCurrentHealth() + LocalHealingDone;
			SetCurrentHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
		}
	}
	
	else if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.0f, GetMaxHealth()));
	}

	else if (Data.EvaluatedData.Attribute == GetHealthRegenAttribute())
	{
		SetHealthRegen(FMath::Clamp(GetHealthRegen(), 0.0f, GetMaxHealth()));
	}
}

void URsHealthSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetCurrentHealthAttribute())
	{
		if (NewValue <= 0.f && OldValue > 0.f)
		{
			FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Death"));
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwningActor(), DeathTag, FGameplayEventData());
		}
		else if (OldValue > 0.f && OldValue > NewValue)
		{
			FGameplayTag HitReactionTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.HitReaction"));
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwningActor(), HitReactionTag, FGameplayEventData());
		}
	}
}

void URsHealthSet::OnRep_CurrentHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URsHealthSet, CurrentHealth, OldValue);
}

void URsHealthSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URsHealthSet, MaxHealth, OldValue);
}

void URsHealthSet::OnRep_HealthRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(URsHealthSet, HealthRegen, OldValue);
}
