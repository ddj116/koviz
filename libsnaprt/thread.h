#ifndef THREAD_H
#define THREAD_H

#include "job.h"
#include "frame.h"
#include "utils.h"
#include "sjobexecthreadinfo.h"
#include "libsnapdata/snaptable.h"
#include <stdexcept>

#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QDataStream>
#include <QRegExp>
#include <QFileInfo>

class Threads;


class Thread
{
  friend class Threads;

  public:
    Thread(const QString& runDir);
    Thread(const QString &runDir, double startTime=0.0, double stopTime=1.0e20);
    ~Thread();

    void addJob(Job* job);  // all jobs added must have same threadId

    double startTime()              const { return _startTime; }
    double stopTime()              const { return _stopTime; }
    bool   isRealTime()             const { return _frameModelIsRealTime; }
    QList<Job*> jobs()               const { return _jobs; }
    Job*   jobAtIndex(int idx)       const { return _jobs.at(idx); }
    int    numJobs()                 const { return _jobs.size() ; }
    int    threadId()                const { return _threadId; }
    double avgRunTime()              const { return _avg_runtime; }
    double runtime(double timestamp)  const;
    SnapTable* runtimeCurve()         const { return _runtimeCurve; }
    double avgLoad()                 const { return _avg_load; }
    int    timeIdxAtMaxRunTime()     const { return _tidx_max_runtime; }
    double maxRunTime()              const { return _max_runtime ; }
    double maxLoad()                 const { return _max_load ; }
    double stdDeviation()            const { return _stdev; }
    double frequency()               const { return _freq; }
    int    numOverruns()               const { return _num_overruns; }

    int numFrames() const; // this differs from number of timestamps
                        //  since frames can span multiple timestamps

    double avgJobRuntime(Job* job) const ; // avg runtime per thread frame
                                        // differs from avg job runtime because
                                        // a job  may be called multiple times
                                        // per frame
                                        // e.g. a 0.1 job is called 10X if the
                                        // thread has a 1.0 second AMF frame
    double avgJobLoad(Job* job) const ;


  private:

    QString _runDir;
    double _startTime;
    double _stopTime;
    int _threadId;
    SJobExecThreadInfo _sJobExecThreadInfo;
    QList<Job*> _jobs;

    double _avg_runtime;
    double _avg_load;       // this is a percentage
    int _tidx_max_runtime;
    double _max_runtime ;
    double _max_load ;      // this is a percentage
    double _stdev;
    double _freq;      // assume freq is freq of highest freq job on the thread
    int _num_overruns;

    void _do_stats();
    double _calcFrequency() ;

    static QString _err_string;
    static QTextStream _err_stream;

    QMap<int,double> _frameidx2runtime;
    QMap<double,int> _jobtimestamp2frameidx;

    SnapTable* _runtimeCurve; // t,runtime curve

    TrickModel* _frameModel;
    int _frameModelRunTimeCol;
    int _frameModelOverrunTimeCol;
    bool _frameModelIsRealTime;
    void _frameModelSet();
    void _frameModelCalcIsRealTime();

    int _calcNumFrames();
};

class Threads
{
  public:
    Threads(const QString& runDir, const QList<Job *> &jobs,
            double startTime=0.0, double stopTime=1.0e20);
    ~Threads();
    const QMap<int,Thread*>* hash() { return &_threads; }

  private:
    QString _runDir;
    QList<Job*> _jobs;
    QList<int> _ids;
    QMap<int,Thread*> _threads;
    double _startTime ;
    double _stopTime ;
};

#endif // THREAD_H
