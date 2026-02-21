#include "OrpProtection.h"

#include <API/ARK/Ark.h>
#include <API/UE/Containers/TArray.h>
#include <Json.hpp>
#include <format>

namespace
{
constexpr auto* kConfigPath = "ArkApi/Plugins/OrpProtection/config.json";

bool IsTurret(AActor* actor)
{
    if (!actor)
        return false;

    const auto actor_name = ArkApi::Tools::Utf16ToUtf8(actor->GetName().ToString());
    return actor_name.find("Turret") != std::string::npos;
}
}

namespace ORP
{
OrpProtection& OrpProtection::Get()
{
    static OrpProtection instance;
    return instance;
}

void OrpProtection::LoadConfig()
{
    try
    {
        const auto json = ArkApi::Tools::GetJson(kConfigPath);
        config_.orp_activation_delay_seconds = json.value("orp_activation_delay_seconds", 900);
        config_.newbie_protection_days = json.value("newbie_protection_days", 3);

        config_.tribe_damage_multiplier_to_structures = json.value("tribe_damage_multiplier_to_structures", 0.1f);
        config_.tribe_damage_multiplier_to_turrets = json.value("tribe_damage_multiplier_to_turrets", 0.1f);
        config_.tribe_damage_multiplier_to_dinos = json.value("tribe_damage_multiplier_to_dinos", 0.25f);

        config_.tribe_defense_multiplier_structures = json.value("tribe_defense_multiplier_structures", 0.25f);
        config_.tribe_defense_multiplier_turrets = json.value("tribe_defense_multiplier_turrets", 0.25f);
        config_.tribe_defense_multiplier_dinos = json.value("tribe_defense_multiplier_dinos", 0.5f);

        config_.announce_state_changes = json.value("announce_state_changes", true);
    }
    catch (const std::exception& e)
    {
        Log::GetLog()->error("[ORP] Failed to load config: {}", e.what());
        SaveConfig();
    }
}

void OrpProtection::SaveConfig() const
{
    nlohmann::json json{};
    json["orp_activation_delay_seconds"] = config_.orp_activation_delay_seconds;
    json["newbie_protection_days"] = config_.newbie_protection_days;

    json["tribe_damage_multiplier_to_structures"] = config_.tribe_damage_multiplier_to_structures;
    json["tribe_damage_multiplier_to_turrets"] = config_.tribe_damage_multiplier_to_turrets;
    json["tribe_damage_multiplier_to_dinos"] = config_.tribe_damage_multiplier_to_dinos;

    json["tribe_defense_multiplier_structures"] = config_.tribe_defense_multiplier_structures;
    json["tribe_defense_multiplier_turrets"] = config_.tribe_defense_multiplier_turrets;
    json["tribe_defense_multiplier_dinos"] = config_.tribe_defense_multiplier_dinos;

    json["announce_state_changes"] = config_.announce_state_changes;

    ArkApi::Tools::SaveJson(kConfigPath, json);
}

void OrpProtection::OnPlayerConnected(AShooterPlayerController* player_controller)
{
    if (!player_controller)
        return;

    const auto tribe_id = GetTribeIdFromPlayerController(player_controller);
    const auto player_id = GetPlayerId(player_controller);

    std::lock_guard<std::mutex> lock(mutex_);

    if (player_states_.find(player_id) == player_states_.end())
    {
        player_states_.emplace(player_id, PlayerState{std::chrono::system_clock::now()});
    }

    auto& tribe_state = tribe_states_[tribe_id];
    tribe_state.online = true;
    tribe_state.orp_active = false;

    if (config_.announce_state_changes)
    {
        ArkApi::GetApiUtils().SendServerMessage(player_controller, FColorList::Green, L"[ORP] Защита трайба отключена: игрок в сети.");
    }
}

void OrpProtection::OnPlayerDisconnected(AShooterPlayerController* player_controller)
{
    if (!player_controller)
        return;

    const auto tribe_id = GetTribeIdFromPlayerController(player_controller);

    std::lock_guard<std::mutex> lock(mutex_);

    auto& tribe_state = tribe_states_[tribe_id];
    tribe_state.online = false;
    tribe_state.offline_since = std::chrono::steady_clock::now();
}

bool OrpProtection::IsNewbieProtected(int64 player_id) const
{
    const auto it = player_states_.find(player_id);
    if (it == player_states_.end())
        return false;

    const auto age = std::chrono::system_clock::now() - it->second.first_seen;
    const auto days = std::chrono::duration_cast<std::chrono::hours>(age).count() / 24;
    return days < config_.newbie_protection_days;
}

bool OrpProtection::IsTribeOrpActive(int tribe_id) const
{
    const auto it = tribe_states_.find(tribe_id);
    if (it == tribe_states_.end())
        return false;

    if (it->second.online)
        return false;

    const auto elapsed = std::chrono::steady_clock::now() - it->second.offline_since;
    return elapsed >= std::chrono::seconds(config_.orp_activation_delay_seconds);
}

std::chrono::seconds OrpProtection::GetTimeToOrp(int tribe_id) const
{
    const auto it = tribe_states_.find(tribe_id);
    if (it == tribe_states_.end() || it->second.online)
        return std::chrono::seconds(0);

    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - it->second.offline_since);
    const auto full = std::chrono::seconds(config_.orp_activation_delay_seconds);

