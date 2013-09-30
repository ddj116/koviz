#include "trickcurve.h"
#include "timeit_linux.h"

TrickCurve::TrickCurve(QCPAxis *keyAxis, QCPAxis *valueAxis) :
    QCPAbstractPlottable(keyAxis, valueAxis) ,
    _model(0),_valueScaleFactor(1.0),
    _isXRangeCalculated(false),_isYRangeCalculated(false)
{
    setAntialiased(false);
    mPen.setColor(QColor(50, 100, 212));
    mPen.setStyle(Qt::SolidLine);
    mPen.setWidthF(0);
    mBrush.setColor(QColor(50, 100, 212));
    mBrush.setStyle(Qt::NoBrush);
    mSelectedPen = mPen;
    mSelectedPen.setWidthF(0);
    mSelectedPen.setColor(QColor(80, 80, 255)); // lighter than Qt::blue of mPen
    mSelectedBrush = mBrush;

    setLineStyle(lsLine);
}

TrickCurve::~TrickCurve()
{ }

void TrickCurve::setLineStyle(TrickCurve::LineStyle style)
{
    mLineStyle = style;
}

double TrickCurve::selectTest(const QPointF &pos,
                            bool onlySelectable, QVariant *details) const
{
    Q_UNUSED(details)
    if ((onlySelectable && !mSelectable) || _model->rowCount() > 0 )
        return -1;

    return pointDistance(pos);
}

void TrickCurve::setData(TrickModel *model, int tcol, int xcol, int ycol)
{
   _model = model;
   setName(model->headerData(ycol,Qt::Horizontal,Param::Name).toString());
   _tcol = tcol;
   _xcol = xcol;
   _ycol = ycol;
}

void TrickCurve::draw(QCPPainter *painter)
{
    if (_model == 0 || _model->rowCount() == 0 ) return;

    // allocate line vector:
    QVector<QPointF> *lineData = new QVector<QPointF>;

    // fill with curve data:
    getCurveData(lineData);

    // draw curve fill:
    if (mainBrush().style() != Qt::NoBrush && mainBrush().color().alpha() != 0)
    {
        applyFillAntialiasingHint(painter);
        painter->setPen(Qt::NoPen);
        painter->setBrush(mainBrush());
        painter->drawPolygon(QPolygonF(*lineData));
    }

    // draw curve line:
    if (mLineStyle != lsNone &&
        mainPen().style() != Qt::NoPen && mainPen().color().alpha() != 0)
    {
        applyDefaultAntialiasingHint(painter);
        painter->setPen(mainPen());
        painter->setBrush(Qt::NoBrush);
        // if drawing solid line and not in PDF,
        // use much faster line drawing instead of polyline:
        if (mParentPlot->plottingHints().testFlag(QCP::phFastPolylines) &&
                painter->pen().style() == Qt::SolidLine &&
                !painter->modes().testFlag(QCPPainter::pmVectorized) &&
                !painter->modes().testFlag(QCPPainter::pmNoCaching))
        {
            for (int i=1; i<lineData->size(); ++i)
                painter->drawLine(lineData->at(i-1), lineData->at(i));
        } else
        {
            painter->drawPolyline(QPolygonF(*lineData));
        }
    }

    // free allocated line data:
    delete lineData;
}

/* inherits documentation from base class */
void TrickCurve::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{
    // draw fill:
    if (mBrush.style() != Qt::NoBrush)
    {
        applyFillAntialiasingHint(painter);
        painter->fillRect(QRectF(rect.left(),
                                 rect.top()+rect.height()/2.0,
                                 rect.width(), rect.height()/3.0), mBrush);
    }
    // draw line vertically centered:
    if (mLineStyle != lsNone)
    {
        applyDefaultAntialiasingHint(painter);
        painter->setPen(mPen);
        // +5 on x2 else last segment is missing from dashed/dotted pens
        painter->drawLine(QLineF(rect.left(),
                                 rect.top()+rect.height()/2.0,
                                 rect.right()+5,
                                 rect.top()+rect.height()/2.0));
    }
}

