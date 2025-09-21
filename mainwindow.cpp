#include "mainwindow.h"
#include "annotationgraphicsview.h"
#include "classmanagerdialog.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QShortcut>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QPushButton>
#include <QTextStream>
#include <QScreen>
#include <QButtonGroup>
#include <QTranslator>
#include <QMenuBar>
#include <QMenu>
#include <QActionGroup>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
      , current_index(0)
      , current_language("zh") {
    // 从设置中读取当前语言
    QSettings settings("ImageLabeler", "ImageLabeler");
    current_language = settings.value("language", "zh").toString();

    init_ui();
    setup_shortcuts();
    create_language_menu();
    create_about_menu();
    update_language_menu();
}

MainWindow::~MainWindow() {
}

void MainWindow::init_ui() {
    setWindowTitle(tr("ImageLabeler"));
    resize(1200, 800);

    // Central widget
    QWidget *central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    // Main layout
    QHBoxLayout *main_layout = new QHBoxLayout(central_widget);

    // Left widget (buttons and class selection)
    QWidget *left_widget = new QWidget(this);
    left_widget->setFixedWidth(200);
    QVBoxLayout *left_layout = new QVBoxLayout(left_widget);

    // Button layout
    load_btn = new QPushButton(tr("加载文件夹"), this);
    connect(load_btn, &QPushButton::clicked, this, &MainWindow::load_folder);
    left_layout->addWidget(load_btn);

    prev_btn = new QPushButton(tr("上一张 (A)"), this);
    connect(prev_btn, &QPushButton::clicked, this, &MainWindow::prev_image);
    prev_btn->setEnabled(false);
    left_layout->addWidget(prev_btn);

    next_btn = new QPushButton(tr("下一张 (D)"), this);
    connect(next_btn, &QPushButton::clicked, this, &MainWindow::next_image);
    next_btn->setEnabled(false);
    left_layout->addWidget(next_btn);

    save_btn = new QPushButton(tr("保存标注 (W)"), this);
    connect(save_btn, &QPushButton::clicked, this, &MainWindow::save_current_annotations);
    left_layout->addWidget(save_btn);

    delete_btn = new QPushButton(tr("删除当前图片(E)"), this);
    connect(delete_btn, &QPushButton::clicked, this, &MainWindow::delete_current_image);
    left_layout->addWidget(delete_btn);

    // 添加绘制模式按钮
    rectangle_mode_btn = new QPushButton(tr("矩形模式 (1)"), this);
    rectangle_mode_btn->setCheckable(true);
    rectangle_mode_btn->setChecked(true);
    connect(rectangle_mode_btn, &QPushButton::clicked, this, &MainWindow::set_rectangle_mode);
    left_layout->addWidget(rectangle_mode_btn);

    polygon_mode_btn = new QPushButton(tr("多边形模式 (2)"), this);
    polygon_mode_btn->setCheckable(true);
    connect(polygon_mode_btn, &QPushButton::clicked, this, &MainWindow::set_polygon_mode);
    left_layout->addWidget(polygon_mode_btn);

    finish_polygon_btn = new QPushButton(tr("完成多边形 (Enter)"), this);
    connect(finish_polygon_btn, &QPushButton::clicked, this, &MainWindow::finish_polygon_drawing);
    left_layout->addWidget(finish_polygon_btn);

    zoom_in_btn = new QPushButton(tr("放大(+)"), this);
    connect(zoom_in_btn, &QPushButton::clicked, this, &MainWindow::zoom_in);
    left_layout->addWidget(zoom_in_btn);

    zoom_out_btn = new QPushButton(tr("缩小(-)"), this);
    connect(zoom_out_btn, &QPushButton::clicked, this, &MainWindow::zoom_out);
    left_layout->addWidget(zoom_out_btn);

    reset_view_btn = new QPushButton(tr("重置视图(R)"), this);
    connect(reset_view_btn, &QPushButton::clicked, this, &MainWindow::reset_view);
    left_layout->addWidget(reset_view_btn);

    // 创建图片列表
    image_list = new QListWidget(this);
    image_list->setFixedHeight(300);
    image_list->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(image_list, &QListWidget::itemClicked, this, &MainWindow::on_image_list_item_clicked);
    left_layout->addWidget(image_list);

    // Add stretch to push class selection to bottom
    left_layout->addStretch();

    // Class selection layout (at bottom left)
    QVBoxLayout *class_layout = new QVBoxLayout();
    class_layout->addWidget(new QLabel(tr("当前标签:")));

    // Combo box and manage button horizontal layout
    QHBoxLayout *class_control_layout = new QHBoxLayout();
    class_combo = new QComboBox(this);
    connect(class_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_class_changed);

    manage_classes_btn = new QPushButton(tr("管理标签"), this);
    manage_classes_btn->setFixedWidth(60);
    connect(manage_classes_btn, &QPushButton::clicked, this, &MainWindow::manage_classes);

    class_control_layout->addWidget(class_combo);
    class_control_layout->addWidget(manage_classes_btn);
    class_layout->addLayout(class_control_layout);

    left_layout->addLayout(class_layout);

    // Add left widget to main layout
    main_layout->addWidget(left_widget);

    // Right content area
    QWidget *right_widget = new QWidget(this);
    QVBoxLayout *right_layout = new QVBoxLayout(right_widget);

    // Image display area
    scroll_area = new QScrollArea(this);
    annotation_widget = new AnnotationGraphicsView();
    connect(annotation_widget, &AnnotationGraphicsView::rectangle_drawn,
            this, &MainWindow::on_rectangle_drawn);
    connect(annotation_widget, &AnnotationGraphicsView::mouse_moved,
            this, &MainWindow::on_mouse_moved);
    connect(annotation_widget, &AnnotationGraphicsView::rectangle_selected,
            this, &MainWindow::on_rectangle_selected);
    connect(annotation_widget, &AnnotationGraphicsView::rectangle_class_changed,
            this, &MainWindow::on_rectangle_class_changed);

    scroll_area->setWidget(annotation_widget);
    scroll_area->setWidgetResizable(true);
    right_layout->addWidget(scroll_area);

    // Status bar showing mouse position and zoom info
    status_label = new QLabel(tr("就绪"));
    right_layout->addWidget(status_label);

    // Image info display
    info_label = new QLabel(tr("未加载图片"));
    right_layout->addWidget(info_label);

    // 连接缩放变化信号
    connect(annotation_widget, &AnnotationGraphicsView::scale_changed, this, &MainWindow::update_status);

    // Set focus policy to receive keyboard events
    annotation_widget->setFocusPolicy(Qt::StrongFocus);
    annotation_widget->setFocus();

    // Add right content area to main layout
    main_layout->addWidget(right_widget);

    // 设置按钮组确保只有一个模式按钮被选中
    QButtonGroup *mode_group = new QButtonGroup(this);
    mode_group->addButton(rectangle_mode_btn);
    mode_group->addButton(polygon_mode_btn);
}

