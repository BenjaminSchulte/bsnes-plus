#ifndef __MEMORY_SCHEMA_JSON_LOADER_H__
#define __MEMORY_SCHEMA_JSON_LOADER_H__

#include <nall/json.hpp>
#include <nall/fifo_map.hpp>

template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

class MemorySchemaJsonLoader {
public:
  //! Constructor
  MemorySchemaJsonLoader(MemorySchema *schema) : schema(schema) {}

  //! Loads the schema from a JSON file
  bool load(const string &source) const;

  //! Loads the types
  bool loadTypes(const json &j) const;

  //! Loads the types
  bool loadVariables(const json &j) const;

private:
  //! Loads the types
  MemoryVariableTypePtr loadType(const json &j) const;

  //! Loads the types
  MemoryVariableTypePtr loadObjectType(const json &j) const;

  //! Loads the types
  MemoryVariableTypePtr loadObjectTypeEnum(const json &j) const;

  //! Loads the types
  MemoryVariableTypePtr loadObjectTypePointer(const json &j) const;

  //! Loads the types
  bool loadObjectTypePointerMembers(const QSharedPointer<MemoryVariableTypePointer> &type, const json &j) const;

  //! Loads the types
  MemoryVariableTypePtr loadObjectTypeList(const json &j) const;

  //! Loads the types
  MemoryVariableTypePtr loadObjectTypeLinkedList(const json &j) const;

  //! Loads the types
  MemoryVariableTypePtr loadObjectTypeFlags(const json &j) const;

  //! Loads the types
  MemoryVariableTypePtr loadObjectTypeStruct(const json &j, bool asGroup) const;

  //! Loads the struct content
  bool loadStructMembers(const QSharedPointer<MemoryVariableTypeStruct> &type, const json &j) const;

  //! Loads a standard object
  bool loadObjectStandardType(const QSharedPointer<MemoryVariableStandardType> &obj, const json &j) const;

  //! Returns a number
  int64_t getNumber(const json &j) const;

  //! Loads an offset
  MemoryReferencePtr loadOffset(const json &j) const;

  //! The schema
  MemorySchema *schema;
};

#endif
