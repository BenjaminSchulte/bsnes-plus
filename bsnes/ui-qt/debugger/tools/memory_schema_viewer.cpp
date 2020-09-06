#include "memory_schema_viewer.moc"

QVector <MemorySchemaViewer*> memorySchemaViewers;

// -----------------------------------------------------------------------------
MemorySchemaViewer::MemorySchemaViewer() {
  setObjectName("memory-schema-viewer");
  setWindowTitle("Memory Schema Viewer");
  setGeometryString(&config().geometry.memorySchemaViewer);
  application.windowList.append(this);

  layout = new QVBoxLayout;
  layout->setMargin(Style::WindowMargin);
  layout->setSpacing(Style::WidgetSpacing);
  setLayout(layout);

  QHBoxLayout *header = new QHBoxLayout();
  layout->addLayout(header);

  schemaSelect = new QComboBox();
  header->addWidget(schemaSelect);

  addressEdit = new QLineEdit();
  addressEdit->setText("0");
  header->addWidget(addressEdit);

  properties = new QTreeWidget();
  properties->setColumnCount(2);
  properties->headerItem()->setText(0, "Property");
  properties->headerItem()->setText(1, "Value");
  layout->addWidget(properties);

  QHBoxLayout *footer = new QHBoxLayout();
  refreshButton = new QPushButton("Refresh");
  footer->addWidget(refreshButton);
  autoUpdateBox = new QCheckBox("Auto update");
  footer->addWidget(autoUpdateBox);
  layout->addLayout(footer);

  connect(schemaSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateOrigin()));
  connect(schemaSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(updateOrigin()));
  connect(addressEdit, SIGNAL(textEdited(const QString&)), this, SLOT(updateOrigin()));
  connect(addressEdit, SIGNAL(returnPressed()), this, SLOT(updateOrigin()));
  connect(refreshButton, SIGNAL(released()), this, SLOT(refresh()));
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::closeEvent(QCloseEvent *ev) {
  memorySchemaViewers.removeOne(this);
  Window::closeEvent(ev);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::show() {
  Window::show();
  memorySchemaViewers.push_back(this);
  refresh();
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::autoUpdate() {
  if(SNES::cartridge.loaded() && autoUpdateBox->isChecked()) {
    refresh();
  }
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::setSchemaList(MemorySchema *schema) {
  memorySchema = schema;

  schemaSelect->clear();
  for (const QString &key : schema->getAllGlobalTypes().keys()) {
    schemaSelect->addItem(key);
  }

  updateOrigin();
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::synchronize() {
  bool enabled = SNES::cartridge.loaded();

  schemaSelect->setEnabled(enabled);
  addressEdit->setEnabled(enabled);

  updateOrigin();
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::updateOrigin() {
  currentSchema = schemaSelect->currentText();
  currentAddress = hex(addressEdit->text().toUtf8().data());
  
  resetView();
  refresh();
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::refresh() {
  MemoryVariableTypePtr schema = memorySchema->getGlobalType(currentSchema);
  if (schema) {
    MemoryContextPtr context(new MemoryRootContext(memorySchema));
    content = schema->resolve("", context, currentAddress);
  } else {
    content = MemoryVariablePtr();
  }

  if (content) {
    setView(content);
  } else {
    resetView();
  }
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::resetView() {
  properties->clear();
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::setView(const MemoryVariablePtr &var) {
  properties->clear();
  addViewItemChildren(var, nullptr);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addItemToParent(QTreeWidgetItem *item, QTreeWidgetItem *parent) {
  if (parent == nullptr) {
    properties->addTopLevelItem(item);
  } else {
    parent->addChild(item);
  }
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewItem(const MemoryVariablePtr &var, QTreeWidgetItem *parent) {
  QString typeId = var->getType()->type();

  if (typeId == "struct") {
    addViewGroupItem(var.dynamicCast<MemoryVariableStruct>(), parent);
  } else if (typeId == "null") {
    addViewNullItem(var, parent);
  } else if (typeId == "group") {
    addViewGroupItem(var.dynamicCast<MemoryVariableStruct>(), parent);
  } else if (typeId == "flags") {
    addViewFlagsItem(var.dynamicCast<MemoryVariableFlags>(), parent);
  } else if (typeId == "number") {
    addViewNumberItem(var.dynamicCast<MemoryVariableNumber>(), parent);
  } else if (typeId == "enum") {
    addViewEnumItem(var.dynamicCast<MemoryVariableEnum>(), parent);
  } else if (typeId == "pointer") {
    addViewPointerItem(var.dynamicCast<MemoryVariablePointer>(), parent);
  } else if (typeId == "linkedlist") {
    addViewLinkedListItem(var.dynamicCast<MemoryTreeVariable>(), parent);
  } else if (typeId == "list") {
    addViewListItem(var.dynamicCast<MemoryTreeVariable>(), parent);
  } else {
    qDebug() << "Unsupported type" << typeId;
  }
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewNullItem(const MemoryVariablePtr &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, "-");
  addItemToParent(item, parent);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewNumberItem(const QSharedPointer<MemoryVariableNumber> &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, QString::number(var->getValue()));
  addItemToParent(item, parent);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewEnumItem(const QSharedPointer<MemoryVariableEnum> &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, var->getText());
  addItemToParent(item, parent);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewPointerItem(const QSharedPointer<MemoryVariablePointer> &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, QString::number(var->getAddress(), 16));
  addItemToParent(item, parent);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewGroupItem(const QSharedPointer<MemoryVariableStruct> &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, "");
  addItemToParent(item, parent);

  addViewItemChildren(var, item);
  item->setExpanded(true);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewListItem(const QSharedPointer<MemoryTreeVariable> &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, "");
  addItemToParent(item, parent);
  addViewItemChildren(var, item);
  item->setExpanded(true);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewLinkedListItem(const QSharedPointer<MemoryTreeVariable> &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, "");
  addItemToParent(item, parent);

  for (const auto &child : var->getChildren()) {
    addViewPointerItem(child.dynamicCast<MemoryVariablePointer>(), item);
  }

  item->setExpanded(true);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewFlagsItem(const QSharedPointer<MemoryVariableFlags> &var, QTreeWidgetItem *parent) {
  QTreeWidgetItem *item = new QTreeWidgetItem();
  item->setText(0, var->getName());
  item->setText(1, "");
  addItemToParent(item, parent);

  addViewItemChildren(var, item);
  item->setExpanded(true);
}

// -----------------------------------------------------------------------------
void MemorySchemaViewer::addViewItemChildren(const MemoryVariablePtr &var, QTreeWidgetItem *parent) {
  for (const auto &child : var->getChildren()) {
    addViewItem(child, parent);
  }
}
