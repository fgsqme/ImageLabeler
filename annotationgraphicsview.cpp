#include "annotationgraphicsview.h"
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QScrollBar>
#include <QPainter>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <cmath>
#include <QDebug>

AnnotationRectItem::AnnotationRectItem(int classId, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), m_classId(classId) {
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}

int AnnotationRectItem::classId() const {
    return m_classId;
}

void AnnotationRectItem::setClassId(int classId) {
    m_classId = classId;
}

void AnnotationRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray};
    QColor color = colors[m_classId % 7];

    QPen pen(color, 2);
    if (isSelected()) {
        pen.setColor(Qt::yellow);
        pen.setWidth(3);
    }
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush); // 确保不填充矩形内部
    painter->drawRect(rect());

    // Draw class label only when zoomed in enough
    // 获取父视图的缩放因子
    qreal scale = 1.0;
    if (scene() && !scene()->views().empty()) {
        scale = scene()->views().first()->transform().m11();
    }

    if (scale > 0.5 && m_classId >= 0) {
        // 获取父视图指针以获取类名列表
        AnnotationGraphicsView *view = nullptr;
        if (scene() && !scene()->views().empty()) {
            view = qobject_cast<AnnotationGraphicsView *>(scene()->views().first());
        }

        if (view) {
            QStringList classes = view->get_classes();
            if (m_classId < classes.size()) {
                const QString &className = classes.at(m_classId);
                painter->setPen(QPen(Qt::white, 1));
                painter->setFont(QFont("Arial", 8));
                painter->drawText(rect().left() + 2, rect().top() - 2,
                                  QString("%1 (%2)").arg(className).arg(m_classId));
            }
        }
    }

    // 绘制调整大小的控制点
    if (isSelected() && scene() && scene()->views().size() > 0) {
        // 在任何缩放比例下都绘制控制点，但调整控制点大小
        painter->setPen(QPen(Qt::white, 1));
        painter->setBrush(QBrush(Qt::green));

        // 控制点大小随缩放变化，但设置最小值以确保可点击
        qreal handleSize = qMax(8.0 / scale, 12.0); // 最小12像素，确保可点击

        // 角落控制点
        painter->drawRect(QRectF(rect().topLeft() - QPointF(handleSize / 2, handleSize / 2),
                                 QSizeF(handleSize, handleSize)));
        painter->drawRect(QRectF(rect().topRight() - QPointF(handleSize / 2, handleSize / 2),
                                 QSizeF(handleSize, handleSize)));
        painter->drawRect(QRectF(rect().bottomLeft() - QPointF(handleSize / 2, handleSize / 2),
                                 QSizeF(handleSize, handleSize)));
        painter->drawRect(QRectF(rect().bottomRight() - QPointF(handleSize / 2, handleSize / 2),
                                 QSizeF(handleSize, handleSize)));

        // 边缘控制点
        QPointF topCenter = QPointF((rect().left() + rect().right()) / 2, rect().top());
        QPointF bottomCenter = QPointF((rect().left() + rect().right()) / 2, rect().bottom());
        QPointF leftCenter = QPointF(rect().left(), (rect().top() + rect().bottom()) / 2);
        QPointF rightCenter = QPointF(rect().right(), (rect().top() + rect().bottom()) / 2);

        painter->drawRect(QRectF(topCenter - QPointF(handleSize / 2, handleSize / 2), QSizeF(handleSize, handleSize)));
        painter->drawRect(
            QRectF(bottomCenter - QPointF(handleSize / 2, handleSize / 2), QSizeF(handleSize, handleSize)));
        painter->drawRect(QRectF(leftCenter - QPointF(handleSize / 2, handleSize / 2), QSizeF(handleSize, handleSize)));
        painter->drawRect(QRectF(rightCenter - QPointF(handleSize / 2, handleSize / 2),
                                 QSizeF(handleSize, handleSize)));

        // 重置画刷，避免影响其他绘制
        painter->setBrush(Qt::NoBrush);
    }
}

// 重写boundingRect方法以包含文字区域，避免残影
QRectF AnnotationRectItem::boundingRect() const {
    // 获取父视图的缩放因子
    qreal scale = 1.0;
    if (scene() && !scene()->views().empty()) {
        scale = scene()->views().first()->transform().m11();
    }

    QRectF baseRect = rect();

    // 如果缩放比例足够大且有类ID，则需要包含文字区域
    if (scale > 0.5 && m_classId >= 0) {
        // 原始矩形加上文字区域的高度
        qreal textHeight = 12; // 估计文字高度
        qreal textWidth = 100; // 估计文字宽度
        baseRect.adjust(-5, -textHeight - 2, textWidth, 5);
    }

    // 如果被选中，需要包含控制点区域（不再依赖缩放比例）
    if (isSelected()) {
        // 使用固定的控制点大小来计算边界，确保在任何缩放比例下都有足够的边界
        qreal handleSize = 12.0; // 与paint方法中最小控制点大小一致
        baseRect.adjust(-handleSize, -handleSize, handleSize, handleSize);
    } else {
        // 默认返回矩形区域
        baseRect.adjust(-5, -5, 5, 5); // 添加一些边距
    }

    return baseRect;
}

AnnotationPolygonItem::AnnotationPolygonItem(int classId, QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent), m_classId(classId) {
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
}

int AnnotationPolygonItem::classId() const {
    return m_classId;
}

void AnnotationPolygonItem::setClassId(int classId) {
    m_classId = classId;
}

void AnnotationPolygonItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray};
    QColor color = colors[m_classId % 7];

    QPen pen(color, 2);
    if (isSelected()) {
        pen.setColor(Qt::yellow);
        pen.setWidth(3);
    }
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPolygon(polygon());

    // Draw class label only when zoomed in enough
    qreal scale = 1.0;
    if (scene() && !scene()->views().empty()) {
        scale = scene()->views().first()->transform().m11();
    }

    if (scale > 0.5 && m_classId >= 0) {
        AnnotationGraphicsView *view = nullptr;
        if (scene() && !scene()->views().empty()) {
            view = qobject_cast<AnnotationGraphicsView *>(scene()->views().first());
        }

        if (view) {
            QStringList classes = view->get_classes();
            if (m_classId < classes.size()) {
                const QString &className = classes.at(m_classId);
                painter->setPen(QPen(Qt::white, 1));
                painter->setFont(QFont("Arial", 8));

                // 在多边形附近绘制标签
                QRectF polyBoundingRect = polygon().boundingRect();
                painter->drawText(polyBoundingRect.left() + 2, polyBoundingRect.top() - 2,
                                  QString("%1 (%2)").arg(className).arg(m_classId));
            }
        }
    }

    // 绘制顶点控制点
    if (isSelected() && scene() && !scene()->views().empty()) {
        // 绘制顶点控制点
        painter->setPen(QPen(Qt::white, 1));
        painter->setBrush(QBrush(Qt::black));

        // 控制点大小随缩放变化，但设置最小值以确保可点击
        qreal handleSize = qMax(8.0 / scale, 12.0); // 最小12像素，确保可点击

        const QPolygonF &poly = polygon();
        for (auto vertex: poly) {
            painter->drawRect(QRectF(vertex - QPointF(handleSize / 2, handleSize / 2), QSizeF(handleSize, handleSize)));
        }
    }
}

QRectF AnnotationPolygonItem::boundingRect() const {
    qreal scale = 1.0;
    if (scene() && !scene()->views().empty()) {
        scale = scene()->views().first()->transform().m11();
    }

    QRectF baseRect = polygon().boundingRect();

    if (scale > 0.5 && m_classId >= 0) {
        qreal textHeight = 12;
        qreal textWidth = 100;
        baseRect.adjust(-5, -textHeight - 2, textWidth, 5);
    }

    baseRect.adjust(-5, -5, 5, 5);

    return baseRect;
}

AnnotationGraphicsView::AnnotationGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
      , m_scene(new QGraphicsScene(this))
      , m_pixmapItem(nullptr)
      , m_currentClassId(0)
      , m_scaleFactor(1.0)
      , m_drawing(false)
      , m_currentDrawingRect(nullptr)
      , m_drawingMode(RectangleMode) // 默认为矩形模式
      , m_currentDrawingPolygon(nullptr)
      , m_selectedItem(nullptr)
      , m_selectedIndex(-1)
      , m_moving(false)
      , m_resizing(false)
      , m_resizeHandle(NoHandle)
      , m_vertexEditing(false)
      , m_vertexEditHandle(NoVertexHandle)
      , m_contextMenu(new QMenu(this)) {
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing, false); // 默认禁用抗锯齿以提高性能
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(NoDrag);
    // 确保默认锚点设置正确
    setTransformationAnchor(AnchorUnderMouse);
    setResizeAnchor(AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setViewportUpdateMode(BoundingRectViewportUpdate); // 优化更新区域
    setOptimizationFlags(DontAdjustForAntialiasing | DontSavePainterState);

    // 确保视图可以接收鼠标事件
    setMouseTracking(true);
}

QStringList AnnotationGraphicsView::get_classes() const {
    return m_classes;
}

void AnnotationGraphicsView::load_image(const QString &imagePath) {
    clear();

    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        QImage image(imagePath);
        if (!image.isNull()) {
            pixmap = QPixmap::fromImage(image);
        }
    }

    if (!pixmap.isNull()) {
        if (m_pixmapItem) {
            m_scene->removeItem(m_pixmapItem);
            delete m_pixmapItem;
        }

        m_pixmapItem = m_scene->addPixmap(pixmap);
        m_pixmapItem->setZValue(-1);
        m_scene->setSceneRect(m_pixmapItem->boundingRect());

        load_annotations(imagePath);
    }

    reset_view();
}

void AnnotationGraphicsView::load_annotations(const QString &imagePath) {
    m_rectangles.clear();
    m_polygons.clear();
    QString txtPath = QFileInfo(imagePath).absolutePath() + "/" + QFileInfo(imagePath).completeBaseName() + ".txt";

    QFile file(txtPath);
    if (!file.exists()) {
        update_rect_items();
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        update_rect_items();
        return;
    }

    QPixmap pixmap = m_pixmapItem->pixmap();
    if (pixmap.isNull()) {
        update_rect_items();
        return;
    }

    int img_w = pixmap.width();
    int img_h = pixmap.height();

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        if (parts.size() >= 5) {
            int class_id = parts[0].toInt();

            // 判断是矩形还是多边形
            if (parts.size() == 5) {
                // 矩形格式: class_id x_center y_center width height
                double x_center = parts[1].toDouble();
                double y_center = parts[2].toDouble();
                double width = parts[3].toDouble();
                double height = parts[4].toDouble();

                // Convert to pixel coordinates
                x_center *= img_w;
                y_center *= img_h;
                width *= img_w;
                height *= img_h;

                int x = static_cast<int>(x_center - width / 2);
                int y = static_cast<int>(y_center - height / 2);
                int w = static_cast<int>(width);
                int h = static_cast<int>(height);

                m_rectangles.append(GraphicsAnnotationRect(x, y, w, h, class_id));
            } else if (parts.size() % 2 == 1) {
                // 多边形格式: class_id x1 y1 x2 y2 ... xn yn
                QVector<QPointF> points;
                for (int i = 1; i < parts.size(); i += 2) {
                    double x = parts[i].toDouble() * img_w;
                    double y = parts[i + 1].toDouble() * img_h;
                    points.append(QPointF(x, y));
                }
                if (points.size() >= 3) {
                    m_polygons.append(GraphicsAnnotationPolygon(points, class_id));
                }
            }
        }
    }

    file.close();
    update_rect_items();
}

