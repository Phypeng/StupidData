#include "s_data.h"
#include <vector>

SData::SData(SData &&_data) noexcept
        : _object_map(std::move(_data._object_map))
{
}

SData &SData::operator=(SData &&_data) noexcept
{
    if(this != &_data)
    {
        _object_map = std::move(_data._object_map);
    }
    return *this;
}

SData &SData::operator=(const SData &_data)
{
    if(this != &_data)
    {
        _object_map = _data._object_map;
    }
    return *this;
}


SData SData::from_buffer(SIZE_TYPE length, unsigned char *buffer)
{
    if(length == 0 || !buffer)
    {
        return {};
    }

    SData data;

    char mgc;
    memcpy(&mgc, buffer, sizeof(MAGIC_TYPE));
    if(mgc != 0x0f)
    {
        return {};
    }
    SIZE_TYPE off = 0;
    off += sizeof(MAGIC_TYPE);

    SIZE_TYPE size_meta;
    memcpy(&size_meta, buffer + off, sizeof(SIZE_TYPE));
    off += sizeof(SIZE_TYPE);
    off += sizeof(RSV_TYPE);

    SIZE_TYPE meta_buf = off + size_meta;

    auto ptr = new_binary_ptr(length - meta_buf);
    memcpy(ptr->buffer, buffer + meta_buf, length - meta_buf);

    std::vector<std::pair<std::string, ValueInfo>> list;
    while(off < size_meta)
    {
        VALUE_TYPE type_id;
        KEY_LENGTH key_size;
        SIZE_TYPE offset;

        memcpy(&type_id, buffer + off, sizeof(VALUE_TYPE));
        off += sizeof(VALUE_TYPE);

        memcpy(&key_size, buffer + off, sizeof(KEY_LENGTH));
        off += sizeof(KEY_LENGTH);

        memcpy(&offset, buffer + off, sizeof(SIZE_TYPE));
        offset -= meta_buf;
        off += sizeof(SIZE_TYPE);

        std::string key((char *)(buffer + off), key_size);

        off += key_size;
        list.emplace_back(key, ValueInfo{type_id, ptr, offset, 0});
    }

    for(std::size_t i = 1; i < list.size(); i++)
    {
        list[i - 1].second.length = list[i].second.offset - list[i - 1].second.offset;
    }
    list[list.size() - 1].second.length = length - meta_buf - list[list.size() - 1].second.offset;

    for(const auto &i:list)
    {
        data._object_map[i.first] = i.second;
    }

    return data;
}

void SData::to_buffer(SIZE_TYPE &length, unsigned char *&buffer) const
{
    //sData header -->  SData_Magic : 1   MetaData_Size : 4  Reserve : 1
    SIZE_TYPE size_header = sizeof(MAGIC_TYPE) + sizeof(SIZE_TYPE) + sizeof(RSV_TYPE);

    SIZE_TYPE size_meta = 0;
    SIZE_TYPE size_buffer = 0;

    //sData meta --> Data_Type_ID : 2  Key_Size : 2 Value_Offset : 4
    for(auto &i:_object_map)
    {
        size_meta += (sizeof(VALUE_TYPE) + sizeof(KEY_LENGTH) + sizeof(SIZE_TYPE) + i.first.size() + 1);
        size_buffer += (i.second.length);
    }

    auto size = size_header + size_meta + size_buffer;
    auto buff = static_cast<unsigned char *>(malloc(size));

    char mgc = 0x0f;
    SIZE_TYPE off_meta = 0;
    SIZE_TYPE off_buff = size_header + size_meta;

    memcpy(buff + off_meta, &mgc, sizeof(MAGIC_TYPE));
    off_meta += sizeof(MAGIC_TYPE);

    memcpy(buff + off_meta, &size_meta, sizeof(SIZE_TYPE));
    off_meta += sizeof(SIZE_TYPE);

    char rsv = 0x00;
    memcpy(buff + off_meta, &rsv, sizeof(RSV_TYPE));
    off_meta += sizeof(RSV_TYPE);

    for(const auto &i:_object_map)
    {
        VALUE_TYPE type = i.second.type;
        memcpy(buff + off_meta, &type, sizeof(VALUE_TYPE));
        off_meta += sizeof(VALUE_TYPE);

        KEY_LENGTH len = i.first.size();
        memcpy(buff + off_meta, &len, sizeof(KEY_LENGTH));
        off_meta += sizeof(KEY_LENGTH);

        memcpy(buff + off_meta, &off_buff, sizeof(SIZE_TYPE));
        off_meta += sizeof(SIZE_TYPE);

        strcpy((char *)buff + off_meta, i.first.data());
        off_meta += len;

        memcpy(buff + off_buff, i.second.bin_ptr->buffer + i.second.offset, i.second.length);
        off_buff += i.second.length;
    }


    buffer = buff;
    length = size;
}

SData::type SData::type_info(const std::string &key) const
{
    auto iter = _object_map.find(key);
    if(iter != _object_map.end())
    {
        return type(iter->second.type);
    }
    else
    {
        return SData::Invalid;
    }
}

std::unordered_set<std::string> SData::keys() const
{
    std::unordered_set<std::string> set;
    for(const auto &i:_object_map)
    {
        set.insert(i.first);
    }
    return set;
}

SIZE_TYPE SData::count() const
{
    return _object_map.size();
}

unsigned char *SData::add_obj_and_get_buffer(const std::string &key, const SIZE_TYPE &length, const VALUE_TYPE &type_id)
{
    if(key.empty() || KEY_LENGTH(key.size()) != key.size())
    {
        return nullptr;
    }

    if(type_id == SData::Invalid)
    {
        return nullptr;
    }

    auto iter = _object_map.find(key);
    if(iter != _object_map.end())
    {
        return nullptr;
    }

    auto ptr = new_binary_ptr(length);
    _object_map.emplace(std::make_pair(key, ValueInfo{type_id, ptr, 0, length}));

    return ptr->buffer;
}

bool SData::remove(const std::string &key)
{
    auto iter = _object_map.find(key);
    if(iter == _object_map.end())
    {
        return false;
    }

    _object_map.erase(iter);
    return true;
}

std::shared_ptr<binary> SData::new_binary_ptr(std::size_t length)
{
    return std::shared_ptr<binary>(new binary{length, static_cast<unsigned char *>(malloc(length))}, [&](binary *bin)
    {
        free(bin->buffer);
        delete bin;
    });
}
