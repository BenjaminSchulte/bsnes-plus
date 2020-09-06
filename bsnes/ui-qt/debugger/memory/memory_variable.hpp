#ifndef __MEMORY_VARIABLE__H__
#define __MEMORY_VARIABLE__H__

class MemoryVariable {
public:
  //! Constructor
  MemoryVariable(const QString &id, const MemoryVariableTypePtr &type, const MemoryContextPtr &context, uint32_t address)
    : id(id), type(type), context(context), address(address) {}

  //! Deconstructor
  virtual ~MemoryVariable() = default;

  //! Sets the name of this variable
  void setName(const QString &newName) { name = newName; }

  //! Returns the id of the variable
  inline const QString &getId() const { return id; }

  //! Returns the name of the variable
  inline const QString &getName() const { return name; }

  //! Returns the type of the variable
  inline const MemoryVariableTypePtr &getType() const { return type; }

  //! Returns the address of the variable
  inline uint32_t getAddress() const { return address; }

  //! Returns a list of all members
  virtual MemoryVariableList getChildren() { return MemoryVariableList(); }

  //! Returns the numeric value
  virtual int64_t getNumericValue() const { return 0; }

protected:
  //! The id
  QString id;

  //! The name
  QString name;

  //! The variable type
  MemoryVariableTypePtr type;

  //! The context
  MemoryContextPtr context;

  //! The address
  uint32_t address;
};

class MemoryNullVariable : public MemoryVariable {
public:
  MemoryNullVariable(const QString &id, const MemoryVariableTypePtr &type, const MemoryContextPtr &context, uint32_t address)
    : MemoryVariable(id, type, context, address) {}
};

class MemoryTreeVariable : public MemoryVariable {
public:
  //! Constructor
  MemoryTreeVariable(const QString &id, const MemoryVariableTypePtr &type, const MemoryContextPtr &context, uint32_t address)
    : MemoryVariable(id, type, context, address) {}

  //! Adds a child
  void addChild(const MemoryVariablePtr &var) { children.push_back(var); }

  //! Returns a list of all members
  virtual MemoryVariableList getChildren() { return children; }

protected:
  //! List of all members
  MemoryVariableList children;
};

class MemoryVariableStruct : public MemoryTreeVariable {
public:
  //! Constructor
  MemoryVariableStruct(const QString &id, const QSharedPointer<MemoryVariableTypeStruct> &type, const MemoryContextPtr &context, uint32_t address)
    : MemoryTreeVariable(id, type, context, address)
    , structType(type)
  {}

protected:
  //! The struct
  QSharedPointer<MemoryVariableTypeStruct> structType;
};

class MemoryVariableFlags : public MemoryTreeVariable {
public:
  //! Constructor
  MemoryVariableFlags(const QString &id, const QSharedPointer<MemoryVariableTypeFlags> &type, const MemoryContextPtr &context, uint32_t address)
    : MemoryTreeVariable(id, type, context, address)
    , flagsType(type)
  {}

protected:
  //! The struct
  QSharedPointer<MemoryVariableTypeFlags> flagsType;
};

class MemoryVariableNumber : public MemoryVariable {
public:
  //! Constructor
  MemoryVariableNumber(const QString &id, const QSharedPointer<MemoryVariableTypeNumber> &type, const MemoryContextPtr &context, uint32_t address, int64_t value)
    : MemoryVariable(id, type, context, address)
    , value(value)
  {}

  //! Returns the value
  inline int64_t getValue() const { return value; }

  //! Returns the numeric value
  virtual int64_t getNumericValue() const { return value; }

protected:
  //! The value of this field
  int64_t value;
};

class MemoryVariablePointer : public MemoryVariable {
public:
  //! Constructor
  MemoryVariablePointer(const QString &id, const QSharedPointer<MemoryVariableTypePointer> &type, const MemoryContextPtr &context, uint32_t address, uint64_t value)
    : MemoryVariable(id, type, context, address)
    , value(value)
  {}

  //! Returns the value
  inline uint64_t getAddress() const { return value; }

  //! Returns the numeric value
  virtual int64_t getNumericValue() const { return value; }

protected:
  //! The value of this field
  uint64_t value;
};

class MemoryVariableEnum : public MemoryVariable {
public:
  //! Constructor
  MemoryVariableEnum(const QString &id, const QSharedPointer<MemoryVariableTypeEnum> &type, const MemoryContextPtr &context, uint32_t address, int64_t value)
    : MemoryVariable(id, type, context, address)
    , value(value)
    , enumType(type)
  {}

  //! Returns the number as string
  QString getText() const { return enumType->getMember(value).name; }

  //! Returns the numeric value
  virtual int64_t getNumericValue() const { return value; }

protected:
  //! The value of this field
  int64_t value;

  //! The enum type
  QSharedPointer<MemoryVariableTypeEnum> enumType;
};

#endif
