#pragma once

#include <cstddef>
#include <cstdint>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace pulsejson::detail {

// ---------------------------------------------------------------------------
// IWriter — abstract writer interface.
//
// Decouples the library's virtual dispatch hierarchy from any specific
// rapidjson output stream.  Concrete instances are created via make_writer().
// ---------------------------------------------------------------------------
struct IWriter
{
    virtual ~IWriter() = default;

    virtual void null()                                          = 0;
    virtual void bool_(bool v)                                   = 0;
    virtual void int_(int v)                                     = 0;
    virtual void uint_(unsigned v)                               = 0;
    virtual void int64_(std::int64_t v)                          = 0;
    virtual void uint64_(std::uint64_t v)                        = 0;
    virtual void double_(double v)                               = 0;
    virtual void string_(const char* s, std::size_t len)         = 0;
    virtual void start_object()                                  = 0;
    virtual void key_(const char* s, std::size_t len)            = 0;
    virtual void end_object()                                    = 0;
    virtual void start_array()                                   = 0;
    virtual void end_array()                                     = 0;
};

// ---------------------------------------------------------------------------
// RapidWriterAdapter<RW> — adapts any rapidjson writer to IWriter.
//
// Works with rapidjson::Writer<S>, rapidjson::PrettyWriter<S>, or any type
// that implements the rapidjson Handler concept.
// ---------------------------------------------------------------------------
template <typename RW>
class RapidWriterAdapter final : public IWriter
{
    RW& rw_;

public:
    explicit RapidWriterAdapter(RW& rw) noexcept : rw_(rw) {}

    void null()                                    override { rw_.Null(); }
    void bool_(bool v)                             override { rw_.Bool(v); }
    void int_(int v)                               override { rw_.Int(v); }
    void uint_(unsigned v)                         override { rw_.Uint(v); }
    void int64_(std::int64_t v)                    override { rw_.Int64(v); }
    void uint64_(std::uint64_t v)                  override { rw_.Uint64(v); }
    void double_(double v)                         override { rw_.Double(v); }

    void string_(const char* s, std::size_t len) override
    {
        rw_.String(s, static_cast<rapidjson::SizeType>(len));
    }

    void start_object()                            override { rw_.StartObject(); }

    void key_(const char* s, std::size_t len) override
    {
        rw_.Key(s, static_cast<rapidjson::SizeType>(len));
    }

    void end_object()                              override { rw_.EndObject(); }
    void start_array()                             override { rw_.StartArray(); }
    void end_array()                               override { rw_.EndArray(); }
};

// ---------------------------------------------------------------------------
// make_writer(rw) — convenience factory; deduces RW from the argument.
// ---------------------------------------------------------------------------
template <typename RW>
RapidWriterAdapter<RW> make_writer(RW& rw) noexcept
{
    return RapidWriterAdapter<RW>(rw);
}

} // namespace pulsejson::detail
