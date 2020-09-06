#include <iostream>

// -----------------------------------------------------------------------------
bool MemorySchemaJsonLoader::load(const string &source) const {
  string buffer;
  if (!buffer.readfile(source)) {
    return false;
  }

  try {
    json j = json::parse(buffer());

    if (!loadTypes(j["types"])
      || !loadVariables(j["variables"])
    ) {
      return false;
    }
  } catch(nlohmann::detail::type_error &err) {
    std::cerr << "Schema JSON type error" << std::endl;
    return false;
  } catch(nlohmann::detail::parse_error &err) {
    std::cerr << "Schema JSON parse error" << std::endl;
    return false;
  }

  return false;
}

// -----------------------------------------------------------------------------
bool MemorySchemaJsonLoader::loadTypes(const json &j) const {
  if (j.is_null()) {
    return true;
  }
  if (!j.is_object()) {
    std::cerr << "'types' must be an object" << std::endl;
    return false;
  }

  for (auto &el : j.items()) {
    MemoryVariableTypePtr type = loadType(el.value());
    if (!type) {
      return false;
    }
    
    schema->addGlobalType(QString(el.key().c_str()), type);
  }

  return true;
}

// -----------------------------------------------------------------------------
bool MemorySchemaJsonLoader::loadVariables(const json &j) const {
  if (j.is_null()) {
    return true;
  }
  if (!j.is_object()) {
    std::cerr << "'variables' must be an object" << std::endl;
    return false;
  }

  QSharedPointer<MemoryVariableTypeStruct> type(new MemoryVariableTypeStruct());
  if (!loadStructMembers(type, j)) {
    return false;
  }

  schema->addGlobalType("@GLOBAL", type);

  return true;
}

// -----------------------------------------------------------------------------
int64_t MemorySchemaJsonLoader::getNumber(const json &j) const {
  if (j.is_number()) {
    return j.get<int64_t>();
  } else if (j.is_object()) {
    if (j.contains("$") && j["$"].is_string()) {
      return hex(j["$"].get<std::string>().c_str());
    }
  }

  std::cerr << "Error: numbers must be integer or {\"$\":\"hex\"}" << std::endl;
  return 0;
}

