// -----------------------------------------------------------------------------
uint32_t ValueOfMemoryReference::resolve(const MemoryContextPtr &context, uint32_t base) {
  MemoryVariablePtr var(context->resolveMember(reference, base));
  if (!var) {
    std::cerr << "Unable to resolve '" << reference.toStdString() << "'" << std::endl;
    return 0;
  }
  
  return var->getNumericValue();
}