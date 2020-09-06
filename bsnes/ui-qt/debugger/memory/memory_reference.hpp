#ifndef __MEMORY_REFERENCE__H__
#define __MEMORY_REFERENCE__H__

class MemoryReference : public QEnableSharedFromThis<MemoryReference> {
public:
  //! Constructor
  MemoryReference() = default;

  //! Deconstructor
  virtual ~MemoryReference() = default;

  //! Resolve
  virtual uint32_t resolve(const MemoryContextPtr &context, uint32_t base)=0;
};

class RelativeMemoryReference : public MemoryReference {
public:
  //! Constructor
  RelativeMemoryReference(int64_t offset) : offset(offset) {}

  //! Resolve
  uint32_t resolve(const MemoryContextPtr &context, uint32_t base) {return base + offset; }

protected:
  int64_t offset;
};

class ValueOfMemoryReference : public MemoryReference {
public:
  //! Constructor
  ValueOfMemoryReference(QString reference) : reference(reference) {}

  //! Resolve
  uint32_t resolve(const MemoryContextPtr &context, uint32_t base);

protected:
  QString reference;
};

class AbsoluteMemoryReference : public MemoryReference {
public:
  //! Constructor
  AbsoluteMemoryReference(uint32_t address) : address(address) {}

  //! Resolve
  uint32_t resolve(const MemoryContextPtr &, uint32_t) {return address; }

protected:
  uint32_t address;
};

#endif
