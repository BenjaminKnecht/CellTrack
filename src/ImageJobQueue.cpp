#include "ImageJobQueue.h"

DECLARE_EVENT_TYPE(wxEVT_IMGTHREAD, -1)
DEFINE_EVENT_TYPE(wxEVT_IMGTHREAD)

void ImageJobQueue::AddJob(Job job, int priority)
{
    wxMutexLocker lock(m_mutexQueue);
    m_jobs.insert(std::pair<int, Job>(priority, job));
    m_queueCount.Post();
}

Job ImageJobQueue::Pop()
{
    Job element;
    m_queueCount.Wait();
    wxMutexLocker lock(m_mutexQueue);
    element = (m_jobs.begin())->second;
    m_jobs.erase(m_jobs.begin());
    return element;
}

void ImageJobQueue::Report(const Job::Command& cmd, int pos, IplImage* ptr, const wxString& filename)
{
    wxCommandEvent evt(wxEVT_IMGTHREAD, cmd);
    evt.SetString(filename.c_str());
    evt.SetInt(pos);
    evt.SetClientData((void*)ptr);
    m_parent->AddPendingEvent(evt);
}

int ImageJobQueue::GetLength()
{
    wxMutexLocker lock(m_mutexQueue);
    return m_jobs.size();
}
