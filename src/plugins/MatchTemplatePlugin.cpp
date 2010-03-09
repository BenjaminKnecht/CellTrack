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
int MatchTemplatePlugin::GetScope2() {	return sidebar->scope2->GetSelection(); }
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

void MatchTemplatePlugin::OnFluorescence()
{
    if (cm->viewFluorescence && GetScope2() == 0)
    {
        sidebar->scope2->SetSelection(1);
    }
    else if (!cm->viewFluorescence && GetScope2() == 1)
    {
        sidebar->scope2->SetSelection(0);
    }
}

void MatchTemplatePlugin::OnOK()
{
	wxBeginBusyCursor();
	if (GetScope() == 0) // single
	{
		if (GetScope2() != 1) // both and normal
        {
            ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), false), cm->GetPos(), cm->GetZPos());
            cm->Release(cm->GetPos(), cm->GetZPos(), false);
        }
        if (GetScope2() != 0) // both and fluorescence
        {
            ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), true), cm->GetPos(), cm->GetZPos());
            cm->Release(cm->GetPos(), cm->GetZPos(), true);
        }
	}
	else if (GetScope() == 2) // t-direction
	{
	    FetchParams();
		ImagePlus *oimg;
		CvRect orect, searchwin;
		CvPoint ocenter;

		int frameCount = cm->GetFrameCount();
		CreateProgressDlg(frameCount*(GetScope2()==2?2:1));
		if (GetScope2() != 1) // both and normal
        {
            oimg = cm->Access(0,cm->GetZPos());
            int numContours = (int) oimg->contourArray.size();
            //cm->Release(0,0, false);
            for (int i=1; i<frameCount; i++)
                cm->Access(i,cm->GetZPos(),false,true)->CloneContours(oimg);
            for (int i=1; i<frameCount && progressDlg->Update(i, wxString::Format(_T("Frame %d of %d"), i+1, frameCount)); i++)
            {
                for (int j=0; j<numContours; j++)
                {
                    ProcessStatic(j, cm->Access(i,cm->GetZPos(), false, false, GetScope()), cm->Access(useFirst ? 0 : i-1,cm->GetZPos()), method, winsize, map);
                }
                cm->Release(i-1,cm->GetZPos(),false);
            }
            cm->Release(frameCount-1,cm->GetZPos(),false);
            cm->Release(0,cm->GetZPos(),false);
        }
        if (GetScope2() != 0) // both and fluorescence
        {
            oimg = cm->Access(0,cm->GetZPos(), true);
            int numContours = (int) oimg->contourArray.size();
            //cm->Release(0,0, false);
            for (int i=1; i<frameCount; i++)
                cm->Access(i,cm->GetZPos(),true,true)->CloneContours(oimg);
            for (int i=1; i<frameCount && progressDlg->Update(i+(GetScope2()==2?frameCount:0), wxString::Format(_T("Frame %d of %d"), i+1, frameCount)); i++)
            {
                for (int j=0; j<numContours; j++)
                {
                    ProcessStatic(j, cm->Access(i,cm->GetZPos(), true, false, GetScope()), cm->Access(useFirst ? 0 : i-1,cm->GetZPos()), method, winsize, map);
                }
                cm->Release(i-1,cm->GetZPos(),true);
            }
            cm->Release(frameCount-1,cm->GetZPos(),true);
            cm->Release(0,cm->GetZPos(),true);
        }
        DestroyProgressDlg();
	}
	else if (GetScope() == 3) // z-direction
	{
	    FetchParams();
		ImagePlus *oimg;
		CvRect orect, searchwin;
		CvPoint ocenter;
		int slideCount = cm->slideCount;
		CreateProgressDlg(slideCount*(GetScope2()==2?2:1));
		if (GetScope2() != 1) // both and normal
        {
            for (int i=1; i<slideCount && progressDlg->Update(i, wxString::Format(_T("Slide %d of %d"), i+1, slideCount)); i++)
            {
                oimg = cm->Access(0,i);
                int numContours = (int) oimg->contourArray.size();
                cm->Access(cm->GetPos(),i,false,true)->CloneContours(oimg);
                for (int j=0; j<numContours; j++)
                {
                    ProcessStatic(j, cm->Access(cm->GetPos(),i, false, false, GetScope()), cm->Access(useFirst ? 0 : cm->GetPos()-1, i), method, winsize, map);
                }
                cm->Release(cm->GetPos(),i,false);
                cm->Release(cm->GetPos()-1,i,false);
                cm->Release(0,i,false);
            }
        }
        if (GetScope2() != 0) // both and fluorescence
        {
            for (int i=1; i<slideCount && progressDlg->Update(i+(GetScope2()==2?slideCount:0), wxString::Format(_T("Slide %d of %d"), i+1, slideCount)); i++)
            {
                oimg = cm->Access(0,i, true);
                int numContours = (int) oimg->contourArray.size();
                cm->Access(cm->GetPos(),i,true,true)->CloneContours(oimg);

                for (int j=0; j<numContours; j++)
                {
                    ProcessStatic(j, cm->Access(cm->GetPos(),i, true, false, GetScope()), cm->Access(useFirst ? 0 : cm->GetPos()-1, i), method, winsize, map);
                }
                cm->Release(cm->GetPos(),i,true);
                cm->Release(cm->GetPos()-1,i,true);
                cm->Release(0,i,true);
            }
        }
        DestroyProgressDlg();
	}
	else
	{
		FetchParams();
		ImagePlus* oimg;
		int frameCount = cm->GetFrameCount();
		int slideCount = cm->slideCount;
		CreateProgressDlg(frameCount*slideCount*(GetScope2()==2?2:1));
		bool cont=true;
		for (int slide = 0; slide < slideCount && cont; slide++)
		{
		    if (GetScope2() != 1) // both and normal
            {
                oimg = cm->Access(0, slide,false);
                for (int i=1; i<frameCount; i++)
                    cm->Access(i,slide,false,true)->CloneContours(oimg);
                int numContours = (int) oimg->contourArray.size();
                for (int i=1; i<frameCount && (cont=progressDlg->Update(slide*frameCount+i, wxString::Format(_T("Frame %d of %d, Slide %d of %d"), i+1, frameCount, slide+1, slideCount))); i++)
                {
                    for (int j=0; j<numContours; j++)
                    {
                        ProcessStatic(j, cm->Access(i,slide, false, false, GetScope()), cm->Access(useFirst ? 0 : i-1,slide), method, winsize, map);
                    }
                    cm->Release(i-1,slide,false);
                }
                cm->Release(frameCount-1,slide,false);
                cm->Release(0,slide,false);
            }
            if (GetScope2() != 0) // both and fluorescence
            {
                oimg = cm->Access(0, slide, true);
                for (int i=1; i<frameCount; i++)
                    cm->Access(i,slide,true,true)->CloneContours(oimg);
                int numContours = (int) oimg->contourArray.size();
                for (int i=1; i<frameCount && cont; i++)
                {
                    for (int j=0; j<numContours && (cont=progressDlg->Update(slide*frameCount+i+(GetScope2()==2?frameCount*slideCount:0), wxString::Format(_T("Cell %d of %d, Frame %d of %d"), j+1,numContours, i+1, frameCount))); j++)
                    {
                        ProcessStatic(j, cm->Access(i,slide, true, false, GetScope()), cm->Access(useFirst ? 0 : i-1,slide), method, winsize, map);
                    }
                    cm->Release(i-1,slide,true);
                }
                cm->Release(frameCount-1,slide,true);
                cm->Release(0,slide,true);
            }
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

