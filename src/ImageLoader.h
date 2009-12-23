#ifndef __IMAGELOADER_H__
#define __IMAGELOADER_H__

#include <wx/thread.h>

#include "ImageJobQueue.h"

class ImageLoader : public wxThread
{
    public:
        ImageLoader(ImageJobQueue* queue);
    private:
        ImageJobQueue* m_queue;

        virtual wxThread::ExitCode Entry();
};

#endif //__IMAGELOADER_H__
