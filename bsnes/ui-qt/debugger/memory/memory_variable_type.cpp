// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypeNumber::resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
  return MemoryVariablePtr(
    new MemoryVariableNumber(id, sharedFromThis().dynamicCast<MemoryVariableTypeNumber>(), context, address, context->readBits(address, sizeInBits, isSigned))
  );
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypeEnum::resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
  return MemoryVariablePtr(
    new MemoryVariableEnum(id, sharedFromThis().dynamicCast<MemoryVariableTypeEnum>(), context, address, context->readBits(address, sizeInBits, false))
  );
}

// -----------------------------------------------------------------------------
MemoryVariableTypePtr MemoryVariableReferenceType::resolved() {
  if (!cachedResolve) {
    forceResolve();
  }

  return cachedResolve;
}

// -----------------------------------------------------------------------------
QSharedPointer<const MemoryVariableType> MemoryVariableReferenceType::resolved() const {
  if (!cachedResolve) {
    forceResolve();
  }

  return cachedResolve;
}

// -----------------------------------------------------------------------------
void MemoryVariableReferenceType::forceResolve() const {
  if      (reference == "boolean") { cachedResolve.reset(new MemoryVariableTypeNumber(1,  false)); return; }
  else if (reference == "uint8")   { cachedResolve.reset(new MemoryVariableTypeNumber(8,  false)); return; }
  else if (reference == "uint16")  { cachedResolve.reset(new MemoryVariableTypeNumber(16, false)); return; }
  else if (reference == "uint24")  { cachedResolve.reset(new MemoryVariableTypeNumber(24, false)); return; }
  else if (reference == "uint32")  { cachedResolve.reset(new MemoryVariableTypeNumber(32, false)); return; }
  else if (reference == "int8")    { cachedResolve.reset(new MemoryVariableTypeNumber(8,  true)); return; }
  else if (reference == "int16")   { cachedResolve.reset(new MemoryVariableTypeNumber(16, true)); return; }
  else if (reference == "int24")   { cachedResolve.reset(new MemoryVariableTypeNumber(24, true)); return; }
  else if (reference == "int32")   { cachedResolve.reset(new MemoryVariableTypeNumber(32, true)); return; }

  if (schema->hasGlobalType(reference)) {
    cachedResolve = schema->getGlobalType(reference);
    return;
  }

  std::cerr << "Unknown Schema type '" << reference.toStdString() << "'";
  cachedResolve.reset(new MemoryVariableTypeUnknown());
}

// -----------------------------------------------------------------------------
MemoryContextPtr MemoryVariableTypeStruct::buildStructContext(const MemoryContextPtr &context, uint32_t address) const {
  return MemoryContextPtr(new MemoryStructContext(context->getSchema(), sharedFromThis().dynamicCast<const MemoryVariableTypeStruct>(), address));
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypeStruct::resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
  QSharedPointer<MemoryVariableStruct> v(new MemoryVariableStruct(id, sharedFromThis().dynamicCast<MemoryVariableTypeStruct>(), context, address));
  MemoryContextPtr newContext(buildStructContext(context, address));

  for (const Member &member : members) {
    MemoryVariablePtr memberVariable = member.type->resolve(id + "." + member.id, newContext, member.offset->resolve(newContext, address));

    if (memberVariable) {
      memberVariable->setName(member.name);
      v->addChild(memberVariable);
    }
  }

  return v;
}