void AnnotationGraphicsView::save_annotations(const QString &imagePath) {
    if (!m_pixmapItem) return;

    QString txtPath = QFileInfo(imagePath).absolutePath() + "/" + QFileInfo(imagePath).completeBaseName() + ".txt";

    if (m_rectangles.isEmpty() && m_polygons.isEmpty()) {
        QFile::remove(txtPath);
        return;
    }

    QPixmap pixmap = m_pixmapItem->pixmap();
    if (pixmap.isNull()) {
        return;
    }

    int img_w = pixmap.width();
    int img_h = pixmap.height();

    QFile file(txtPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);

    // 保存矩形注释
    for (const auto &rect: m_rectangles) {
        double x_center = (rect.x + rect.width / 2.0) / img_w;
        double y_center = (rect.y + rect.height / 2.0) / img_h;
        double width = static_cast<double>(rect.width) / img_w;
        double height = static_cast<double>(rect.height) / img_h;

        out << rect.classId << " "
                << QString::number(x_center, 'f', 6) << " "
                << QString::number(y_center, 'f', 6) << " "
                << QString::number(width, 'f', 6) << " "
                << QString::number(height, 'f', 6) << "\n";
    }

    // 保存多边形注释
    for (const auto &polygon: m_polygons) {
        out << polygon.classId;
        for (const auto &point: polygon.points) {
            double x = point.x() / img_w;
            double y = point.y() / img_h;
            out << " " << QString::number(x, 'f', 6)
                    << " " << QString::number(y, 'f', 6);
        }
        out << "\n";
    }

    file.close();
}

void AnnotationGraphicsView::clear() {
    m_rectangles.clear();
    m_rectItems.clear();
    m_polygons.clear();
    m_polygonItems.clear();
    m_undoStack.clear();
    m_redoStack.clear();
    m_polygonUndoStack.clear();
    m_polygonRedoStack.clear();

    if (m_pixmapItem) {
        m_scene->removeItem(m_pixmapItem);
        delete m_pixmapItem;
        m_pixmapItem = nullptr;
    }

    // 清理当前正在绘制的图形
    if (m_currentDrawingRect) {
        m_scene->removeItem(m_currentDrawingRect);
        delete m_currentDrawingRect;
        m_currentDrawingRect = nullptr;
    }

    if (m_currentDrawingPolygon) {
        m_scene->removeItem(m_currentDrawingPolygon);
        delete m_currentDrawingPolygon;
        m_currentDrawingPolygon = nullptr;
    }

    m_drawing = false;
    m_currentPolygonPoints.clear();
    m_drawingMode = RectangleMode; // 重置为矩形模式

    m_selectedItem = nullptr;
    m_selectedIndex = -1;

    m_scene->clear();
}

void AnnotationGraphicsView::set_current_class(int classId) {
    m_currentClassId = classId;
}

void AnnotationGraphicsView::set_classes(const QStringList &class_list) {
    m_classes = class_list;
}

void AnnotationGraphicsView::save_state() {
    // 保存矩形状态
    m_undoStack.push_back(m_rectangles);

    // 保存多边形状态
    m_polygonUndoStack.push_back(m_polygons);

    // 限制撤销栈大小
    if (m_undoStack.size() > 100) {
        m_undoStack.removeFirst();
    }

    // 限制多边形撤销栈大小
    if (m_polygonUndoStack.size() > 100) {
        m_polygonUndoStack.removeFirst();
    }

    // 清空重做栈
    m_redoStack.clear();
    m_polygonRedoStack.clear();
}

void AnnotationGraphicsView::undo() {
    if (!m_undoStack.isEmpty() || !m_polygonUndoStack.isEmpty()) {
        // 保存当前状态到重做栈
        m_redoStack.push_back(m_rectangles);
        m_polygonRedoStack.push_back(m_polygons);

        // 从撤销栈中恢复状态
        if (!m_undoStack.isEmpty()) {
            m_rectangles = m_undoStack.takeLast();
        }

        if (!m_polygonUndoStack.isEmpty()) {
            m_polygons = m_polygonUndoStack.takeLast();
        }

        update_rect_items();

        // 限制重做栈大小
        if (m_redoStack.size() > 100) {
            m_redoStack.removeFirst();
        }

        // 限制多边形重做栈大小
        if (m_polygonRedoStack.size() > 100) {
            m_polygonRedoStack.removeFirst();
        }
    }
}

void AnnotationGraphicsView::redo() {
    if (!m_redoStack.isEmpty() || !m_polygonRedoStack.isEmpty()) {
        // 保存当前状态到撤销栈
        m_undoStack.push_back(m_rectangles);
        m_polygonUndoStack.push_back(m_polygons);

        // 从重做栈中恢复状态
        if (!m_redoStack.isEmpty()) {
            m_rectangles = m_redoStack.takeLast();
        }

        if (!m_polygonRedoStack.isEmpty()) {
            m_polygons = m_polygonRedoStack.takeLast();
        }

        update_rect_items();

        // 限制撤销栈大小
        if (m_undoStack.size() > 100) {
            m_undoStack.removeFirst();
        }

        // 限制多边形撤销栈大小
        if (m_polygonUndoStack.size() > 100) {
            m_polygonUndoStack.removeFirst();
        }
    }
}

void AnnotationGraphicsView::update_rect_items() {
    // 清理现有的矩形项
    qDeleteAll(m_rectItems);
    m_rectItems.clear();

    // 清理现有的多边形项
    qDeleteAll(m_polygonItems);
    m_polygonItems.clear();

    if (!m_pixmapItem) return;

    // 创建新的矩形项
    for (const auto &rect: m_rectangles) {
        QRectF sceneRect = graphics_annotation_rect_to_scene(rect);
        auto *item = new AnnotationRectItem(rect.classId);
        item->setRect(sceneRect);
        m_scene->addItem(item);
        m_rectItems.append(item);
    }

    // 创建新的多边形项
    for (const auto &polygon: m_polygons) {
        QPolygonF scenePolygon;
        for (const auto &point: polygon.points) {
            scenePolygon.append(point);
        }

        auto *item = new AnnotationPolygonItem(polygon.classId);
        item->setPolygon(scenePolygon);
        m_scene->addItem(item);
        m_polygonItems.append(item);
    }
}

int AnnotationGraphicsView::get_selected_rectangle_index() const {
    return m_selectedIndex;
}

int AnnotationGraphicsView::get_rectangle_count() const {
    return m_rectangles.size();
}

void AnnotationGraphicsView::zoom_in() {
    // 保存当前变换锚点
    ViewportAnchor anchor = transformationAnchor();

    // 设置以视图中心为中心进行变换
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    scale(1.2, 1.2);
    m_scaleFactor *= 1.2;

    // 发送缩放变化信号
    emit scale_changed(m_scaleFactor);

    // 恢复原来的变换锚点
    setTransformationAnchor(anchor);
}

