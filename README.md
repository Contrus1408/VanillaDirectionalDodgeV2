# VanillaDirectionalDodge

SKSE-плагин для Skyrim SE/AE 1.7.104 (SKSE 2.3.1): по нажатию клавиши персонаж
из обычной стойки делает перекат, используя **только ванильную анимацию переката**.
Перки, бег и присед не нужны.

## Как работает

1. Плагин ловит нажатие клавиши (`Key` в ini, по умолчанию V).
2. Если зажаты WASD, персонаж разворачивается в эту сторону относительно камеры.
3. Игре отправляется анимационное событие из `RollEvents` (`NotifyAnimationGraph`).
4. Результат каждой попытки пишется в `Documents\My Games\Skyrim Special Edition\SKSE\VanillaDirectionalDodge.log`.

## Сборка в GitHub Actions

```
git init
git submodule add -b ng https://github.com/alandtse/CommonLibSSE-NG.git extern/CommonLibSSE-NG
git add .
git commit -m "init"
git remote add origin <url>
git push -u origin main
```

Сабмодуль необязателен: если папка `extern/CommonLibSSE-NG` пуста, workflow сам клонирует ветку `ng`.
Готовая сборка: вкладка Actions -> последний запуск -> артефакт `VanillaDirectionalDodge`
(внутри `SKSE/Plugins/*.dll` и `*.ini`, распаковывать в папку `Data`).
Релиз создаётся автоматически при пуше тега `v*`.

## Требования для игры

- Skyrim 1.7.104, SKSE64 2.3.1
- Address Library for SKSE Plugins v13 или новее

## Лицензия

CommonLibSSE-NG распространяется под GPL-3.0-or-later (с исключениями для модов),
поэтому плагин при распространении тоже должен быть под GPL-совместимой лицензией
с открытым исходным кодом.
