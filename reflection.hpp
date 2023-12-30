//
// Created by Manjusaka on 2023/12/27.
//

#ifndef EMBER_REFLECTION_HPP
#define EMBER_REFLECTION_HPP

#include "any.hpp"
#include "string_view.hpp"

namespace Ember {

class Field {
public:
    Field() = default;

    template<typename Class, typename T>
    Field(T Class::* var):
        field(var),
        fieldName(StringView(typeid(var).name())) {}

    StringView name() const {
        return fieldName;
    }

    void setName(StringView name) {
        fieldName = name;
    }

    template<typename T, typename Class>
    T getValue(const Class& obj) {
        return obj.*field.to<T Class::*>();
    }

    template<typename Class, typename T>
    void setValue(Class& obj, T val) {
        obj.*field.to<T Class::*>() = val;
    }

private:
    StringView fieldName;
    Any field;
};

class Method {
public:
    Method() = default;

    template<typename T = void, typename... Args, typename Class>
    Method(T (Class::* func)(Args...)):
        isConst(false),
        method(func),
        methodName(StringView(typeid(func).name())) {}

    template<typename T = void, typename... Args, typename Class>
    Method(T (Class::* func)(Args...) const):
        isConst(true),
        method(func),
        methodName(StringView(typeid(func).name())) {}

    StringView name() const {
        return methodName;
    }

    void setName(StringView name) {
        methodName = name;
    }

    template<typename T = void, typename Class, typename... Args>
    T invoke(Class& obj, Args... args) {
        using MethodType = T (Class::*)(Args...);
        using ConstMethodType = T (Class::*)(Args...) const;
        if (isConst) {
            return (static_cast<Class*>(&obj)->*method.to<ConstMethodType>())(std::forward<Args>(args)...);
        }
        return (static_cast<Class*>(&obj)->*method.to<MethodType>())(std::forward<Args>(args)...);
    }

private:
    StringView methodName;
    Any method;
    bool isConst{false};
};

namespace Builder {
    template<typename T>
    class ReflectionBuilder {
    public:
        ReflectionBuilder() = default;

        template<typename Class, typename R>
        ReflectionBuilder<T>& addField(StringView fieldName, R Class::* var) {
            Field field = var;
            field.setName(fieldName);
            type.fields.push_back(field);
            return *this;
        }

        template<typename Class, typename R, typename... Args>
        ReflectionBuilder<T>& addMethod(StringView methodName, R (Class::* func)(Args...)) {
            Method method = func;
            method.setName(methodName);
            type.methods.push_back(method);
            return *this;
        }

        template<typename Class, typename R, typename... Args>
        ReflectionBuilder<T>& addMethod(StringView methodName, R (Class::* func)(Args...) const) {
            Method method = func;
            method.setName(methodName);
            type.methods.push_back(method);
        }

        T build() const {
            return type;
        }
    private:
        T type;
    };
}

class Type {
public:
    Type() = default;

    StringView name() const {
        return typeName;
    }

    void setName(StringView name) {
        typeName = name;
    }

    const std::vector<Field>& listFields() const {
        return fields;
    }

    const std::vector<Method>& listMethods() const {
        return methods;
    }

    Field getField(StringView fieldName) const {
        for (const auto & field : fields) {
            if (field.name() == fieldName) {
                return field;
            }
        }
        return {};
    }

    Method getMethod(StringView methodName) const {
        for (const auto & method : methods) {
            if (method.name() == methodName) {
                return method;
            }
        }
        return {};
    }
private:
    friend Builder::ReflectionBuilder<Type>;

    StringView typeName;
    std::vector<Field> fields;
    std::vector<Method> methods;
};
using TypeBuilder = Builder::ReflectionBuilder<Type>;

class TypeRegister {
public:
    static TypeRegister& instance() {
        static TypeRegister instance;
        return instance;
    }

    void registerType(StringView typeName, const Type& type) {
        types[typeName] = type;
    }

    Type& getType(StringView typeName) {
        return types[typeName];
    }
private:
    std::unordered_map<StringView, Type, StringHash<StringView>> types;
};

class Reflect {
public:
    static Type TypeOf(StringView typeName) {
        return TypeRegister::instance().getType(typeName);
    }

    static void Register(StringView typeName, const TypeBuilder& builder) {
        TypeRegister::instance().registerType(typeName, builder.build());
    }

    template<typename T, typename... Args>
    static std::shared_ptr<T> ValueOf(Args &&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
};

#define EmberUseReflect(type) \
__attribute__((constructor)) void init##Type()

} // namespace Ember

#endif //EMBER_REFLECTION_HPP
