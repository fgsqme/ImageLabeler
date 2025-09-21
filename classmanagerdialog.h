#ifndef CLASSMANAGERDIALOG_H
#define CLASSMANAGERDIALOG_H

#include <QDialog>
#include <QStringList>

class QListWidget;
class QLineEdit;
class QPushButton;

// 类管理对话框类
class ClassManagerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ClassManagerDialog(const QStringList &classes, QWidget *parent = nullptr);
    QStringList getClasses() const;

private slots:
    void on_item_selected(int row);
    void add_class();
    void edit_class();
    void delete_class();

private:
    void init_ui();

    QStringList classes;
    QListWidget *list_widget;
    QLineEdit *name_edit;
    QPushButton *add_btn;
    QPushButton *edit_btn;
    QPushButton *delete_btn;
    QPushButton *ok_btn;
    QPushButton *cancel_btn;
};

#endif // CLASSMANAGERDIALOG_H