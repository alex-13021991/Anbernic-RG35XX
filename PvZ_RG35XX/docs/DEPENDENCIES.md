# Происхождение и лицензии
Машиночитаемая фиксация исходных commit: provenance.json в этом каталоге.

| Компонент | Где хранится | Происхождение / условия |
|---|---|---|
| PvZ Portable Miyoo | sources/pvz, vendor/pvz-miyoo-2b3c06c-source.tar.gz | Fnava1/pvz-portable-miyoo-, commit 2b3c06c203cedf18193f53386b9111364a359e55; сохранены исходные уведомления |
| RG framebuffer/input | sources/pvz/platform/rg35xx | адаптация Jazz² Resurrection из alex-13021991/Anbernic-RG35XX; GPL-3.0-or-later |
| PopCap Framework | sources/pvz/src/SexyAppFramework | LICENSES/PopCap-Framework.txt |
| SDL-Mixer-X | sources/pvz/src/SexyAppFramework/sound/SDL-Mixer-X | LICENSES/SDL-Mixer-X.txt, исходные уведомления |
| libopenmpt 0.5.34 | vendor/libopenmpt-0.5.34.tar.gz | LICENSES/libopenmpt.txt, включает собственные third-party notices |
| ghc/filesystem 1.5.14 | sources/pvz/include/ghc | MIT, уведомление в заголовке |
| fmt, JPEG и прочие вложенные части | sources/pvz | лицензии и copyright сохранены в исходных каталогах/заголовках |
| SDK Miyoo GCC 7.3.0 | ВНЕ комплекта | используется установленный SDK из RG35XX-Development через RG_TOOLCHAIN |
| GarlicOS 1.4.9 sysroot | vendor/garlicos-1.4.9-sysroot.tar.gz | снимок среды RG35XX-Development; SDL2, ALSA, mpg123, ogg/vorbis, PNG/zlib и системные библиотеки сохраняют собственные лицензии |

Полные исходники игры/наших изменений и libopenmpt включены. Системный sysroot поставляется как бинарная среда разработки; внешний SDK не включён; это не заявление о включении исходников каждого системного пакета или GCC. При публичном распространении системных бинарных библиотек учитывайте обязательства их собственных лицензий; этот комплект не изменяет их.

Личный ready-to-play содержит ресурсы из предоставленной пользователем копии PC GOTY. Они исключены из исходного комплекта и публичного установщика. Не публикуйте личный архив.
