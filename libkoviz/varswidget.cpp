#include "varswidget.h"

#ifdef __linux
#include "timeit_linux.h"
#endif

VarsWidget::VarsWidget(const QString &timeName,
                       QStandardItemModel* varsModel,
                       const QStringList& runDirs,
                       const QStringList &unitOverrides,
                       PlotBookModel *plotModel,
                       QItemSelectionModel *plotSelectModel,
                       MonteInputsView *monteInputsView,
                       QWidget *parent) :
    QWidget(parent),
    _timeName(timeName),
    _varsModel(varsModel),
    _runDirs(runDirs),
    _unitOverrides(unitOverrides),
    _plotModel(plotModel),
    _plotSelectModel(plotSelectModel),
    _monteInputsView(monteInputsView),
    _qpId(0)
{
    // Setup models
    _varsFilterModel = new QSortFilterProxyModel;
    _varsFilterModel->setDynamicSortFilter(true);
    _varsFilterModel->setSourceModel(_varsModel);
    QRegExp rx(QString(".*"));
    _varsFilterModel->setFilterRegExp(rx);
    _varsFilterModel->setFilterKeyColumn(0);
    _varsSelectModel = new QItemSelectionModel(_varsFilterModel);

    // Search box
    _gridLayout = new QGridLayout(parent);
    _searchBox = new QLineEdit(parent);
    connect(_searchBox,SIGNAL(textChanged(QString)),
            this,SLOT(_varsSearchBoxTextChanged(QString)));
    _gridLayout->addWidget(_searchBox,0,0);

    // Vars list view
    _listView = new QListView(parent);
    _listView->setModel(_varsFilterModel);
    _gridLayout->addWidget(_listView,1,0);
    _listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    _listView->setSelectionModel(_varsSelectModel);
    _listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _listView->setFocusPolicy(Qt::ClickFocus);
    connect(_varsSelectModel,
         SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
         this,
         SLOT(_varsSelectModelSelectionChanged(QItemSelection,QItemSelection)));
}

VarsWidget::~VarsWidget()
{
    if ( _varsSelectModel ) {
        delete _varsSelectModel;
    }
    if ( _varsFilterModel ) {
        delete _varsFilterModel;
    }
}


