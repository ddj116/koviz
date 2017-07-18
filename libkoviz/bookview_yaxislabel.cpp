#include "bookview_yaxislabel.h"

YAxisLabelView::YAxisLabelView(QWidget *parent) :
    BookIdxView(parent)
{
    setFrameShape(QFrame::NoFrame);
}

void YAxisLabelView::_update()
{
}

void YAxisLabelView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if ( !model() ) return;

    int vw = viewport()->width();
    int vh = viewport()->height();
    QFontMetrics fm = viewport()->fontMetrics();
    QString txt = fm.elidedText(_yAxisLabelText(), Qt::ElideLeft, vh);
    QRect bb = fm.tightBoundingRect(txt);
    int bw = bb.width();
    int bh = bb.height();

    // Draw!
    QPainter painter(viewport());
    painter.save();
    painter.translate(vw/2+bh-1,(vh+bw)/2);
    painter.rotate(270);
    painter.drawText(0,0,txt);
    painter.restore();
}

QSize YAxisLabelView::minimumSizeHint() const
{
    return sizeHint();
}

// Note: this accounts for 270 degree rotation
QSize YAxisLabelView::sizeHint() const
{
    QSize s;
    QFontMetrics fm = viewport()->fontMetrics();
    QRect bb = fm.boundingRect(_yAxisLabelText());
    s.setWidth(2*bb.height());  // 2 is arbitrary
    s.setHeight(bb.width());
    return s;
}

void YAxisLabelView::dataChanged(const QModelIndex &topLeft,
                                 const QModelIndex &bottomRight)
{
    if ( !model()) return;
    if ( topLeft.column() != 1 ) return;
    if ( topLeft != bottomRight ) return; // TODO: support multiple changes
    QModelIndex tagIdx = model()->index(topLeft.row(),0,topLeft.parent());
    QString tag = model()->data(tagIdx).toString();
    if ( tag == "PlotYAxisLabel" || tag == "CurveYUnit" ) {

        QString label;
        if ( _bookModel()->isChildIndex(rootIndex(),"Plot","PlotYAxisLabel")) {
            label = _bookModel()->getDataString(rootIndex(),
                                                "PlotYAxisLabel", "Plot");
        }

        QString unit;
        QModelIndex curvesIdx;
        if ( _bookModel()->isChildIndex(rootIndex(),"Plot","Curves") ) {
            curvesIdx = _bookModel()->getIndex(rootIndex(),"Curves","Plot");
            if ( curvesIdx.isValid() ) {
                unit = _bookModel()->getCurvesYUnit(curvesIdx);
            }
        }

        //_yAxisLabelText = label + '{' + unit + '}';

        viewport()->update();
    }
}

void YAxisLabelView::rowsInserted(const QModelIndex &pidx, int start, int end)
{
    if ( rootIndex() != pidx ) return;
    if ( !pidx.isValid() ) return;
    if ( !model()) return;

    for ( int i = start; i <= end; ++i ) {
        QModelIndex idx = model()->index(i,0,pidx);
        if ( model()->data(idx).toString() == "PlotYAxisLabel" ) {
            // TODOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO   if curve y unit changes, must change
            //idx = model()->sibling(i,1,idx);
            //_yAxisLabelText = model()->data(idx).toString();
            viewport()->update();
            break;
        }
    }
}

// Changes unit for some common conversions
//
// TODO: There is code duplication in XAxisLabelView
void YAxisLabelView::wheelEvent(QWheelEvent *e)
{
    QModelIndex curvesIdx;
    if ( _bookModel()->isChildIndex(rootIndex(),"Plot","Curves") ) {
        curvesIdx = _bookModel()->getIndex(rootIndex(),"Curves","Plot");
    }

    if ( curvesIdx.isValid() ) {
        QString unit = _bookModel()->getCurvesYUnit(curvesIdx);

        QList<QString> times;
        times << "s" << "ms" << "us" << "min" << "hr" << "day" ;
        QList<QString> lengths;
        lengths <<  "m" << "ft" << "in" << "mm" << "cm" << "km" ;
        QList<QString> angles;
        angles << "r" << "d" ;
        QList<QString> masses;
        masses << "kg" << "sl" << "lbm" << "g" ;
        QList<QString> forces;
        forces <<   "N" << "kN" << "oz" << "lbf";
        QList<QString> speeds;
        speeds << "m/s" << "cm/s" << "ft/s" ;

        QList<QList<QString> > units;
        units << times << lengths << angles << masses << forces << speeds;

        QString nextUnit = unit;
        int len = units.length();
        for (int i = 0; i < len; ++i) {
            QList<QString> unitFamily = units.at(i);
            if ( unitFamily.contains(unit) ) {
                int j = unitFamily.indexOf(unit);
                int k = 0;
                if ( e->delta() > 0 ) {
                    k = j+1;
                    if ( k >= unitFamily.length() ) {
                        k = 0;
                    }
                    nextUnit = unitFamily.at(k);
                } else {
                    k = j-1;
                    if ( k < 0 ) {
                        k = unitFamily.length()-1;
                    }
                }
                nextUnit = unitFamily.at(k);
            }
        }

        QModelIndex curvesIdx = _bookModel()->getIndex(rootIndex(),
                                                       "Curves","Plot");

        QModelIndexList curveIdxs = _bookModel()->getIndexList(curvesIdx,
                                                              "Curve","Curves");
        // Set all curves to next unit
        foreach (QModelIndex curveIdx, curveIdxs ) {
            QModelIndex yUnitIdx = _bookModel()->getDataIndex(curveIdx,
                                                         "CurveYUnit", "Curve");
            model()->setData(yUnitIdx,nextUnit);
        }

        // Recalculate and update bounding box (since unit change)
        QRectF bbox = _bookModel()->calcCurvesBBox(curvesIdx);
        _bookModel()->setPlotMathRect(bbox,rootIndex());
    }
}

QString YAxisLabelView::_yAxisLabelText() const
{
    QString label;

    if ( !model() ) return label;
    QModelIndex plotIdx = rootIndex();
    if ( !plotIdx.isValid() ) return label;
    QString plotTag = model()->data(plotIdx).toString();
    if ( plotTag != "Plot" ) return label;

    label = _bookModel()->getDataString(plotIdx,"PlotYAxisLabel");

    QModelIndex curvesIdx = _bookModel()->getIndex(plotIdx,"Curves","Plot");
    QString unit = _bookModel()->getCurvesYUnit(curvesIdx);

    label = label + " {" + unit + "}";

    return label;
}