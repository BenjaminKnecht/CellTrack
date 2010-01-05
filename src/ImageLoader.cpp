#include "ImageLoader.h"
#include <iostream>

ImageLoader::ImageLoader(ImageJobQueue* queue) : m_queue(queue)
{
    loadedElements = 0;
    if(queue)
        wxThread::Create();
}

wxThread::ExitCode ImageLoader::Entry()
{
    m_queue->Report(Job::thread_started);
    bool keepRunning = true;
    IplImage* img = NULL;
    while(keepRunning)
    {
        Job j = m_queue->Pop();
        std::multiset<int>::iterator it;
        //std::cout << "Queue Size: " << m_queue->GetLength() << std::endl;
        switch(j.m_cmd)
        {
            case Job::thread_exit:
                keepRunning = false;
                break;
            case Job::thread_load:
                //std::cout << "loading " << j.m_imgPos << std::endl;
                it = toIgnore.find(j.m_imgPos);
                if (it != toIgnore.end())
                {
                    toIgnore.erase(it);
                }
                else
                {
                    loadedElements++;
                    img = cvLoadImage((j.m_filename).mb_str());
                    m_queue->Report(Job::thread_loaded, j.m_imgPos, img, j.m_filename);
                }
                break;
            case Job::thread_delete:
                if (j.m_imgPtr)
                {
                    loadedElements--;
                    cvReleaseImage(&(j.m_imgPtr));
                    m_queue->Report(Job::thread_deleted, j.m_imgPos);
                }
                else
                {
                    toIgnore.insert(j.m_imgPos);
                }
                break;
            case Job::thread_null:
            default: break;
        }
    }
    std::cout << "running elements: " << loadedElements << std::endl;
    return (wxThread::ExitCode)img;
}
