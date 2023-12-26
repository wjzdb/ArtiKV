#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <sys/types.h>
namespace art{


template<typename T>
concept SliceTrait = requires(T t){
    {t.data()} -> std::convertible_to<const uint8_t*>;
    {t.begin()} -> std::convertible_to<const uint8_t*>;
    {t.end()} -> std::convertible_to<const uint8_t*>;
    {t.ToString()} -> std::convertible_to<std::string_view>;
    {t.ToHexString()} -> std::convertible_to<std::string>;
    {t.size()} -> std::convertible_to<std::ptrdiff_t>;
};

class Slice{
private:
    const uint8_t *data_;
    std::ptrdiff_t len_;
public:
    Slice(const uint8_t* data, std::ptrdiff_t len):data_(data), len_(len){}
    Slice(const int8_t* data, std::ptrdiff_t len):data_(reinterpret_cast<const uint8_t*>(data)), len_(len){}
    Slice(const std::string_view s):data_(reinterpret_cast<const uint8_t*>(s.data())), len_(s.length()){}

    template<size_t N>
    Slice(const uint8_t (&a)[N]):data_(reinterpret_cast<const uint8_t*>(a)), len_(N){}
    
    template<size_t N>
    Slice(const int8_t (&a)[N]):data_(reinterpret_cast<const uint8_t*>(a)), len_(N){}
    
    template<size_t N>
    Slice(const char (&a)[N]):data_(reinterpret_cast<const uint8_t*>(a)), len_(N){}

    uint8_t operator[](size_t n) const {return data_[n];}
    uint8_t* data() const {return const_cast<uint8_t*>(data_);}
    std::ptrdiff_t size() const {return len_;}
    std::ptrdiff_t length() const {return len_;}
    bool empty() const {return len_ == 0;}

    const uint8_t* begin() const {return const_cast<uint8_t*>(data_);}
    const uint8_t* end() const {return const_cast<uint8_t*>(data_ + len_);}

    std::span<uint8_t> as_span() const {return std::span<uint8_t>(const_cast<uint8_t*>(data_), len_);}

    std::string_view ToString() const {return std::string_view(reinterpret_cast<const char*>(data_), len_);}
    std::string ToHexString() const {
        std::string s;
        s.reserve(len_ * 2);
        for (size_t i = 0; i < len_; i++){
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", data_[i]);
            s.append(buf);
        }
        return s;
    }
};
// assert that Slice is SliceBase
static_assert(SliceTrait<Slice>);

class OwnedSlice{
private:
struct OwnedBase{
    virtual ~OwnedBase() = default;
    virtual uint8_t* data() = 0;
    virtual std::ptrdiff_t size() const = 0;
    virtual std::unique_ptr<OwnedBase> clone() const = 0;
};

template<std::ranges::contiguous_range T>
struct Data : OwnedBase {
    T data_;
    Data(const T& t) : data_(t) {}
    virtual std::unique_ptr<OwnedBase> clone() const override {
        return std::make_unique<Data<T>>(data_);
    }
    virtual uint8_t* data() override {
        return reinterpret_cast<uint8_t*>(data_.data());
    }
    virtual std::ptrdiff_t size() const override {
        return data_.size();
    }
};
std::unique_ptr<OwnedBase> owned_;

template<typename T>
T& get_data() {
    return static_cast<Data<T>&>(*owned_).data;
}

public:
template<std::ranges::contiguous_range T>
OwnedSlice(T&& t) : owned_(std::make_unique<Data<T>>(std::move(t))) {}
template<std::ranges::contiguous_range T>
OwnedSlice(T& _) = delete;
OwnedSlice(OwnedSlice&& rhs) noexcept {
    owned_ = std::move(rhs.owned_);
}
OwnedSlice& operator=(OwnedSlice&& rhs) noexcept {
    owned_ = std::move(rhs.owned_);
    return *this;
}
OwnedSlice(const OwnedSlice& rhs) {
    owned_ = rhs.owned_->clone();
}
OwnedSlice& operator=(const OwnedSlice& rhs) {
    owned_ = rhs.owned_->clone();
    return *this;
}
uint8_t* data(){
    return owned_->data();
}
std::ptrdiff_t size() const {
    return owned_->size();
}
std::string_view ToString() const {
    return std::string_view(reinterpret_cast<const char*>(owned_->data()), owned_->size());
}
uint8_t* begin() const {
    return owned_->data();
}
uint8_t* end() const {
    return owned_->data() + owned_->size();
}
std::span<uint8_t> as_span() const {
    return std::span<uint8_t>(owned_->data(), owned_->size());
}
std::string ToHexString() const {
    std::string s;
    s.reserve(owned_->size() * 2);
    for (size_t i = 0; i < owned_->size(); i++){
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", owned_->data()[i]);
        s.append(buf);
    }
    return s;
}

};
static_assert(SliceTrait<OwnedSlice>);
}