void MainWindow::set_rectangle_mode() {
    annotation_widget->set_drawing_mode(RectangleMode);
    rectangle_mode_btn->setChecked(true);
}

void MainWindow::set_polygon_mode() {
    annotation_widget->set_drawing_mode(PolygonMode);
    polygon_mode_btn->setChecked(true);
}

void MainWindow::finish_polygon_drawing() {
    annotation_widget->finish_polygon_drawing();
    // 重置为矩形模式
    annotation_widget->set_drawing_mode(RectangleMode);
    rectangle_mode_btn->setChecked(true);
    polygon_mode_btn->setChecked(false);
}

void MainWindow::setup_shortcuts() {
    // A key - Previous image
    prev_shortcut = new QShortcut(QKeySequence("A"), this);
    connect(prev_shortcut, &QShortcut::activated, this, &MainWindow::prev_image);

    // D key - Next image
    next_shortcut = new QShortcut(QKeySequence("D"), this);
    connect(next_shortcut, &QShortcut::activated, this, &MainWindow::next_image);

    // W key - Save annotations
    save_shortcut = new QShortcut(QKeySequence("W"), this);
    connect(save_shortcut, &QShortcut::activated, this, &MainWindow::save_current_annotations);

    // Ctrl+Z - Undo
    undo_shortcut = new QShortcut(QKeySequence("Ctrl+Z"), this);
    connect(undo_shortcut, &QShortcut::activated, annotation_widget, &AnnotationGraphicsView::undo);

    // Ctrl+Shift+Z - Redo
    redo_shortcut = new QShortcut(QKeySequence("Ctrl+Shift+Z"), this);
    connect(redo_shortcut, &QShortcut::activated, annotation_widget, &AnnotationGraphicsView::redo);

    // Delete key - Delete selected rectangle
    delete_shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(delete_shortcut, &QShortcut::activated, this, &MainWindow::delete_selected_rectangle);

    // E key - Delete current image with backup
    QShortcut *delete_shortcut2 = new QShortcut(QKeySequence("E"), this);
    connect(delete_shortcut2, &QShortcut::activated, this, &MainWindow::delete_current_image_with_backup);

    // R key - Reset view
    reset_view_shortcut = new QShortcut(QKeySequence("R"), this);
    connect(reset_view_shortcut, &QShortcut::activated, this, &MainWindow::reset_view);

    // + key - Zoom in
    zoom_in_shortcut = new QShortcut(QKeySequence("+"), this);
    connect(zoom_in_shortcut, &QShortcut::activated, this, &MainWindow::zoom_in);

    // - key - Zoom out
    zoom_out_shortcut = new QShortcut(QKeySequence("-"), this);
    connect(zoom_out_shortcut, &QShortcut::activated, this, &MainWindow::zoom_out);

    // 添加多边形相关快捷键
    // 1 key - Rectangle mode
    rectangle_mode_shortcut = new QShortcut(QKeySequence("1"), this);
    connect(rectangle_mode_shortcut, &QShortcut::activated, this, &MainWindow::set_rectangle_mode);

    // 2 key - Polygon mode
    polygon_mode_shortcut = new QShortcut(QKeySequence("2"), this);
    connect(polygon_mode_shortcut, &QShortcut::activated, this, &MainWindow::set_polygon_mode);

    // Enter key - Finish polygon
    finish_polygon_shortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);
    connect(finish_polygon_shortcut, &QShortcut::activated, this, &MainWindow::finish_polygon_drawing);

    // 回车键 - 完成多边形绘制
    QShortcut *finish_polygon_shortcut2 = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(finish_polygon_shortcut2, &QShortcut::activated, this, &MainWindow::finish_polygon_drawing);
}