/*
  called by TrickCurve::draw to generate a point vector (pixels) which represents
  the line of the curve. Line segments that aren't visible in the
  current axis rect are handled in an optimized
  way.
*/
void TrickCurve::getCurveData(QVector<QPointF> *lineData) const
{
    /* Extended sides of axis rect R divide space into 9 regions:
     1__|_4_|__7
     2__|_R_|__8
     3  | 6 |  9
     General idea: If the two points of a line segment are in the same region
     (that is not R), the line segment corner is removed. Curves outside R
     become straight lines closely outside of R which greatly reduces drawing
     time, yet keeps the look of lines and fills inside R consistent. The region
     R has index 5.
  */
    QCPAxis *keyAxis = mKeyAxis.data();
    QCPAxis *valueAxis = mValueAxis.data();
    if (!keyAxis || !valueAxis) {
        qDebug() << Q_FUNC_INFO << "invalid key or value axis"; return;
    }

    QRect axisRect = mKeyAxis.data()->axisRect()->rect() &
                     mValueAxis.data()->axisRect()->rect();
    lineData->reserve(_model->rowCount());
    TrickModelIterator it(_valueScaleFactor);
    int lastRegion = 5;
    int currentRegion = 5;
    double RLeft = keyAxis->range().lower;
    double RRight = keyAxis->range().upper;
    double RBottom = valueAxis->range().lower;
    double RTop = valueAxis->range().upper;
    double x, y; // current key/value
    bool addedLastAlready = true;
    bool firstPoint = true; // first point must always be drawn,
                           // to make sure fill works correctly

    const TrickModelIterator e = _model->end(_tcol,_xcol,_ycol);
    for (it = _model->begin(_tcol,_xcol,_ycol); it != e; ++it) {

        x = it.x();
        y = it.y();

        // determine current region:
        if (x < RLeft) // region 123
        {
            if (y > RTop)
                currentRegion = 1;
            else if (y < RBottom)
                currentRegion = 3;
            else
                currentRegion = 2;
        } else if (x > RRight) // region 789
        {
            if (y > RTop)
                currentRegion = 7;
            else if (y < RBottom)
                currentRegion = 9;
            else
                currentRegion = 8;
        } else // region 456
        {
            if (y > RTop)
                currentRegion = 4;
            else if (y < RBottom)
                currentRegion = 6;
            else
                currentRegion = 5;
        }

        // Watch out, the next part is very tricky. It modifies the curve such
        // that it seems like the whole thing is still drawn, but actually the
        // points outside the axisRect are simplified ("optimized") greatly.
        // There are some subtle special cases when line segments are large and
        // thereby each subsequent point may be in a different region or even
        // skip some.

        // determine whether to keep current point:
        if (currentRegion == 5 || (firstPoint && mBrush.style() != Qt::NoBrush)) {
           // current is in R, add current and last if it wasn't added already
            if (!addedLastAlready) {
                // in case curve just entered R,
                // make sure the last point outside R is also drawn correctly
                lineData->append(coordsToPixels((it-1).x(),(it-1).y()));

            } else if (lastRegion != 5) {
                // added last already. If that's the case,
                // we probably added it at optimized position. So go back and
                // make sure it's at original position (else the angle changes
                // under which this segment enters R)
                if (!firstPoint) {
                    // because on firstPoint, currentRegion is 5 and
                    // addedLastAlready is true, although there is no last point
                    lineData->replace(lineData->size()-1,
                                      coordsToPixels((it-1).x(),(it-1).y()));
                }
            }
            // add current point to vector
            lineData->append(coordsToPixels(it.x(), it.y()));
            addedLastAlready = true; // so in next iteration, we don't add
                                     // this point twice

        } else if (currentRegion != lastRegion) {

            // changed region, add current
            // and last if not added already

            // using outsideCoordsToPixels instead of coorsToPixels
            // for optimized point placement (places points just outside axisRect
            // instead of potentially far away)

            // if we're coming from R or we skip diagonally over
            // the corner regions (so line might still be visible in R),
            // we can't place points optimized
            if (lastRegion == 5 || // coming from R
                    ((lastRegion==2 && currentRegion==4) ||
                     (lastRegion==4 && currentRegion==2)) || // skip tleft diag
                    ((lastRegion==4 && currentRegion==8) ||
                     (lastRegion==8 && currentRegion==4)) || // skip tright diag
                    ((lastRegion==8 && currentRegion==6) ||
                     (lastRegion==6 && currentRegion==8)) || // skip bright diag
                    ((lastRegion==6 && currentRegion==2) ||
                     (lastRegion==2 && currentRegion==6))    // skip bleft diag
                    )
            {
                // always add last point if not added already, original:
                if (!addedLastAlready)
                    lineData->append(coordsToPixels((it-1).x(),(it-1).y()));
                // add current point, original:
                lineData->append(coordsToPixels(it.x(),it.y()));
            } else {
                // no special case that forbids optimized point placement,
                // so do it:

                // always add last point if not added already, optimized:
                if (!addedLastAlready) {
                    lineData->append(
                        outsideCoordsToPixels((it-1).x(), (it-1).y(),
                                              currentRegion, axisRect));
                }
                // add current point, optimized:
                lineData->append(
                        outsideCoordsToPixels(it.x(),it.y(),
                                              currentRegion, axisRect));
            }
            // so that if next point enters 5, or crosses another
            // region boundary, we don't add this point twice
            addedLastAlready = true;
        } else // neither in R, nor crossed a region boundary, skip current point
        {
            addedLastAlready = false;
        }
        lastRegion = currentRegion;
        firstPoint = false;
    }
    // If curve ends outside R, we want to add very last
    // point so the fill looks like it should when the curve started inside R:
    if (lastRegion != 5 && mBrush.style() != Qt::NoBrush &&
        _model->rowCount()>0) {

        lineData->append(coordsToPixels((_model->end(_tcol,_xcol,_ycol)-1).x(),
                                       (_model->end(_tcol,_xcol,_ycol)-1).y()));
    }

}

