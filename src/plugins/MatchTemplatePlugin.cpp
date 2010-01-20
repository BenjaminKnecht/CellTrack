#include "MatchTemplatePlugin.h"
#include "Util.h"

MatchTemplatePlugin::MatchTemplatePlugin( wxWindow* parent_, MyFrame *win_ )
: PluginBase(GetStaticName(), parent_, win_, true, true,true),
map(NULL)
{
	sidebar =  new MatchTemplateSidebar(parent_, this);
	sidebarw = sidebar;
	DoPreview();
}
void MatchTemplatePlugin::ReleaseTemps()
{
	if (map) cvReleaseImage(&map);
}
int MatchTemplatePlugin::GetScope() {	return sidebar->scope->GetSelection() ? 1 : 0; }
bool MatchTemplatePlugin::IsPreviewOn(){ return sidebar->preview->GetValue(); }
void MatchTemplatePlugin::DoPreview()
{
	if (!IsPreviewOn())
		return;
	cm->ReloadCurrentFrameContours(false);
	ProcessImage(&cm->img, cm->GetPos(), cm->GetZPos());
	cm->Redraw(false);
}
void MatchTemplatePlugin::FetchParams()
{
	const static int METHOD_MAP[] = {CV_TM_SQDIFF, CV_TM_SQDIFF_NORMED, CV_TM_CCORR, CV_TM_CCORR_NORMED, CV_TM_CCOEFF, CV_TM_CCOEFF_NORMED};
	method = METHOD_MAP[sidebar->method->GetSelection()];
	winsize = cvSize(2*sidebar->width->GetValue()-1, 2*sidebar->height->GetValue()-1);
	useFirst = sidebar->useFirst->GetValue();
}

void MatchTemplatePlugin::OnOK()
{
	wxBeginBusyCursor();
	if (!GetScope())
		ProcessImage(cm->Access(cm->GetPos(), cm->GetZPos()), cm->GetPos(), cm->GetZPos());
	else{
		FetchParams();
		ImagePlus* oimg = cm->Access(0, 0, false, true);
		int numContours = (int) oimg->contourArray.size();
		CreateProgressDlg();
		int frameCount = cm->GetFrameCount();
		int slideCount = cm->slideCount;
		CreateProgressDlg(numContours*slideCount*frameCount);
		bool cont=true;
		for (int slide = 0; slide < slideCount; slide++)
		{
		    oimg = cm->Access(0, slide);
		    for (int i=1; i<cm->GetFrameCount(); i++)
                cm->Access(i,slide,false,true)->CloneContours(oimg);
            for (int i=1; i<frameCount && cont; i++)
            {
                for (int j=0; j<numContours && (cont=progressDlg->Update(slide*slideCount+j*frameCount+i, wxString::Format(_T("Cell %d of %d, Frame %d of %d"), j+1,numContours, i+1, frameCount))); j++)
                {
                    ProcessStatic(j, cm->Access(i,slide, false, false, true), cm->Access(useFirst ? 0 : i-1,slide), method, winsize, map);
                }
                cm->Release(i-1,slide,false);
            }
            cm->Release(frameCount-1,slide,false);
            cm->Release(0,slide,false);
		}
		DestroyProgressDlg();
	}
	cm->ReloadCurrentFrameContours(true, false);
	wxEndBusyCursor();
}
void MatchTemplatePlugin::ProcessImage( ImagePlus* img, int pos, int zPos )
{
	if (pos==0)
		return;
	FetchParams();
	ImagePlus *oimg = cm->Access(useFirst ? 0 : pos-1, zPos);

	img->CloneContours(oimg);
	int numContours = (int) oimg->contourArray.size();
	for (int i=0; i<numContours; i++){
		ProcessStatic(i, img, oimg, method, winsize, map);
	}
	cm->Release(useFirst ? 0 : pos-1, zPos, false);
}
void MatchTemplatePlugin::ProcessStatic
( int i, ImagePlus *img, ImagePlus *oimg,
 int method, CvSize winsize, IplImage* &map){
	CvRect orect = cvBoundingRect(oimg->contourArray[i],1);
	RestrictRectLoc(orect, cvRect(0,0,img->orig->width,img->orig->height));
	cvSetImageROI(oimg->orig, orect);
	CvRect rect = cvRect(MAX(0,orect.x-winsize.width), MAX(0,orect.y-winsize.height),orect.width+2*winsize.width, orect.height+2*winsize.height);
	rect.width = MIN(rect.width,oimg->orig->width-rect.x);
	rect.height = MIN(rect.height,oimg->orig->height-rect.y);
	cvSetImageROI(img->orig, rect);

	CvSize mapsize = MyPoint(MyPoint(rect)-MyPoint(orect)+wxPoint(1,1)).ToCvSize();
	if (map && MyPoint(cvGetSize(map))!=MyPoint(mapsize))
		cvReleaseImage(&map);
	if( !map )
        map = cvCreateImage(mapsize, IPL_DEPTH_32F, 1);

	cvMatchTemplate( img->orig, oimg->orig, map, method );
	cvResetImageROI(img->orig);
	cvResetImageROI(oimg->orig);
	CvPoint minloc;
	CvPoint maxloc;
	double minval, maxval;
	cvMinMaxLoc( map, &minval, &maxval, &minloc, &maxloc);
	bool minisbest = (method == CV_TM_SQDIFF || method==CV_TM_SQDIFF_NORMED);
	rect.x = rect.x + (minisbest ? minloc.x : maxloc.x);
	rect.y = rect.y + (minisbest ? minloc.y : maxloc.y);

	CvPoint shift = cvPoint(rect.x - orect.x, rect.y - orect.y);
	ShiftContour(oimg->contourArray[i],img->contourArray[i],shift);
	ShiftFeatPoints(oimg->feats[i], img->feats[i], cvPointTo32f(shift));
}