    if (elapsed >= full)
        return std::chrono::seconds(0);

    return full - elapsed;
}

float OrpProtection::ApplyOutgoingDamage(float incoming_damage, AActor* victim, AShooterPlayerController* attacker_pc) const
{
    if (!victim)
        return incoming_damage;

    std::lock_guard<std::mutex> lock(mutex_);

    const auto tribe_id = GetTribeIdFromActor(victim);
    if (tribe_id == 0)
        return incoming_damage;

    if (IsTribeOrpActive(tribe_id))
    {
        if (IsTurret(victim))
            return incoming_damage * config_.tribe_damage_multiplier_to_turrets;

        if (victim->IsA(APrimalStructure::GetPrivateStaticClass()))
            return incoming_damage * config_.tribe_damage_multiplier_to_structures;

        if (victim->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
            return incoming_damage * config_.tribe_damage_multiplier_to_dinos;
    }

    if (attacker_pc && IsNewbieProtected(GetPlayerId(attacker_pc)))
    {
        return incoming_damage * 0.0f;
    }

    return incoming_damage;
}

float OrpProtection::ApplyIncomingDamage(float incoming_damage, AActor* victim) const
{
    if (!victim)
        return incoming_damage;

    std::lock_guard<std::mutex> lock(mutex_);

    const auto tribe_id = GetTribeIdFromActor(victim);
    if (tribe_id == 0)
        return incoming_damage;

    if (!IsTribeOrpActive(tribe_id))
        return incoming_damage;

    if (IsTurret(victim))
        return incoming_damage * config_.tribe_defense_multiplier_turrets;

    if (victim->IsA(APrimalStructure::GetPrivateStaticClass()))
        return incoming_damage * config_.tribe_defense_multiplier_structures;

    if (victim->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
        return incoming_damage * config_.tribe_defense_multiplier_dinos;

    return incoming_damage;
}

std::wstring OrpProtection::BuildHoverStatusText(AActor* target_actor) const
{
    if (!target_actor)
        return L"";

    std::lock_guard<std::mutex> lock(mutex_);

    const auto tribe_id = GetTribeIdFromActor(target_actor);
    if (tribe_id == 0)
        return L"";

    if (IsTribeOrpActive(tribe_id))
    {
        return L"[ORP] OFFLINE PROTECTION: ON";
    }

    const auto left = GetTimeToOrp(tribe_id);
    if (left.count() > 0)
    {
        return std::format(L"[ORP] OFFLINE PROTECTION IN: {}s", left.count());
    }

    return L"[ORP] OFFLINE PROTECTION: OFF";
}

int OrpProtection::GetTribeIdFromActor(AActor* actor) const
{
    if (!actor)
        return 0;

    if (actor->IsA(APrimalStructure::GetPrivateStaticClass()))
    {
        auto* structure = static_cast<APrimalStructure*>(actor);
        return structure->TargetingTeamField();
    }

    if (actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass()))
    {
        auto* dino = static_cast<APrimalDinoCharacter*>(actor);
        return dino->TargetingTeamField();
    }

    return 0;
}

int OrpProtection::GetTribeIdFromPlayerController(AShooterPlayerController* pc) const
{
    if (!pc)
        return 0;

    auto* player_state = static_cast<AShooterPlayerState*>(pc->PlayerStateField());
    if (!player_state)
        return 0;

    return player_state->CurrentTribeIDField();
}

int64 OrpProtection::GetPlayerId(AShooterPlayerController* pc) const
{
    if (!pc)
        return 0;

    auto* player_state = static_cast<AShooterPlayerState*>(pc->PlayerStateField());
    if (!player_state)
        return 0;

    return player_state->UniqueIdField().ToUInt64();
}
} // namespace ORP