// -----------------------------------------------------------------------------
MemoryVariableTypeStruct::Member MemoryVariableTypeStruct::getMember(const QString &id) const {
  QStringList parts = id.split('.');
  QString firstPart = parts[0];

  for (const auto &member : members) {
    if (member.id != firstPart) {
      continue;
    }

    if (parts.length() == 1) {
      return member;
    }

    MemoryVariableTypePtr currentType(member.type);
    Member currentMember;
    // LIST!!
    for (int i=1; i<parts.length(); i++) {
      if (QString(currentType->type()) != "group") {
        return Member();
      }

      QSharedPointer<MemoryVariableTypeGroup> group = currentType->resolved().dynamicCast<MemoryVariableTypeGroup>();
      if (!group) {
        return Member();
      }

      currentMember = group->getMember(parts[i]);
      currentType = currentMember.type;
    }

    return currentMember;
  }

  return Member();
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypeStruct::resolveMember(const QString &id, const MemoryContextPtr &context, uint32_t address) const {
  Member member(getMember(id));
  if (!member.valid()) {
    return MemoryVariablePtr();
  }

  MemoryContextPtr newContext(buildStructContext(context, address));
  return member.type->resolve(member.id, newContext, member.offset->resolve(newContext, address));
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypeFlags::resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
  QSharedPointer<MemoryVariableFlags> v(new MemoryVariableFlags(id, sharedFromThis().dynamicCast<MemoryVariableTypeFlags>(), context, address));

  for (const Member &member : members) {
    MemoryContextPtr newContext(new MemoryFlagsContext(context->getSchema(), address, member.offset, member.bits));
    MemoryVariablePtr memberVariable = member.type->resolve(id + "." + member.id, newContext, address);

    if (memberVariable) {
      memberVariable->setName(member.name);
      v->addChild(memberVariable);
    }
  }

  return v;
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypePointer::resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
  if (!sourceType) {
    return MemoryVariablePtr(new MemoryNullVariable(id, MemoryVariableTypePtr(new MemoryVariableTypeNull()), context, address));
  }

  uint64_t target = sourceType->resolve(id, context, address)->getNumericValue();
  if (target == nullValue) {
    return MemoryVariablePtr(new MemoryNullVariable(id, MemoryVariableTypePtr(new MemoryVariableTypeNull()), context, address));
  } else if (bank == -1) {
    target |= address & 0xFF0000;
  } else {
    target |= bank << 16;
  }

  return MemoryVariablePtr(
    new MemoryVariablePointer(id, sharedFromThis().dynamicCast<MemoryVariableTypePointer>(), context, address, target)
  );
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypeList::resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
  QSharedPointer<MemoryTreeVariable> tree(new MemoryTreeVariable(id, sharedFromThis(), context, address));

  uint64_t numRecords = length->resolve(context, 0);
  if (numRecords > maxLength) {
    numRecords = maxLength;
  }

  uint32_t sizeInBytes = targetType->getSizeInBytes();
  for (uint64_t i=0; i<numRecords; i++) {
    MemoryVariablePtr result(targetType->resolve(id + "." + QString::number(i, 10), context, address + i * sizeInBytes));
    result->setName("#" + QString::number(i));
    tree->addChild(result);
  }

  return tree;
}

// -----------------------------------------------------------------------------
MemoryVariablePtr MemoryVariableTypeLinkedList::resolve(const QString &id, const MemoryContextPtr &context, uint32_t address) {
  QSharedPointer<MemoryTreeVariable> tree(new MemoryTreeVariable(id, sharedFromThis(), context, address));
  
  MemoryVariablePtr currentPointer = MemoryVariableTypePointer::resolve(id, context, address);

  int membersLeft = maxMembers;
  int index = 0;
  while (membersLeft-- && currentPointer && (QString(currentPointer->getType()->type()) == "pointer" || QString(currentPointer->getType()->type()) == "linkedlist")) {
    currentPointer->setName("#" + QString::number(index++));
    tree->addChild(currentPointer);

    uint64_t pointerAddress = currentPointer->getNumericValue();
    if (targetType.isNull() || QString(targetType->type()) != "struct") {
      break;
    }

    QSharedPointer<MemoryVariableTypeStruct> targetStruct = targetType->resolved().dynamicCast<MemoryVariableTypeStruct>();
    if (!targetStruct) {
      break;
    }

    MemoryVariablePtr result = targetStruct->resolveMember(nextPointerReference, context, pointerAddress);
    if (!result) {
      break;
    }

    currentPointer = result;
  }

  return tree;
}