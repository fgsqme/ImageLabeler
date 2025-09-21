#ifndef ANNOTATIONGRAPHICSVIEW_H
#define ANNOTATIONGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QList>
#include <QMenu>

// 图形注释矩形结构体
struct GraphicsAnnotationRect {
    int x, y, width, height, classId;
    GraphicsAnnotationRect(int x, int y, int w, int h, int id) : x(x), y(y), width(w), height(h), classId(id) {}
};

// 添加多边形注释结构
struct GraphicsAnnotationPolygon {
    QVector<QPointF> points;
    int classId;
    GraphicsAnnotationPolygon(const QVector<QPointF>& pts, int id) : points(pts), classId(id) {}
};

// 注释矩形项类
class AnnotationRectItem : public QGraphicsRectItem {
public:
    AnnotationRectItem(int classId, QGraphicsItem *parent = nullptr);

    int classId() const;
    void setClassId(int classId);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;

private:
    int m_classId;
};

// 添加多边形图形项类
class AnnotationPolygonItem : public QGraphicsPolygonItem {
public:
    AnnotationPolygonItem(int classId, QGraphicsItem *parent = nullptr);

    int classId() const;
    void setClassId(int classId);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    QRectF boundingRect() const override;

private:
    int m_classId;
};

// 添加枚举类型表示当前绘制模式
enum DrawingMode {
    RectangleMode,  // 矩形模式
    PolygonMode     // 多边形模式
};

// 注释图形视图类
class AnnotationGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit AnnotationGraphicsView(QWidget *parent = nullptr);
    void load_image(const QString &imagePath);
    void save_annotations(const QString &imagePath);
    void clear();
    void set_current_class(int classId);
    void set_classes(const QStringList &class_list);
    void undo();
    void redo();
    int get_selected_rectangle_index() const;
    int get_rectangle_count() const;
    void zoom_in();
    void zoom_out();
    void reset_view();
    double get_scale_factor() const;
    void delete_selected_rectangle();
    QStringList get_classes() const; // 添加此方法

    // 添加多边形相关方法
    void set_drawing_mode(DrawingMode mode);
    void finish_polygon_drawing();

signals:
    void rectangle_drawn(QRect rect);          // 矩形绘制信号
    void mouse_moved(QPoint pos);              // 鼠标移动信号
    void rectangle_selected(int index);        // 矩形选中信号
    void rectangle_class_changed(int index, int classId);  // 矩形类别更改信号
    void scale_changed(double scale);          // 添加缩放变化信号

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void on_action_class_changed();

private:
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_pixmapItem;
    QList<AnnotationRectItem*> m_rectItems;
    QList<GraphicsAnnotationRect> m_rectangles;

    // 添加多边形相关成员
    QList<AnnotationPolygonItem*> m_polygonItems;
    QList<GraphicsAnnotationPolygon> m_polygons;

    QStringList m_classes;
    int m_currentClassId;
    double m_scaleFactor;

    // 绘制状态
    bool m_drawing;
    QPointF m_startPoint;
    QPointF m_endPoint;
    QGraphicsRectItem *m_currentDrawingRect;

    // 添加多边形绘制状态
    DrawingMode m_drawingMode;
    QGraphicsPolygonItem *m_currentDrawingPolygon;
    QVector<QPointF> m_currentPolygonPoints;

    // 选择和操作状态
    AnnotationRectItem *m_selectedItem;
    int m_selectedIndex;
    bool m_moving;
    bool m_resizing;
    int m_resizeHandle;
    QPointF m_lastMousePos;
    QPointF m_dragStartPos;

    // 添加顶点编辑状态
    bool m_vertexEditing;
    int m_vertexEditHandle;

    // 添加多边形顶点编辑相关成员
    enum VertexHandle {
        NoVertexHandle = -1
    };

    // 添加多边形顶点编辑相关方法
    QRectF get_vertex_handle_rect(const QPolygonF &polygon, int vertexIndex) const;
    int get_vertex_handle_at(const QPointF &pos) const;

    // 撤销/重做栈
    QList<QList<GraphicsAnnotationRect>> m_undoStack;
    QList<QList<GraphicsAnnotationRect>> m_redoStack;
    
    // 添加多边形撤销/重做栈
    QList<QList<GraphicsAnnotationPolygon>> m_polygonUndoStack;
    QList<QList<GraphicsAnnotationPolygon>> m_polygonRedoStack;

    // 上下文菜单
    QMenu *m_contextMenu;

    void load_annotations(const QString &imagePath);
    void save_state();
    void update_rect_items();
    void show_context_menu(const QPoint &pos);
    void change_rectangle_class(int classId);
    AnnotationRectItem* item_at(const QPointF &pos);
    QRectF graphics_annotation_rect_to_scene(const GraphicsAnnotationRect &rect) const;
    GraphicsAnnotationRect scene_rect_to_annotation(const QRectF &rect) const;

    // 调整大小的控制点
    enum ResizeHandle {
        NoHandle = 0,
        TopLeftHandle,      // 左上控制点
        TopRightHandle,     // 右上控制点
        BottomLeftHandle,   // 左下控制点
        BottomRightHandle,  // 右下控制点
        TopHandle,          // 上方控制点
        LeftHandle,         // 左侧控制点
        RightHandle,        // 右侧控制点
        BottomHandle        // 下方控制点
    };

    QRectF get_resize_handle_rect(const QRectF &rect, int handle) const;
    int get_resize_handle_at(const QPointF &pos) const;
};

#endif // ANNOTATIONGRAPHICSVIEW_H