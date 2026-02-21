#pragma once

#include <ArkApi/Ark/Ark.h>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

namespace ORP
{
struct Config
{
    int orp_activation_delay_seconds{900};
    int newbie_protection_days{3};

    float tribe_damage_multiplier_to_structures{0.1f};
    float tribe_damage_multiplier_to_turrets{0.1f};
    float tribe_damage_multiplier_to_dinos{0.25f};

    float tribe_defense_multiplier_structures{0.25f};
    float tribe_defense_multiplier_turrets{0.25f};
    float tribe_defense_multiplier_dinos{0.5f};

    bool announce_state_changes{true};
};

struct TribeState
{
    bool online{false};
    bool orp_active{false};
    std::chrono::steady_clock::time_point offline_since{};
};

struct PlayerState
{
    std::chrono::system_clock::time_point first_seen{};
};

class OrpProtection final
{
public:
    static OrpProtection& Get();

    void LoadConfig();
    void SaveConfig() const;

    void OnPlayerConnected(AShooterPlayerController* player_controller);
    void OnPlayerDisconnected(AShooterPlayerController* player_controller);

    float ApplyOutgoingDamage(float incoming_damage, AActor* victim, AShooterPlayerController* attacker_pc) const;
    float ApplyIncomingDamage(float incoming_damage, AActor* victim) const;

    std::wstring BuildHoverStatusText(AActor* target_actor) const;

private:
    OrpProtection() = default;

    int GetTribeIdFromActor(AActor* actor) const;
    int GetTribeIdFromPlayerController(AShooterPlayerController* pc) const;
    int64 GetPlayerId(AShooterPlayerController* pc) const;

    bool IsNewbieProtected(int64 player_id) const;
    bool IsTribeOrpActive(int tribe_id) const;
    std::chrono::seconds GetTimeToOrp(int tribe_id) const;

private:
    Config config_{};

    mutable std::mutex mutex_;
    std::unordered_map<int, TribeState> tribe_states_;
    std::unordered_map<int64, PlayerState> player_states_;
};
} // namespace ORP
