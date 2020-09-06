#include <QSharedPointer>
class MemoryTreeVariable;
class MemoryVariableNumber;
class MemoryVariableEnum;
class MemoryVariableFlags;
class MemoryVariableStruct;
class MemoryVariablePointer;
typedef QSharedPointer<class MemoryVariable> MemoryVariablePtr;

class MemorySchemaViewer : public Window {
  Q_OBJECT

public:
  //! Constructor
  MemorySchemaViewer();

  //! Close event
  void closeEvent(QCloseEvent*);

  //! Update cartidge status
  void synchronize();

  //! Auto update
  void autoUpdate();

public slots:
  //! Shows this window
  void show();

  //! Refreshs this window
  void refresh();

  //! Updates the origin
  void updateOrigin();

  //! Updates the schema list
  void setSchemaList(MemorySchema*);

private:
  //! Resets the view
  void resetView();

  //! Sets the view
  void setView(const MemoryVariablePtr &);

  //! Adds the view
  void addItemToParent(QTreeWidgetItem *, QTreeWidgetItem *);

  //! Adds the view
  void addViewItem(const MemoryVariablePtr &, QTreeWidgetItem *);

  //! Adds the view
  void addViewNullItem(const MemoryVariablePtr &, QTreeWidgetItem *);

  //! Adds the view
  void addViewGroupItem(const QSharedPointer<MemoryVariableStruct> &, QTreeWidgetItem *);

  //! Adds the view
  void addViewFlagsItem(const QSharedPointer<MemoryVariableFlags> &, QTreeWidgetItem *);

  //! Adds the view
  void addViewNumberItem(const QSharedPointer<MemoryVariableNumber> &, QTreeWidgetItem *);

  //! Adds the view
  void addViewEnumItem(const QSharedPointer<MemoryVariableEnum> &, QTreeWidgetItem *);

  //! Adds the view
  void addViewPointerItem(const QSharedPointer<MemoryVariablePointer> &, QTreeWidgetItem *);

  //! Adds the view
  void addViewLinkedListItem(const QSharedPointer<MemoryTreeVariable> &, QTreeWidgetItem *);

  //! Adds the view
  void addViewListItem(const QSharedPointer<MemoryTreeVariable> &, QTreeWidgetItem *);

  //! Adds the view
  void addViewItemChildren(const MemoryVariablePtr &, QTreeWidgetItem *);

  //! The main layout
  QVBoxLayout *layout;

  //! The main tree
  QTreeWidget *properties;

  //! The type select
  QComboBox *schemaSelect;

  //! The address
  QLineEdit *addressEdit;

  //! Refresh button
  QPushButton *refreshButton;

  //! Auto update
  QCheckBox *autoUpdateBox;

  //! The current schema list
  MemorySchema *memorySchema = nullptr;

  //! The current schema name
  QString currentSchema;

  //! The current address
  uint32_t currentAddress;

  //! The current displayed variable
  MemoryVariablePtr content;
};
