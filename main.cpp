#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    int exit_code = 777;
    QString language = "zh"; // 默认语言
    
    // 读取保存的语言设置
    QSettings settings("ImageLabeler", "ImageLabeler");
    language = settings.value("language", "zh").toString();
    
    do {
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

        QApplication app(argc, argv);
        
        // 安装翻译器
        QTranslator translator;
        if (language == "en") {
            if (translator.load(":/translations/label_en.qm") || 
                translator.load("label_en.qm") ||
                translator.load(QDir::currentPath() + "/translations/label_en.qm")) {
                app.installTranslator(&translator);
            } else {
                QMessageBox::warning(nullptr, "Warning", "Failed to load English translation file");
            }
        }
        // 中文为默认语言，不需要额外加载翻译文件
        
        MainWindow window;
        window.show();
        exit_code = app.exec();
        
        // 根据退出码判断是否需要切换语言
        if (exit_code == 777) {
            language = "zh";
            // 保存语言设置
            settings.setValue("language", "zh");
        } else if (exit_code == 778) {
            language = "en";
            // 保存语言设置
            settings.setValue("language", "en");
        }
        
    } while (exit_code == 777 || exit_code == 778); // 语言切换的特殊退出码
    
    return exit_code;
}