void VarsWidget::_varsSelectModelSelectionChanged(
                                const QItemSelection &currVarSelection,
                                const QItemSelection &prevVarSelection)
{
    Q_UNUSED(prevVarSelection); // TODO: handle deselection (prevSelection)

    if ( currVarSelection.size() == 0 ) return;

    QModelIndexList selIdxs = _varsSelectModel->selection().indexes();

    QModelIndex currIdx = _plotSelectModel->currentIndex();
    Qt::KeyboardModifiers keymods = QApplication::keyboardModifiers();
    if ( keymods & Qt::AltModifier && currIdx.isValid() ) {
        // Add variable to last plot on current page
        QModelIndex pageIdx = _plotModel->getIndex(currIdx, "Page");
        QModelIndex plotsIdx = _plotModel->getIndex(pageIdx,"Plots","Page");
        int nplots = _plotModel->rowCount(plotsIdx);
        QModelIndex plotIdx = _plotModel->index(nplots-1,0,plotsIdx);
        QModelIndex curvesIdx = _plotModel->getIndex(plotIdx,
                                                     "Curves", "Plot");
        QModelIndexList currVarIdxs = currVarSelection.indexes();
        foreach (QModelIndex varIdx, currVarIdxs) {
            QString yName = _varsFilterModel->data(varIdx).toString();
            _addCurves(curvesIdx,yName);
        }

        // Set y axis label to empty string when adding multivars to plot
        QModelIndex yAxisLabelIdx = _plotModel->getDataIndex(plotIdx,
                                                     "PlotYAxisLabel","Plot");
        _plotModel->setData(yAxisLabelIdx, "");

    } else {

        // Add variable to a new plot
        QModelIndex pageIdx; // for new or selected qp page

        if ( selIdxs.size() == 1 ) { // Single selection

            QString yName = _varsFilterModel->data(selIdxs.at(0)).toString();
            pageIdx = _findSinglePlotPageWithCurve(yName) ;

            if ( ! pageIdx.isValid() ) {
                // No page w/ single plot of selected var,so create plot of var
                QStandardItem* pageItem = _createPageItem();
                _addPlotToPage(pageItem,currVarSelection.indexes().at(0));
                pageIdx = _plotModel->indexFromItem(pageItem);
                _plotSelectModel->setCurrentIndex(pageIdx,
                                                  QItemSelectionModel::Current);
                //_selectCurrentRunOnPageItem(pageItem);
            } else {
                _plotSelectModel->setCurrentIndex(pageIdx,
                                                 QItemSelectionModel::NoUpdate);
            }

        } else {  // Multiple items selected (make pages of 6 plots per page)
            QModelIndex currIdx = _plotSelectModel->currentIndex();
            if ( !currIdx.isValid() ) {
                QStandardItem* pageItem = _createPageItem();
                pageIdx = _plotModel->indexFromItem(pageItem);
            } else {
                pageIdx = _plotModel->getIndex(currIdx, "Page");
            }
            QStandardItem* pageItem = _plotModel->itemFromIndex(pageIdx);
            QModelIndexList currVarIdxs = currVarSelection.indexes();
            int rc = currVarIdxs.size();
            QProgressDialog progress("Loading curves...", "Abort", 0, rc, this);
            progress.setWindowModality(Qt::WindowModal);
            progress.setMinimumDuration(500);
#ifdef __linux
            TimeItLinux timer;
            timer.start();
#endif
            int r = 0;
            while ( ! currVarIdxs.isEmpty() ) {
                // Update progress dialog
                progress.setValue(r++);
                if (progress.wasCanceled()) {
                    break;
                }
                QModelIndex varIdx = currVarIdxs.takeFirst();
                int nPlots = _plotModel->plotIdxs(pageIdx).size();
                if ( nPlots == 6 ) {
                    pageItem = _createPageItem();
                    pageIdx = _plotModel->indexFromItem(pageItem);
                }
                _addPlotToPage(pageItem,varIdx);
                _plotSelectModel->setCurrentIndex(pageIdx,
                                                  QItemSelectionModel::Current);
                //_selectCurrentRunOnPageItem(pageItem);
#ifdef __linux
                int secs = qRound(timer.stop()/1000000.0);
                div_t d = div(secs,60);
                QString msg = QString("Loaded %1 of %2 curves (%3 min %4 sec)")
                             .arg(r+1).arg(rc).arg(d.quot).arg(d.rem);
                progress.setLabelText(msg);
#endif
            }
            progress.setValue(rc);
        }
    }
}

void VarsWidget::_varsSearchBoxTextChanged(const QString &rx)
{
    _varsFilterModel->setFilterRegExp(rx);
}

QModelIndex VarsWidget::_findSinglePlotPageWithCurve(const QString& curveYName)
{
    QModelIndex retPageIdx;

    bool isExists = false;
    foreach ( QModelIndex pageIdx, _plotModel->pageIdxs() ) {
        foreach ( QModelIndex plotIdx, _plotModel->plotIdxs(pageIdx) ) {
            QModelIndex curvesIdx = _plotModel->getIndex(plotIdx,
                                                         "Curves", "Plot");
            isExists = true;
            foreach ( QModelIndex curveIdx, _plotModel->curveIdxs(curvesIdx) ) {
                QModelIndex yIdx =  _plotModel->getDataIndex(curveIdx,
                                                         "CurveYName", "Curve");
                QString yName =  _plotModel->data(yIdx).toString();
                isExists = isExists && (yName == curveYName);
            }
            if ( isExists ) {
                retPageIdx = pageIdx;
                break;
            }
            if (isExists) break;
        }
        if (isExists) break;
    }

    return retPageIdx;
}

void VarsWidget::clearSelection()
{
    _varsSelectModel->clear();
}

void VarsWidget::selectAllVars()
{
    _listView->selectAll();
}

QStandardItem* VarsWidget::_createPageItem()
{
    QModelIndex pagesIdx = _plotModel->getIndex(QModelIndex(), "Pages");
    QStandardItem* pagesItem = _plotModel->itemFromIndex(pagesIdx);
    QStandardItem* pageItem = _addChild(pagesItem, "Page");

    QString fg = _plotModel->getDataString(QModelIndex(),"ForegroundColor");
    if ( fg.isEmpty() ) {
        fg = "#000000";
    }
    QString bg = _plotModel->getDataString(QModelIndex(),"BackgroundColor");
    if ( bg.isEmpty() ) {
        bg = "#FFFFFF";
    }

    QString pageName = QString("QP_%0:qp.page.%0").arg(_qpId++);
    _addChild(pageItem, "PageName", pageName);
    _addChild(pageItem, "PageTitle", "Koviz");
    _addChild(pageItem, "PageStartTime", -DBL_MAX);
    _addChild(pageItem, "PageStopTime",   DBL_MAX);
    _addChild(pageItem, "PageBackgroundColor", bg);
    _addChild(pageItem, "PageForegroundColor", fg);
    _addChild(pageItem, "Plots");

    return pageItem;
}