void AnnotationGraphicsView::zoom_out() {
    // 保存当前变换锚点
    ViewportAnchor anchor = transformationAnchor();

    // 设置以视图中心为中心进行变换
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    scale(1.0 / 1.2, 1.0 / 1.2);
    m_scaleFactor /= 1.2;

    // 发送缩放变化信号
    emit scale_changed(m_scaleFactor);

    // 恢复原来的变换锚点
    setTransformationAnchor(anchor);
}

void AnnotationGraphicsView::reset_view() {
    if (m_pixmapItem) {
        fitInView(m_pixmapItem, Qt::KeepAspectRatio);
        m_scaleFactor = 1.0;
        // 发送缩放变化信号
        emit scale_changed(m_scaleFactor);
    }
}

double AnnotationGraphicsView::get_scale_factor() const {
    return m_scaleFactor;
}

void AnnotationGraphicsView::delete_selected_rectangle() {
    if (m_selectedIndex >= 0) {
        if (m_selectedIndex < m_rectangles.size()) {
            // 删除矩形
            save_state();
            m_rectangles.removeAt(m_selectedIndex);
            update_rect_items();
            m_selectedIndex = -1;
            m_selectedItem = nullptr;
        } else if (m_selectedIndex >= m_rectItems.size() &&
                   m_selectedIndex < m_rectItems.size() + m_polygons.size()) {
            // 删除多边形
            save_state();
            int polygonIndex = m_selectedIndex - m_rectItems.size();
            m_polygons.removeAt(polygonIndex);
            update_rect_items();
            m_selectedIndex = -1;
            m_selectedItem = nullptr;
        }
    }
}

