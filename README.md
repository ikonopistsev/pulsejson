# pulsejson

`pulsejson` is a header-only, RapidJSON-backed struct-descriptor library for
pulsemou service configuration files.

Declaration style is member-pointer based and intentionally close to the old
[json2](../../../exampl/json2/) prototype, but parsing and serialization are
delegated to RapidJSON instead of a custom scanner.

## Purpose

`pulsejson` exists to give pulsemou services a small and explicit way to
generate JSON from C++ structs and, secondarily, read JSON configuration files
without manual parse loops.

Target use cases:
- service startup configuration files
- safety/health policy tables
- integration-test snapshots
- diagnostic state dumps

Non-goals:
- high-throughput streaming or large-document processing
- full JSON schema validation
- generic DOM manipulation
- DBus runtime parameter values

Generic UI/control values must stay in native DBus types. JSON is only a
configuration and diagnostic interchange format here.

## Design Goals

| Goal | Implementation |
|---|---|
| Header-only dependency | `pulsejson` is an `INTERFACE` CMake target with public alias `PulseJSON::pulsejson` |
| Type-safe descriptors | field types are deduced from member pointers |
| RapidJSON output compatibility | descriptors write directly to `Writer`, `PrettyWriter` or any RapidJSON handler-style writer |
| Explicit config invariants | mandatory fields, null handling and enum literals fail at parse time |
| Forward-compatible configs | unknown keys are ignored by default, or rejected with `error_unknown` |
| Small system-library style | no shared ownership, no runtime schema engine, no dynamic DOM abstraction |

## Supported Types

Scalar types:
- `bool`
- `int`
- `unsigned`
- `std::int64_t`
- `std::uint64_t`
- `double`
- `float`
- `std::string`
- `std::optional<T>`
- `std::chrono::milliseconds`

Containers:
- sequential containers with `value_type`, `clear()` and `push_back()`, for example `std::vector<T>` and `std::list<T>`
- string-keyed associative containers with `mapped_type`, for example `std::map<std::string, T>`
- enum values through `pulsejson::enum_string<T>`
- nested structs through `pulsejson::object_t<T>`

`null` is accepted only by readers that explicitly support it, currently
`std::optional<T>`. For plain scalar fields, `null` is a type error.

## Descriptor API

```cpp
#include <pulsejson/pulsejson.hh>

static const auto desc = pulsejson::make_object<MyStruct>(
    pulsejson::must("name", &MyStruct::name),
    pulsejson::kv("timeout_ms", &MyStruct::timeout),
    pulsejson::arr("ids", &MyStruct::ids)
);
```

Unknown keys are ignored by default. Use `pulsejson::error_unknown` when a
config file should reject typos:

```cpp
static const auto strict_desc = pulsejson::make_object<MyStruct>(
    pulsejson::error_unknown,
    pulsejson::must("name", &MyStruct::name),
    pulsejson::kv("timeout_ms", &MyStruct::timeout)
);
```

Field helpers:

```cpp
pulsejson::kv("name", &T::name)
pulsejson::obj("sub", &T::sub, sub_desc)
pulsejson::arr("ids", &T::ids)
pulsejson::arr("items", &T::items, item_desc)
pulsejson::dict("flags", &T::flags)
pulsejson::dict("policies", &T::policies, policy_json)

pulsejson::must("name", &T::name)
pulsejson::must_obj("sub", &T::sub, sub_desc)
pulsejson::must_arr("ids", &T::ids)
pulsejson::must_dict("policies", &T::policies, policy_json)
```

`must*` is the preferred mandatory spelling. Older `required*` aliases are kept
for compatibility:

```cpp
pulsejson::required("name", &T::name)
pulsejson::required_obj("sub", &T::sub, sub_desc)
pulsejson::required_arr("ids", &T::ids)
pulsejson::required_dict("policies", &T::policies, policy_json)
```

Legacy-compatible long forms are still available:

```cpp
pulsejson::pair("name", &T::name)
pulsejson::pair("sub", pulsejson::object(&T::sub, sub_desc))
pulsejson::pair("ids", pulsejson::array(&T::ids))
```

Enum mapping:

```cpp
enum class recovery_policy {
    manual,
    auto_clear,
    reboot_required
};

static const auto recovery_policy_json =
    pulsejson::enum_string<recovery_policy>({
        {"Manual", recovery_policy::manual},
        {"AutoClear", recovery_policy::auto_clear},
        {"RebootRequired", recovery_policy::reboot_required},
    });

static const auto desc = pulsejson::make_object<cfg>(
    pulsejson::kv("policy", &cfg::policy, recovery_policy_json)
);
```