void VarsWidget::_addPlotToPage(QStandardItem* pageItem,
                                const QModelIndex &varIdx)
{
    QModelIndex pageIdx = _plotModel->indexFromItem(pageItem);
    QModelIndex plotsIdx = _plotModel->getIndex(pageIdx, "Plots", "Page");
    QStandardItem* plotsItem = _plotModel->itemFromIndex(plotsIdx);
    QStandardItem* plotItem = _addChild(plotsItem, "Plot");

    QString yName = _varsFilterModel->data(varIdx).toString();

    int plotId = plotItem->row();
    QString plotName = QString("qp.plot.%0").arg(plotId);

    _addChild(plotItem, "PlotName", plotName);
    _addChild(plotItem, "PlotTitle", "");
    _addChild(plotItem, "PlotMathRect", QRectF());
    _addChild(plotItem, "PlotStartTime", -DBL_MAX);
    _addChild(plotItem, "PlotStopTime",   DBL_MAX);
    _addChild(plotItem, "PlotGrid", true);
    _addChild(plotItem, "PlotRatio", "");
    _addChild(plotItem, "PlotXScale", "linear");
    _addChild(plotItem, "PlotYScale", "linear");
    _addChild(plotItem, "PlotXMinRange", -DBL_MAX);
    _addChild(plotItem, "PlotXMaxRange",  DBL_MAX);
    _addChild(plotItem, "PlotYMinRange", -DBL_MAX);
    _addChild(plotItem, "PlotYMaxRange",  DBL_MAX);
    _addChild(plotItem, "PlotBackgroundColor", "#FFFFFF");
    _addChild(plotItem, "PlotForegroundColor", "#000000");
    int rc = _runDirs.count(); // a curve per run, so, rc == nCurves
    if ( rc == 2 ) {
        QString presentation = _plotModel->getDataString(QModelIndex(),
                                                         "Presentation");
        _addChild(plotItem, "PlotPresentation", presentation);
    } else {
        _addChild(plotItem, "PlotPresentation", "compare");
    }
    _addChild(plotItem, "PlotXAxisLabel", _timeName);
    _addChild(plotItem, "PlotYAxisLabel", yName);
    _addChild(plotItem, "PlotRect", QRect(0,0,0,0));

    QStandardItem *curvesItem = _addChild(plotItem,"Curves");
    QModelIndex curvesIdx = _plotModel->indexFromItem(curvesItem);
    _addCurves(curvesIdx,yName);

    // Reset monte carlo input view current idx to signal current changed
    int currRunId = -1;
    if ( _monteInputsView ) {
        currRunId = _monteInputsView->currentRun();
    }

    if ( currRunId >= 0 ) {
        foreach (QModelIndex curveIdx, _plotModel->curveIdxs(curvesIdx)) {
            int curveRunId = _plotModel->getDataInt(curveIdx,
                                                    "CurveRunID","Curve");
            if ( curveRunId == currRunId ) {
                // Reset monte input view's current index which will set
                // plot view's current index (by way of signal/slot connections)
                QModelIndex currIdx = _monteInputsView->currentIndex();
                _monteInputsView->setCurrentIndex(QModelIndex());
                _monteInputsView->setCurrentIndex(currIdx);
                break;
            }
        }
    }
}

QStandardItem* VarsWidget::_addChild(QStandardItem *parentItem,
                             const QString &childTitle,
                             const QVariant& childValue)
{
    return(_plotModel->addChild(parentItem,childTitle,childValue));
}

