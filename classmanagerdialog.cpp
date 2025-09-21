#include "classmanagerdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

ClassManagerDialog::ClassManagerDialog(const QStringList &classes, QWidget *parent)
    : QDialog(parent), classes(classes) {
    init_ui();
}

void ClassManagerDialog::init_ui() {
    setWindowTitle(tr("标签管理"));
    resize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Class list
    list_widget = new QListWidget(this);
    list_widget->addItems(classes);
    layout->addWidget(list_widget);

    // Input field
    QFormLayout *form_layout = new QFormLayout();
    name_edit = new QLineEdit(this);
    form_layout->addRow(tr("标签名称:"), name_edit);
    layout->addLayout(form_layout);

    // Buttons
    QHBoxLayout *btn_layout = new QHBoxLayout();
    add_btn = new QPushButton(tr("添加"), this);
    connect(add_btn, &QPushButton::clicked, this, &ClassManagerDialog::add_class);

    edit_btn = new QPushButton(tr("编辑"), this);
    connect(edit_btn, &QPushButton::clicked, this, &ClassManagerDialog::edit_class);

    delete_btn = new QPushButton(tr("删除"), this);
    connect(delete_btn, &QPushButton::clicked, this, &ClassManagerDialog::delete_class);

    ok_btn = new QPushButton(tr("确定"), this);
    connect(ok_btn, &QPushButton::clicked, this, &QDialog::accept);

    cancel_btn = new QPushButton(tr("取消"), this);
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);

    btn_layout->addWidget(add_btn);
    btn_layout->addWidget(edit_btn);
    btn_layout->addWidget(delete_btn);
    btn_layout->addWidget(ok_btn);
    btn_layout->addWidget(cancel_btn);
    layout->addLayout(btn_layout);

    // Connect list item selection signal
    connect(list_widget, &QListWidget::currentRowChanged, this, &ClassManagerDialog::on_item_selected);
}

QStringList ClassManagerDialog::getClasses() const {
    return classes;
}

void ClassManagerDialog::on_item_selected(int row) {
    if (row >= 0 && row < classes.size()) {
        name_edit->setText(classes.at(row));
    }
}

void ClassManagerDialog::add_class() {
    QString name = name_edit->text().trimmed();
    if (!name.isEmpty() && !classes.contains(name)) {
        classes.append(name);
        list_widget->addItem(name);
        name_edit->clear();
    }
}

void ClassManagerDialog::edit_class() {
    int current_row = list_widget->currentRow();
    if (current_row >= 0 && current_row < classes.size()) {
        QString name = name_edit->text().trimmed();
        if (!name.isEmpty() && !classes.contains(name)) {
            classes[current_row] = name;
            list_widget->currentItem()->setText(name);
        }
    }
}

void ClassManagerDialog::delete_class() {
    int current_row = list_widget->currentRow();
    if (current_row >= 0 && current_row < classes.size()) {
        classes.removeAt(current_row);
        delete list_widget->takeItem(current_row);
        name_edit->clear();
    }
}
