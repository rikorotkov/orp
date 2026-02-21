# ORP Protection Plugin (ASE / ArkServerApi)

Плагин-основа для собственного аналога ORP (Offline Raid Protection) под **ARK: Survival Evolved** через **ArkServerApi**.

## Что реализовано

- Таймер включения ORP после выхода последнего участника трайба.
- Настройка коэффициентов урона/защиты для:
  - построек,
  - турелей (как отдельный тип коэффициента в конфиге),
  - дино.
- Защита новичков по времени на сервере (например, первые 3 дня).
- Загрузка настроек из JSON.

## Конфиг

Путь: `ArkApi/Plugins/OrpProtection/config.json`

Пример параметров:

```json
{
  "orp_activation_delay_seconds": 900,
  "newbie_protection_days": 3,

  "tribe_damage_multiplier_to_structures": 0.1,
  "tribe_damage_multiplier_to_turrets": 0.1,
  "tribe_damage_multiplier_to_dinos": 0.25,

  "tribe_defense_multiplier_structures": 0.25,
  "tribe_defense_multiplier_turrets": 0.25,
  "tribe_defense_multiplier_dinos": 0.5,

  "announce_state_changes": true
}
```

## Важное по отображению статуса при наведении

В API ASE нет одной универсальной «из коробки» точки, которая гарантированно подходит для любого UI-мода сервера. В этом шаблоне уже есть метод `BuildHoverStatusText(...)`, который формирует текст:

- `OFFLINE PROTECTION: ON`,
- `OFFLINE PROTECTION IN: <seconds>`,
- `OFFLINE PROTECTION: OFF`.

Чтобы показывать его именно **при наведении** на строение/дино, нужно подключить этот метод к вашему текущему HUD/overlay-хуку (в зависимости от того, какой способ рендера текста вы используете в проекте).

## Сборка (Visual Studio)

1. Откройте проект плагина в VS 2026.
2. Подключите include/lib ArkServerApi SDK.
3. Соберите DLL (x64, Release).
4. Разместите DLL в папке плагинов сервера.

## Файлы

- `src/PluginMain.cpp` — инициализация, хуки входа/выхода и обработка урона.
- `src/OrpProtection.h` — модели конфига/состояний и публичный API менеджера.
- `src/OrpProtection.cpp` — бизнес-логика ORP.
