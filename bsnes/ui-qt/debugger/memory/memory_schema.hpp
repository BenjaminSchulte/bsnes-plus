#ifndef __MEMORY_SCHEMA_H__
#define __MEMORY_SCHEMA_H__

class MemorySchema {
public:
  //! Resets the whole schema
  void reset();

  //! Returns a list of all global types
  const QMap<QString, MemoryVariableTypePtr> getAllGlobalTypes() const {
    return globalTypes;
  }

  //! Returns whether a global type exists or not
  inline bool hasGlobalType(const QString &id) const {
    return globalTypes.contains(id);
  }

  //! Returns a global type
  MemoryVariableTypePtr getGlobalType(const QString &id) const {
    return globalTypes[id];
  }

  //! Adds a new type
  void addGlobalType(const QString &id, const MemoryVariableTypePtr &type) {
    globalTypes.insert(id, type);
  }

private:
  //! List of all global types
  QMap<QString, MemoryVariableTypePtr> globalTypes;
};

#endif
