#ifndef VARSWIDGET_H
#define VARSWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>
#include <QGridLayout>
#include <QLineEdit>
#include <QListView>
#include <QFileInfo>
#include <QDir>
#include <QProgressDialog>
#include <float.h>
#include "dp.h"
#include "montemodel.h"
#include "bookmodel.h"
#include "monteinputsview.h"

class VarsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VarsWidget(const QString& timeName,
                        QStandardItemModel* varsModel,
                        MonteModel* monteModel,
                        PlotBookModel* plotModel,
                        QItemSelectionModel*  plotSelectModel,
                        MonteInputsView* monteInputsView,
                        QWidget *parent = 0);

    void clearSelection();


signals:
    
public slots:

private:
    QString _timeName;
    QStandardItemModel* _varsModel;
    MonteModel* _monteModel;
    PlotBookModel* _plotModel;
    QItemSelectionModel*  _plotSelectModel;
    MonteInputsView* _monteInputsView;
    QGridLayout* _gridLayout ;
    QLineEdit* _searchBox;
    QListView* _listView ;

    QSortFilterProxyModel* _varsFilterModel;
    QItemSelectionModel* _varsSelectModel;

    int _currQPIdx;

    QModelIndex _findSinglePlotPageWithCurve(const QString& curveYName);
    QStandardItem* _createPageItem();
    QStandardItem* _addChild(QStandardItem* parentItem,
                   const QString& childTitle,
                   const QVariant &childValue=QVariant());
    void _addPlotToPage(QStandardItem* pageItem,
                                 const QModelIndex &varIdx);
    void _selectCurrentRunOnPageItem(QStandardItem* pageItem);
    int _currSelectedRun();


private slots:
     void _varsSearchBoxTextChanged(const QString& rx);
     void _varsSelectModelSelectionChanged(
                              const QItemSelection& currVarSelection,
                              const QItemSelection& prevVarSelection);
};

#endif // VARSWIDGET_H