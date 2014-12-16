// For conditions of distribution and use, see copyright notice in License.txt

#pragma once

#include "../Base/AutoPtr.h"
#include "../Base/Ptr.h"
#include "../IO/Stream.h"

namespace Turso3D
{

class JSONValue;
class Serializable;
class Stream;

/// Supported attribute types.
enum AttributeType
{
    ATTR_NONE = 0,
    ATTR_BOOL,
    ATTR_INT,
    ATTR_UNSIGNED,
    ATTR_BYTE,
    ATTR_FLOAT,
    ATTR_STRING,
    ATTR_VECTOR2,
    ATTR_VECTOR3,
    ATTR_VECTOR4,
    ATTR_QUATERNION,
    ATTR_BOUNDINGBOX,
    ATTR_RESOURCEREF,
    ATTR_RESOURCEREFLIST,
    ATTR_OBJECTREF,
    ATTR_JSONVALUE
};

/// Helper class for accessing serializable variables via getter and setter functions.
class TURSO3D_API AttributeAccessor
{
public:
    /// Destruct.
    virtual ~AttributeAccessor();
    
    /// Get the current value of the variable.
    virtual void Get(const Serializable* instance, void* dest) = 0;
    /// Set new value for the variable.
    virtual void Set(Serializable* instance, const void* source) = 0;
};

/// Description of an automatically serializable variable.
class TURSO3D_API Attribute : public RefCounted
{
public:
    /// Construct.
    Attribute(const char* name, AttributeAccessor* accessor, const char** enumNames = 0);
    
    /// Deserialize from binary.
    virtual void FromBinary(Serializable* instance, Stream& source) = 0;
    /// Serialize to binary.
    virtual void ToBinary(Serializable* instance, Stream& dest) = 0;
    /// Deserialize from JSON.
    virtual void FromJSON(Serializable* instance, const JSONValue& source) = 0;
    /// Serialize to JSON.
    virtual void ToJSON(Serializable* instance, JSONValue& dest) = 0;
    /// Return type.
    virtual AttributeType Type() const = 0;
    /// Return whether is default value.
    virtual bool IsDefault(Serializable* instance) = 0;
    
    /// Set from a value in memory.
    void FromValue(Serializable* instance, const void* source);
    /// Copy to a value in memory.
    void ToValue(Serializable* instance, void* dest);
    
    /// Return variable name.
    const String& Name() const { return name; }
    /// Return zero-based enum names, or null if none.
    const char** EnumNames() const { return enumNames; }
    
