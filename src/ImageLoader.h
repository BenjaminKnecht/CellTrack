#ifndef __IMAGELOADER_H__
#define __IMAGELOADER_H__

#include <wx/thread.h>
#include <set>

#include "ImageJobQueue.h"

class ImageLoader : public wxThread
{
    public:
        ImageLoader(ImageJobQueue* queue);
    private:
        ImageJobQueue* m_queue;
        int imgInMem;
        int loadingCalls;
        virtual wxThread::ExitCode Entry();
        std::multiset<int> toIgnore;
};

#endif //__IMAGELOADER_H__