void AnnotationGraphicsView::mousePressEvent(QMouseEvent *event) {
    if (!m_pixmapItem) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPointF scenePos = mapToScene(event->pos());

    if (event->button() == Qt::LeftButton) {
        // 左键按下
        if (m_drawingMode == PolygonMode) {
            // 多边形绘制模式
            if (scenePos.x() >= 0 && scenePos.y() >= 0 &&
                scenePos.x() <= m_pixmapItem->pixmap().width() &&
                scenePos.y() <= m_pixmapItem->pixmap().height()) {
                if (!m_drawing) {
                    // 开始绘制多边形
                    m_drawing = true;
                    m_currentPolygonPoints.clear();
                    m_currentPolygonPoints.append(scenePos);

                    m_selectedItem = nullptr;
                    m_selectedIndex = -1;
                    emit rectangle_selected(-1);

                    // 创建临时绘制多边形
                    if (m_currentDrawingPolygon) {
                        m_scene->removeItem(m_currentDrawingPolygon);
                        delete m_currentDrawingPolygon;
                    }

                    QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray};
                    QColor color = colors[m_currentClassId % 7];

                    m_currentDrawingPolygon = new QGraphicsPolygonItem();
                    m_currentDrawingPolygon->setPen(QPen(color, 2, Qt::DashLine));
                    m_scene->addItem(m_currentDrawingPolygon);
                } else {
                    // 添加点到多边形
                    m_currentPolygonPoints.append(scenePos);

                    // 更新临时多边形
                    QPolygonF polygon(m_currentPolygonPoints);
                    m_currentDrawingPolygon->setPolygon(polygon);
                }
            }
        } else {
            // 矩形绘制模式（原有逻辑）
            // 首先检查是否点击在当前选中项的控制点上
            bool handled = false;

            // 检查矩形控制点
            if (m_selectedItem) {
                int resizeHandle = get_resize_handle_at(scenePos);
                if (resizeHandle != NoHandle) {
                    // Start resizing
                    m_resizeHandle = resizeHandle;
                    m_resizing = true;
                    m_moving = false;
                    m_lastMousePos = scenePos;
                    save_state();
                    viewport()->setCursor(Qt::SizeAllCursor); // 设置光标
                    handled = true;
                }
            }

            // 检查多边形顶点控制点
            if (!handled && m_selectedIndex >= m_rectItems.size()) {
                int vertexHandle = get_vertex_handle_at(scenePos);
                if (vertexHandle != NoVertexHandle) {
                    // 开始编辑顶点
                    m_vertexEditHandle = vertexHandle;
                    m_vertexEditing = true;
                    m_moving = false;
                    m_resizing = false;
                    m_lastMousePos = scenePos;
                    save_state();
                    viewport()->setCursor(Qt::SizeAllCursor); // 设置光标
                    handled = true;
                }
            }

            if (!handled) {
                // Check if clicked on a rectangle
                AnnotationRectItem *clickedItem = item_at(scenePos);

                // 检查是否点击在多边形上
                AnnotationPolygonItem *clickedPolygonItem = nullptr;
                if (!clickedItem) {
                    QList<QGraphicsItem *> items = m_scene->items(scenePos);
                    for (QGraphicsItem *item: items) {
                        if (item != m_pixmapItem && item != m_currentDrawingRect && item != m_currentDrawingPolygon) {
                            clickedPolygonItem = qgraphicsitem_cast<AnnotationPolygonItem *>(item);
                            if (clickedPolygonItem) {
                                break;
                            }
                        }
                    }
                }

                if (clickedItem) {
                    // Select rectangle
                    m_selectedItem = clickedItem;
                    m_selectedIndex = m_rectItems.indexOf(m_selectedItem);
                    emit rectangle_selected(m_selectedIndex);

                    // Check if clicked on resize handle
                    m_resizeHandle = get_resize_handle_at(scenePos);
                    if (m_resizeHandle != NoHandle) {
                        // Start resizing
                        m_resizing = true;
                        m_moving = false;
                        m_lastMousePos = scenePos;
                        save_state();
                        viewport()->setCursor(Qt::SizeAllCursor); // 设置光标
                    } else {
                        // Start moving
                        m_moving = true;
                        m_resizing = false;
                        m_lastMousePos = scenePos;
                        save_state();
                    }
                } else if (clickedPolygonItem) {
                    // 选择多边形项
                    m_selectedItem = nullptr; // 矩形项设为nullptr
                    m_selectedIndex = m_rectItems.size() + m_polygonItems.indexOf(clickedPolygonItem);
                    emit rectangle_selected(m_selectedIndex);

                    // 检查是否点击在顶点上
                    m_vertexEditHandle = get_vertex_handle_at(scenePos);
                    if (m_vertexEditHandle != NoVertexHandle) {
                        // 开始编辑顶点
                        m_vertexEditing = true;
                        m_moving = false;
                        m_resizing = false;
                        m_lastMousePos = scenePos;
                        save_state();
                        viewport()->setCursor(Qt::SizeAllCursor); // 设置光标
                    } else {
                        // 开始移动多边形
                        m_moving = true;
                        m_resizing = false;
                        m_vertexEditing = false;
                        m_lastMousePos = scenePos;
                        save_state();
                    }
                } else {
                    // Start drawing new rectangle
                    if (scenePos.x() >= 0 && scenePos.y() >= 0 &&
                        scenePos.x() <= m_pixmapItem->pixmap().width() &&
                        scenePos.y() <= m_pixmapItem->pixmap().height()) {
                        m_drawing = true;
                        m_startPoint = scenePos;
                        m_endPoint = scenePos;
                        m_moving = false;
                        m_resizing = false;

                        m_selectedItem = nullptr;
                        m_selectedIndex = -1;
                        emit rectangle_selected(-1);

                        // Create temporary drawing rectangle
                        if (m_currentDrawingRect) {
                            m_scene->removeItem(m_currentDrawingRect);
                            delete m_currentDrawingRect;
                        }

                        QColor colors[] = {Qt::red, Qt::green, Qt::blue, Qt::cyan, Qt::magenta, Qt::yellow, Qt::gray};
                        QColor color = colors[m_currentClassId % 7];

                        m_currentDrawingRect = new QGraphicsRectItem();
                        m_currentDrawingRect->setPen(QPen(color, 2, Qt::DashLine));
                        m_scene->addItem(m_currentDrawingRect);
                    }
                }
            }
        }
    } else if (event->button() == Qt::RightButton) {
        // 右键点击
        if (m_drawingMode == PolygonMode && m_drawing) {
            // 右键完成多边形绘制
            finish_polygon_drawing();
        } else {
            // 原有矩形模式逻辑
            QPointF pos = mapToScene(event->pos());
            AnnotationRectItem *clickedItem = item_at(pos);

            // 检查是否点击在多边形上
            AnnotationPolygonItem *clickedPolygonItem = nullptr;
            if (!clickedItem) {
                QList<QGraphicsItem *> items = m_scene->items(pos);
                for (QGraphicsItem *item: items) {
                    if (item != m_pixmapItem && item != m_currentDrawingRect && item != m_currentDrawingPolygon) {
                        clickedPolygonItem = qgraphicsitem_cast<AnnotationPolygonItem *>(item);
                        if (clickedPolygonItem) {
                            break;
                        }
                    }
                }
            }

            if (clickedItem) {
                // Select rectangle and show context menu
                m_selectedItem = clickedItem;
                m_selectedIndex = m_rectItems.indexOf(m_selectedItem);
                emit rectangle_selected(m_selectedIndex);
                show_context_menu(event->globalPos());
            } else if (clickedPolygonItem) {
                // 选择多边形并显示上下文菜单
                m_selectedItem = nullptr; // 矩形项设为nullptr
                m_selectedIndex = m_rectItems.size() + m_polygonItems.indexOf(clickedPolygonItem);
                emit rectangle_selected(m_selectedIndex);
                show_context_menu(event->globalPos());
            } else {
                // Start panning
                m_dragStartPos = event->pos();
                setDragMode(ScrollHandDrag);
                // 在鼠标按下时也确保锚点设置正确
                setTransformationAnchor(AnchorUnderMouse);
                QMouseEvent fakeEvent(QEvent::MouseButtonPress, event->pos(), Qt::LeftButton, Qt::LeftButton,
                                      Qt::NoModifier);
                QGraphicsView::mousePressEvent(&fakeEvent);
            }
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void AnnotationGraphicsView::mouseMoveEvent(QMouseEvent *event) {
    QPointF scenePos = mapToScene(event->pos());
    emit mouse_moved(event->pos());

    if (m_drawingMode == PolygonMode && m_drawing) {
        // 多边形绘制模式下的鼠标移动处理
        if (!m_currentPolygonPoints.empty() && m_currentDrawingPolygon) {
            // 创建临时多边形用于预览（包含当前鼠标位置）
            QVector<QPointF> tempPoints = m_currentPolygonPoints;
            tempPoints.append(scenePos);

            QPolygonF polygon(tempPoints);
            m_currentDrawingPolygon->setPolygon(polygon);
        }
    } else if (m_drawing) {
        // 矩形绘制模式（原有逻辑）
        m_endPoint = scenePos;
        QRectF rect(m_startPoint, m_endPoint);
        rect = rect.normalized();
        m_currentDrawingRect->setRect(rect);
    } else if (m_resizing && m_selectedItem) {
        // 调整矩形大小
        QRectF currentRect = m_selectedItem->rect();
        QRectF newRect = currentRect;

        // 根据不同的控制点调整矩形
        switch (m_resizeHandle) {
            case TopLeftHandle:
                newRect.setTopLeft(scenePos);
                break;
            case TopRightHandle:
                newRect.setTopRight(scenePos);
                break;
            case BottomLeftHandle:
                newRect.setBottomLeft(scenePos);
                break;
            case BottomRightHandle:
                newRect.setBottomRight(scenePos);
                break;
            case TopHandle:
                newRect.setTop(scenePos.y());
                break;
            case LeftHandle:
                newRect.setLeft(scenePos.x());
                break;
            case RightHandle:
                newRect.setRight(scenePos.x());
                break;
            case BottomHandle:
                newRect.setBottom(scenePos.y());
                break;
            default:
                break;
        }

        // 确保矩形有最小尺寸
        if (newRect.width() >= 1 && newRect.height() >= 1) {
            // 规范化矩形（确保宽度和高度为正）
            newRect = newRect.normalized();

            // 更新矩形位置和大小
            m_selectedItem->setRect(newRect);

            // 计算更新区域以避免残影
            QRectF updateRect = currentRect.united(newRect);
            // 添加边距确保完全更新
            updateRect.adjust(-10, -10, 10, 10);
            // 只更新需要更新的区域
            viewport()->update(mapFromScene(updateRect).boundingRect());
        }

        m_lastMousePos = scenePos;
    } else if (m_moving) {
        QPointF delta = scenePos - m_lastMousePos;

        if (m_selectedItem) {
            // 移动矩形项
            QRectF currentRect = m_selectedItem->rect();
            QRectF newRect = currentRect.translated(delta.x(), delta.y());
            m_selectedItem->setRect(newRect);

            // 计算更新区域以避免残影
            QRectF updateRect = currentRect.united(newRect);
            // 添加边距确保完全更新
            updateRect.adjust(-10, -10, 10, 10);
            // 只更新需要更新的区域
            viewport()->update(mapFromScene(updateRect).boundingRect());
        } else if (m_selectedIndex >= m_rectItems.size()) {
            // 移动多边形项
            int polygonIndex = m_selectedIndex - m_rectItems.size();
            if (polygonIndex >= 0 && polygonIndex < m_polygonItems.size()) {
                AnnotationPolygonItem *polygonItem = m_polygonItems[polygonIndex];
                QPolygonF currentPolygon = polygonItem->polygon();
                QPolygonF newPolygon = currentPolygon.translated(delta.x(), delta.y());
                polygonItem->setPolygon(newPolygon);

                // 计算更新区域以避免残影
                QRectF currentBoundingRect = currentPolygon.boundingRect();
                QRectF newBoundingRect = newPolygon.boundingRect();
                QRectF updateRect = currentBoundingRect.united(newBoundingRect);
                // 添加边距确保完全更新
                updateRect.adjust(-10, -10, 10, 10);
                // 只更新需要更新的区域
                viewport()->update(mapFromScene(updateRect).boundingRect());
            }
        }

        m_lastMousePos = scenePos;
    } else if (m_vertexEditing) {
        // 编辑多边形顶点
        if (m_selectedIndex >= m_rectItems.size()) {
            int polygonIndex = m_selectedIndex - m_rectItems.size();
            if (polygonIndex >= 0 && polygonIndex < m_polygonItems.size() &&
                m_vertexEditHandle >= 0) {
                AnnotationPolygonItem *polygonItem = m_polygonItems[polygonIndex];
                QPolygonF polygon = polygonItem->polygon();

                if (m_vertexEditHandle < polygon.size()) {
                    // 更新顶点位置
                    polygon[m_vertexEditHandle] = scenePos;
                    polygonItem->setPolygon(polygon);

                    // 更新视图
                    viewport()->update();
                }
            }
        }

        m_lastMousePos = scenePos;
    } else {
        // 更新鼠标光标
        // 优先检查当前选中项的控制点
        bool cursorSet = false;

        // 检查矩形控制点
        if (m_selectedItem) {
            int handle = get_resize_handle_at(scenePos);
            switch (handle) {
                case TopLeftHandle:
                case BottomRightHandle:
                    viewport()->setCursor(Qt::SizeFDiagCursor);
                    cursorSet = true;
                    break;
                case TopRightHandle:
                case BottomLeftHandle:
                    viewport()->setCursor(Qt::SizeBDiagCursor);
                    cursorSet = true;
                    break;
                case TopHandle:
                case BottomHandle:
                    viewport()->setCursor(Qt::SizeVerCursor);
                    cursorSet = true;
                    break;
                case LeftHandle:
                case RightHandle:
                    viewport()->setCursor(Qt::SizeHorCursor);
                    cursorSet = true;
                    break;
                default:
                    break;
            }
        }

        // 检查多边形顶点控制点
        if (!cursorSet && m_selectedIndex >= m_rectItems.size()) {
            int polygonIndex = m_selectedIndex - m_rectItems.size();
            if (polygonIndex >= 0 && polygonIndex < m_polygonItems.size()) {
                AnnotationPolygonItem *polygonItem = m_polygonItems[polygonIndex];
                QPolygonF polygon = polygonItem->polygon();

                bool onVertex = false;
                for (int i = 0; i < polygon.size(); ++i) {
                    if (get_vertex_handle_rect(polygon, i).contains(scenePos)) {
                        onVertex = true;
                        break;
                    }
                }

                if (onVertex) {
                    viewport()->setCursor(Qt::PointingHandCursor);
                    cursorSet = true;
                }
            }
        }

        // 如果以上都没设置光标，则使用默认光标
        if (!cursorSet) {
            viewport()->setCursor(Qt::ArrowCursor);
        }

        QGraphicsView::mouseMoveEvent(event);
    }
}

void AnnotationGraphicsView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if (m_drawingMode == PolygonMode) {
            // 多边形模式不需要在释放时执行额外操作
            // 所有操作都在按下时处理
        } else if (m_drawing) {
            m_drawing = false;

            if (m_currentDrawingRect) {
                QRectF rect = m_currentDrawingRect->rect();
                if (rect.width() > 5 && rect.height() > 5) {
                    save_state();
                    m_rectangles.append(scene_rect_to_annotation(rect));
                    update_rect_items();
                    emit rectangle_drawn(rect.toRect());
                }

                m_scene->removeItem(m_currentDrawingRect);
                delete m_currentDrawingRect;
                m_currentDrawingRect = nullptr;
            }
        } else if (m_resizing) {
            m_resizing = false;
            // Update the annotation data with the new size
            if (m_selectedItem && m_selectedIndex >= 0 && m_selectedIndex < m_rectangles.size()) {
                QRectF rect = m_selectedItem->rect();
                GraphicsAnnotationRect annotationRect = scene_rect_to_annotation(rect);
                m_rectangles[m_selectedIndex] = annotationRect;
            }
        } else if (m_moving) {
            m_moving = false;
            // Update the annotation data with the new position
            if (m_selectedItem && m_selectedIndex >= 0 && m_selectedIndex < m_rectangles.size()) {
                // 矩形项位置更新
                QRectF rect = m_selectedItem->rect();
                GraphicsAnnotationRect annotationRect = scene_rect_to_annotation(rect);
                m_rectangles[m_selectedIndex] = annotationRect;
            } else if (m_selectedIndex >= m_rectItems.size()) {
                // 多边形项位置更新
                int polygonIndex = m_selectedIndex - m_rectItems.size();
                if (polygonIndex >= 0 && polygonIndex < m_polygonItems.size()) {
                    AnnotationPolygonItem *polygonItem = m_polygonItems[polygonIndex];
                    QPolygonF polygon = polygonItem->polygon();

                    // 更新多边形数据
                    if (polygonIndex < m_polygons.size()) {
                        GraphicsAnnotationPolygon annotationPolygon(m_polygons[polygonIndex].points,
                                                                    m_polygons[polygonIndex].classId);
                        annotationPolygon.points.clear();
                        for (const QPointF &point: polygon) {
                            annotationPolygon.points.append(point);
                        }
                        m_polygons[polygonIndex] = annotationPolygon;
                    }
                }
            }
        } else if (m_vertexEditing) {
            m_vertexEditing = false;
            // 更新多边形顶点数据
            if (m_selectedIndex >= m_rectItems.size()) {
                int polygonIndex = m_selectedIndex - m_rectItems.size();
                if (polygonIndex >= 0 && polygonIndex < m_polygonItems.size() &&
                    m_vertexEditHandle >= 0) {
                    AnnotationPolygonItem *polygonItem = m_polygonItems[polygonIndex];
                    QPolygonF polygon = polygonItem->polygon();

                    // 更新多边形数据
                    if (polygonIndex < m_polygons.size() && m_vertexEditHandle < polygon.size()) {
                        GraphicsAnnotationPolygon annotationPolygon(m_polygons[polygonIndex].points,
                                                                    m_polygons[polygonIndex].classId);
                        annotationPolygon.points[m_vertexEditHandle] = polygon[m_vertexEditHandle]; // 只更新变化的顶点
                        m_polygons[polygonIndex] = annotationPolygon;
                    }
                }
            }
        }
    } else if (event->button() == Qt::RightButton) {
        if (dragMode() == QGraphicsView::ScrollHandDrag) {
            setDragMode(QGraphicsView::NoDrag);
            QMouseEvent fakeEvent(QEvent::MouseButtonRelease, event->pos(), Qt::LeftButton, Qt::LeftButton,
                                  Qt::NoModifier);
            QGraphicsView::mouseReleaseEvent(&fakeEvent);
            viewport()->setCursor(Qt::ArrowCursor);
            // 恢复锚点设置
            setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
            return;
        }
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void AnnotationGraphicsView::wheelEvent(QWheelEvent *event) {
    if (!m_pixmapItem) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    // 设置以鼠标位置为中心进行变换
    setTransformationAnchor(AnchorUnderMouse);

    // 根据缩放级别动态调整渲染质量
    double newScaleFactor = m_scaleFactor * pow(1.2, event->angleDelta().y() / 120.0);

    // 在高缩放比例下启用抗锯齿，在低缩放比例下禁用以提高性能
    if (newScaleFactor > 2.0) {
        setRenderHint(QPainter::Antialiasing, true);
    } else {
        setRenderHint(QPainter::Antialiasing, false);
    }

    // Zoom in/out based on wheel movement
    double scaleFactor = pow(1.2, event->angleDelta().y() / 120.0);
    scale(scaleFactor, scaleFactor);
    m_scaleFactor *= scaleFactor;

    // Limit zoom range
    m_scaleFactor = qBound(0.1, m_scaleFactor, 10.0);

    // 发送缩放变化信号
    emit scale_changed(m_scaleFactor);

    event->accept();
}

void AnnotationGraphicsView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete && m_selectedIndex >= 0) {
        delete_selected_rectangle();
    } else {
        QGraphicsView::keyPressEvent(event);
    }
}

