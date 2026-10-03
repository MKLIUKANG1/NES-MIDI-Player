DPCM Samples
============

Сюда кладите .dmc файлы (raw byte format, 1 bit per sample, MSB first).
Имя файла без расширения = имя сэмпла в редакторе.

Поддерживаются: *.dmc, *.bin (одинаковый формат).

Как сделать .dmc:
 - FamiTracker: File -> Export -> Export DMC
 - Или сконвертировать WAV через nesdoug.com/dcm или подобные утилиты
 - Или вручную: 1 bit = 1 сэмпл, byte отдан MSB-first

Рекомендации:
 - Kick:  короткий (~200-500 байт), записывайте при rate index 15
 - Snare: ~300-800 байт, rate index 12-15
 - HiHat: очень короткий, rate index 6-10
 - Cymbal: длинный, rate index 15
