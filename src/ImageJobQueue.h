#ifndef __IMAGEJOBQUEUE_H__
#define __IMAGEJOBQUEUE_H__

#include <wx/event.h>
#include <highgui.h>
#include <map>

class Job
{
    public:
        enum Command
        {
            thread_exit=wxID_EXIT,
            thread_null=wxID_HIGHEST+1,
            thread_started,
            thread_load,
            thread_delete,
            thread_loaded,
            thread_deleted,
        };
        Job() : m_cmd(thread_null){}
        Job(Command cmd) : m_imgPos(0), m_cmd(cmd), m_imgPtr(NULL), m_filename(_T("")) {}
        Job(Command cmd, const int pos, const wxString& filename) : m_imgPos(pos), m_cmd(cmd), m_filename(filename), m_imgPtr(NULL) {}
        Job(Command cmd, const int pos, IplImage* ptr) : m_imgPos(pos), m_cmd(cmd), m_imgPtr(ptr), m_filename(_T("")) {}
        Job(Command cmd, const int pos, IplImage* ptr, const wxString& filename) : m_imgPos(pos), m_cmd(cmd), m_imgPtr(ptr), m_filename(filename) {}

        Command m_cmd;
        wxString m_filename;
        IplImage* m_imgPtr;
        int m_imgPos;
};

class ImageJobQueue
{
    public:
        ImageJobQueue(wxEvtHandler* parent) : m_parent(parent) {}
        void AddJob(Job job, int priority);
        Job Pop();
        void Report(const Job::Command& cmd, int pos = -1, IplImage* ptr = NULL, const wxString& filename = _T(""));
        int GetLength();
    private:
        std::multimap<int, Job> m_jobs;
        wxEvtHandler* m_parent;
        wxMutex m_mutexQueue;
        wxSemaphore m_queueCount;
};

#endif //__IMAGEJOBQUEUE_H__
