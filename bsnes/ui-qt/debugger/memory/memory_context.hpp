#ifndef __MEMORY_CONTENT_H__
#define __MEMORY_CONTENT_H__

class MemorySchema;
class MemoryVariableTypeStruct;

class MemoryContext : public QEnableSharedFromThis<MemoryContext> {
public:
  //! Constructor
  MemoryContext(MemorySchema *schema) : schema(schema) {}

  //! Deconstructor
  virtual ~MemoryContext() = default;

  //! Returns the memory source
  inline SNES::Debugger::MemorySource memorySource() const {
    return SNES::Debugger::MemorySource::CPUBus;
  }

  //! Returns the schema
  inline MemorySchema *getSchema() const { return schema; }

  //! Returns the current struct, if any possible
  virtual MemoryVariablePtr resolveMember(const QString &id, uint32_t address) {
    return MemoryVariablePtr();
  }

  //! Reads data from the bus
  virtual int64_t readBits(uint32_t address, uint32_t numBits, bool isSigned) const;

protected:
  //! The schema
  MemorySchema *schema;
};

class MemoryStructContext : public MemoryContext {
public:
  //! Constructor
  MemoryStructContext(MemorySchema *schema, const QSharedPointer<const MemoryVariableTypeStruct> &str, uint32_t address)
    : MemoryContext(schema), currentStruct(str), address(address) {}

  MemoryVariablePtr resolveMember(const QString &id, uint32_t address);

protected:
  QSharedPointer<const MemoryVariableTypeStruct> currentStruct;

  uint32_t address;
};

class MemoryFlagsContext : public MemoryContext {
public:
  //! Constructor
  MemoryFlagsContext(MemorySchema *schema, uint32_t address, uint32_t offset, uint32_t bits)
    : MemoryContext(schema)
    , address(address)
    , offset(offset)
    , bits(bits)
  {}

  //! Reads data from the bus
  virtual int64_t readBits(uint32_t address, uint32_t numBits, bool isSigned) const;

protected:
  uint32_t address;
  uint32_t offset;
  uint32_t bits;
};

class MemoryRootContext : public MemoryContext {
public:
  //! Constructor
  MemoryRootContext(MemorySchema *schema) : MemoryContext(schema) {}
};

#endif

