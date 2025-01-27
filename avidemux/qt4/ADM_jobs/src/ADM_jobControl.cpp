/**
    \file   ADM_jobControl
    \author mean fixounet@free.Fr (c) 2010

*/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QAction>
#include <QDir>
#include <QIcon>

#include "ADM_coreJobs.h"
#include "ADM_default.h"
#include "DIA_coreToolkit.h"
#include "T_jobs.h"
#include "T_progress.h"

extern void loadTranslator(void);
extern void initTranslator(void);
extern void destroyTranslator(void);

static QTableWidgetItem *fromText(const string &t, int id)
{
    QString s(QString::fromUtf8(t.c_str()));
    QTableWidgetItem *w = new QTableWidgetItem(s, id);
    Qt::ItemFlags flags = w->flags();
    flags &= ~Qt::ItemIsEditable;
    w->setFlags(flags);
    return w;
}
string date2String(uint64_t date)
{
    if (!date)
        return string("N/A");
    return string(ADM_epochToString(date));
}
string duration2String(uint64_t date)
{
    char tmp[100];
    if (!date)
        return string("N/A");
    //    printf("Delta : %d\n",(int)date);
    sprintf(tmp, "%d s", (int)date);
    return string(tmp);
}

/**
    \fn refreshList
*/
void jobWindow::refreshList(void)
{
    QTableWidget *list = ui.tableWidget;
    list->clear();
    listOfJob.clear();

    // set titles
    QTableWidgetItem *jb = fromText(QT_TRANSLATE_NOOP("jobs", "Job"), 255);
    QTableWidgetItem *outputFile = fromText(QT_TRANSLATE_NOOP("jobs", "Output"), 255);
    QTableWidgetItem *status = fromText(QT_TRANSLATE_NOOP("jobs", "Status"), 255);
    QTableWidgetItem *start = fromText(QT_TRANSLATE_NOOP("jobs", "Start Time"), 255);
    QTableWidgetItem *end = fromText(QT_TRANSLATE_NOOP("jobs", "End Time"), 255);
    QTableWidgetItem *duration = fromText(QT_TRANSLATE_NOOP("jobs", "Duration"), 255);
    ui.tableWidget->setHorizontalHeaderItem(1, jb);
    ui.tableWidget->setHorizontalHeaderItem(2, outputFile);
    ui.tableWidget->setHorizontalHeaderItem(3, start);
    ui.tableWidget->setHorizontalHeaderItem(4, end);
    ui.tableWidget->setHorizontalHeaderItem(5, duration);
    ui.tableWidget->setHorizontalHeaderItem(0, status);

    if (false == ADMJob::jobGet(listOfJob))
        return;

    int n = listOfJob.size();
    ADM_info("Found %d jobs\n", (int)n);
    list->setRowCount(n);
    for (int i = 0; i < list->actions().size(); i++)
        list->actions().at(i)->setEnabled(!!n);

    ui.pushButtonRunAll->setEnabled(!!n);
    ui.pushButtonCleanup->setEnabled(!!n);

    if (!n)
        return;

    for (int i = 0; i < n; i++)
    {
        QTableWidgetItem *nm = fromText(listOfJob[i].jobName, i);
        QTableWidgetItem *out = fromText(listOfJob[i].outputFileName, i);
        string s;
        string dur = "N/A";
        string start = "X";
        string end = "X";
        uint64_t timeTaken = 0;

        switch (listOfJob[i].status)
        {
        case ADM_JOB_IDLE:
            s = string(QT_TRANSLATE_NOOP("jobs", "Ready"));
            break;
        case ADM_JOB_RUNNING:
            s = string(QT_TRANSLATE_NOOP("jobs", "Running...."));
            break;
        case ADM_JOB_OK:
            s = string(QT_TRANSLATE_NOOP("jobs", "Success"));
            start = date2String(listOfJob[i].startTime);
            end = date2String(listOfJob[i].endTime);
            timeTaken = listOfJob[i].endTime - listOfJob[i].startTime;
            dur = duration2String(timeTaken);

            break;
        case ADM_JOB_KO:
            s = string(QT_TRANSLATE_NOOP("jobs", "Failed"));
            start = date2String(listOfJob[i].startTime);
            end = date2String(listOfJob[i].endTime);
            timeTaken = listOfJob[i].endTime - listOfJob[i].startTime;
            dur = duration2String(timeTaken);
            break;
        default:
            s = string(QT_TRANSLATE_NOOP("jobs", "???"));
            break;
        }
        QTableWidgetItem *status = fromText(s, i);
        QTableWidgetItem *startItem = fromText(start, i);
        QTableWidgetItem *endItem = fromText(end, i);
        QTableWidgetItem *durItem = fromText(dur, i);

#define MX(x, y)                                                                                                       \
    case ADM_JOB_##x:                                                                                                  \
        status->setIcon(QIcon(":/jobs/" y));                                                                           \
        break;
        switch (listOfJob[i].status)
        {
            MX(KO, "gtk-no.png");
            MX(OK, "gtk-ok.png");
            MX(RUNNING, "gtk-media-play.png");
        default:
            break;
        }
        list->setItem(i, 0 + 1, nm);
        list->setItem(i, 1 + 1, out);
        list->setItem(i, 2 + 1, startItem);
        list->setItem(i, 3 + 1, endItem);
        list->setItem(i, 4 + 1, durItem);
        list->setItem(i, 0, status);
    }
    list->resizeColumnsToContents();
}

