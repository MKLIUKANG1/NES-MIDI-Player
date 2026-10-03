# 🎮 NES MIDI Player

[![Version](https://img.shields.io/badge/version-1.0-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey.svg)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B17-orange.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Built with](https://img.shields.io/badge/built%20with-MSYS2%20%2B%20MinGW--w64-red.svg)]()

**MIDI-плеер, который синтезирует музыку через программную эмуляцию звукового чипа NES (Ricoh 2A03 APU).**

**Автор:** MKLIUKANG1  
**Версия:** 1.0  
**Платформа:** Windows 10/11 (x64)

---

## ✨ Возможности

### 🎵 Эмуляция NES APU — 5 каналов

| Канал | Реализация |
|-------|-----------|
| **Pulse 1, 2** | Прямоугольная волна, 4 режима duty (12.5%, 25%, 50%, 75%) |
| **Triangle** | 16-шаговая bipolar wave, без громкости (физически корректно) |
| **Noise** | 15-битный LFSR, режимы long (bits 0,1) и short (bits 0,6) |
| **DPCM** | Точная NTSC-таблица частот (4181–33143 Гц), MSB-first, ±2 уровня на бит |

### 🎛️ Редактор 128 GM-инструментов

- **Канал APU**: Pulse / Triangle / Noise
- **Duty**: 12.5% / 25% / 50% / 75%
- **ADSR-огибающая**: Attack, Decay, Sustain, Release
- **Duty sweep**: off / up-down / cycle / alt (настраиваемая скорость)
- **Noise-параметры**: LFSR mode, множитель частоты, фиксированная частота
- **Графический envelope-редактор**: Volume, Pitch, Duty, Arpeggio (рисование мышью)
- **Фильтр по имени** и сброс к дефолту отдельного инструмента

### 🥁 DPCM Drum Slots

- 8 слотов: **Kick, Snare, Snare 2, Clap, HiHat, HiHat Open, Tom, Cymbal**
- Автоматический маппинг по MIDI-питчам (35/36 → Kick, 38 → Snare, …)
- Загрузка `.dmc` / `.bin` файлов из папки `DPCM Samples/`
- Для каждого слота: sample, rate (0–15), volume, loop
- Кнопка **Test** — проигрывание через отдельное SDL-устройство
- Глобальный переключатель **DPCM / Noise** в транспорте

### 📊 Piano Roll

- Цветовая кодировка по каналам: Pulse (зелёный), Triangle (оранжевый), Noise (розовый)
- Zoom колесом мыши (1×–64×), центрирование по курсору
- Панорамирование СКМ-драгом
- Перемотка кликом ЛКМ
- Подсветка канала при наведении мыши на «Сейчас играет»

### 🎼 Встроенные пресеты

- **Default** — заводские настройки
- **Castlevania-style** — плавные strings, церковный орган, тяжёлый triangle bass
- **Mega Man-style** — punchy атаки, яркий лид duty 25%, staccato барабаны
- **Contra-style** — агрессивный лид, sweep-пады, резкие барабаны
- **Chrono Trigger-style** — мягкие колокольчики, атмосферные pad-свипы

### 💾 Пресеты и настройки

- Формат `.nespreset` версии 1–5 (**обратная совместимость**)
- Сохраняет всё: 128 инструментов, drum-слоты, envelope-кривые
- Автозагрузка `Presets/default.nespreset` при старте
- Пункт меню **«Сохранить как пресет по умолчанию»**

### 🌊 Экспорт в WAV

- 16-bit stereo 44.1 kHz
- Фоновый рендер в `std::thread` — GUI остаётся отзывчивым
- Прогресс-бар с кнопкой **Отмена**

### 🌍 Мультиязычность

- **English** (по умолчанию) / **Русский** / **中文**
- Мгновенное переключение через **View → Language**
- Сохранение выбора в `Presets/language.txt`

### 🖼️ Прочее

- Нативные диалоги Windows (`GetOpenFileNameW` / `GetSaveFileNameW`)
- Полная поддержка кириллицы в путях файлов
- Иконка приложения (генерируется Python-скриптом)
- Горячие клавиши: **Space** — play/pause, **Ctrl+O** — open, **Esc** — выход

---

## 📸 Скриншоты

> Добавьте сюда свои скриншоты: перетащите PNG прямо в редактор GitHub — он сам сгенерирует ссылки.
https://docs/screenshot-main.png
https://docs/screenshot-editor.png
https://docs/screenshot-envelope.png
https://docs/screenshot-pianoroll.png

text

---

## 🚀 Сборка

### Требования

- **MSYS2** — https://www.msys2.org/
- Открыть терминал **MINGW64** (не MSYS, не UCRT64)

### Установка зависимостей

```bash
pacman -Syu
pacman -S git gcc make cmake
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-SDL2
pacman -S mingw-w64-x86_64-binutils   # для windres (иконка)
Клонирование и сборка
bash
git clone https://github.com/MKLIUKANG1/nes-midi-player.git
cd nes-midi-player
mingw32-make
Готовый бинарник: nes_midi_player_v1.0.exe

Распространяемая сборка
Скопируйте нужные DLL рядом с .exe:

bash
cp /mingw64/bin/libgcc_s_seh-1.dll .
cp /mingw64/bin/libstdc++-6.dll .
cp /mingw64/bin/libwinpthread-1.dll .
cp /mingw64/bin/SDL2.dll .
Запуск
bash
./nes_midi_player_v1.0.exe your_file.mid
🎹 Использование
Горячие клавиши
Клавиша	Действие
Space	Play / Pause
Ctrl+O	Открыть MIDI
Esc	Выход / закрыть popup
Быстрый старт
Запустите nes_midi_player_v1.0.exe

Файл → Открыть MIDI… (Ctrl+O) — выберите .mid

Вид → Редактор инструментов — настройте звук

Вид → Piano Roll — посмотрите ноты

Файл → Экспорт в WAV… — сохраните результат

Настройка DPCM-барабанов
Положите .dmc файлы в папку DPCM Samples/

В редакторе откройте секцию DMC — Drum Slots

Назначьте сэмплы на слоты Kick / Snare / HiHat / …

В транспорте нажмите «Барабаны: DPCM»

Графические envelope-кривые
В редакторе инструментов нажмите Env у нужного инструмента

Откроется окно с 4 кривыми:

Volume (0–15) — множитель громкости

Pitch (-24..+24) — сдвиг высоты в полутонах

Duty (0–3) — тип импульса (для Pulse)

Arp (-12..+12) — арпеджио

Рисуйте ЛКМ, стирайте ПКМ, настройте длину и скорость

Изменения применяются мгновенно

📁 Структура проекта
text
nes-midi-player/
├── src/
│   ├── main.cpp              # GUI на Dear ImGui
│   ├── apu.cpp               # Ядро эмуляции NES APU
│   ├── midi_reader.cpp       # Собственный парсер SMF
│   ├── dmc.cpp               # Загрузка DMC-сэмплов
│   ├── i18n.cpp              # Локализация (EN/RU/ZH)
│   ├── preset.cpp            # Сохранение/загрузка пресетов
│   ├── wav_writer.cpp        # Экспорт в WAV
│   ├── builtin_presets.cpp   # 5 встроенных пресетов
│   └── gui/                  # Dear ImGui + бэкенды
├── include/
│   ├── apu.h                 # NesApu, ApuPlayer, ChipSettings
│   ├── dmc.h                 # DmcSample
│   ├── gm_names.h            # Имена 128 GM-инструментов
│   ├── i18n.h                # Интерфейс локализации
│   ├── midi_reader.h         # MidiFile, MidiNote
│   ├── preset.h              # Формат .nespreset
│   ├── util.h                # wide ↔ UTF-8
│   ├── version.h             # Имя, версия, автор
│   ├── wav_writer.h
│   ├── builtin_presets.h
│   └── env_editor.h          # GUI envelope-редактора
├── DPCM Samples/             # Пользовательские .dmc файлы
│   └── README.txt
├── Presets/
│   ├── default.nespreset
│   └── language.txt
├── third_party/imgui/        # Dear ImGui v1.91.5
├── icon.ico                  # Иконка приложения
├── app.rc                    # Ресурсный файл
├── Makefile
├── LICENSE                   # MIT
└── README.md
🔬 Как это работает
Парсер MIDI
Собственная реализация Standard MIDI File (форматы 0 и 1) — без внешних зависимостей:

Чтение MThd / MTrk чанков

Variable-Length Quantity (VLQ)

Running status, SysEx-события

События Note On/Off, Program Change, Tempo

Точное преобразование тиков → секунды с учётом смены темпа

Эмуляция APU
Каждый канал генерируется на сэмпл-уровне в NesApu::tick_one():

Канал	Метод
Pulse	Phase accumulator + сравнение с duty threshold
Triangle	16-шаговое bipolar преобразование
Noise	15-битный LFSR с tap bits 0,1 (long) или 0,6 (short)
DPCM	Побайтное чтение .dmc, MSB-first, ±2 к level (0..127)
Раскладка MIDI → APU
Drums (MIDI ch 9) → Noise или DPCM (по питчу)

Bass (низкие ноты) → Triangle

Мелодия / гармония → Pulse 1 или Pulse 2 (авто-балансировка: нота идёт в тот канал, что раньше освободится)

Формулы
Частота ноты:

text
f = 440 · 2^((pitch − 69) / 12)
ADSR-огибающая:

text
t < A           → t / A
t < A + D       → 1 − (1−S)·(t−A)/D
t < note_dur    → S
t < note_dur+R  → S·(1 − (t−note_dur)/R)
DMC rate (NTSC): DMC_RATE_HZ[index] — 16 значений от 4181 до 33143 Гц.

🛠️ Технические решения
Проблема	Решение
portsmf из MSYS2 урезан (нет smf.h, другой API)	Написан собственный парсер SMF (~250 строк)
libremidi отсутствует в репозитории MSYS2	Отказ от внешних зависимостей
Гонка между GUI и audio-callback при смене файла	SDL_LockAudioDevice + обнуление g_player перед delete
Preview DMC не слышен (конфликт с callback)	Отдельное SDL-устройство с SDL_QueueAudio
DMC играл как щелчки и низкий гул	Исправлена таблица частот: 4181…33143 Hz
Кириллица в fopen	_wfopen + WideCharToMultiByte
Envelope popup не закрывался	IsPopupOpen + CloseCurrentPopup перед сбросом индекса
🤝 Вклад
Pull requests приветствуются!

Для крупных изменений — сначала откройте issue для обсуждения

Соблюдайте стиль кода (C++17, 4 пробела, brace на той же строке)

Пишите осмысленные сообщения коммитов

📜 Лицензия
MIT License — см. LICENSE.

Вы можете свободно использовать, модифицировать и распространять этот проект при условии сохранения копирайта.

🙏 Благодарности
Dear ImGui (Omar Cornut) — гениальный GUI-фреймворк

SDL2 (Sam Lantinga) — кроссплатформенный аудио/видео ввод-вывод

Nesdev Wiki — документация по APU (LFSR, DPCM, NTSC rate table)

Сообщество чиптюн — за вдохновение и поддержку сцены 🎶

⭐ Если проект вам понравился — поставьте звезду на GitHub!