AnnotationRectItem *AnnotationGraphicsView::item_at(const QPointF &pos) {
    QList<QGraphicsItem *> items = m_scene->items(pos);
    for (QGraphicsItem *item: items) {
        if (item != m_pixmapItem && item != m_currentDrawingRect && item != m_currentDrawingPolygon) {
            // 检查是否为矩形项或多边形项
            auto *rectItem = qgraphicsitem_cast<AnnotationRectItem *>(item);
            if (rectItem) {
                return rectItem;
            }

            // 检查是否为多边形项
            auto *polygonItem = qgraphicsitem_cast<AnnotationPolygonItem *>(item);
            if (polygonItem) {
                // 为了保持接口一致，需要返回AnnotationRectItem*类型
                // 这里需要特殊处理，暂时返回nullptr，后续在调用处处理多边形项
                return nullptr;
            }
        }
    }
    return nullptr;
}

QRectF AnnotationGraphicsView::graphics_annotation_rect_to_scene(const GraphicsAnnotationRect &rect) const {
    return QRectF(rect.x, rect.y, rect.width, rect.height);
}

GraphicsAnnotationRect AnnotationGraphicsView::scene_rect_to_annotation(const QRectF &rect) const {
    return {
        static_cast<int>(rect.x()),
        static_cast<int>(rect.y()),
        static_cast<int>(rect.width()),
        static_cast<int>(rect.height()),
        m_currentClassId
    };
}

