#ifndef __MEMORY_VARIABLE_TYPE__H__
#define __MEMORY_VARIABLE_TYPE__H__

class MemoryVariableType : public QEnableSharedFromThis<MemoryVariableType> {
public:
  //! Constructor
  MemoryVariableType() = default;

  //! Deconstructor
  virtual ~MemoryVariableType() = default;
  
  //! The size in byts
  uint32_t getSizeInBytes() const { return (getSizeInBits() + 7) / 8; }

  //! The size in bits
  virtual uint32_t getSizeInBits() const = 0;

  //! Returns the type ID of this memory variable
  virtual const char *type() const = 0;

  //! Returns the type as nonreference
  virtual MemoryVariableTypePtr resolved() { return sharedFromThis(); }

  //! Returns the type as nonreference
  virtual QSharedPointer<const MemoryVariableType> resolved() const { return sharedFromThis(); }

  //! Resolves this variable
  virtual MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
    std::cerr << "Unable to resolve type " << type() << std::endl;
    return MemoryVariablePtr();
  }
};

class MemoryVariableTypeNull : public MemoryVariableType {
public:
  //! Returns the type IF of this memory variable
  static const char *Type() { return "null"; }
  virtual const char *type() const { return Type(); }
  virtual uint32_t getSizeInBits() const { return 0; }
};

class MemoryVariableStandardType : public MemoryVariableType {
public:
  //! Sets the size in bits
  void setSizeInBits(uint32_t size) { sizeInBits = size; }

  //! The size in bits
  uint32_t getSizeInBits() const { return sizeInBits; }

protected:
  //! The size in bits
  uint32_t sizeInBits = 0;
};

class MemoryVariableTypeNumber : public MemoryVariableType {
public:
  //! Constructor
  MemoryVariableTypeNumber(uint32_t sizeInBits, bool isSigned)
    : sizeInBits(sizeInBits)
    , isSigned(isSigned)
  {}

  //! Returns the type IF of this memory variable
  static const char *Type() { return "number"; }
  virtual const char *type() const { return Type(); }

  //! The size in bits
  uint32_t getSizeInBits() const { return sizeInBits; }

  //! Resolves this variable
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) override;

private:
  uint32_t sizeInBits;
  bool isSigned;
};

class MemoryVariableTypeUnknown : public MemoryVariableType {
public:
  //! Returns the type IF of this memory variable
  static const char *Type() { return "unknown"; }
  virtual const char *type() const { return Type(); }

  //! The size in bits
  uint32_t getSizeInBits() const { return 0; }
};

class MemoryVariableTypeEnum : public MemoryVariableStandardType {
public:
  struct Member {
    Member() {}
    Member(const QString &id, const QString &name, uint32_t value)
      : id(id), name(name), value(value)
    {}
    
    QString id;
    QString name;
    uint32_t value;
  };

  //! Returns the type IF of this memory variable
  static const char *Type() { return "enum"; }
  virtual const char *type() const { return Type(); }

  //! Adds a value
  void add(const QString &id, const QString &name, uint32_t value) {
    members.insert(value, Member(id, name, value));
  }

  //! Returns the name of a member
  inline Member getMember(uint32_t value) const { return members[value]; }

  //! Resolves this variable
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) override;

protected:
  //! List of all enum types
  QHash<uint32_t, Member> members;
};

class MemoryVariableTypeFlags : public MemoryVariableStandardType {
public:
  struct Member {
    Member() {}
    Member(const QString &id, const QString &name, uint32_t offset, uint32_t bits, const MemoryVariableTypePtr &type)
      : id(id), name(name), offset(offset), bits(bits), type(type) {}

    QString id;
    QString name;
    uint32_t offset;
    uint32_t bits;
    MemoryVariableTypePtr type;
  };

  //! Returns the type IF of this memory variable
  static const char *Type() { return "flags"; }
  virtual const char *type() const { return Type(); }

  //! Adds a member
  void add(const QString &id, const QString &name, uint32_t offset, uint16_t size, const MemoryVariableTypePtr &type) {
    members.push_back(Member(id, name, offset, size, type));
  }
  //! Resolves this variable
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) override;


protected:
  QVector<Member> members;
};

class MemoryVariableTypeStruct : public MemoryVariableStandardType {
public:
  struct Member {
    Member() {}
    Member(const QString &id, const QString &name, const MemoryReferencePtr &offset, const MemoryVariableTypePtr &type)
      : id(id), name(name), offset(offset), type(type) {}

