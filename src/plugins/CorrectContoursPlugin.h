#ifndef __CorrectContoursPlugin__
#define __CorrectContoursPlugin__
#include "PluginBase.h"
#include "CorrectContoursSidebar.h"
#include "ImagePlus.h"
#include <cv.h>
#include <highgui.h>

class CorrectContoursSidebar;

class CorrectContoursPlugin :
	public PluginBase
{
private:
	CorrectContoursSidebar *sidebar;
public:
    CorrectContoursPlugin( wxWindow* parent_, MyFrame *win_ );
	~CorrectContoursPlugin() {};
	static string GetStaticName() { return "CorrectContours"; }
	virtual void OnOK();
    virtual int GetScope();
	virtual int GetScope2();
	virtual bool IsPreviewOn();
	virtual void DoPreview();
	virtual void OnFluorescence();
	static double modifiedPointPolygonTest(CvPoint2D32f pt, CvPoint*& bottomPointsInt, CvPoint2D32f& min0, CvPoint2D32f& min, int& index, int size);

	void ProcessImage( ImagePlus *img, int pos, int zPos, bool fluorescence );
	static void ProcessImage_static( ImagePlus *source, ImagePlus* target );
};

#endif /*__CorrectContoursPlugin__*/