void MainWindow::load_folder() {
    QString folder = QFileDialog::getExistingDirectory(this, tr("选择图片文件夹"));
    if (!folder.isEmpty()) {
        image_folder = folder;
        load_classes();
        load_images_from_folder();
    }
}

void MainWindow::load_classes() {
    if (image_folder.isEmpty()) {
        return;
    }

    QString classes_file = image_folder + "/classes.txt";
    classes.clear();

    QFile file(classes_file);
    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                classes.append(line);
            }
        }
        file.close();
    }

    // Update combo box
    class_combo->clear();
    for (int i = 0; i < classes.size(); ++i) {
        class_combo->addItem(QString("%1 (%2)").arg(classes.at(i)).arg(i));
    }

    // Update annotation widget with classes
    annotation_widget->set_classes(classes);

    if (!classes.isEmpty()) {
        annotation_widget->set_current_class(0);
        class_combo->setCurrentIndex(0);
    }
}

void MainWindow::save_classes() {
    if (image_folder.isEmpty()) {
        return;
    }

    QString classes_file = image_folder + "/classes.txt";
    QFile file(classes_file);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &className: classes) {
            out << className << "\n";
        }
        file.close();
    }
}

void MainWindow::manage_classes() {
    ClassManagerDialog dialog(classes, this);
    if (dialog.exec() == QDialog::Accepted) {
        classes = dialog.getClasses();
        save_classes();
        load_classes();
    }
}

