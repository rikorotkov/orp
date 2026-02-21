#include "OrpProtection.h"

#include <ArkApi/Ark/Ark.h>

DECLARE_HOOK(AShooterGameMode_Login, void, AShooterGameMode*, AShooterPlayerController*);
DECLARE_HOOK(AShooterGameMode_Logout, void, AShooterGameMode*, AController*);
DECLARE_HOOK(APrimalStructure_TakeDamage, float, APrimalStructure*, float, FDamageEvent*, AController*, AActor*);
DECLARE_HOOK(APrimalDinoCharacter_TakeDamage, float, APrimalDinoCharacter*, float, FDamageEvent*, AController*, AActor*);

void Hook_AShooterGameMode_Login(AShooterGameMode* game_mode, AShooterPlayerController* pc)
{
    AShooterGameMode_Login_original(game_mode, pc);
    ORP::OrpProtection::Get().OnPlayerConnected(pc);
}

void Hook_AShooterGameMode_Logout(AShooterGameMode* game_mode, AController* controller)
{
    if (auto* pc = static_cast<AShooterPlayerController*>(controller))
    {
        ORP::OrpProtection::Get().OnPlayerDisconnected(pc);
    }

    AShooterGameMode_Logout_original(game_mode, controller);
}

float Hook_APrimalStructure_TakeDamage(APrimalStructure* structure, float damage, FDamageEvent* damage_event, AController* event_instigator, AActor* damage_causer)
{
    auto* pc = event_instigator ? static_cast<AShooterPlayerController*>(event_instigator) : nullptr;

    damage = ORP::OrpProtection::Get().ApplyOutgoingDamage(damage, structure, pc);
    damage = ORP::OrpProtection::Get().ApplyIncomingDamage(damage, structure);

    return APrimalStructure_TakeDamage_original(structure, damage, damage_event, event_instigator, damage_causer);
}

float Hook_APrimalDinoCharacter_TakeDamage(APrimalDinoCharacter* dino, float damage, FDamageEvent* damage_event, AController* event_instigator, AActor* damage_causer)
{
    auto* pc = event_instigator ? static_cast<AShooterPlayerController*>(event_instigator) : nullptr;

    damage = ORP::OrpProtection::Get().ApplyOutgoingDamage(damage, dino, pc);
    damage = ORP::OrpProtection::Get().ApplyIncomingDamage(damage, dino);

    return APrimalDinoCharacter_TakeDamage_original(dino, damage, damage_event, event_instigator, damage_causer);
}

void Load()
{
    ORP::OrpProtection::Get().LoadConfig();

    ArkApi::GetHooks().SetHook("AShooterGameMode.Login", &Hook_AShooterGameMode_Login, &AShooterGameMode_Login_original);
    ArkApi::GetHooks().SetHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout, &AShooterGameMode_Logout_original);
    ArkApi::GetHooks().SetHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage, &APrimalStructure_TakeDamage_original);
    ArkApi::GetHooks().SetHook("APrimalDinoCharacter.TakeDamage", &Hook_APrimalDinoCharacter_TakeDamage, &APrimalDinoCharacter_TakeDamage_original);

    Log::GetLog()->info("[ORP] Plugin loaded");
}

void Unload()
{
    ArkApi::GetHooks().DisableHook("AShooterGameMode.Login", &Hook_AShooterGameMode_Login);
    ArkApi::GetHooks().DisableHook("AShooterGameMode.Logout", &Hook_AShooterGameMode_Logout);
    ArkApi::GetHooks().DisableHook("APrimalStructure.TakeDamage", &Hook_APrimalStructure_TakeDamage);
    ArkApi::GetHooks().DisableHook("APrimalDinoCharacter.TakeDamage", &Hook_APrimalDinoCharacter_TakeDamage);

    Log::GetLog()->info("[ORP] Plugin unloaded");
}

extern "C" __declspec(dllexport) void Plugin_Init()
{
    Load();
}

extern "C" __declspec(dllexport) void Plugin_Unload()
{
    Unload();
}