// -----------------------------------------------------------------------------
MemoryReferencePtr MemorySchemaJsonLoader::loadOffset(const json &j) const {
  if (j.is_object()) {
    if (j.contains("address")) {
      return MemoryReferencePtr(new AbsoluteMemoryReference(getNumber(j["address"])));
    }
    if (j.contains("value_of") && j["value_of"].is_string()) {
      return MemoryReferencePtr(new ValueOfMemoryReference(j["value_of"].get<std::string>().c_str()));
    }
  }

  return MemoryReferencePtr(new RelativeMemoryReference(getNumber(j)));
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadType(const json &j) const {
  if (j.is_object()) {
    return loadObjectType(j);
  } else if (j.is_string()) {
    return MemoryVariableTypePtr(new MemoryVariableReferenceType(QString(j.get<std::string>().c_str()), schema));
  } else if (j.is_null()) {
    return MemoryVariableTypePtr(new MemoryVariableTypeUnknown());
  } else {
    std::cerr << "json type must be string or object" << std::endl;
    std::cerr << j.dump() << std::endl;
    return MemoryVariableTypePtr();
  }
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadObjectType(const json &j) const {
  if (j.begin() == j.end()) {
    std::cerr << "type object must have exactly one key" << std::endl;
    return MemoryVariableTypePtr();
  }

  std::string type = j.begin().key();
  if (type == "enum") {
    return loadObjectTypeEnum(j.begin().value());
  } else if (type == "flags") {
    return loadObjectTypeFlags(j.begin().value());
  } else if (type == "struct") {
    return loadObjectTypeStruct(j.begin().value(), false);
  } else if (type == "group") {
    return loadObjectTypeStruct(j.begin().value(), true);
  } else if (type == "pointer") {
    return loadObjectTypePointer(j.begin().value());
  } else if (type == "list") {
    return loadObjectTypeList(j.begin().value());
  } else if (type == "linkedlist") {
    return loadObjectTypeLinkedList(j.begin().value());
  } else {
    std::cerr << "Unsupported object type: " << type.c_str() << std::endl;
    return MemoryVariableTypePtr();
  }
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadObjectTypeEnum(const json &j) const {
  QSharedPointer<MemoryVariableTypeEnum> type(new MemoryVariableTypeEnum());
  if (!loadObjectStandardType(type, j)) {
    return MemoryVariableTypePtr();
  }

  if (j.contains("values") && j["values"].is_object()) {
    for (auto &it : j["values"].items()) {
      if (!it.value().is_object()) {
        std::cerr << "Enum values must be object" << std::endl;
        continue;
      }

      auto &tj = it.value();
      if (!tj.contains("name") || !tj["name"].is_string()) {
        std::cerr << "Enum values must contain 'name'" << std::endl;
        continue;
      }
      if (!tj.contains("value") || !tj["value"].is_number()) {
        std::cerr << "Enum values must contain 'value'" << std::endl;
        continue;
      }

      type->add(QString(it.key().c_str()), QString(tj["name"].get<std::string>().c_str()), getNumber(tj["value"]));
    }
  }

  return type;
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadObjectTypeList(const json &j) const {
  QSharedPointer<MemoryVariableTypeList> type(new MemoryVariableTypeList());
  if (!loadObjectStandardType(type, j)) {
    return MemoryVariableTypePtr();
  }

  if (j.contains("type")) {
    type->setTargetType(loadType(j["type"]));
  }

  if (j.contains("length")) {
    type->setLength(loadOffset(j["length"]));
  }

  if (j.contains("max_length")) {
    type->setMaxLength(getNumber(j["max_length"]));
  }

  return type;
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadObjectTypeFlags(const json &j) const {
  QSharedPointer<MemoryVariableTypeFlags> type(new MemoryVariableTypeFlags());
  if (!loadObjectStandardType(type, j)) {
    return MemoryVariableTypePtr();
  }

  if (j.contains("members") && j["members"].is_object()) {
    for (auto &it : j["members"].items()) {
      if (!it.value().is_object()) {
        std::cerr << "Flags members must be object" << std::endl;
        continue;
      }

      auto &tj = it.value();
      if (!tj.contains("name") || !tj["name"].is_string()) {
        std::cerr << "Flags values must contain 'name'" << std::endl;
        continue;
      }
      if (!tj.contains("offset") || !tj["offset"].is_number()) {
        std::cerr << "Flags values must contain 'offset'" << std::endl;
        continue;
      }
      if (!tj.contains("bits") || !tj["bits"].is_number()) {
        std::cerr << "Flags values must contain 'bits'" << std::endl;
        continue;
      }
      if (!tj.contains("type")) {
        std::cerr << "Flags values must contain 'type'" << std::endl;
        continue;
      }

      auto memberType = loadType(tj["type"]);
      if (!memberType) {
        return MemoryVariableTypePtr();
      }

      type->add(QString(it.key().c_str()), QString(tj["name"].get<std::string>().c_str()), getNumber(tj["offset"]), getNumber(tj["bits"]), memberType);
    }
  }

  return type;
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadObjectTypePointer(const json &j) const {
  QSharedPointer<MemoryVariableTypePointer> type(new MemoryVariableTypePointer());
  if (!loadObjectTypePointerMembers(type, j)) {
    return MemoryVariableTypePtr();
  }

  return type;
}

// -----------------------------------------------------------------------------
bool MemorySchemaJsonLoader::loadObjectTypePointerMembers(const QSharedPointer<MemoryVariableTypePointer> &type, const json &j) const {
  if (!loadObjectStandardType(type, j)) {
    return false;
  }

  if (j.contains("to")) {
    type->setTargetType(loadType(j["to"]));
  }
  if (j.contains("type")) {
    type->setSourceType(loadType(j["type"]));
  }
  if (j.contains("nullable")) {
    if (j["nullable"].is_boolean() && j["nullable"].get<bool>()) {
      type->setNullValue(0);
    } else { 
      type->setNullValue(getNumber(j["nullable"]));
    }
  }
  if (j.contains("bank")) {
    if (j["bank"].is_string() && j["bank"].get<std::string>() == "same") {
      type->setBank(-1);
    } else if(j["bank"].is_number()) {
      type->setBank(getNumber(j["bank"]));
    } else {
      std::cerr << "Unsupported 'bank' parameter for pointer" << std::endl;
    }
  }

  return true;
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadObjectTypeLinkedList(const json &j) const {
  QSharedPointer<MemoryVariableTypeLinkedList> type(new MemoryVariableTypeLinkedList());
  if (!loadObjectTypePointerMembers(type, j)) {
    return MemoryVariableTypePtr();
  }

  if (j.contains("next") && j["next"].is_string()) {
    type->setNextPointerReference(j["next"].get<std::string>().c_str());
  }

  return type;
}

// -----------------------------------------------------------------------------
bool MemorySchemaJsonLoader::loadStructMembers(const QSharedPointer<MemoryVariableTypeStruct> &type, const json &j) const {
  for (auto &it : j.items()) {
    if (!it.value().is_object()) {
      std::cerr << "Struct members must be object" << std::endl;
      continue;
    }

    std::string name = it.key();
    auto &tj = it.value();
    if (tj.contains("name") && tj["name"].is_string()) {
      name = tj["name"].get<std::string>();
    }
    MemoryReferencePtr offset;
    if (tj.contains("offset")) {
      offset = loadOffset(tj["offset"]);
    } else {
      offset = MemoryReferencePtr(new RelativeMemoryReference(0));
    }

    if (!tj.contains("type")) {
      std::cerr << "Struct values must contain 'type': " << it.key() << std::endl;
      continue;
    }

    auto memberType = loadType(tj["type"]);
    if (!memberType) {
      return false;
    }

    type->add(QString(it.key().c_str()), QString(name.c_str()), offset, memberType);
  }
  
  return true;
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemorySchemaJsonLoader::loadObjectTypeStruct(const json &j, bool asGroup) const {
  QSharedPointer<MemoryVariableTypeStruct> type(asGroup ? new MemoryVariableTypeGroup() : new MemoryVariableTypeStruct());
  if (!loadObjectStandardType(type, j)) {
    return MemoryVariableTypePtr();
  }

  if (j.contains("members") && j["members"].is_object()) {
    if (!loadStructMembers(type, j["members"])) {
      return MemoryVariableTypePtr();
    }
  }

  return type;
}

// -----------------------------------------------------------------------------
bool MemorySchemaJsonLoader::loadObjectStandardType(const QSharedPointer<MemoryVariableStandardType> &obj, const json &j) const {
  if (!j.is_object()) {
    return false;
  }

  if (j.contains("bits") && j["bits"].is_number()) {
    obj->setSizeInBits(getNumber(j["bits"]));
  } else if (j.contains("bytes") && j["bytes"].is_number()) {
    obj->setSizeInBits(getNumber(j["bytes"]) * 8);
  }

  return true;
}