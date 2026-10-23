# 🎮 NES MIDI Player

[![Version](https://img.shields.io/badge/version-1.0.1-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey.svg)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B17-orange.svg)]()
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)
[![Built with](https://img.shields.io/badge/built%20with-MSYS2%20%2B%20MinGW--w64-red.svg)]()

**MIDI-плеер, который синтезирует музыку через программную эмуляцию звукового чипа NES (Ricoh 2A03 APU).**

**Автор:** MKLIUKANG1  
**Версия:** 1.0.1  
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

### 🥁 Noise Drum Slots *(новое в v1.0.1)*

- **8 слотов**: Kick, Snare, Snare 2, Clap, HiHat, HiHat Open, Tom, Cymbal
- **Независимые настройки для каждого слота**: LFSR mode (long/short), base rate (0–15), pitch offset, ADSR, envelope-кривые
- Per-slot настройки **применяются из MIDI** в реальном времени
- Кнопка **🔊 Test** — мгновенное прослушивание через отдельное SDL-устройство
- Fallback ADSR: при `D = 0` и `S = 0` автоматически ставится короткий удар

### 🥁 DPCM Drum Slots

- 8 слотов: **Kick, Snare, Snare 2, Clap, HiHat, HiHat Open, Tom, Cymbal**
- Автоматический маппинг по MIDI-питчам (35/36 → Kick, 38 → Snare, …)
- Загрузка `.dmc` / `.bin` файлов из папки `DPCM Samples/`
- Для каждого слота: sample, rate (0–15), volume, loop
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

- Формат `.nespreset` (**обратная совместимость** версий 1–9)
- Сохраняет всё: 128 инструментов, drum-слоты (DPCM + Noise), envelope-кривые, цвета инструментов
- Автозагрузка `Presets/Default.nespreset` при старте
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
- Иконка приложения
- Горячие клавиши: **Space** — play/pause, **Ctrl+O** — open, **Esc** — выход

---

## 📸 Скриншоты
<img width="1239" height="932" alt="NES MIDI Player" src="https://github.com/user-attachments/assets/34087928-b8ce-4a87-9578-d1b9595e6e59" />
<img width="1239" height="932" alt="NES MIDI Player - Instrument Editor" src="https://github.com/user-attachments/assets/08a1f4ce-65ff-4cd3-8298-1beec9dabd21" />


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
```

### 🚀 Быстрый старт

1. Запустите nes_midi_player_v1.0.1.exe
2. **Файл → Открыть MIDI… (Ctrl+O)** — выберите .mid файл.
3. **Вид → Редактор инструментов** — настройте звучание каналов.
4. **Вид → Piano Roll** — посмотрите и проверьте ноты трека.
5. **Файл → Экспорт в WAV…** — сохраните готовый результат.

### 🥁 Настройка Noise-барабанов

1. В редакторе откройте секцию **Drum Slots → Noise (per slot)**.
2. Выберите слот кнопкой: Kick / Snare / HiHat / …
3. Настройте параметры: 

  * **LFSR mode**: long (тоны) или short (металлическая перкуссия).
  * **Base Rate**: 0 (447 кГц) … 15 (440 Гц).
  * **Pitch offset**: от -24 до +24 полутона.
  * **ADSR и envelope-кривые** (Volume + Rate).
4. Нажмите **🔊 Test** для мгновенной проверки звука.
5. В панели транспорта переключите режим на **«Барабаны: Noise»**.

### 🥁 Настройка DPCM-барабанов

1. Положите файлы сэмплов .dmc в папку DPCM Samples/.
2. В редакторе откройте секцию **Drum Slots → DPCM Samples**.
3. Назначьте загруженные сэмплы на слоты Kick / Snare / HiHat / …
4. В панели транспорта нажмите кнопку **«Барабаны: DPCM»**.

### 📊 Графические envelope-кривые

В редакторе инструментов нажмите кнопку **Env** у нужного инструмента. Откроется окно со следующими кривыми: 

* **Volume (0–15)** — множитель громкости.
* **Pitch (-24..+24)** — сдвиг высоты в полутонах.
* **Duty (0–3)** — тип импульса (только для каналов Pulse).
* **Arp (-12..+12)** — арпеджио.
* **Rate (-15..+15)** — для канала Noise (заменяет Pitch).

💡 **Управление:** Рисуйте левой кнопкой мыши (**ЛКМ**), стирайте правой кнопкой мыши (**ПКМ**). Там же можно настроить длину огибающей и скорость. Все изменения применяются мгновенно! 

### 📁 Структура проекта

text

NES-MIDI-Player/
├── src/
│   ├── main.cpp              # GUI на Dear ImGui
│   ├── apu.cpp               # Ядро эмуляции NES APU
│   ├── midi_reader.cpp       # Собственный парсер SMF
│   ├── dmc.cpp               # Загрузка DMC-сэмплов
│   ├── i18n.cpp              # Локализация (EN/RU/ZH)
│   ├── preset.cpp            # Сохранение/загрузка пресетов
│   ├── wav_writer.cpp        # Экспорт в WAV
│   ├── builtin_presets.cpp   # 5 встроенных пресетов
│   ├── instrument_editor.cpp # FamiStudio-style редактор
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
│   ├── instrument_editor.h
│   └── env_editor.h          # GUI envelope-редактора
├── DPCM Samples/             # Пользовательские .dmc файлы
│   └── README.txt
├── Presets/
│   └── Default.nespreset
├── third_party/imgui/        # Dear ImGui v1.91.5
├── icon.ico                  # Иконка приложения
├── app.rc                    # Ресурсный файл
├── Makefile
├── LICENSE                   # MIT
└── README.md

Используйте код с осторожностью.

### 🔬 Как это работает

### Парсер MIDI

Собственная реализация Standard MIDI File (форматы 0 и 1) выполнена без внешних зависимостей: 

* Чтение чанков MThd / MTrk.
* Поддержка Variable-Length Quantity (VLQ).
* Обработка Running status и SysEx-событий.
* Отслеживание событий Note On/Off, Program Change, Tempo.
* Точное преобразование тиков в секунды с учётом динамической смены темпа.

### Эмуляция APU

Каждый канал генерируется на сэмпл-уровне внутри метода NesApu::tick_one(): 

КаналМетод модуляции
****Pulse****
Phase accumulator + сравнение с duty threshold
****Triangle****
16-шаговое bipolar преобразование
****Noise****
15-битный LFSR с tap-битами 0,1 (long) или 0,6 (short)
****DPCM****
Побайтное чтение .dmc, MSB-first, ±2 к уровню (0..127)

### Раскладка MIDI → APU

* **Drums (MIDI ch 9)** → Маршрутизируются на Noise (per-slot) или DPCM (в зависимости от питча).
* **Bass (низкие ноты)** → Направляются на канал Triangle.
* **Мелодия / гармония** → Распределяются на Pulse 1 или Pulse 2 (используется авто-балансировка: нота уходит в тот канал, который освободится раньше).

### Математические формулы

**Частота ноты:**

f=440⋅2pitch−6912f equals 440 center dot 2 raised to the the fraction with numerator p i t c h minus 69 and denominator 12 end-fraction power
𝑓=440⋅2𝑝𝑖𝑡𝑐ℎ−6912
 

**ADSR-огибающая:** 

* 𝑡

<𝐴

⟹𝑡

/𝐴
* 𝑡

<𝐴

+𝐷

⟹1

−

(

1

−𝑆

)

⋅𝑡−𝐴𝐷
* 𝑡

<𝑛𝑜𝑡𝑒

_𝑑𝑢𝑟

⟹𝑆
* 𝑡

<𝑛𝑜𝑡𝑒

_𝑑𝑢𝑟

+𝑅

⟹𝑆

⋅

(

1

−𝑡−𝑛𝑜𝑡𝑒_𝑑𝑢𝑟𝑅

)

*Примечание: для DMC rate (NTSC) используется таблица DMC_RATE_HZ[index] на 16 фиксированных значений от 4181 до 33143 Гц.* 

### 🛠️ Технические решения

Выявленная проблемаРеализованное решение
**Библиотека portsmf из MSYS2 урезана (нет smf.h, другой API)**
Написан собственный надежный парсер SMF (~250 строк).
**Библиотека libremidi полностью отсутствует в репозитории MSYS2**
Принято решение о полном отказе от тяжелых внешних зависимостей.
**Гонка данных между GUI и audio-callback при быстрой смене файла**
Добавлен SDL_LockAudioDevice + корректное обнуление g_player перед delete.
**Прослушивание (Preview) DMC неслышно из-за конфликта с основным callback**
Выделено отдельное SDL-устройство с использованием SDL_QueueAudio.
**Сэмплы DPCM воспроизводились как хаотичные щелчки и низкий гул**
Полностью переписана и исправлена аппаратная таблица частот: 4181…33143 Hz.
**Проблемы и вылеты из-за кириллицы в путях при вызове обычного fopen**
Реализована связка _wfopen + WideCharToMultiByte.
**Модальное окно Envelope popup не закрывалось при переключениях**
Добавлены проверки IsPopupOpen + явный CloseCurrentPopup перед сбросом индекса.
**Per-slot настройки noise drums не применялись из MIDI трека (v1.0.1)**
Внедрена явная сквозная маршрутизация через параметр ApuNote.drum_slot.
**Полная тишина при выставленных параметрах D=0 и S=0 (v1.0.1)**
Добавлен fallback-механизм: автоматически выставляется минимальная длина D = 0.08f.

### 🗓️ Changelog

### v1.0.1 (2026-10-23)

* 🐛 **Per-slot Noise drums** — настройки параметров Kick/Snare/HiHat/… теперь корректно применяются прямо из MIDI.
* 🐛 **Fallback ADSR** — при D=0 и S=0 генерируется короткий, четкий удар (0.08 s) вместо тишины.
* ✨ **ApuNote.drum_slot** — реализована явная и точная маршрутизация per-slot.
* 🧹 Полностью вырезаны лишние отладочные логи [DRUM-IN] / [NOISE-DRUM].
* 📜 В репозиторий добавлен официальный файл LICENSE (MIT).

### v1.0.0 (2026-10-03)

* 🎉 Первый публичный релиз плеера.

### 🤝 Вклад в проект

Pull requests только приветствуются! 

* Перед внесением крупных изменений — пожалуйста, сначала откройте новый issue для предварительного обсуждения.
* Строго соблюдайте общий стиль кода проекта: стандарт **C++17**, отступы в **4 пробела**, открывающая фигурная скобка { на той же строке.
* Пишите понятные и осмысленные сообщения для коммитов.

### 📜 Лицензия

Проект распространяется под лицензией **MIT License** — подробности см. в файле LICENSE.
Вы можете свободно использовать, модифицировать и распространять этот плеер при обязательном условии сохранения копирайта автора. 

### 🙏 Благодарности

* **Dear ImGui (Omar Cornut)** — за потрясающий, быстрый и удобный GUI-фреймворк.
* **SDL2 (Sam Lantinga)** — за надежный кроссплатформенный аудио/видео ввод-вывод.
* **Nesdev Wiki** — за подробнейшую документацию по архитектуре APU (LFSR, DPCM, NTSC rate table).
* **Сообщество чиптюн сцены** — за крутое вдохновение и постоянную поддержку! 🎶

⭐ **Если проект вам понравился — поставьте звезду на GitHub!**