QRectF AnnotationGraphicsView::get_resize_handle_rect(const QRectF &rect, int handle) const {
    qreal handleSize = qMax(25.0, 15.0 / m_scaleFactor); // 与顶点控制点大小保持一致
    QPointF center;

    switch (handle) {
        case TopLeftHandle:
            center = rect.topLeft();
            break;
        case TopRightHandle:
            center = rect.topRight();
            break;
        case BottomLeftHandle:
            center = rect.bottomLeft();
            break;
        case BottomRightHandle:
            center = rect.bottomRight();
            break;
        case TopHandle:
            center = QPointF(rect.center().x(), rect.top());
            break;
        case LeftHandle:
            center = QPointF(rect.left(), rect.center().y());
            break;
        case RightHandle:
            center = QPointF(rect.right(), rect.center().y());
            break;
        case BottomHandle:
            center = QPointF(rect.center().x(), rect.bottom());
            break;
        default:
            return {};
    }

    return {center.x() - handleSize / 2, center.y() - handleSize / 2, handleSize, handleSize};
}

QRectF AnnotationGraphicsView::get_vertex_handle_rect(const QPolygonF &polygon, int vertexIndex) const {
    if (vertexIndex < 0 || vertexIndex >= polygon.size()) {
        return {};
    }

    // 使用固定的控制点检测大小，确保在任何缩放比例下都能容易点击
    qreal handleSize = qMax(12.0, 8.0 / m_scaleFactor); // 最小12像素，确保可点击

    QPointF vertex = polygon.at(vertexIndex);
    return {vertex.x() - handleSize / 2, vertex.y() - handleSize / 2, handleSize, handleSize};
}

