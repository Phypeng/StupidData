# StupidData

简单的序列化反序列化工具,用于以二进制形式传递任意数据结构.详见代码.

### 功能

#### 设置 Key 对应的 Value
    bool add_value(const std::string &key, const T &value);

#### 通过 Key 获取 Value 
    T get_value(const std::string &key, bool *isOK = nullptr) const;

#### 移除 Key
    bool remove(const std::string &key);

#### 列举 Key
    std::unordered_set<std::string> keys() const;

#### 获取 Key 的数量
    SIZE_TYPE count() const;

#### 根据 Key 获取对应 Value 的类型
    SData::type type_info(const std::string &key) const;

#### 将类型转换为文字
    static std::string type_to_string(SData::type type);

#### 序列化成二进制
    void to_buffer(SIZE_TYPE &length, unsigned char *&buffer) const;

#### 从二进制转换为 Data
    static SData from_buffer(SIZE_TYPE length, unsigned char *buffer);
