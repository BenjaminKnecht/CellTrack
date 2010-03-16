#pragma once
#include "PluginBase.h"
#include "FilterContoursSidebar.h"
#include "ImagePlus.h"
#include <cv.h>
#include <highgui.h>
#include "wxTextCtrl_double.h"

class FilterContoursSidebar;

class FilterContoursPlugin :
	public PluginBase
{
private:
	IplImage *gray, *edge; //temporary images.
	FilterContoursSidebar *sidebar;
public:
	FilterContoursPlugin( wxWindow* parent_, MyFrame *win_ );
	~FilterContoursPlugin(void) {}
	static string GetStaticName() { return "FilterContours"; }
	virtual int GetScope();
    virtual int GetScope2();
	virtual bool IsPreviewOn();
	virtual void DoPreview();
	virtual void OnFluorescence();

	void ProcessImage( ImagePlus *img );
	static void ProcessImage_static( ImagePlus *img, int isMinArea, int isMaxArea, int minArea, int maxArea, int isMinDist, int minDist, CvSize size );
};
