#ifndef BOOKIDXVIEW_H
#define BOOKIDXVIEW_H

#include <QAbstractItemView>
#include <QMouseEvent>
#include <qmath.h>
#include "bookmodel.h"
#include "unit.h"

class BookIdxView : public QAbstractItemView
{
    Q_OBJECT
public:
    explicit BookIdxView(QWidget *parent = 0);

public:
    void setCurvesView(QAbstractItemView* view);
    virtual void setCurrentCurveRunID(int runID);

public:
    virtual void setModel(QAbstractItemModel *model);
    virtual void setRootIndex(const QModelIndex &index);
    virtual QModelIndex indexAt( const QPoint& point) const;
    virtual QRect visualRect(const QModelIndex &index) const;
    virtual void scrollTo(const QModelIndex &index,
                          ScrollHint hint = EnsureVisible);

    // Necessary for ignoring events from middle mouse for rubberbanding
    // in PlotView.  If events are not ignored, the event filtering will
    // not catch mouse events.  This keeps from doing a
    // qApp->installEventFilter which would have to filter *all* app events
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);

    QTransform _coordToPixelTransform() ;

    QRectF _mathRect() ;
    QRectF _plotMathRect(const QModelIndex &plotIdx) const ;
    QModelIndex _plotMathRectIdx(const QModelIndex& plotIdx) const;

    QList<double> _majorXTics(const QModelIndex &plotIdx) const;
    QList<double> _minorXTics() const;
    QList<double> _majorYTics(const QModelIndex &plotIdx) const;
    QList<double> _minorYTics() const;
    QList<double> _calcTicSet(double aIn, double bIn,
                              double u, double n) const;
    QList<double> _calcMinorTicSet(double a, double b) const;
    bool _isEqual(double a, double b, ulong maxD=10,
                  bool isNeighborMethod=true) const;

protected:
    virtual QModelIndex moveCursor(CursorAction cursorAction,
                                   Qt::KeyboardModifiers modifiers);
    virtual int horizontalOffset() const;
    virtual int verticalOffset() const;
    virtual bool isIndexHidden(const QModelIndex &index) const;
    virtual void setSelection(const QRect &rect,
                              QItemSelectionModel::SelectionFlags command);
    virtual QRegion visualRegionForSelection(
                              const QItemSelection &selection) const;

protected:
    PlotBookModel *_bookModel() const;
    QAbstractItemView* _curvesView;

    QString _curvesXUnit(const QModelIndex& plotIdx) const;
    QString _curvesUnit(const QModelIndex& plotIdx, QChar axis) const;

protected:
    QList<QAbstractItemView*> _childViews;

signals:

public slots:

protected slots:
    virtual void dataChanged(const QModelIndex &topLeft,
                             const QModelIndex &bottomRight);
    virtual void rowsInserted(const QModelIndex &parent, int start, int end);
    //virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    //virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    //virtual void currentChanged(const QModelIndex &current, const QModelIndex &previous);


signals:

public slots:

};

#endif // BOOKIDXVIEW_H