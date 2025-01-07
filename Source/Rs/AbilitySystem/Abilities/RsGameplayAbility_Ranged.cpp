// Copyright 2024 Team BH.


#include "RsGameplayAbility_Ranged.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Rs/AbilitySystem/AbilityTask/RsAbilityTask_TurnToLocation.h"
#include "Rs/Battle/RsBattleLibrary.h"
#include "Rs/Battle/Actor/RsProjectile.h"
#include "Rs/Character/RsCharacterBase.h"

void URsGameplayAbility_Ranged::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (MontageToPlay)
	{
		if (UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MontageToPlay))
		{
			MontageTask->OnCompleted.AddDynamic(this, &ThisClass::HandleMontageCompleted);
			MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::HandleMontageCompleted);
			MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleMontageCancelled);
			MontageTask->OnCancelled.AddDynamic(this, &ThisClass::HandleMontageCancelled);
			MontageTask->ReadyForActivation();
		}
	}

	if (FocusTargetingPreset)
	{
		TArray<AActor*> Victims;
		if (URsBattleLibrary::ExecuteTargeting(GetAvatarActorFromActorInfo(), FocusTargetingPreset, Victims))
		{
			CachedVictim = Victims[0];
			URsAbilityTask_TurnToLocation* TurnTask = URsAbilityTask_TurnToLocation::TurnToLocation(this, Victims[0]->GetActorLocation(), RotatingSpeed, RotatingMaxDuration);
			TurnTask->ReadyForActivation();
		}
	}

	if (ProjectileClass)
	{
		if (FireProjectileEventTag.IsValid())
		{
			UAbilityTask_WaitGameplayEvent* FireProjectileTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FireProjectileEventTag);
			FireProjectileTask->EventReceived.AddDynamic(this, &ThisClass::HandleFireProjectile);
			FireProjectileTask->ReadyForActivation();
		}
		else
		{
			// Spawn projectile immediately if Event tag is not set.
			HandleFireProjectile(FGameplayEventData());
		}
	}
	else
	{
		// Deal damage immediately if Projectile is not set.
		HandleInstantDamage();
	}
}

void URsGameplayAbility_Ranged::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	CachedVictim = nullptr;
}

void URsGameplayAbility_Ranged::HandleMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void URsGameplayAbility_Ranged::HandleMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void URsGameplayAbility_Ranged::HandleFireProjectile(FGameplayEventData EventData)
{
	ARsCharacterBase* Source = GetAvatarCharacter();

	FTransform ProjectileTransform = Source->GetActorTransform();
	if (CachedVictim.IsValid())
	{
		FVector Start = Source->GetActorLocation();
		FVector End = CachedVictim->GetActorLocation();
		ProjectileTransform.SetRotation(UKismetMathLibrary::FindLookAtRotation(Start, End).Quaternion());
	}
	
	FGameplayEffectSpecHandle DamageEffectSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffect, GetAbilityLevel());
	
	ARsProjectile* Projectile = GetWorld()->SpawnActorDeferred<ARsProjectile>(ProjectileClass, ProjectileTransform, Source, Source, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	Projectile->DamageSpecHandle = DamageEffectSpecHandle;
	if (CachedVictim.IsValid())
	{
		Projectile->ProjectileMovement->HomingTargetComponent = CachedVictim.Get()->GetRootComponent();
	}
	Projectile->FinishSpawning(AvatarCharacter->GetActorTransform());
}

void URsGameplayAbility_Ranged::HandleInstantDamage()
{
	if (CachedVictim.IsValid() && DamageEffect)
	{
		URsBattleLibrary::ApplyDamageEffect(GetAvatarCharacter(), CachedVictim.Get(), DamageEffect);
	}
}
