#include "MyCapture.h"
#include <fstream>


MyCapture_Files::MyCapture_Files(const wxArrayString& files_)
: MyCapture()
{
	files = files_;
	if(!(frame = cvLoadImage(files[0].mb_str()))){
		wxLogError(_T("Failed to read image file [ %s ]. Corrupt file or unsupported codec."), files[0]);
		return;
	}

	width = frame->width;
	height = frame->height;
	size = cvSize(width, height);
	frameCount = (int) files.GetCount();
	fps = 0;
	frame_flip = cvCreateImage(size,8,3);
}
MyCapture_Movie::MyCapture_Movie(const char* avi)
: MyCapture(), capture(NULL)
{
	frame_flip = NULL;
	if(!( capture = cvCaptureFromAVI(avi) )) {
		wxLogError(_T("Failed to read movie file [ %s ]. Corrupt file or unsupported codec."), avi);
		return;
	}
	size = cvSize(cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH),
		cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT));
	width = size.width;
	height = size.height;
	frameCount = cvGetCaptureProperty(capture, CV_CAP_PROP_FRAME_COUNT);
	fps = cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);
	frame_flip = cvCreateImage(size,8,3);
}
MyCapture_Confocal::MyCapture_Confocal(const wxArrayString& files_, int zslides)
: MyCapture_Files(files_)
{
	zSlides = zslides;
}
MyCapture::~MyCapture()
{
	if ( frame_flip )
		cvReleaseImage( &frame_flip );
}
MyCapture_Files::~MyCapture_Files(void)
{
	if (frame)
		cvReleaseImage(&frame);
}
MyCapture_Movie::~MyCapture_Movie()
{
	if ( capture )
		cvReleaseCapture( &capture );
}
MyCapture_Confocal::~MyCapture_Confocal()
{
}
void MyCapture::setPos(int pos_)
{
	if ( pos_ < 0 || pos_ >= frameCount )
		throw "invalid frame position";
	pos=pos_;
}
void MyCapture_Movie::setPos(int pos_)
{
	MyCapture::setPos(pos_);
	cvSetCaptureProperty(capture, CV_CAP_PROP_POS_FRAMES, pos);
}
// The frame is already queried in the derived classes. Here, we just set the
// image parameters from frame obtained upon the first queryFrame call.
IplImage* MyCapture::queryFrame(int pos_){
	if (!step) {
		unsigned char *raw;
		cvGetRawData(frame, &raw, &step);
	}
	return frame;
}
IplImage* MyCapture_Files::queryFrame(int pos_)
{
	if ( pos_ >= 0 )
		setPos(pos_);
	if ( pos >= frameCount )
		throw "no more frames left to query. position passed end of movie.";
	if (frame)
		cvReleaseImage(&frame);
	if (!(frame = cvLoadImage(files[pos].mb_str()))) {
		wxLogError(_T("Failed to read image file [ %s ]. Corrupt file or unsupported codec. Frame %d is replaced with a blank image."), files[0], pos);
		if (size.width)
			frame = cvCreateImage(size,8,3);
	}
	return MyCapture::queryFrame();
}
IplImage* MyCapture_Movie::queryFrame(int pos_){
	if ( pos_ >= 0 )
		setPos(pos_);
	if ( pos >= frameCount )
		throw "no more frames left to query. position passed end of movie.";
	frame = cvQueryFrame(capture);
	if ( frame->origin ){
		cvConvertImage( frame, frame_flip, CV_CVTIMG_FLIP | CV_CVTIMG_SWAP_RB );
		frame = frame_flip;
	}
	return MyCapture::queryFrame();
}
IplImage* MyCapture_Confocal::queryFrame(int pos_)
{
	if ( pos_ >= 0 )
		setPos(pos_);
	if ( pos >= frameCount )
		throw "no more frames left to query. position passed end of movie.";
	if (frame)
		cvReleaseImage(&frame);

    std::ifstream file (files[pos].mb_str(), std::ios::in|std::ios::binary|std::ios::ate);
    if (file.is_open())
    {
        frame = cvCreateImage(size,IPL_DEPTH_8U,3);
        uchar* data = (uchar*)frame->imageData;
        std::ifstream::pos_type end = file.tellg();
        char* memblock = new char [2];
        file.seekg (768, std::ios::beg);
        while(file.tellg() < end)
        {
            file.read (memblock, 2);
            int pixVal = ((int)(memblock[0]<<8) + (uchar)memblock[1])/16;
            *(data++) = (uchar)pixVal;
            *(data++) = (uchar)pixVal;
            *(data++) = (uchar)pixVal;
        }
        file.close();
        delete[] memblock;
    }
    else
    {
        wxLogError(_T("Failed to read image file [ %s ]. Corrupt file or unsupported codec. Frame %d is replaced with a blank image."), files[0], pos);
        frame = cvCreateImage(size,IPL_DEPTH_8U,3);
    }
}
int MyCapture_Confocal::getSlideNumber()
{
    return zSlides;
}
