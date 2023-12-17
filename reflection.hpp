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

class Object {
public:
    std::unordered_map<std::string, Any>metaData;
};

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