void MainWindow::load_images_from_folder() {
    if (image_folder.isEmpty()) {
        return;
    }

    // Supported image formats
    QStringList image_extensions;
    image_extensions << "png" << "jpg" << "jpeg" << "bmp" << "tiff";
    image_files.clear();

    QDir dir(image_folder);
    QStringList files = dir.entryList(QDir::Files, QDir::Name);

    for (const QString &file: files) {
        QFileInfo file_info(file);
        if (image_extensions.contains(file_info.suffix().toLower())) {
            image_files.append(file);
        }
    }

    if (!image_files.isEmpty()) {
        current_index = 0;
        load_current_image();
        prev_btn->setEnabled(true);
        next_btn->setEnabled(true);
        update_image_list();
    } else {
        QMessageBox::warning(this, tr("警告"), tr("文件夹中没有找到图片文件"));
    }
}

void MainWindow::load_current_image() {
    if (current_index >= 0 && current_index < image_files.size()) {
        QString image_path = image_folder + "/" + image_files.at(current_index);
        annotation_widget->load_image(image_path);

        // Reset view to default state
        annotation_widget->reset_view();

        // 获取标注框数量
        int rect_count = annotation_widget->get_rectangle_count();
        info_label->setText(QString("图片: %1 (%2/%3) 标注数量: %4")
            .arg(image_files.at(current_index))
            .arg(current_index + 1)
            .arg(image_files.size())
            .arg(rect_count));
        update_status();
        update_image_list();
    } else {
        info_label->setText(tr("未加载图片"));
    }
}

void MainWindow::prev_image() {
    if (current_index > 0) {
        save_current_annotations();
        current_index--;
        load_current_image();
        // 自动选中当前图片
        if (current_index < image_list->count()) {
            image_list->setCurrentRow(current_index);
        }
    }
}

void MainWindow::next_image() {
    if (current_index < image_files.size() - 1) {
        save_current_annotations();
        current_index++;
        load_current_image();
        // 自动选中当前图片
        if (current_index < image_list->count()) {
            image_list->setCurrentRow(current_index);
        }
    }
}

void MainWindow::save_current_annotations() {
    if (current_index >= 0 && current_index < image_files.size()) {
        QString image_path = image_folder + "/" + image_files.at(current_index);
        annotation_widget->save_annotations(image_path);
        status_label->setText(tr("标注已保存"));
    }
}

void MainWindow::delete_current_image() {
    if (current_index >= 0 && current_index < image_files.size()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, tr("确认删除"),
            QString(tr("确定要删除图片 %1 及其标注文件吗？")).arg(image_files.at(current_index)),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            QString image_path = image_folder + "/" + image_files.at(current_index);
            QString txt_path = QFileInfo(image_path).absolutePath() + "/" +
                               QFileInfo(image_path).completeBaseName() + ".txt";

            // Delete image file
            if (QFile::exists(image_path)) {
                QFile::remove(image_path);
            }

            // Delete annotation file
            if (QFile::exists(txt_path)) {
                QFile::remove(txt_path);
            }

            // Remove from list
            QString deleted_file = image_files.takeAt(current_index);

            // Update index
            if (current_index >= image_files.size() && !image_files.isEmpty()) {
                current_index = image_files.size() - 1;
            }

            // Load new image or clear
            if (!image_files.isEmpty()) {
                load_current_image();
            } else {
                annotation_widget->clear();
                info_label->setText(tr("未加载图片"));
                prev_btn->setEnabled(false);
                next_btn->setEnabled(false);
            }

            status_label->setText(QString(tr("已删除: %1")).arg(deleted_file));
            update_image_list();
        }
    }
}