Forward-compatibility for removed fields:

```cpp
pulsejson::dummy<std::string, T>("deprecated_field")
```

`dummy` consumes a legacy input key and emits `"deprecated_field": null` during
serialization.

## Reading

```cpp
MyStruct obj;

// Preferred config entry point: constructs MyStruct{} before reading.
MyStruct cfg = desc.parse(json_string_view);

// Low-level overlay read into an existing object.
desc.read(json_string_view, obj);
desc.read(rapidjson_value, obj);
```

Read behavior:
- `parse()` uses `MyStruct{}` first, so missing optional fields keep default-constructor values
- `read()` overlays into an existing object, so missing optional fields keep the existing values
- missing mandatory fields throw `std::runtime_error`
- unknown fields are ignored unless `error_unknown` is enabled
- parse errors include RapidJSON error offset
- type errors include field/index context where possible

## Generation

```cpp
std::string compact = desc.to_string(obj);
std::string pretty = desc.to_pretty_string(obj);

rapidjson::StringBuffer buf;
rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
desc.write_to(writer, obj);
```

`write_to()` is the primary output API: it works with both
`rapidjson::Writer` and `rapidjson::PrettyWriter`.

## Example

```cpp
#include <pulsejson/pulsejson.hh>
#include <chrono>
#include <map>
#include <string>
#include <vector>

struct recovery_cfg {
    enum class policy {
        manual,
        auto_clear,
        reboot_required
    };

    policy mode = policy::manual;
    std::chrono::milliseconds timeout{};
};

struct safetyd_cfg {
    std::string service_id;
    recovery_cfg recovery;
    std::vector<std::string> ignored_services;
    std::map<std::string, bool> service_enabled;
};

static const auto recovery_policy_json =
    pulsejson::enum_string<recovery_cfg::policy>({
        {"Manual", recovery_cfg::policy::manual},
        {"AutoClear", recovery_cfg::policy::auto_clear},
        {"RebootRequired", recovery_cfg::policy::reboot_required},
    });

static const auto recovery_desc = pulsejson::make_object<recovery_cfg>(
    pulsejson::kv("policy", &recovery_cfg::mode, recovery_policy_json),
    pulsejson::kv("timeout_ms", &recovery_cfg::timeout)
);

static const auto safetyd_desc = pulsejson::make_object<safetyd_cfg>(
    pulsejson::error_unknown,
    pulsejson::must("service_id", &safetyd_cfg::service_id),
    pulsejson::obj("recovery", &safetyd_cfg::recovery, recovery_desc),
    pulsejson::arr("ignored_services", &safetyd_cfg::ignored_services),
    pulsejson::dict("service_enabled", &safetyd_cfg::service_enabled)
);

auto cfg = safetyd_desc.parse(R"({
    "service_id": "pulse_safetyd",
    "recovery": {"policy": "AutoClear", "timeout_ms": 30000},
    "ignored_services": ["pulse_psu_deviced"],
    "service_enabled": {"pulse_healthd": true}
})");

std::string out = safetyd_desc.to_pretty_string(cfg);
```

## CMake Integration

`pulsejson` is included as a bundled dependency via the root `CMakeLists.txt`.
It links against a RapidJSON CMake target when one exists. The preferred target
name is `RapidJSON::RapidJSON`; `RapidJSON` is also accepted for compatibility
and normalized to `RapidJSON::RapidJSON`. If neither target exists, `pulsejson`
falls back to
`PULSEJSON_RAPIDJSON_INCLUDE_DIR`.

```cmake
option(PULSEMOU_WITH_BUNDLED_RAPIDJSON ...)
option(PULSEMOU_WITH_BUNDLED_PULSEJSON ...)
```

When both are enabled, `pulsejson` is linked into `pulsemou-deps`, so services
can include it without extra target wiring:

```cpp
#include <pulsejson/pulsejson.hh>
```

For standalone use:

```cmake
add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
target_include_directories(RapidJSON::RapidJSON INTERFACE /path/to/rapidjson/include)

add_subdirectory(path/to/pulsejson)
target_link_libraries(my-target PRIVATE PulseJSON::pulsejson)
```

Tests are registered as `pulsejson_smoke` when `BUILD_TESTING` and
`PULSEJSON_BUILD_TESTS` are enabled.

## File Layout

```text
deps/pulsejson/
    CMakeLists.txt
    README.md
    include/
        pulsejson/
            pulsejson.hh
            object.hh
            array.hh
            dict.hh
            enum.hh
            value.hh
            detail/
                error.hh
                writer.hh
                scalars.hh
                prototype.hh
                tuple.hh
    tests/
        smoke.cc
```