/**
    \fn ctor
*/
#define QT_NOOP(x) x
jobWindow::jobWindow(bool mode) : QDialog()
{
    ui.setupUi(this);
    ui.tableWidget->setColumnCount(6); // Job name, fileName, Status
#if !defined(__APPLE__) && !defined(_WIN32)
    setWindowIcon(QIcon(":/jobs/jobs-window-icon-linux.png"));
#endif
    // Add some right click menu...
    ui.tableWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    // Add some right click menu...
    QAction *del = new QAction(QString(QT_TRANSLATE_NOOP("jobs", "Delete")), this);
    QAction *runNow = new QAction(QString(QT_TRANSLATE_NOOP("jobs", "Run Now")), this);
    QAction *setOk = new QAction(QString(QT_TRANSLATE_NOOP("jobs", "Force Status to success")), this);
    QAction *setReady = new QAction(QString(QT_TRANSLATE_NOOP("jobs", "Force Status to ready")), this);
    ui.tableWidget->addAction(del);
    ui.tableWidget->addAction(runNow);
    ui.tableWidget->addAction(setOk);
    ui.tableWidget->addAction(setReady);
    connect(del, SIGNAL(triggered()), this, SLOT(del()));
    connect(runNow, SIGNAL(triggered()), this, SLOT(runNow()));
    connect(setOk, SIGNAL(triggered()), this, SLOT(setOk()));
    connect(setReady, SIGNAL(triggered()), this, SLOT(setReady()));

    connect(ui.pushButtonQuit, SIGNAL(pressed()), this, SLOT(quit()));
    connect(ui.pushButtonRunAll, SIGNAL(pressed()), this, SLOT(runAllJob()));
    connect(ui.pushButtonCleanup, SIGNAL(pressed()), this, SLOT(cleanup()));

    QAction *bye = new QAction(QString(QT_TRANSLATE_NOOP("jobs", "Quit")), this);
    bye->setShortcut(Qt::Key_Q | Qt::CTRL);
    connect(bye, SIGNAL(triggered()), this, SLOT(quit()));
    this->addAction(bye);

    // Start our socket
    localPort = 0;
    if (false == mySocket.createBindAndAccept(&localPort))
    {
        popup("Cannot bind socket");
        exit(-1);
    }
    ADM_info("Socket bound to %" PRIu32 "\n", localPort);
    // ADM_socket *n=mySocket.waitForConnect(5000);
    refreshList();
    dialog = NULL;
    portable = mode;
}
/**
    \fn dtor
*/
jobWindow::~jobWindow()
{
    if (dialog)
        delete dialog;
    dialog = NULL;
}
/**
 */
bool jobWindow::popup(const char *errorMessage)
{
    ADM_error("%s\n", errorMessage);
    return true;
}
/**
    \fn jobRun
*/
bool jobRun(int ac, char **av)
{
    initTranslator();
#if defined(_WIN32) && QT_VERSION >= QT_VERSION_CHECK(5, 11, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#if defined(_WIN32) && QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Hide unhelpful context help buttons on Windows.
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
    QApplication *app = new QApplication(ac, av, 0);
#if !defined(_WIN32) && !defined(__APPLE__)
    Q_INIT_RESOURCE(jobs_linux);
#else
    Q_INIT_RESOURCE(jobs);
#endif
    loadTranslator();
    jobWindow *jWindow = new jobWindow(isPortableMode(ac, av));

    jWindow->exec();
    destroyTranslator();
    delete jWindow;
    delete app;
    return true;
}
/**
    \fn del
    \brief delete the currently selected jobs
*/
int jobWindow::getActiveIndex(void)
{
    QTableWidgetItem *item = ui.tableWidget->currentItem();
    if (!item)
        return -1;
    int index = item->type();
    return index;
}
/**
    \fn runAction
*/
void jobWindow::runAction(JobAction action)
{
    int index = getActiveIndex();
    ADM_info("%d command for index %d\n", action, index);
    if (index == -1)
        return;
    // get the job
    if (index > listOfJob.size())
    {
        ADM_error("index out of bound : %d/%d\n", index, listOfJob.size());
        return;
    }
    ADMJob *j = &(listOfJob[index]);
    switch (action)
    {
    case JobAction_delete:
        ADMJob::jobDelete(*j);
        refreshList();
        break;
    case JobAction_setReady:
        j->status = ADM_JOB_IDLE;
        ADMJob::jobUpdate(*j);
        refreshList();
        break;
    case JobAction_setOk:
        j->status = ADM_JOB_OK;
        ADMJob::jobUpdate(*j);
        refreshList();
        break;
    case JobAction_runNow: {
        if (dialog)
            break;
        dialog = new jobProgress(1);
        dialog->setCurrentJob(0);
        dialog->setCurrentOutputName(j->outputFileName);
        dialog->open();
        QApplication::processEvents();
        runOneJob(*j);
        delete dialog;
        dialog = NULL;
    }
    break;

    default:
        ADM_warning("Command %d no handled\n", action);
        break;
    }
}