void MainWindow::delete_current_image_with_backup() {
    if (current_index >= 0 && current_index < image_files.size()) {
        QString image_path = image_folder + "/" + image_files.at(current_index);
        QString txt_path = QFileInfo(image_path).absolutePath() + "/" +
                           QFileInfo(image_path).completeBaseName() + ".txt";

        // Create bak folder
        QString bak_folder = image_folder + "/bak";
        QDir dir;
        if (!dir.exists(bak_folder)) {
            dir.mkpath(bak_folder);
        }

        // Backup image file
        if (QFile::exists(image_path)) {
            QString image_bak_path = bak_folder + "/" + image_files.at(current_index);
            QFile::copy(image_path, image_bak_path);
        }

        // Backup annotation file
        if (QFile::exists(txt_path)) {
            QString txt_filename = QFileInfo(image_files.at(current_index)).completeBaseName() + ".txt";
            QString txt_bak_path = bak_folder + "/" + txt_filename;
            QFile::copy(txt_path, txt_bak_path);
        }

        // Delete original files
        if (QFile::exists(image_path)) {
            QFile::remove(image_path);
        }

        if (QFile::exists(txt_path)) {
            QFile::remove(txt_path);
        }

        // Remove from list
        QString deleted_file = image_files.takeAt(current_index);

        // Update index
        if (current_index >= image_files.size() && !image_files.isEmpty()) {
            current_index = image_files.size() - 1;
        }

        // Load new image or clear
        if (!image_files.isEmpty()) {
            load_current_image();
        } else {
            annotation_widget->clear();
            info_label->setText(tr("未加载图片"));
            prev_btn->setEnabled(false);
            next_btn->setEnabled(false);
        }

        status_label->setText(QString(tr("已删除并备份: %1")).arg(deleted_file));
    }
}

void MainWindow::zoom_in() {
    annotation_widget->zoom_in();
    update_status();
}

void MainWindow::zoom_out() {
    annotation_widget->zoom_out();
    update_status();
}

void MainWindow::reset_view() {
    annotation_widget->reset_view();
    update_status();
}

void MainWindow::on_rectangle_drawn(QRect rect) {
    status_label->setText(QString(tr("已添加标注框: 位置(%1, %2) 大小(%3x%4)"))
        .arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height()));

    // 更新信息标签中的标注数量
    if (current_index >= 0 && current_index < image_files.size()) {
        int rect_count = annotation_widget->get_rectangle_count();
        info_label->setText(QString("图片: %1 (%2/%3) 标注数量: %4")
            .arg(image_files.at(current_index))
            .arg(current_index + 1)
            .arg(image_files.size())
            .arg(rect_count));
    }
}

void MainWindow::on_mouse_moved(QPoint pos) {
    update_status();
}

void MainWindow::on_rectangle_selected(int index) {
    // 可以在这里添加处理矩形选择的代码
    // 例如更新状态栏或其他UI元素
    Q_UNUSED(index);
}

void MainWindow::on_rectangle_class_changed(int index, int classId) {
    // 更新状态栏信息
    status_label->setText(QString(tr("已更改标注框 %1 的类别为 %2")).arg(index).arg(classId));

    // 更新信息标签中的标注数量
    if (current_index >= 0 && current_index < image_files.size()) {
        int rect_count = annotation_widget->get_rectangle_count();
        info_label->setText(QString("图片: %1 (%2/%3) 标注数量: %4")
            .arg(image_files.at(current_index))
            .arg(current_index + 1)
            .arg(image_files.size())
            .arg(rect_count));
    }
}

void MainWindow::on_class_changed(int index) {
    annotation_widget->set_current_class(index);
}

void MainWindow::delete_selected_rectangle() {
    annotation_widget->delete_selected_rectangle();
}

