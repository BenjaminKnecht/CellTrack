#include "ImageLoader.h"

ImageLoader::ImageLoader(ImageJobQueue* queue) : m_queue(queue)
{
    if(queue)
        wxThread::Create();
}

wxThread::ExitCode ImageLoader::Entry()
{
    m_queue->Report(Job::thread_started);
    bool keepRunning = true;
    IplImage* img;
    while(keepRunning)
    {
        Job j = m_queue->Pop();
        switch(j.m_cmd)
        {
            case Job::thread_exit:
                keepRunning = false;
                break;
            case Job::thread_load:
                img = cvLoadImage((j.m_filename).mb_str());
                m_queue->Report(Job::thread_loaded, j.m_imgPos, img, j.m_filename);
                break;
            case Job::thread_delete:
                cvReleaseImage(&(j.m_imgPtr));
                m_queue->Report(Job::thread_loaded, j.m_imgPos, j.m_imgPtr);
                break;
            case Job::thread_null:
            default: break;
        }
    }
    return (wxThread::ExitCode)img;
}
