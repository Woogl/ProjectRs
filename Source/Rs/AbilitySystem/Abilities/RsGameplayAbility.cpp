// Copyright 2024 Team BH.


#include "RsGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "Rs/AbilitySystem/Component/RsAbilitySystemComponent.h"
#include "Rs/Character/RsCharacterBase.h"

URsGameplayAbility::URsGameplayAbility()
{
	// Sets the ability to default to Instanced Per Actor.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;	
}

void URsGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	// Set the "Avatar Character" reference.
	AvatarCharacter = Cast<ARsCharacterBase>(ActorInfo->AvatarActor);

	// Set up Bindings for Enhanced Input.
	SetupEnhancedInputBindings(ActorInfo, Spec);

	// Try to Activate immediately if "Activate Ability On Granted" is true.
	if (ActivateAbilityOnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle);
	}
	
	if (URsAbilitySystemComponent* RsASC = Cast<URsAbilitySystemComponent>(ActorInfo->AbilitySystemComponent))
	{
		RsASC->OnAnyGameplayEvent.AddUObject(this, &ThisClass::HandleGameplayEvent);
	}
}

void URsGameplayAbility::SetupEnhancedInputBindings(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	// Check to see if the "Activation Input Action" is valid.
	URsGameplayAbility* const AbilityInstance = Cast<URsGameplayAbility>(Spec.Ability.Get());
	if (!AbilityInstance || !AbilityInstance->ActivationInputAction)
	{
		return;
	}
	
	const APawn* const AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!AvatarPawn || !Spec.Ability)
	{
		return;
	}
	
	const AController* const PawnController = AvatarPawn->GetController();
	UEnhancedInputComponent* const EnhancedInputComponent = PawnController ? Cast<UEnhancedInputComponent>(PawnController->InputComponent.Get()) : nullptr;
	if (!EnhancedInputComponent)
	{
		return;
	}
	
	// Input pressed event.
	EnhancedInputComponent->BindAction(AbilityInstance->ActivationInputAction, ETriggerEvent::Triggered, AbilityInstance, &ThisClass::HandleInputPressedEvent, ActorInfo, Spec.Handle);

	// Input released event.
	EnhancedInputComponent->BindAction(AbilityInstance->ActivationInputAction, ETriggerEvent::Completed, AbilityInstance, &ThisClass::HandleInputReleasedEvent, ActorInfo, Spec.Handle);
}

void URsGameplayAbility::HandleInputPressedEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpecHandle SpecHandle)
{
	// Find the Ability Spec based on the passed in information and set a reference.
	FGameplayAbilitySpec* const Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(SpecHandle);
	if (!Spec || !Spec->Ability || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* const AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		return;
	}
	
	Spec->InputPressed = true;
	
	if (!Spec->IsActive())
	{
		// Ability is not active, so try to activate it
		AbilitySystemComponent->TryActivateAbility(SpecHandle);
		return;
	}

	if (Spec->Ability->bReplicateInputDirectly && !AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		AbilitySystemComponent->ServerSetInputPressed(Spec->Ability.Get()->GetCurrentAbilitySpecHandle());
	}

	AbilitySystemComponent->AbilitySpecInputPressed(*Spec);

	// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
	TArray<UGameplayAbility*> Instances = Spec->GetAbilityInstances();
	const FGameplayAbilityActivationInfo& ActivationInfo = Instances.Last()->GetCurrentActivationInfoRef();
	AbilitySystemComponent->InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec->Handle, ActivationInfo.GetActivationPredictionKey());
}

void URsGameplayAbility::HandleInputReleasedEvent(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpecHandle SpecHandle)
{
	// Find the Ability Spec based on the passed in information and set a reference.
	FGameplayAbilitySpec* const Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(SpecHandle);
	if (!Spec || !Spec->IsActive() || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* const AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();
	if (!AbilitySystemComponent)
	{
		return;
	}
	
	Spec->InputPressed = false;
	if (Spec->Ability->bReplicateInputDirectly && !AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		AbilitySystemComponent->ServerSetInputReleased(SpecHandle);
	}

	AbilitySystemComponent->AbilitySpecInputReleased(*Spec);

	// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
	TArray<UGameplayAbility*> Instances = Spec->GetAbilityInstances();
	const FGameplayAbilityActivationInfo& ActivationInfo = Instances.Last()->GetCurrentActivationInfoRef();
	AbilitySystemComponent->InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec->Handle, ActivationInfo.GetActivationPredictionKey());
}

void URsGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	const APawn* const AvatarPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	if (!AvatarPawn || !Spec.Ability)
	{
		return;
	}
	
	if (const AController* const PawnController = AvatarPawn->GetController())
	{
		if (UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(PawnController->InputComponent.Get()))
		{
			// We need to clear the bindings from the Enhanced Input Component when the Ability is removed.
			EnhancedInputComponent->ClearBindingsForObject(Spec.Ability.Get());
		}
	}

	if (URsAbilitySystemComponent* RsASC = Cast<URsAbilitySystemComponent>(ActorInfo->AbilitySystemComponent))
	{
		RsASC->OnAnyGameplayEvent.RemoveAll(this);
	}
	
	Super::OnRemoveAbility(ActorInfo, Spec);
}

void URsGameplayAbility::HandleGameplayEvent(const FGameplayEventData* EventData)
{
	if (IsActive())
	{
		K2_OnGameplayEvent(EventData->EventTag);
	}
}