void MainWindow::update_status() {
    // Update status bar info
    QString status_text = QString(tr("缩放: %1x")).arg(annotation_widget->get_scale_factor(), 0, 'f', 2);
    if (!image_files.isEmpty()) {
        status_text += QString(tr(" | 图片: %1/%2")).arg(current_index + 1).arg(image_files.size());
    }
    status_label->setText(status_text);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    // Pass unhandled key events to annotation widget
    // QApplication::sendEvent(annotation_widget, event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    save_current_annotations();
    event->accept();
}

void MainWindow::update_image_list() {
    image_list->clear();
    for (int i = 0; i < image_files.size(); ++i) {
        QString text = image_files.at(i);
        QListWidgetItem *item = new QListWidgetItem();

        // 设置省略号模式，当文件名过长时在中间显示省略号
        QFontMetrics metrics(image_list->font());
        QString elidedText = metrics.elidedText(text, Qt::ElideMiddle, image_list->width() - 20);
        item->setText(elidedText);

        if (i == current_index) {
            item->setSelected(true);
            image_list->setCurrentItem(item);
        }
        image_list->addItem(item);
    }
}

void MainWindow::on_image_list_item_clicked(QListWidgetItem *item) {
    int row = image_list->row(item);
    if (row >= 0 && row < image_files.size() && row != current_index) {
        save_current_annotations();
        current_index = row;
        load_current_image();
        update_image_list();
    }
}

void MainWindow::create_language_menu() {
    QMenu *language_menu = menuBar()->addMenu(tr("语言"));

    QActionGroup *language_group = new QActionGroup(this);

    chinese_action = new QAction(tr("中文"), this);
    chinese_action->setCheckable(true);
    language_group->addAction(chinese_action);
    language_menu->addAction(chinese_action);
    connect(chinese_action, &QAction::triggered, this, &MainWindow::switch_to_chinese);

    english_action = new QAction(tr("English"), this);
    english_action->setCheckable(true);
    language_group->addAction(english_action);
    language_menu->addAction(english_action);
    connect(english_action, &QAction::triggered, this, &MainWindow::switch_to_english);
}

void MainWindow::update_language_menu() {
    // 根据当前语言设置更新菜单选中状态
    if (current_language == "zh") {
        chinese_action->setChecked(true);
        english_action->setChecked(false);
    } else if (current_language == "en") {
        chinese_action->setChecked(false);
        english_action->setChecked(true);
    }
}

void MainWindow::switch_to_chinese() {
    QMessageBox::information(this, tr("语言切换"), tr("程序将重启以应用更改"));
    // 保存语言设置
    QSettings settings("ImageLabeler", "ImageLabeler");
    settings.setValue("language", "zh");
    current_language = "zh";
    qApp->exit(777); // 特殊退出码表示语言切换
}

void MainWindow::switch_to_english() {
    QMessageBox::information(this, tr("Language Switch"), tr("The program will restart to apply changes"));
    // 保存语言设置
    QSettings settings("ImageLabeler", "ImageLabeler");
    settings.setValue("language", "en");
    current_language = "en";
    qApp->exit(778); // 特殊退出码表示语言切换
}

void MainWindow::create_about_menu() {
    QMenu *about_menu = menuBar()->addMenu(tr("关于"));

    QAction *about_action = new QAction(tr("关于 ImageLabeler"), this);
    connect(about_action, &QAction::triggered, this, &MainWindow::show_about);
    about_menu->addAction(about_action);
}

void MainWindow::show_about() {
    QString about_text = QString(
                "<h2>ImageLabeler</h2>"
                "<p>%1</p>"
                "<p>%2: 1.0</p>"
                "<p><a href='https://github.com/fgsqme/ImageLabeler'>%3</a></p>"
                "<p>%4 Qt %5</p>")
            .arg(tr("图像标注工具"))
            .arg(tr("版本"))
            .arg(tr("GitHub 地址"))
            .arg(tr("本程序使用"))
            .arg(QT_VERSION_STR);

    QMessageBox::about(this, tr("关于 ImageLabeler"), about_text);
}
