#ifndef SDATA_H
#define SDATA_H

#include <unordered_map>
#include <utility>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <list>
#include <cstring>
#include <memory>

///////////////////
//  Stupid-Data  //
///////////////////

class SData;

class binary;

#define KEY_LENGTH std::int16_t
#define VALUE_TYPE std::int16_t
#define SIZE_TYPE std::size_t
#define MAGIC_TYPE std::int8_t
#define RSV_TYPE std::uint8_t

#define FOR_EACH_PRIMITIVE_TYPE(F)\
    F(Bool_s,1,bool) \
    F(Char_s,2,char) \
    F(SChar_s,3,std::int8_t) \
    F(UChar_s,4,std::uint8_t) \
    F(Int16_s,5,std::int16_t) \
    F(UInt16_s,6,std::uint16_t) \
    F(Int32_s,7,std::int32_t) \
    F(UInt32_s,8,std::uint32_t) \
    F(Int64_s,9,std::int64_t) \
    F(Uint64_s,10,std::uint64_t) \
    F(Float_s,11,float) \
    F(Double_s,12,double) \

#define FOR_EACH_STD_TYPE(F)\
    F(String_s,13,std::string) \

#define FOR_EACH_CUSTOM_TYPE(F)\
    F(Binary_s,14,binary) \
//    F(SData_s,15,SData) \

#define FOR_EACH_STATIC_TYPE(F)\
    FOR_EACH_PRIMITIVE_TYPE(F) \
    FOR_EACH_STD_TYPE(F) \
    FOR_EACH_CUSTOM_TYPE(F) \

#define TO_ENUM(TypeName, Id, Name)\
    TypeName = Id,\

#define TO_SWITCH(TypeName, Id, Name)\
    case TypeName : return #TypeName;            \


template<typename T>
struct TypeId_To
{
    enum
    {
        IsBuiltIn = false,
        MetaType = 0
    };

    static inline VALUE_TYPE type_id()
    { return MetaType; }
};

template<typename T>
struct TypeId_To<const T &> : TypeId_To<T>
{
};

template<typename T>
struct TypeId_To<T &> : TypeId_To<T>
{
};

#define BUILTIN_TYPE(TYPE, META_TYPE_ID, NAME)\
    template<> struct TypeId_To<NAME> \
    {                                       \
        enum {IsBuiltIn = true, MetaType = META_TYPE_ID }; \
        static inline VALUE_TYPE type_id() { return META_TYPE_ID; } \
    };\


FOR_EACH_STATIC_TYPE(BUILTIN_TYPE)

class binary
{
public:
    SIZE_TYPE length;
    unsigned char *buffer;
};


class SData
{
public:
    enum type
    {
        FOR_EACH_STATIC_TYPE(TO_ENUM)
        Invalid = 0
    };

    static SData from_buffer(SIZE_TYPE length, unsigned char *buffer);

    static std::string type_to_string(SData::type type)
    {
        switch(type)
        {
            case SData::Invalid:
                return "Invalid";
            FOR_EACH_STATIC_TYPE(TO_SWITCH)
            default:
                return "unknown";
        }
    }

    SData() = default;
    SData(const SData &_data) = default;
    SData(SData &&) noexcept;
    SData &operator=(const SData &);
    SData &operator=(SData &&) noexcept;
    ~SData() = default;

    void to_buffer(SIZE_TYPE &length, unsigned char *&buffer) const;

    std::unordered_set<std::string> keys() const;

    SIZE_TYPE count() const;

    SData::type type_info(const std::string &key) const;

    bool remove(const std::string &key);


    template<typename T>
    bool add_value(const std::string &key, const T &value)
    {
        if(!TypeId_To<T>::IsBuiltIn)
        {
            return false;
        }

        SIZE_TYPE length = sizeof(value);
        unsigned char *buffer = add_obj_and_get_buffer(key, length, TypeId_To<T>::type_id());
        if(!buffer)
        {
            return false;
        }

        memcpy(buffer, &value, length);
        return true;
    }

