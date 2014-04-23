#include "dpfilterproxymodel.h"
#include <QDebug>

// static to get around const
QHash<QString,bool> DPFilterProxyModel::_acceptedDPFileCache;

// Filter for DPs that have params in paramList
DPFilterProxyModel::DPFilterProxyModel(MonteModel *monteModel,
                                       QObject *parent) :
    QSortFilterProxyModel(parent),
    _monteModel(monteModel)
{
    for ( int i = 0 ; i < _monteModel->columnCount(); ++i ) {
        QString paramName = _monteModel->headerData(i).toString();
        _modelParams.insert(paramName,0);
    }
}

bool DPFilterProxyModel::filterAcceptsRow(int row,
                                          const QModelIndex &pidx) const
{
    if ( ! pidx.isValid() ) {
        return false;
    }

    bool isAccept = false;
    QRegExp rx(filterRegExp());
    QFileSystemModel* m = (QFileSystemModel*) sourceModel();
    int ncols = m->columnCount(pidx);
    for ( int col = 0; col < ncols ; ++col) {
        QModelIndex idx = m->index(row,col,pidx);
        if ( _isAccept(idx,m,rx) ) {
            isAccept = true;
            break;
        }
    }

    return isAccept;
}

bool DPFilterProxyModel::filterAcceptsColumn(int col,
                                             const QModelIndex &pidx) const
{
    if ( !pidx.isValid() ) {
        return false;
    }

    if ( col > 0 ) {
        return true;
    }

    bool isAccept = false;
    QRegExp rx(filterRegExp());
    QFileSystemModel* m = (QFileSystemModel*) sourceModel();
    int nrows = m->rowCount(pidx);
    for ( int row = 0; row < nrows ; ++row) {
        QModelIndex idx = m->index(row,col,pidx);
        if ( _isAccept(idx,m,rx) ) {
            isAccept = true;
            break;
        }
    }

    return isAccept;
}

bool DPFilterProxyModel::_isAccept(const QModelIndex &idx,
                                   QFileSystemModel *m, const QRegExp &rx) const
{
    bool isAccept = false ;

    QString dpFilePath = m->fileInfo(idx).absoluteFilePath();
    bool isCached = _acceptedDPFileCache.contains(dpFilePath);
    if ( isCached ) {
        return _acceptedDPFileCache.value(dpFilePath);
    }

    if ( m->isDir(idx) ) {
        isAccept = true;
    } else {
        QString path = m->filePath(idx);
        if ( path.contains(rx) && (m->fileInfo(idx).suffix() == "xml" ||
             ((m->fileInfo(idx).fileName().startsWith("DP_") &&
               m->fileInfo(idx).suffix().isEmpty() &&
              m->fileInfo(idx).isFile()))) ) {
            isAccept = true;
        }

        // Filter for DPs that have params in paramList constructor argument
        if ( isAccept ) {
            foreach ( QString param,DPProduct::paramList(dpFilePath) ) {
                if ( !_modelParams.contains(param) ) {
                    isAccept = false;
                    break;
                }
            }
        }
    }

    _acceptedDPFileCache.insert(dpFilePath,isAccept);

    return isAccept;
}