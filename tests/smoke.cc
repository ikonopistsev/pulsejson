#include <pulsejson/pulsejson.hh>

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cassert>
#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

enum class recovery_mode
{
    manual,
    auto_clear
};

struct module_cfg
{
    bool enabled{};
};

struct service_cfg
{
    std::string name;
    int timeout_ms{5000};
    std::optional<std::string> label;
    std::chrono::milliseconds boot_grace{std::chrono::milliseconds{3000}};
    recovery_mode recovery{};
    module_cfg module;
    std::vector<int> channels;
    std::vector<module_cfg> modules;
    std::map<std::string, bool> flags;
    std::map<std::string, recovery_mode> policies;
};

const auto module_desc = pulsejson::make_object<module_cfg>(
    pulsejson::val("enabled", &module_cfg::enabled));

const auto recovery_json = pulsejson::enum_string<recovery_mode>({
    {"Manual", recovery_mode::manual},
    {"AutoClear", recovery_mode::auto_clear},
});

const auto tolerant_service_desc = pulsejson::make_object<service_cfg>(
    pulsejson::must("name", &service_cfg::name),
    pulsejson::val("timeout_ms", &service_cfg::timeout_ms),
    pulsejson::val("label", &service_cfg::label),
    pulsejson::val("boot_grace_ms", &service_cfg::boot_grace),
    pulsejson::val("recovery", &service_cfg::recovery, recovery_json),
    pulsejson::obj("module", &service_cfg::module, module_desc),
    pulsejson::arr("channels", &service_cfg::channels),
    pulsejson::arr("modules", &service_cfg::modules, module_desc),
    pulsejson::dict("flags", &service_cfg::flags),
    pulsejson::dict("policies", &service_cfg::policies, recovery_json));

const auto strict_service_desc = pulsejson::make_object<service_cfg>(
    pulsejson::error_unknown,
    pulsejson::must("name", &service_cfg::name),
    pulsejson::val("timeout_ms", &service_cfg::timeout_ms));

void expect_throw_contains(const std::function<void()>& fn,
    const std::string_view expected)
{
    try {
        fn();
    } catch (const std::runtime_error& e) {
        const std::string_view message{e.what()};
        assert(message.find(expected) != std::string_view::npos);
        return;
    }

    assert(false && "expected std::runtime_error");
}

void test_roundtrip()
{
    auto cfg = tolerant_service_desc.parse(R"json({
        "name": "pulse-safetyd",
        "timeout_ms": 30000,
        "label": null,
        "boot_grace_ms": 30000,
        "recovery": "Manual",
        "module": {"enabled": true},
        "channels": [1, 2, 3],
        "modules": [{"enabled": true}, {"enabled": false}],
        "flags": {"watchdog": true},
        "policies": {"normal": "AutoClear"},
        "future_field": "ignored"
    })json");

    assert(cfg.name == "pulse-safetyd");
    assert(cfg.timeout_ms == 30000);
    assert(!cfg.label);
    assert(cfg.boot_grace == std::chrono::milliseconds{30000});
    assert(cfg.recovery == recovery_mode::manual);
    assert(cfg.module.enabled);
    assert(cfg.channels.size() == 3);
    assert(cfg.modules.size() == 2);
    assert(cfg.flags.at("watchdog"));
    assert(cfg.policies.at("normal") == recovery_mode::auto_clear);

    const auto compact = tolerant_service_desc.to_string(cfg);
    assert(compact.find("\"name\":\"pulse-safetyd\"") != std::string::npos);
    assert(compact.find("\"label\":null") != std::string::npos);
    assert(compact.find("\"recovery\":\"Manual\"") != std::string::npos);
}

void test_default_constructed_parse()
{
    const auto cfg = tolerant_service_desc.parse(R"json({
        "name": "pulse-healthd",
        "future_field": true
    })json");

    assert(cfg.name == "pulse-healthd");
    assert(cfg.timeout_ms == 5000);
    assert(cfg.boot_grace == std::chrono::milliseconds{3000});
    assert(!cfg.label);
    assert(!cfg.module.enabled);
    assert(cfg.channels.empty());
}

void test_generator_writers()
{
    service_cfg cfg;
    cfg.name = "pulse-safetyd";
    cfg.timeout_ms = 30000;
    cfg.label = "Safety";
    cfg.boot_grace = std::chrono::milliseconds{30000};
    cfg.recovery = recovery_mode::auto_clear;
    cfg.module.enabled = true;
    cfg.channels = {1, 2};
    cfg.modules = {{true}};
    cfg.flags = {{"watchdog", true}};
    cfg.policies = {{"normal", recovery_mode::manual}};

    rapidjson::StringBuffer writer_buf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(writer_buf);
    tolerant_service_desc.write_to(writer, cfg);

    const std::string generated{writer_buf.GetString(),
        writer_buf.GetSize()};
    const std::string expected{
        R"json({"name":"pulse-safetyd","timeout_ms":30000,"label":"Safety","boot_grace_ms":30000,"recovery":"AutoClear","module":{"enabled":true},"channels":[1,2],"modules":[{"enabled":true}],"flags":{"watchdog":true},"policies":{"normal":"Manual"}})json"};
    assert(generated == expected);

    rapidjson::StringBuffer pretty_buf;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> pretty_writer(pretty_buf);
    tolerant_service_desc.write_to(pretty_writer, cfg);

    const std::string pretty{pretty_buf.GetString(), pretty_buf.GetSize()};
    assert(pretty.find('\n') != std::string::npos);
    assert(pretty.find(R"json("name": "pulse-safetyd")json") != std::string::npos);
    assert(pretty.find(R"json("recovery": "AutoClear")json") != std::string::npos);
}

void test_validation()
{
    expect_throw_contains([] {
        service_cfg cfg;
        tolerant_service_desc.read(R"json({"timeout_ms": 100})json", cfg);
    }, "name");

    expect_throw_contains([] {
        service_cfg cfg;
        strict_service_desc.read(
            R"json({"name": "pulse-safetyd", "unknown": true})json", cfg);
    }, "unknown field");

    expect_throw_contains([] {
        service_cfg cfg;
        tolerant_service_desc.read(
            R"json({"name": "pulse-safetyd", "timeout_ms": null})json", cfg);
    }, "timeout_ms");

    expect_throw_contains([] {
        service_cfg cfg;
        tolerant_service_desc.read(R"json({"name": "pulse-safetyd")json", cfg);
    }, "parse error at offset");

    expect_throw_contains([] {
        service_cfg cfg;
        tolerant_service_desc.read(
            R"json({"name": "pulse-safetyd", "recovery": "Unknown"})json", cfg);
    }, "unknown enum literal");
}

} // namespace

int main()
{
    test_roundtrip();
    test_default_constructed_parse();
    test_generator_writers();
    test_validation();
}