    /// Skip binary data of an attribute.
    static void Skip(AttributeType type, Stream& source);
    
protected:
    /// Variable name.
    String name;
    /// Attribute accessor.
    AutoPtr<AttributeAccessor> accessor;
    /// Enum names.
    const char** enumNames;

private:
    /// Prevent copy construction.
    Attribute(const Attribute& rhs);
    /// Prevent assignment.
    Attribute& operator = (const Attribute& rhs);
};

/// Template implementation of an attribute description with specific type.
template <class T> class AttributeImpl : public Attribute
{
public:
    /// Construct.
    AttributeImpl(const char* name, AttributeAccessor* accessor, const T& defaultValue_, const char** enumNames = 0) :
        Attribute(name, accessor, enumNames),
        defaultValue(defaultValue_)
    {
    }
    
    /// Deserialize from binary.
    void FromBinary(Serializable* instance, Stream& source) override
    {
        T value = source.Read<T>();
        FromValue(instance, &value);
    }
    
    /// Serialize to binary.
    void ToBinary(Serializable* instance, Stream& dest) override
    {
        T value;
        ToValue(instance, &value);
        dest.Write<T>(value);
    }
    
    /// Return whether is default value.
    bool IsDefault(Serializable* instance) override { return Value(instance) == defaultValue; }
    
    /// Deserialize from JSON.
    void FromJSON(Serializable* instance, const JSONValue& source) override;
    /// Serialize to JSON.
    void ToJSON(Serializable* instance, JSONValue& dest) override;
    /// Return type.
    AttributeType Type() const override;
    
    /// Set new attribute value.
    void SetValue(Serializable* instance, const T& source) { accessor->Set(instance, &source); }
    /// Copy current attribute value.
    void Value(Serializable* instance, T& dest) { accessor->Get(instance, &dest); }
    
    /// Return current attribute value.
    T Value(Serializable* instance)
    {
        T ret;
        accessor->Get(instance, &ret);
        return ret;
    }
    
    /// Return default value.
    const T& DefaultValue() const { return defaultValue; }
    
private:
    /// Default value.
    T defaultValue;
};

/// Template implementation for accessing serializable variables.
template <class T, class U> class AttributeAccessorImpl : public AttributeAccessor
{
public:
    typedef U (T::*GetFunctionPtr)() const;
    typedef void (T::*SetFunctionPtr)(U);

    /// Construct with function pointers.
    AttributeAccessorImpl(GetFunctionPtr getPtr, SetFunctionPtr setPtr) :
        get(getPtr),
        set(setPtr)
    {
        assert(get);
        assert(set);
    }

    /// Get current value of the variable.
    void Get(const Serializable* instance, void* dest) override
    {
        assert(instance);

        U& value = *(reinterpret_cast<U*>(dest));
        const T* classInstance = static_cast<const T*>(instance);
        value = (classInstance->*get)();
    }

    /// Set new value for the variable.
    void Set(Serializable* instance, const void* source) override
    {
        assert(instance);

        const U& value = *(reinterpret_cast<const U*>(source));
        T* classInstance = static_cast<T*>(instance);
        (classInstance->*set)(value);
    }

private:
    /// Getter function pointer.
    GetFunctionPtr get;
    /// Setter function pointer.
    SetFunctionPtr set;
};

/// Template implementation for accessing serializable variables via functions that use references.
template <class T, class U> class RefAttributeAccessorImpl : public AttributeAccessor
{
public:
    typedef const U& (T::*GetFunctionPtr)() const;
    typedef void (T::*SetFunctionPtr)(const U&);

    /// Set new value for the variable.
    RefAttributeAccessorImpl(GetFunctionPtr getPtr, SetFunctionPtr setPtr) :
        get(getPtr),
        set(setPtr)
    {
        assert(get);
        assert(set);
    }

    /// Get current value of the variable.
    void Get(const Serializable* instance, void* dest) override
    {
        assert(instance);

        U& value = *(reinterpret_cast<U*>(dest));
        const T* classPtr = static_cast<const T*>(instance);
        value = (classPtr->*get)();
    }

    /// Set new value for the variable.
    void Set(Serializable* instance, const void* source) override
    {
        assert(instance);

        const U& value = *(reinterpret_cast<const U*>(source));
        T* classPtr = static_cast<T*>(instance);
        (classPtr->*set)(value);
    }

private:
    /// Getter function pointer.
    GetFunctionPtr get;
    /// Setter function pointer.
    SetFunctionPtr set;
};

/// Template implementation for accessing serializable variables via functions where the setter uses reference, but the getter does not.
template <class T, class U> class MixedRefAttributeAccessorImpl : public AttributeAccessor
{
public:
    typedef U (T::*GetFunctionPtr)() const;
    typedef void (T::*SetFunctionPtr)(const U&);

    /// Set new value for the variable.
    MixedRefAttributeAccessorImpl(GetFunctionPtr getPtr, SetFunctionPtr setPtr) :
        get(getPtr),
        set(setPtr)
    {
        assert(get);
        assert(set);
    }

    /// Get current value of the variable.
    void Get(const Serializable* instance, void* dest) override
    {
        assert(instance);

        U& value = *(reinterpret_cast<U*>(dest));
        const T* classPtr = static_cast<const T*>(instance);
        value = (classPtr->*get)();
    }

    /// Set new value for the variable.
    void Set(Serializable* instance, const void* source) override
    {
        assert(instance);

        const U& value = *(reinterpret_cast<const U*>(source));
        T* classPtr = static_cast<T*>(instance);
        (classPtr->*set)(value);
    }

private:
    /// Getter function pointer.
    GetFunctionPtr get;
    /// Setter function pointer.
    SetFunctionPtr set;
};

}
