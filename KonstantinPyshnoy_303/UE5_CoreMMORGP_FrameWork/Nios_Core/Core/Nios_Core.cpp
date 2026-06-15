// Copyright Epic Games, Inc. All Rights Reserved.
#include "Core/Nios_Core.h"
#include "Modules/ModuleManager.h"
#include "GameplayTagsManager.h"


#define LOCTEXT_NAMESPACE "FNios_CoreModule"

void FNios_CoreModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	//Ability
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Ablity.Data"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Ablity.Data.Value"),
		TEXT(""));
	
	//CUE
	Manager.AddNativeGameplayTag(
		FName("GameplayCue.Skill.Execute"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("GameplayCue.Skill.CastLoop"),
		TEXT(""));
	
	//Messages
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.ActionHotKay"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.ActionSlotPressed"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.ActionSlotCallGroundTargetting"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnRequestDrop"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnDropConfirmed"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnRequestAutoEquip"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnRequestTargetEquip"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnRequestAutoUnequip"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnRequestTargetUnequip"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnShowDialogue"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnUpdateDialogue"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnShowTooltip"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnReviveCall"),
		TEXT(""));
	Manager.AddNativeGameplayTag(
		FName("NiosCore.Message.OnCallLooting"),
		TEXT(""));

}

void FNios_CoreModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FNios_CoreModule, Nios_Core)