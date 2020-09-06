#include <iostream>
#include <QSharedPointer>
#include <QMap>
#include <QVector>

typedef QSharedPointer<class MemoryVariable> MemoryVariablePtr;
typedef QSharedPointer<class MemoryContext> MemoryContextPtr;
typedef QVector<MemoryVariablePtr> MemoryVariableList;
typedef QSharedPointer<class MemoryVariableType> MemoryVariableTypePtr;
typedef QSharedPointer<class MemoryReference> MemoryReferencePtr;

#include "memory_context.hpp"
#include "memory_variable_type.hpp"
#include "memory_variable.hpp"
#include "memory_reference.hpp"
#include "memory_struct.hpp"
#include "memory_schema.hpp"
#include "memory_schema_json_loader.hpp"

#include "memory_context.cpp"
#include "memory_schema.cpp"
#include "memory_schema_json_loader.cpp"
#include "memory_variable_type.cpp"
#include "memory_variable.cpp"
#include "memory_reference.cpp"
