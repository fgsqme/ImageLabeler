#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Qlistwidget>
#include <QMainWindow>
#include <QStringList>

class QAction;
class QLabel;
class QPushButton;
class QComboBox;
class QScrollArea;
class QShortcut;
class QButtonGroup;
class QListWidget;
class QTranslator;
class QSettings;

class AnnotationGraphicsView;

// 主窗口类
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void load_folder();
    void prev_image();
    void next_image();
    void save_current_annotations();
    void delete_current_image();
    void zoom_in();
    void zoom_out();
    void reset_view();
    void on_rectangle_drawn(QRect rect);
    void on_mouse_moved(QPoint pos);
    void on_class_changed(int index);
    void delete_selected_rectangle();
    void delete_current_image_with_backup();
    void manage_classes();
    void on_rectangle_selected(int index);
    void on_rectangle_class_changed(int index, int classId);

    // 添加多边形相关槽函数
    void set_rectangle_mode();
    void set_polygon_mode();
    void finish_polygon_drawing();

    // 图片列表相关槽函数
    void on_image_list_item_clicked(QListWidgetItem *item);

    // 语言切换槽函数
    void switch_to_chinese();
    void switch_to_english();
    
    // 关于菜单槽函数
    void show_about();

private:
    void init_ui();
    void setup_shortcuts();
    void load_classes();
    void save_classes();
    void load_images_from_folder();
    void load_current_image();
    void update_status();
    void update_image_list();
    void create_language_menu();
    void create_about_menu();
    void update_language_menu();

    // 数据相关
    QString image_folder;
    QStringList image_files;
    int current_index;

    // 类别相关
    QStringList classes;

    // 界面元素
    QPushButton *load_btn;
    QPushButton *prev_btn;
    QPushButton *next_btn;
    QPushButton *save_btn;
    QPushButton *delete_btn;
    QPushButton *zoom_in_btn;
    QPushButton *zoom_out_btn;
    QPushButton *reset_view_btn;
    QPushButton *manage_classes_btn;

    // 添加绘制模式按钮
    QPushButton *rectangle_mode_btn;
    QPushButton *polygon_mode_btn;
    QPushButton *finish_polygon_btn;

    QComboBox *class_combo;
    QScrollArea *scroll_area;
    AnnotationGraphicsView *annotation_widget;
    QLabel *status_label;
    QLabel *info_label;

    // 图片列表
    QListWidget *image_list;

    // 语言切换动作
    QAction *chinese_action;
    QAction *english_action;

    // 快捷键
    QShortcut *prev_shortcut;
    QShortcut *next_shortcut;
    QShortcut *save_shortcut;
    QShortcut *undo_shortcut;
    QShortcut *redo_shortcut;
    QShortcut *delete_shortcut;
    QShortcut *reset_view_shortcut;
    QShortcut *zoom_in_shortcut;
    QShortcut *zoom_out_shortcut;

    // 添加多边形相关快捷键
    QShortcut *rectangle_mode_shortcut;
    QShortcut *polygon_mode_shortcut;
    QShortcut *finish_polygon_shortcut;
    
    // 当前语言设置
    QString current_language;
};
#endif // MAINWINDOW_H