    inline bool valid() const { return id != ""; }

    QString id;
    QString name;
    MemoryReferencePtr offset;
    MemoryVariableTypePtr type;
  };

  //! Returns the type IF of this memory variable
  static const char *Type() { return "struct"; }
  virtual const char *type() const { return Type(); }

  //! Builds the struct context
  virtual MemoryContextPtr buildStructContext(const MemoryContextPtr &context, uint32_t address) const;

  //! Adds a member
  void add(const QString &id, const QString &name, const MemoryReferencePtr &offset, const MemoryVariableTypePtr &type) {
    members.push_back(Member(id, name, offset, type));
  }

  //! Resolves this structure
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address);

  //! Returns all members
  MemoryVariablePtr resolveMember(const QString &id, const MemoryContextPtr &context, uint32_t address) const;

  //! Returns all members
  Member getMember(const QString &id) const;

protected:
  // List of all members
  QVector<Member> members;
};

class MemoryVariableTypeGroup : public MemoryVariableTypeStruct {
public:
  //! Returns the type IF of this memory variable
  static const char *Type() { return "group"; }
  virtual const char *type() const { return Type(); }

  //! Builds the struct context
  virtual MemoryContextPtr buildStructContext(const MemoryContextPtr &context, uint32_t address) const {
    return context;
  }
};

class MemoryVariableTypeList : public MemoryVariableStandardType {
public:
  //! Returns the type IF of this memory variable
  static const char *Type() { return "list"; }
  virtual const char *type() const { return Type(); }

  //! Sets the target type
  void setTargetType(const MemoryVariableTypePtr &type) { targetType = type; }

  //! Sets the maximum length
  void setMaxLength(uint32_t max) { maxLength = max; }

  //! Sets the length
  void setLength(const MemoryReferencePtr &ref) { length = ref; }

  //! Resolves this variable
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address);

protected:
  //! The target type
  MemoryVariableTypePtr targetType;

  //! The length
  MemoryReferencePtr length;

  //! The maximum length
  uint16_t maxLength = 16;


};

class MemoryVariableTypePointer : public MemoryVariableStandardType {
public:
  //! Returns the type IF of this memory variable
  static const char *Type() { return "pointer"; }
  virtual const char *type() const { return Type(); }

  //! Sets the target type
  void setTargetType(const MemoryVariableTypePtr &type) { targetType = type; }

  //! Sets the target type
  void setSourceType(const MemoryVariableTypePtr &type) { sourceType = type; }

  //! Sets the null value
  void setNullValue(int64_t value) { nullValue = value; }

  //! Sets the target bank
  void setBank(int16_t newBank) { bank = newBank; }

  //! Resolves this variable
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address);

protected:
  //! The target type
  MemoryVariableTypePtr targetType;

  //! The target type
  MemoryVariableTypePtr sourceType;

  //! Sets the bank
  int16_t bank = 0;

  //! Whether the value is nullable
  int64_t nullValue = -1;
};

class MemoryVariableTypeLinkedList : public MemoryVariableTypePointer {
public:
  //! Returns the type IF of this memory variable
  static const char *Type() { return "linkedlist"; }
  virtual const char *type() const { return Type(); }

  //! Resolves this variable
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address);

  //! Sets the next pointer reference
  void setNextPointerReference(const QString &ref) { nextPointerReference = ref; }

protected:
  //! Member name of the 'next' pointer
  QString nextPointerReference;

  //! Maximum members
  uint64_t maxMembers = 16;
};

class MemoryVariableReferenceType : public MemoryVariableType {
public:
  //! Constructor
  MemoryVariableReferenceType(const QString &reference, MemorySchema *schema)
    : reference(reference)
    , schema(schema)
  {
  }

  //! Returns the type IF of this memory variable
  virtual const char *type() const { return resolved()->type(); }

  //! The size in bits
  uint32_t getSizeInBits() const { return resolved()->getSizeInBits(); }

  //! Resolves this variable
  MemoryVariablePtr resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) override {
    return resolved()->resolve(id, context, address);
  }

  //! Returns the resolved record
  virtual MemoryVariableTypePtr resolved();

  //! Returns the resolved record
  virtual QSharedPointer<const MemoryVariableType> resolved() const;

private: 
  //! Returns the resolved record
  void forceResolve() const;

  //! Reference name
  QString reference;

  //! The cached resolved
  mutable MemoryVariableTypePtr cachedResolve;

  //! Schema pointer
  MemorySchema *schema;
};

#endif