void jobWindow::del(void)
{
    ADM_info("Del\n");
    runAction(JobAction_delete);
}
void jobWindow::setOk(void)
{
    ADM_info("Ok\n");
    runAction(JobAction_setOk);
}
void jobWindow::setReady(void)
{
    ADM_info("setReady\n");
    runAction(JobAction_setReady);
}
void jobWindow::runNow(void)
{
    ADM_info("runNow\n");
    runAction(JobAction_runNow);
}

/**
    \fn quit
    \brief run selected jobs whatever its status
*/

void jobWindow::quit(void)
{
    done(1);
}
/**
 * \fn cleanup
 * \brief purge finished jobs
 */
void jobWindow::cleanup(void)
{
    int n = listOfJob.size();
    ADM_info("Cleaning up based on total of %d jobs\n", n);
    std::vector<int> toDel;
    for (int i = 0; i < n; i++)
    {
        ADMJob *j = &(listOfJob[i]);

        if (j->status == ADM_JOB_OK)
        {
            toDel.push_back(i);
        }
    }
    n = toDel.size();
    if (!n)
    {
        ADM_info("No completed jobs, nothing to do.\n");
        return;
    }
    ADM_info("%d completed jobs to delete.\n", n);
    for (int i = n - 1; i >= 0; i--)
    {
        int dex = toDel[i];
        ADM_info("Deleting job %d\n", dex);
        ADMJob *j = &(listOfJob[dex]);
        ADMJob::jobDelete(*j);
    }
    toDel.clear();
    ADM_info("%d jobs deleted.\n", n);
    refreshList();
    return;
}
/**
    \fn runAllJob
*/
void jobWindow::runAllJob(void)
{
    if (dialog)
        return;
    int n = listOfJob.size();
    dialog = new jobProgress(n);

    for (int i = 0; i < n; i++)
    {
        ADMJob *j = &(listOfJob[i]);

        if (j->status == ADM_JOB_IDLE)
        {
            dialog->setCurrentJob(i);
            dialog->setCurrentOutputName(j->outputFileName);
            dialog->open();
            QApplication::processEvents();
            runOneJob(*j);
        }
    }
    delete dialog;
    dialog = NULL;
    return;
}
#if 0
bool jobRun(void)
{

        ADM_jobDropAllJobs();

        vector <ADMJob> jobs;
        ADM_jobGet(jobs);

        int n=jobs.size();
        printf("Found %d jobs...\n",n);

        ADMJob job;
        job.endTime=2;
        job.startTime=3;
        job.scriptName="myscript1";
        job.jobName="myjob1";
        job.outputFileName="output1";
        
        ADM_jobAdd(job);


        
        ADM_jobGet(jobs);

        n=jobs.size();
        printf("Found %d jobs...\n",n);
        for(int i=0;i<n;i++)
        {
            printf("%d/%d ",i,n);
            ADM_jobDump(jobs[i]);
        }
        printf("\n***************************\n");
        jobs[0].status=ADM_JOB_KO;
        ADM_jobUpdate(jobs[0]);
        n=jobs.size();
        printf("Found %d jobs...\n",n);
        for(int i=0;i<n;i++)
        {
            printf("%d/%d ",i,n);
            ADM_jobDump(jobs[i]);
        }
        printf("\n***************************\n");
        // Delete
        ADM_jobDelete(jobs[0]);
        ADM_jobGet(jobs);

        n=jobs.size();
        printf("Found %d jobs...\n",n);
        for(int i=0;i<n;i++)
        {
            printf("%d/%d ",i,n);
            ADM_jobDump(jobs[i]);
        }
        return true;
}
#endif
