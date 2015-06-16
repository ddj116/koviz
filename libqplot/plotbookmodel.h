#ifndef PLOTBOOKMODEL_H
#define PLOTBOOKMODEL_H

#include <QStandardItemModel>
#include <QStandardItem>
#include "libsnapdata/montemodel.h"
#include "libsnaprt/utils.h"

class PlotBookModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit PlotBookModel( MonteModel* monteModel,
                            QObject *parent = 0);
    explicit PlotBookModel( MonteModel* monteModel,
                            int rows, int columns, QObject * parent = 0 );

    virtual QVariant data(const QModelIndex &idx,
                          int role = Qt::DisplayRole) const;

    QModelIndex sessionStartIdx() const ;
    QModelIndex sessionStopIdx() const ;

    bool isPageIdx(const QModelIndex& idx) const ;
    QModelIndex pageIdx(const QModelIndex& idx) const ;
    QModelIndexList pageIdxs() const ;

    QModelIndexList plotIdxs(const QModelIndex& pageIdx) const ;

    QModelIndex curvesIdx(const QModelIndex& plotIdx) const ;
    QModelIndexList curveIdxs(const QModelIndex& curvesIdx) const ;
    QModelIndex curveLineColorIdx(const QModelIndex& curveIdx) const ;
    bool isCurveLineColorIdx(const QModelIndex& idx) const;


    enum IdxEnum
    {
        Invalid,
        Page,
            SessionStartTime,
            SessionStopTime,
            PageTitle,
            PageStartTime,
            PageStopTime,
                Plot,
                    PlotXAxisLabel,
                    PlotYAxisLabel,
                    Curves,
                        Curve,
                            CurveTime,
                            CurveX,
                            CurveY,
                            CurveTimeUnit,
                            CurveXUnit,
                            CurveYUnit,
                            CurveRunID,
                            CurveData,
                            CurveLineColor,
                    PlotTitle,
                    PlotXMin,
                    PlotXMax,
                    PlotYMin,
                    PlotYMax,
                    PlotStartTime,
                    PlotStopTime,
                    PlotGrid,
                    PlotGridColor,
                    PlotBGColor
    };

    IdxEnum indexEnum(const QModelIndex& idx) const;

signals:
    
public slots:

private:
    MonteModel* _monteModel;
    void _initModel();
    
};

#endif // PLOTBOOKMODEL_H
