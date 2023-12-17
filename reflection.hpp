//
// Created by Manjusaka on 2023/12/17.
//

#ifndef EMBER_REFLECTION_HPP
#define EMBER_REFLECTION_HPP

#include<iostream>
#include<string>
#include<unordered_map>
#include<memory>
#include<functional>
#include<memory>
#include<typeindex>

namespace Ember {

struct Any {

    Any(void) : tpIndex_(std::type_index(typeid(void))) {}
    Any(const Any& that) :
            ptr_(that.clone()),
            tpIndex_(that.tpIndex_) {}
    Any(Any&& that) :
            ptr_(std::move(that.ptr_)),
            tpIndex_(that.tpIndex_) {}

    template<typename U, class = typename std::enable_if<
            !std::is_same<typename std::decay<U>::type, Any>::value, U>::type> Any(U&& value) :
            ptr_(new AnyDerived<typename std::decay<U>::type>(std::forward<U>(value))),
            tpIndex_(std::type_index(typeid(typename std::decay<U>::type))) {}

    bool isNull() const {
        return !bool(ptr_);
    }

    template<class U>
    bool is() const {
        return tpIndex_ == std::type_index(typeid(U));
    }

    //转换为实际类型
    template<class U>
    U& to() {
        if (!is<U>()) {
            throw std::invalid_argument("type error");
        }

        auto* Derived = dynamic_cast<AnyDerived<U>*>(ptr_.get());
        return Derived->value_;
    }

    Any& operator=(const Any& a) {
        if (ptr_ == a.ptr_) {
            return *this;
        }
        ptr_ = a.clone();
        tpIndex_ = a.tpIndex_;
        return *this;
    }

private:
    struct AnyBase;
    using AnyBasePtr = std::unique_ptr<AnyBase>;

    struct AnyBase {
        virtual ~AnyBase() = default;
        virtual AnyBasePtr clone() const = 0;
    };

    template<typename T>
    struct AnyDerived : AnyBase {

        template<typename U>
        AnyDerived(U&& value) :
                value_(value) {}

        AnyBasePtr clone() const override {
            return AnyBasePtr(new AnyDerived<T>(value_));
        }

        T& ReturnValue() const {
            return value_;
        }

        T value_;
    };

    AnyBasePtr clone() const {
        if (ptr_ != nullptr) {
            return ptr_->clone();
        }
        return nullptr;
    }

    AnyBasePtr ptr_;
    std::type_index tpIndex_;

};

#if __cplusplus >= 201402L
template<typename T>
class FixedBasicString {
private:
    using CharType = T;
    CharType const *data;
    size_t length;
public:
    template <size_t N>
    constexpr FixedBasicString(CharType const (&data)[N]) : data(data), length(N - 1) {}
    constexpr FixedBasicString(CharType const *data, size_t size): data(data), length(size) {}

    constexpr FixedBasicString(const FixedBasicString&) noexcept = default;
    constexpr FixedBasicString &operator=(const FixedBasicString&) noexcept = default;

    constexpr size_t getLength() const {
        return length;
    }

    constexpr CharType operator[](size_t index) const {
        return data[index];
    }

    constexpr bool operator==(const FixedBasicString &other) const {
        if (length != other.length) {
            return false;
        }
        for (size_t i = 0;i < length; i++) {
            if (data[i] != other.data[i]) {
                return false;
            }
        }
        return true;
    }

    constexpr bool operator<(const FixedBasicString &other) const {
        unsigned i = 0;
        for (; i < length and i < other.length; i++) {
            if ((*this)[i] < other[i]) {
                return true;
            }
            if ((*this)[i] > other[i]) {
                return false;
            }
        }
        return length < other.length;
    }

    constexpr const CharType* getData() const {
        return data;
    }

    constexpr const CharType* begin() const {
        return data;
    }

    constexpr const CharType * end() const {
        return data + length;
    }
};

template<typename String>
constexpr size_t stringHash(const String& value) {
    size_t d = 5381;
    for (const auto c : value) {
        d = d * 33 + static_cast<size_t>(c);
    }
    return d;
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// With the lowest bits removed, based on experimental setup.
template<typename String>
constexpr size_t stringHash(const String& value, size_t seed) {
    std::size_t d = (0x811c9dc5 ^ seed) * static_cast<size_t>(0x01000193);
    for (const auto& c : value)
        d = (d ^ static_cast<size_t>(c)) * static_cast<size_t>(0x01000193);
    return d >> 8;
}

template<typename T>
struct StringHash {
    constexpr size_t operator() (T value) const {
        return stringHash(value);
    }
    constexpr size_t operator() (T value, size_t seed) const {
        return stringHash(value, seed);
    }
};

using FixedString = FixedBasicString<char>;
#endif

#if __cplusplus >= 201402L
class Object {
public:
    std::unordered_map<FixedString, Any, StringHash<FixedString>>metaData;
};
#else
class Object {
public:
    std::unordered_map<std::string, Any>metaData;
};
#endif

#if __cplusplus >= 201402L
#define EMBER_REFLECTION(className) \
template<typename T> \
T getValue(const Ember::FixedString& fieldName) { \
    return this->*metaData[fieldName].to<T className::*>(); \
} \
template<typename T> \
void setValue(const Ember::FixedString& fieldName, const T& value) { \
    this->*metaData[fieldName].to<T className::*>() = value; \
} \
template<typename T = void, typename... Args> \
T invokeMethod(const Ember::FixedString& methodName, Args &&... args) { \
    using Method = T (className::*)(Args...); \
    auto method = metaData[methodName].to<Method>(); \
    return (static_cast<className*>(this)->*method)(std::forward<Args>(args)...); \
} \

#else
#define EMBER_REFLECTION(className) \
template<typename T> \
T getValue(const std::string& fieldName) { \
    return this->*metaData[fieldName].to<T className::*>(); \
} \
template<typename T> \
void setValue(const std::string& fieldName, const T& value) { \
    this->*metaData[fieldName].to<T className::*>() = value; \
} \
template<typename T = void, typename... Args> \
T invokeMethod(const std::string& methodName, Args &&... args) { \
    using Method = T (className::*)(Args...); \
    auto method = metaData[methodName].to<Method>(); \
    return (static_cast<className*>(this)->*method)(std::forward<Args>(args)...); \
} \

#endif

#define EMBER_PROPERTY(className, fieldName) \
int fieldName##InitMetaData = [this]() -> int { \
    metaData[#fieldName] = &className::fieldName; \
    return 0; \
}();

#define EMBER_METHOD(className, methodName) \
int methodName##InitMetaData = [this]() -> int { \
    metaData[#methodName] = &className::methodName; \
    return 0; \
}();

#define EMBER_PROPERTY_REGISTER(className, fieldName) \
metaData[#fieldName] = &className::fieldName;

#define EMBER_METHOD_REGISTER(className, methodName) \
metaData[#methodName] = &className::methodName;

#define EMBER_PROPERTY_REGISTER_ALIAS(className, fieldName, aliasName) \
metaData[aliasName] = &className::fieldName;

#define EMBER_METHOD_REGISTER_ALIAS(className, methodName, aliasName) \
metaData[aliasName] = &className::methodName;

} // namespace Ember



#endif //EMBER_REFLECTION_HPP