#ifndef __CopyContoursPlugin__
#define __CopyContoursPlugin__
#include "PluginBase.h"
#include "CopyContoursSidebar.h"
#include "ImagePlus.h"
#include <cv.h>
#include <highgui.h>
//#include "wxTextCtrl_double.h"

class CopyContoursSidebar;

class CopyContoursPlugin :
	public PluginBase
{
private:
	IplImage *temp, *edge; //temporary images.
	CopyContoursSidebar *sidebar;
public:
	~CopyContoursPlugin(){ ReleaseTemps(); }
	CopyContoursPlugin( wxWindow* parent_, MyFrame *win_ );
	static string GetStaticName() { return "CopyContours"; }
	virtual int GetScope();
	virtual int GetScope2();
	virtual bool IsPreviewOn();
	virtual void DoPreview();
	virtual void ReleaseTemps();

	void ProcessImage( ImagePlus *img, int pos, int zPos );
	static void ProcessImage_static( ImagePlus *source, ImagePlus* target );
};
#endif /* __CopyContoursPlugin__ */
