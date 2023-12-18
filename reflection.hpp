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
    using Iterator = CharType *;
    using ConstIterator = const CharType *;
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

    constexpr ConstIterator getData() const {
        return data;
    }

    constexpr ConstIterator begin() const {
        return data;
    }

    constexpr ConstIterator end() const {
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

#define EMBER_REFLECTION(className) \
static std::unordered_map<std::string, Ember::Any>metaData; \
static std::unordered_map<std::string, Ember::Any>methodList; \
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
    auto method = methodList[methodName].to<Method>(); \
    return (static_cast<className*>(this)->*method)(std::forward<Args>(args)...); \
} \
std::unordered_map<std::string , Ember::Any>& listFieldProperty() const { \
    return metaData; \
} \
std::unordered_map<std::string, Ember::Any>& listMethodProperty() const { \
    return methodList; \
}

#define EMBER_REFLECTION_ANNOTATION(className) \
std::unordered_map<std::string, Ember::Any> className::metaData; \
std::unordered_map<std::string, Ember::Any> className::methodList; \

#define EMBER_PROPERTY(className, fieldName) \
int fieldName##InitMetaData = []() -> int { \
    metaData[#fieldName] = &className::fieldName; \
    return 0; \
}();

#define EMBER_METHOD(className, methodName) \
int methodName##InitMetaData = []() -> int { \
    methodList[#methodName] = &className::methodName; \
    return 0; \
}();

#define EMBER_PROPERTY_REGISTER(className, fieldName) \
metaData[#fieldName] = &className::fieldName;

#define EMBER_METHOD_REGISTER(className, methodName) \
methodList[#methodName] = &className::methodName;

#define EMBER_PROPERTY_REGISTER_ALIAS(className, fieldName, aliasName) \
metaData[aliasName] = &className::fieldName;

#define EMBER_METHOD_REGISTER_ALIAS(className, methodName, aliasName) \
methodList[aliasName] = &className::methodName;

class Monster {
public:
    EMBER_REFLECTION(Monster)

    int getHP() const{
        return HP;
    }

    void setHP(int value) {
        HP = value;
    }

    std::string getName() const {
        return name;
    }

    void setName(const std::string& value) {
        name = value;
    }

    EMBER_METHOD(Monster, attack)
    void attack() {
        HP -= 10;
    }

private:
    EMBER_PROPERTY(Monster, HP) int HP{};
    EMBER_PROPERTY(Monster, name) std::string name;
};
EMBER_REFLECTION_ANNOTATION(Monster)

void test() {
    Monster monster;
    monster.setHP(3);
    monster.setName("monster");
    auto t = monster.listFieldProperty();
    for (auto& d : t) {
        using MonsterInt = int Monster::*;
        using MonsterString = std::string Monster::*;
        if (d.first == "HP") {
            std::cout << d.first << ": " << monster.*d.second.to<MonsterInt>() << std::endl;
        } else if (d.first == "name") {
            std::cout << d.first << ": " << monster.*d.second.to<MonsterString>() << std::endl;
        }
    }
    std::cout << std::endl;
    auto m = monster.listMethodProperty();
    for (const auto& d : m) {
        std::cout << d.first << ' ';
    }
    std::cout << std::endl;
    std::cout << monster.getValue<int>("HP") << std::endl;
    monster.setValue<int>("HP", 16);
    std::cout << monster.getValue<int>("HP") << std::endl;
    monster.invokeMethod<>("attack");
    std::cout << monster.getValue<int>("HP") << std::endl;
}

} // namespace Ember



#endif //EMBER_REFLECTION_HPP