int AnnotationGraphicsView::get_resize_handle_at(const QPointF &pos) const {
    if (!m_selectedItem) return NoHandle;

    QRectF rect = m_selectedItem->rect();

    // 检查角落控制点
    for (int handle = TopLeftHandle; handle <= BottomRightHandle; handle++) {
        if (get_resize_handle_rect(rect, handle).contains(pos)) {
            return handle;
        }
    }

    // 检查边缘控制点
    for (int handle = TopHandle; handle <= BottomHandle; handle++) {
        if (get_resize_handle_rect(rect, handle).contains(pos)) {
            return handle;
        }
    }

    return NoHandle;
}

int AnnotationGraphicsView::get_vertex_handle_at(const QPointF &pos) const {
    // 检查是否选择了多边形
    if (m_selectedIndex < m_rectItems.size()) {
        return NoVertexHandle; // 没有选择多边形
    }

    int polygonIndex = m_selectedIndex - m_rectItems.size();
    if (polygonIndex < 0 || polygonIndex >= m_polygonItems.size()) {
        return NoVertexHandle;
    }

    AnnotationPolygonItem *polygonItem = m_polygonItems[polygonIndex];
    QPolygonF polygon = polygonItem->polygon();

    // 检查每个顶点
    for (int i = 0; i < polygon.size(); ++i) {
        if (get_vertex_handle_rect(polygon, i).contains(pos)) {
            return i;
        }
    }

    return NoVertexHandle;
}

void AnnotationGraphicsView::show_context_menu(const QPoint &pos) {
    m_contextMenu->clear();

    if (m_classes.isEmpty()) {
        QAction *action = m_contextMenu->addAction(tr("未定义类别"));
        action->setEnabled(false);
    } else {
        for (int i = 0; i < m_classes.size(); ++i) {
            QAction *action = m_contextMenu->addAction(QString("%1 (%2)").arg(m_classes.at(i)).arg(i));
            action->setData(i);

            // 检查当前选中的是否为矩形或多边形，并设置相应的选中状态
            if (m_selectedIndex >= 0) {
                if (m_selectedIndex < m_rectangles.size()) {
                    // 矩形项
                    if (m_rectangles[m_selectedIndex].classId == i) {
                        action->setCheckable(true);
                        action->setChecked(true);
                    }
                } else {
                    // 多边形项
                    int polygonIndex = m_selectedIndex - m_rectItems.size();
                    if (polygonIndex >= 0 && polygonIndex < m_polygons.size()) {
                        if (m_polygons[polygonIndex].classId == i) {
                            action->setCheckable(true);
                            action->setChecked(true);
                        }
                    }
                }
            }

            connect(action, &QAction::triggered, this, &AnnotationGraphicsView::on_action_class_changed);
        }
    }

    m_contextMenu->popup(pos);
}

void AnnotationGraphicsView::on_action_class_changed() {
    auto *action = qobject_cast<QAction *>(sender());
    if (action) {
        bool ok;
        int classId = action->data().toInt(&ok);
        if (ok) {
            change_rectangle_class(classId);
        }
    }
}

void AnnotationGraphicsView::change_rectangle_class(int classId) {
    if (m_selectedIndex >= 0 && m_selectedIndex < m_rectangles.size()) {
        // 更改矩形项的类别
        save_state();
        m_rectangles[m_selectedIndex].classId = classId;

        if (m_selectedItem) {
            m_selectedItem->setClassId(classId);
            m_scene->update(m_selectedItem->rect());
        }

        emit rectangle_class_changed(m_selectedIndex, classId);
    } else if (m_selectedIndex >= m_rectItems.size()) {
        // 更改多边形项的类别
        int polygonIndex = m_selectedIndex - m_rectItems.size();
        if (polygonIndex >= 0 && polygonIndex < m_polygons.size()) {
            save_state();
            m_polygons[polygonIndex].classId = classId;

            if (polygonIndex < m_polygonItems.size()) {
                m_polygonItems[polygonIndex]->setClassId(classId);
                m_scene->update(m_polygonItems[polygonIndex]->polygon().boundingRect());
            }

            emit rectangle_class_changed(m_selectedIndex, classId);
        }
    }
}

void AnnotationGraphicsView::set_drawing_mode(DrawingMode mode) {
    // 如果正在绘制，先取消当前绘制
    if (m_drawing) {
        if (m_drawingMode == RectangleMode && m_currentDrawingRect) {
            m_scene->removeItem(m_currentDrawingRect);
            delete m_currentDrawingRect;
            m_currentDrawingRect = nullptr;
        } else if (m_drawingMode == PolygonMode && m_currentDrawingPolygon) {
            m_scene->removeItem(m_currentDrawingPolygon);
            delete m_currentDrawingPolygon;
            m_currentDrawingPolygon = nullptr;
        }
        m_drawing = false;
    }

    m_drawingMode = mode;
    m_currentPolygonPoints.clear();
}

void AnnotationGraphicsView::finish_polygon_drawing() {
    if (m_drawingMode == PolygonMode && m_drawing && m_currentPolygonPoints.size() >= 3) {
        save_state();

        // 创建多边形注释
        GraphicsAnnotationPolygon polygonAnnotation(m_currentPolygonPoints, m_currentClassId);
        m_polygons.append(polygonAnnotation);

        // 更新图形项
        update_rect_items();
    }

    // 清理当前绘制状态
    if (m_currentDrawingPolygon) {
        m_scene->removeItem(m_currentDrawingPolygon);
        delete m_currentDrawingPolygon;
        m_currentDrawingPolygon = nullptr;
    }

    m_drawing = false;
    m_currentPolygonPoints.clear();
}
