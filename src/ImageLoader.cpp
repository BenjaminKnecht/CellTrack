#include "ImageLoader.h"
#include <iostream>

ImageLoader::ImageLoader(ImageJobQueue* queue) : m_queue(queue)
{
    imgInMem = 0;
    loadingCalls = 0;
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
                loadingCalls++;
                //std::cout << "img loader: loading " << j.m_imgPos << std::endl;
                it = toIgnore.find(j.m_imgPos);
                if (it != toIgnore.end())
                {
                    toIgnore.erase(it);
                }
                else
                {
                    imgInMem++;
                    img = cvLoadImage((j.m_filename).mb_str());
                    m_queue->Report(Job::thread_loaded, j.m_imgPos, img, j.m_filename);
                }
                break;
            case Job::thread_delete:
                loadingCalls--;
                if (j.m_imgPtr)
                {
                    imgInMem--;
                    cvReleaseImage(&(j.m_imgPtr));
                    m_queue->Report(Job::thread_deleted, j.m_imgPos);
                }
                else
                {
                    if (j.m_imgPos >= 0)
                        toIgnore.insert(j.m_imgPos);
                }
                break;
            case Job::thread_null:
            default: break;
        }
    }
    std::cout << "Images left in Memory: " << imgInMem << std::endl;
    std::cout << "Calls: " << loadingCalls << std::endl;
    return (wxThread::ExitCode)img;
}