    template<typename T>
    T get_value(const std::string &key, bool *isOK = nullptr) const
    {
        if(!TypeId_To<T>::IsBuiltIn)
        {
            if(isOK)
            {
                *isOK = false;
            }
            return T();
        }
        auto iter = _object_map.find(key);
        if(iter == _object_map.end())
        {
            if(isOK)
            {
                *isOK = false;
            }
            return T();
        }
        if(TypeId_To<T>::type_id() != iter->second.type)
        {
            if(isOK)
            {
                *isOK = false;
            }
            return T();
        }
        if(sizeof(T) != iter->second.length)
        {
            if(isOK)
            {
                *isOK = false;
            }
            return T();
        }
        if(iter->second.offset + iter->second.length > iter->second.bin_ptr->length)
        {
            if(isOK)
            {
                *isOK = false;
            }
            return T();
        }

        T t;
        memcpy(&t, iter->second.bin_ptr->buffer + iter->second.offset, iter->second.length);
        if(isOK)
        {
            *isOK = true;
        }
        return t;
    }

private:
    struct ValueInfo
    {
        VALUE_TYPE type;
        std::shared_ptr<binary> bin_ptr;
        SIZE_TYPE offset;
        SIZE_TYPE length;
    };
    std::unordered_map<std::string, ValueInfo> _object_map;

    unsigned char *add_obj_and_get_buffer(const std::string &bin, const SIZE_TYPE &length, const VALUE_TYPE &type_id);

    static std::shared_ptr<binary> new_binary_ptr(SIZE_TYPE length);
};


template<>
inline bool SData::add_value<std::string>(const std::string &key, const std::string &value)
{
    SIZE_TYPE length = value.size();
    unsigned char *buffer = add_obj_and_get_buffer(key, length, TypeId_To<std::string>::type_id());
    if(!buffer)
    {
        return false;
    }

    strcpy((char *)buffer, value.data());
    return true;
}

template<>
inline std::string SData::get_value<std::string>(const std::string &key, bool *isOK) const
{
    auto iter = _object_map.find(key);
    if(iter == _object_map.end())
    {
        if(isOK)
        {
            *isOK = false;
        }
        return "";
    }

    if(TypeId_To<std::string>::type_id() != iter->second.type)
    {
        if(isOK)
        {
            *isOK = false;
        }
        return "";
    }

    if(iter->second.offset + iter->second.length > iter->second.bin_ptr->length)
    {
        if(isOK)
        {
            *isOK = false;
        }
        return "";
    }

    if(isOK)
    {
        *isOK = true;
    }

    return std::string((char *)(iter->second.bin_ptr->buffer + iter->second.offset), iter->second.length);
}


template<>
inline bool SData::add_value<binary>(const std::string &key, const binary &value)
{
    SIZE_TYPE length = value.length;
    unsigned char *buffer = add_obj_and_get_buffer(key, length, TypeId_To<binary>::type_id());
    if(!buffer)
    {
        return false;
    }

    memcpy(buffer, value.buffer, length);
    return true;
}

template<>
inline binary SData::get_value<binary>(const std::string &key, bool *isOK) const
{
    auto iter = _object_map.find(key);
    if(iter == _object_map.end())
    {
        if(isOK)
        {
            *isOK = false;
        }
        return {};
    }
    if(TypeId_To<std::string>::type_id() != iter->second.type)
    {
        if(isOK)
        {
            *isOK = false;
        }
        return {};
    }
    if(iter->second.offset + iter->second.length > iter->second.bin_ptr->length)
    {
        if(isOK)
        {
            *isOK = false;
        }
        return {};
    }

    if(isOK)
    {
        *isOK = true;
    }

    auto *buffer = static_cast<unsigned char *>(malloc(iter->second.length));
    memcpy(buffer, iter->second.bin_ptr->buffer + iter->second.offset, iter->second.length);
    return binary{iter->second.length, buffer};
}

#endif //SDATA_H