// Calculates the (minimum) distance (in pixels) the curve's representation
// has from the given pixelPoint in pixels. This is used to determine whether
// the curve was clicked or not, e.g. in selectTest.
double TrickCurve::pointDistance(const QPointF &pixelPoint) const
{
    if (_model->rowCount() == 0) {
        qDebug() << Q_FUNC_INFO
                 << "requested point distance on curve"
                 << mName << "without data";
        return 500;
    }
    if (_model->rowCount() == 1) {
        QPointF dataPoint = coordsToPixels(_model->begin(_tcol,_xcol,_ycol).x(),
                                          _model->begin(_tcol,_xcol,_ycol).y());
        return QVector2D(dataPoint-pixelPoint).length();
    }

    // calculate minimum distance to line segments:
    QVector<QPointF> *lineData = new QVector<QPointF>;
    getCurveData(lineData);
    double minDistSqr = std::numeric_limits<double>::max();
    for (int i=0; i<lineData->size()-1; ++i)
    {
        double currentDistSqr = distSqrToLine(lineData->at(i),
                                              lineData->at(i+1), pixelPoint);
        if (currentDistSqr < minDistSqr)
            minDistSqr = currentDistSqr;
    }
    delete lineData;
    return sqrt(minDistSqr);
}



// This is a specialized coordsToPixels function for points that
// are outside the visible axisRect and just crossing a boundary (since
// getCurveData reduces non-visible curve segments to those line segments
// that cross region boundaries, see documentation there). It only uses the
// coordinate parallel to the region boundary of the axisRect. The other
// coordinate is picked just outside the axisRect (how far is determined
// by the scatter size and the line width). Together with the optimization
// in getCurveData this improves performance for large curves (or zoomed
// in ones) significantly while keeping the illusion the whole curve and its
// filling is still being drawn for the viewer.
QPointF TrickCurve::outsideCoordsToPixels(double key, double value, int region,
                                       QRect axisRect) const
{
    int margin = qCeil((double)mPen.widthF()) + 2;
    QPointF result = coordsToPixels(key, value);
    switch (region)
    {
    case 2: result.setX(axisRect.left()-margin); break; // left
    case 8: result.setX(axisRect.right()+margin); break; // right
    case 4: result.setY(axisRect.top()-margin); break; // top
    case 6: result.setY(axisRect.bottom()+margin); break; // bottom
    case 1: result.setX(axisRect.left()-margin);
        result.setY(axisRect.top()-margin); break; // top left
    case 7: result.setX(axisRect.right()+margin);
        result.setY(axisRect.top()-margin); break; // top right
    case 9: result.setX(axisRect.right()+margin);
        result.setY(axisRect.bottom()+margin); break; // bottom right
    case 3: result.setX(axisRect.left()-margin);
        result.setY(axisRect.bottom()+margin); break; // bottom left
    }
    return result;
}

