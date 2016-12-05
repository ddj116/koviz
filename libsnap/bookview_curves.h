#ifndef CURVESVIEW_H
#define CURVESVIEW_H

#include <QtGlobal>
#include <QVector>
#include <QPolygonF>
#include <QPainter>
#include <QPixmap>
#include <QList>
#include <QColor>
#include <QMouseEvent>
#include <QRubberBand>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QSizeF>
#include <QLineF>
#include <QItemSelectionModel>
#include <QEasingCurve>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include "bookidxview.h"
#include "libsnap/unit.h"

// ---------------------------------------------------------
// Use . since backslash makes a C++ multi-line comment
//
//                b
// (x,y)<-m->|..........Y
//                       .
//                        .  a
//                         .
//                          _|
//                            .   --------------------Z
//
//      angle(Y.Z)===angle===arrowAngle===(2k+1)*45degrees
//
// ---------------------------------------------------------
//
//            A
//            |.
//            | .
//            |  .
//            |   .
//            |    .
//            |<-h->.       Arrow Head
//            |    .
//            |   .
//            |  .
//            | .
//            |.
//            B     angle(A.B)===tipAngle===arrowTipAngle===22.5
//
// ---------------------------------------------------------
class CoordArrow
{
  public:
    CoordArrow();
    CoordArrow(
               const QString& txt,
               const QPointF& coord,
               double r, double h,
               double a, double b, double m,
               double angle, double tipAngle);

  public:
    QString txt;    // Text e.g. "(10.375,3.141593)"
    QPointF coord;  // math coord
    double r;       // radius of circle in window coords
    double h;       // height of arrow head in window coords
    double a;       // length of part1 of tail (see above)
    double b;       // length of part2 of tail (see above)
    double m;       // dist between text box and 'b'
    double angle;   // angle of arrow off of horizon
    double tipAngle; // tip of arrow angle (22.5)

    QRectF boundingBox(const QPainter &painter, const QTransform &T) const;
    QRectF txtBoundingBox(const QPainter &painter, const QTransform &T) const;
    void paintMe(QPainter &painter, const QTransform &T) const;
};

class CurvesView : public BookIdxView
{
    Q_OBJECT

public:
    explicit CurvesView(QWidget *parent = 0);
    ~CurvesView();

public:
    virtual void setCurrentCurveRunID(int runID);

protected:
    virtual void paintEvent(QPaintEvent * event);
    virtual QSize minimumSizeHint() const;
    virtual QSize sizeHint() const;
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* mouseEvent);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void currentChanged(const QModelIndex& current,
                                const QModelIndex& previous);


private:
    QPainterPath* _errorPath;

    QPainterPath _sinPath();
    QPainterPath _stepPath();

    QList<QColor> _colorBandsNormal;
    QList<QColor> _colorBandsRainbow;
    QList<QColor> _createColorBands(int nBands, bool isRainbow);

    QRectF _mousePressMathRect;
    QPoint _mousePressPos;
    QPointF _mousePressMathTopLeft;
    QModelIndex _mousePressCurrentIndex;

    QRectF _curveBBox(const QModelIndex &curveIdx) const ;

    void _paintCoplot(const QTransform& T,QPainter& painter,QPen& pen);
    void _paintErrorplot(QPainter& painter, const QPen &pen,
                         const QModelIndex &plotIdx);
    void _paintCurve(const QModelIndex& curveIdx,
                     const QTransform &T, QPainter& painter, QPen& pen);
    void _paintLiveCoordArrow(TrickCurveModel *curveModel,
                          const QModelIndex &curveIdx, QPainter &painter);
    void _paintLegend(const QModelIndex& curvesIdx, QPainter &painter);
    void __paintLegend(const QList<QPen*>& pens,
                       const QStringList& symbols,
                       const QStringList& labels,
                       QPainter& painter);
    void __paintSymbol(const QPointF &p, const QString& symbol, QPainter& painter);

    QList<QModelIndex> _curvesInsideMouseRect(const QRectF& R);

    // Key Events
    void _keyPressSpace();
    void _keyPressUp();
    void _keyPressDown();

protected slots:
    virtual void dataChanged(const QModelIndex &topLeft,
                             const QModelIndex &bottomRight);
    virtual void rowsInserted(const QModelIndex &pidx, int start, int end);


};

#endif // CURVESVIEW_H
