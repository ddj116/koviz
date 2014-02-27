#ifndef JOB_H
#define JOB_H

#include <QString>
#include "libsnapdata/trickcurvemodel.h"

class Job;

bool jobAvgTimeGreaterThan(Job* a,Job* b);
bool jobMaxTimeGreaterThan(Job* a,Job* b);

class Job
{
  public:
    // job_id is logged job name
    // e.g. JOB_bus.SimBus##read_ObcsRouter_C1.1828.00(read_simbus_0.100)
    Job(TrickCurveModel* curve);
    Job(const QString& jobId);

    QString log_name() const; // trick binary logged jobname
    QString job_num() const { return _job_num; }
    QString job_name() const { return _job_name; }
    QString sim_object_name() const ;
    int thread_id() const { return _thread_id; }
    double freq() ;
    QString job_class() const { return _job_class ;}

    double avg_runtime();
    double max_runtime();
    double max_timestamp();
    double stddev_runtime(); // TODO: make unit test

    inline TrickCurveModel* curve() const { return _curve; }
    inline int npoints() const { return _npoints; }

private:
    Job() {}

    void _parseJobId(const QString& jobId);

    TrickCurveModel* _curve;
    int _npoints;

    QString _log_name;
    QString _job_name;
    QString _job_num;   // e.g. 1831.04
    int _thread_id;
    double _freq;
    QString _job_class;

    bool _is_stats;
    bool _is_stddev;
    void _do_stats();
    double _avg_runtime;
    double _stddev_runtime;
    double _max_runtime;
    double _max_timestamp;
};

#endif // JOB_H