//
// Since getKeyRange() is const, can't do the caching thing without a headache
//
QCPRange TrickCurve::xRange(bool &validRange, SignDomain inSignDomain)
{
    if ( ! _isXRangeCalculated ) {
        _xrange = getKeyRange(validRange,inSignDomain);
        _isXRangeCalculated = true;
    }
    return _xrange;
}

//
// Since getValueRange() is const, can't do the caching thing without a headache
//
QCPRange TrickCurve::yRange(bool &validRange, SignDomain inSignDomain)
{
    if ( ! _isYRangeCalculated ) {
        _yrange = getValueRange(validRange,inSignDomain);
        _isYRangeCalculated = true;
    }
    return _yrange;
}

/* inherits documentation from base class */
QCPRange TrickCurve::getKeyRange(bool &validRange, SignDomain inSignDomain) const
{
    QCPRange range;
    bool haveLower = false;
    bool haveUpper = false;

    double current;

    TrickModelIterator it = _model->begin(_tcol,_xcol,_ycol);
    const TrickModelIterator e = _model->end(_tcol,_xcol,_ycol);
    while (it != e) {
        current = it.x();
        if (inSignDomain == sdBoth ||
            (inSignDomain == sdNegative && current < 0) ||
            (inSignDomain == sdPositive && current > 0)) {

            if (current < range.lower || !haveLower) {
                range.lower = current;
                haveLower = true;
            }
            if (current > range.upper || !haveUpper) {
                range.upper = current;
                haveUpper = true;
            }
        }
        ++it;
    }

    validRange = haveLower && haveUpper;
    return range;
}

/* inherits documentation from base class */
QCPRange TrickCurve::getValueRange(bool &validRange, SignDomain inSignDomain) const
{
    QCPRange range;
    bool haveLower = false;
    bool haveUpper = false;

    double current;

    TrickModelIterator it = _model->begin(_tcol,_xcol,_ycol);
    it.setValueScaleFactor(_valueScaleFactor);
    const TrickModelIterator e = _model->end(_tcol,_xcol,_ycol);
    while (it != e) {
        current = it.y();
        if (inSignDomain == sdBoth ||
           (inSignDomain == sdNegative && current < 0) ||
                (inSignDomain == sdPositive && current > 0)) {

            if (current < range.lower || !haveLower) {
                range.lower = current;
                haveLower = true;
            } if (current > range.upper || !haveUpper)
            {
                range.upper = current;
                haveUpper = true;
            }
        }
        ++it;
    }

    validRange = haveLower && haveUpper;
    return range;
}