#pragma once
#include "MyCapture.h"
#include "MyCanvas.h"
#include <wx/image.h>
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include "ImagePlus.h"
#include <wx/arrstr.h>
#include "PluginBase.h"

class MyCanvas;
class PluginBase;
class ImageJobQueue;

//manages read/process/write functions from a starting folder/file capture
class CaptureManager
{
private:
	bool OpenMovie_initialize(MyCapture &capture);
	bool OpenConfocal_initialize();
    ImagePlus** book;

public:
	ImagePlus img;
	int *frameMap;
	int frameCount;
	int slideCount;
	int fps;
	int totalFrameCount;
	int offset;
	int pos;
	int zPos;
	bool hasFluorescence;
	bool viewFluorescence;
	MyCanvas *canvas;
	CvSize size;
	int test;
	PluginBase *ReloadListener;
	PluginBase *RedrawListener;
	PluginBase *BookChangeListener;
	ImageJobQueue* m_queue;
	int loadRadius;
	int loadedImgs;

	MyCapture_Confocal* m_capture;

    bool drawFluorescence;
	bool drawTopBorder;
	bool drawBottomBorder;

public:
	CvSeq *contours;

	CaptureManager(void);
	~CaptureManager(void);
	ImagePlus* Access(int pos, int z=0, bool fluorescence=false, bool noImage=false, bool preload=false);
	ImagePlus* DirectAccess(int x);
	void Reset();
	void Release(int pos, int z, bool fluorescence);
	void SetCanvas(MyCanvas *canvas_);
	MyCanvas* GetCanvas();
	bool OpenMovie(const char* avi);
	bool OpenMovie(const wxArrayString &files);
	bool OpenConfocal(const wxArrayString &files, int zSlides, bool fluorescence);
	bool SaveMovie(const char* avi);
	int GetFrameCount();
	CvSize GetSize();
	int GetPos();
	int GetTotalPos();
	bool SetPos(int newpos, bool reload=false);
	int GetZPos();
	bool SetZPos(int newpos, bool reload=false);
	bool SetPos(int newpos, int newZPos, bool reload=false);
	bool OnDelete();
	bool OnDeleteBefore();
	bool OnDeleteAfter();
	bool OnPrev();
	bool OnNext();
	bool ShowFluorescence(bool show);
	void Redraw(bool callPlugin=true);
	void SetQueue(ImageJobQueue* queue);
	void SetReloadListener(PluginBase *ReloadListener_=NULL);
	void SetRedrawListener(PluginBase *RedrawListener_=NULL);
	void SetBookChangeListener(PluginBase *BookChangeListener_=NULL);
	void ReloadCurrentFrame(bool redraw=true, bool callPlugin=true);
	void ReloadCurrentFrameContours(bool redraw=true, bool callPlugin=true);
	void PushbackCurrentFrame();
	bool SaveTrackImage(wxBitmap &bmp);
	bool SaveTrajectoryImage(wxBitmap &bmp);
	bool SaveData_setup(const char* file, FILE* &fp);
	bool SaveTrackData(const char* file);
	bool SaveTrajectoryData(const char* file);
	bool SaveSpeedData(const char* file);
	bool SaveAreaData(const char* file);
	bool SaveDeformationData(const char* file);
	bool ImportTrackData(const char* file);
	void LoadNeighborhood(int newPos, int newZPos);
	bool IsInNeighborhood(int testPos, int testZPos) const;

	std::vector<CvPoint> GetTrajectory(int c); //get trajectory of cth object
	std::vector<float> GetSpeeds(int c, float &totalDisp, float &avgSpeed); //get speeds of cth object
	std::vector<float> GetAreas(int c, float &avgArea); //get areas of cth object
	std::vector<float> GetAreaDiff(int c, float &avgDiff);
	std::vector<float> GetDeformation(int c, float &avgDef);

	void Resize(int width, int height, int method);
	void Crop(CvPoint topleft, CvSize size);
};