// Search for yparam with a *single plot* on a page with curve
// This is really a hackish helper for _varsSelectModelSelectionChanged()
void VarsWidget::_selectCurrentRunOnPageItem(QStandardItem* pageItem)
{
    int runId = -1;
    if ( _monteInputsView ) {
        runId = _monteInputsView->currentRun();
    }

    if ( runId >= 0 ) {
        QItemSelection currSel = _plotSelectModel->selection();
        QModelIndex pageIdx = _plotModel->indexFromItem(pageItem);
        foreach ( QModelIndex plotIdx, _plotModel->plotIdxs(pageIdx) ) {
            QModelIndex curvesIdx = _plotModel->getIndex(plotIdx,
                                                         "Curves", "Plot");
            QModelIndex curveIdx = _plotModel->curveIdxs(curvesIdx).at(runId);
            if ( ! currSel.contains(curveIdx) ) {
                QItemSelection curveSel(curveIdx,curveIdx) ;
                _plotSelectModel->select(curveSel,QItemSelectionModel::Select);
            }
        }
    }
}

void VarsWidget::_addCurves(QModelIndex curvesIdx, const QString &yName)
{
    // Turn off model signals when adding children for significant speedup
    bool block = _plotModel->blockSignals(true);

    int rc = _runDirs.count();
    QList<QColor> colors = _plotModel->createCurveColors(rc);

    QHash<int,QString> run2color;
    for ( int r = 0; r < rc; ++r ) {
        QModelIndex runIdx = _monteInputsView->model()->index(r,0);
        int runId = _monteInputsView->model()->data(runIdx).toInt();
        run2color.insert(runId, colors.at(r).name());
    }

    // Setup progress bar dialog for time intensive loads
    QProgressDialog progress("Loading curves...", "Abort", 0, rc, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);

#ifdef __linux
    TimeItLinux timer;
    timer.start();
#endif

    bool isGroups = false;
    QStringList groups;
    QModelIndex groupsIdx = _plotModel->getIndex(QModelIndex(),"Groups","");
    int nGroups = _plotModel->rowCount(groupsIdx);
    for ( int i = 0; i < nGroups; ++i ) {
        QModelIndex groupIdx = _plotModel->index(i,1,groupsIdx);
        QString group = _plotModel->data(groupIdx).toString();
        groups << group;
        if ( !group.isEmpty() ) {
            isGroups = true;
        }
    }

    int nCurves = _plotModel->rowCount(curvesIdx);
    int ii = nCurves; // When alt+click adding curves, this is needed
    int jj = 0;
    QString u0;
    QString r0;
    for ( int r = 0; r < rc; ++r) {

        // Update progress dialog
        progress.setValue(r);
        if (progress.wasCanceled()) {
            break;
        }

        //
        // Create curves
        //
        CurveModel* curveModel = _plotModel->createCurve(r,_timeName,
                                                         _timeName,yName);
        if ( !curveModel ) {
            // This should not happen
            // It could be ignored but I'll exit(-1) because I think
            // if this happens it's a programming error, not a user error
            fprintf(stderr, "koviz [bad scoobs]: varswidget.cpp\n"
                            "curve(%d,%s,%s,%s) failed.  Aborting!!!\n",
                    r,
                    _timeName.toLatin1().constData(),
                    _timeName.toLatin1().constData(),
                    yName.toLatin1().constData());
            exit(-1);
        }


        QStandardItem* curvesItem = _plotModel->itemFromIndex(curvesIdx);

        QStandardItem *curveItem = _addChild(curvesItem,"Curve");

        _addChild(curveItem, "CurveRunID", r);
        _addChild(curveItem, "CurveTimeName", _timeName);
        _addChild(curveItem, "CurveTimeUnit", curveModel->t()->unit());
        _addChild(curveItem, "CurveXName", _timeName);
        _addChild(curveItem, "CurveXUnit", curveModel->t()->unit()); // yes,t
        _addChild(curveItem, "CurveYName", yName);
        _addChild(curveItem, "CurveXMinRange", -DBL_MAX);
        _addChild(curveItem, "CurveXMaxRange",  DBL_MAX);
        _addChild(curveItem, "CurveYMinRange", -DBL_MAX);
        _addChild(curveItem, "CurveYMaxRange",  DBL_MAX);
        _addChild(curveItem, "CurveSymbolSize", "");

        QString yunit;
        if ( r == 0 ) {
            if ( _plotModel->rowCount(curvesIdx) > 1 ) {
                // Multiple vars on plot with single run
                QModelIndex curveIdx0 = _plotModel->getIndex(curvesIdx,"Curve",
                                                             "Curves");
                u0 = _plotModel->getDataString(curveIdx0,"CurveYUnit","Curve");
                QString u1 = curveModel->y()->unit();
                if ( Unit::canConvert(u0,u1) ) {
                    yunit = u0;
                } else {
                    yunit = u1;
                }
            } else {
                u0 = curveModel->y()->unit();
                yunit = u0;
            }
            r0 = QFileInfo(curveModel->fileName()).dir().dirName();
        } else {
            yunit = curveModel->y()->unit();
            QString u1 = yunit;
            if ( Unit::canConvert(u0,u1) ) {
                // Use unit of first curve
                yunit = u0;
            }
        }
        if ( !_unitOverrides.isEmpty() ) {
            foreach ( QString overrideUnit, _unitOverrides ) {
                Unit mUnit = Unit::map(yunit,overrideUnit);
                if ( !mUnit.isEmpty() ) {
                    // No break if found, so last override in list used
                    yunit = mUnit.name();
                }
            }
        }
        _addChild(curveItem, "CurveYUnit", yunit);

        _addChild(curveItem, "CurveXScale", curveModel->x()->scale());
        QHash<QString,QVariant> shifts = _plotModel->getDataHash(QModelIndex(),
                                                              "RunToShiftHash");
        QString curveRunDir = QFileInfo(curveModel->fileName()).absolutePath();
        if ( shifts.contains(curveRunDir) ) {
            double shiftVal = shifts.value(curveRunDir).toDouble();
            _addChild(curveItem, "CurveXBias", shiftVal);
        } else {
            // x bias can be set in a mapfile
            _addChild(curveItem, "CurveXBias", curveModel->x()->bias());
        }
        _addChild(curveItem, "CurveYScale", curveModel->y()->scale());
        _addChild(curveItem, "CurveYBias", curveModel->y()->bias());

        // Color
        QString color;
        nCurves = _plotModel->rowCount(curvesIdx);
        if ( rc == 1 ) {
            // Color each variable differently
            QList<QColor> curveColors = _plotModel->createCurveColors(nCurves);
            color = curveColors.at(nCurves-1).name();
        } else {
            // Color RUNs the same
            color = run2color.value(r);
        }
        QModelIndex lcIdx = _plotModel->getIndex(QModelIndex(),
                                                 "LegendColors","");
        if ( ii < _plotModel->rowCount(lcIdx) ) {
            // Possible commandline color override
            QModelIndex legendColorIdx = _plotModel->index(ii,1,lcIdx);
            QString legendColor = _plotModel->data(legendColorIdx).toString();
            if ( !legendColor.isEmpty() ) {
                QString group;
                if ( ii < groups.size() ) {
                    group = groups.at(ii);
                }
                if ( group.isEmpty() ) {
                    color = legendColor;
                }
            }
        }

        // Linestyle
        QString style;
        QStringList styles = _plotModel->lineStyles();
        nCurves = _plotModel->rowCount(curvesIdx);
        if ( rc == 1 ) {
            style = styles.at(0);
        } else {
            // Style each variable differently
            div_t q = div(nCurves-1,rc);
            style = styles.at((q.quot)%(styles.size()));
        }
        QModelIndex lsIdx = _plotModel->getIndex(QModelIndex(),
                                                 "Linestyles","");
        if ( ii < _plotModel->rowCount(lsIdx) ) {
            // Possible commandline linestyle override
            QModelIndex legendStyleIdx = _plotModel->index(ii,1,lsIdx);
            QString legendStyle = _plotModel->data(legendStyleIdx).toString();
            if ( !legendStyle.isEmpty() ) {
                QString group;
                if ( ii < groups.size() ) {
                    group = groups.at(ii);
                }
                if ( group.isEmpty() ) {
                    // E.g. if -l4 dog -ls4 thick_line -g4 99,
                    //      then override handled in the group logic below
                    style = legendStyle;
                }
            }
        }

        // Symbol Style
        QString symbolStyle = "none";
        QModelIndex ssIdx = _plotModel->getIndex(QModelIndex(),
                                                     "Symbolstyles","");
        if ( ii < _plotModel->rowCount(ssIdx) ) {
            QModelIndex symbolStyleIdx = _plotModel->index(ii,1,ssIdx);
            QString ss = _plotModel->data(symbolStyleIdx).toString();
            if ( !ss.isEmpty() ) {
                QString group;
                if ( ii < groups.size() ) {
                    group = groups.at(ii);
                }
                if ( group.isEmpty() ) {
                    // Use symbolstyle from commandline
                    symbolStyle = ss;
                }
            }
        }

        // Label
        QString yLabel = _runDirs.at(r) + ":" + yName;
        QModelIndex llIdx = _plotModel->getIndex(QModelIndex(),
                                                 "LegendLabels","");
        if ( ii < _plotModel->rowCount(llIdx) ) {
            QString llTag = QString("Label%1").arg(ii+1);
            QString ll = _plotModel->getDataString(llIdx,llTag,"LegendLabels");
            if ( !ll.isEmpty() ) {
                QString group;
                if ( ii < groups.size() ) {
                    group = groups.at(ii);
                }
                if ( group.isEmpty() ) {
                    // Use commandline label
                    yLabel = ll;
                }
            }
        }

        // Handle groups (-g1, -g2... -g7 options)
        if ( isGroups ) {
            int i = 0;
            foreach ( QString group, groups ) {
                if ( !group.isEmpty() ) {
                    if ( _plotModel->isMatch(curveRunDir,group) ) {
                        // Color
                        QModelIndex idx = _plotModel->index(i,1,lcIdx);
                        QString cc = _plotModel->data(idx).toString();
                        if ( !cc.isEmpty() ) {
                            color = cc ;
                        }

                        // Label
                        QString llTag = QString("Label%1").arg(i+1);
                        QString ll = _plotModel->getDataString(llIdx,llTag,
                                                               "LegendLabels");
                        if ( !ll.isEmpty() ) {
                            yLabel = ll ;
                        } else {
                            yLabel = group;
                        }

                        // Linestyle
                        idx = _plotModel->index(i,1,lsIdx);
                        QString ls = _plotModel->data(idx).toString();
                        if ( !ls.isEmpty() ) {
                            style = ls;
                        }

                        // Symbolstyle
                        idx = _plotModel->index(i,1,ssIdx);
                        QString ss = _plotModel->data(idx).toString();
                        if ( !ss.isEmpty() ) {
                            symbolStyle = ss;
                        }

                        // Match found and handled
                        break;
                    }
                }
                ++i;
            }
        }

        _addChild(curveItem, "CurveYLabel", yLabel);
        _addChild(curveItem, "CurveColor", color);
        _addChild(curveItem, "CurveLineStyle",style);
        _addChild(curveItem, "CurveSymbolStyle", symbolStyle);

        // Add actual curve model data
        if ( r == rc-1) {
            // Turn signals on for last curve for pixmap update
            _plotModel->blockSignals(false);
        }
        QVariant v = PtrToQVariant<CurveModel>::convert(curveModel);
        _addChild(curveItem, "CurveData", v);
        if ( r == rc-1) {
            _plotModel->blockSignals(true);
        }

#ifdef __linux
        int secs = qRound(timer.stop()/1000000.0);
        div_t d = div(secs,60);
        QString msg = QString("Loaded %1 of %2 curves (%3 min %4 sec)")
                             .arg(r+1).arg(rc).arg(d.quot).arg(d.rem);
        progress.setLabelText(msg);
#endif

        ++ii;
        ++jj;
    }

    // Turn signals back on before adding curveModel
    _plotModel->blockSignals(block);

    // Update progress dialog
    progress.setValue(rc);

    // Initialize plot math rect
    QRectF bbox = _plotModel->calcCurvesBBox(curvesIdx);
    QModelIndex plotIdx = curvesIdx.parent();
    QModelIndex pageIdx = plotIdx.parent().parent();
    QModelIndex plotMathRectIdx = _plotModel->getDataIndex(plotIdx,
                                                           "PlotMathRect",
                                                           "Plot");
    QModelIndexList siblingPlotIdxs = _plotModel->plotIdxs(pageIdx);
    foreach ( QModelIndex siblingPlotIdx, siblingPlotIdxs ) {
        bool isXTime = _plotModel->isXTime(siblingPlotIdx);
        if ( isXTime ) {
            QRectF sibPlotRect = _plotModel->getPlotMathRect(siblingPlotIdx);
            if ( sibPlotRect.width() > 0 ) {
                bbox.setLeft(sibPlotRect.left());
                bbox.setRight(sibPlotRect.right());
            }
            break;
        }
    }
    _plotModel->setData(plotMathRectIdx,bbox);
}
