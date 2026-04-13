# Changelog

All notable changes to pulsejson are documented here.

## [0.1.1] - 2026-04-12

### Changed

- `kv()` renamed to `val()` — читается естественнее рядом с `obj()`, `arr()`, `dict()`, `must()`.
- `sorted_pairs` использует `std::less<>` (прозрачный компаратор) — позволяет `map::find()` принимать `std::string_view` без создания `std::string`.
- `sort_next::operator()` принимает `const std::pair<std::string, Value>&` вместо non-const ref.
- `value_pointer` изменён с `value<C>*` на `const value<C>*` — все виртуальные методы `value<C>` являются `const`.

### Fixed

- `object_t::read()` — ключи JSON теперь обрабатываются через `std::string_view` вместо `std::string`, устраняя heap-аллокацию на каждый ключ при каждом вызове `parse()`/`read()`. `seen`-сет также переведён на `string_view`.
- `pulsejson-config.cmake` — корректное создание `RapidJSON::RapidJSON` для старых инсталляций RapidJSON, которые устанавливают только переменную `RAPIDJSON_INCLUDE_DIRS` без CMake-таргетов (в частности, системный пакет Gentoo 1.1.0).

### Added

- `array_t::read()` вызывает `out.reserve(v.Size())` перед заполнением контейнера, если контейнер поддерживает `reserve()` (через `if constexpr`). Устраняет лишние реаллокации при парсинге больших массивов в `std::vector`.

---

## [0.1.0] - 2026-04-05

### Added

- Первый выпуск. Struct-descriptor JSON-библиотека на базе rapidjson.
- Регистрация полей через member-pointer: `val`, `obj`, `arr`, `dict`, `must`, `dummy`.
- Поддержка типов: `bool`, `int`, `unsigned`, `int64_t`, `uint64_t`, `double`, `float`, `std::string`, `std::optional<T>`, `std::chrono::milliseconds`.
- Перечисления через `enum_string()` с пользовательской таблицей строковых имён.
- Вложенные объекты (`obj`/`must_obj`) и массивы (`arr`/`must_arr`).
- Словари (`dict`/`must_dict`) — `std::map<std::string, T>`.
- Политика неизвестных ключей: `ignore_unknown` (по умолчанию) и `error_unknown`.
- Сериализация в compact JSON (`to_string`), pretty JSON (`to_pretty_string`), произвольный rapidjson writer (`write_to`).
- Абстракция `IWriter` / `RapidWriterAdapter<RW>` для поддержки `Writer` и `PrettyWriter` без смены иерархии виртуального диспетчеризирования.
- Standalone-сборка: собственный `project(pulsejson)`, автоопределение rapidjson, `pulsejson-config.cmake` для `find_package`.
