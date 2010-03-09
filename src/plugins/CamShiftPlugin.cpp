#include "CamShiftPlugin.h"
#include "Util.h"

CamShiftPlugin::CamShiftPlugin( wxWindow* parent_, MyFrame *win_ )
: PluginBase(GetStaticName(), parent_, win_, true, true,true),
backproject(NULL), hist(NULL), rotation(NULL), shift(NULL), planes(NULL)
{
	sidebar =  new CamShiftSidebar(parent_, this);
	sidebarw = sidebar;
	DoPreview();
}
void CamShiftPlugin::ReleaseTemps()
{
	if (backproject)
	{
        cvReleaseImage(&backproject);
        backproject = NULL;
	}
	if (hist) cvReleaseHist(&hist);
	if (planes){
		for (int p=0; p<3; p++){
			if (planes[p])
				cvReleaseImage(&planes[p]);
		}
		free(planes); planes = NULL;
	}
	if (rotation) cvReleaseMat(&rotation);
	if (shift) cvReleaseMat(&shift);
}
int CamShiftPlugin::GetScope() {	return sidebar->scope->GetSelection(); }
int CamShiftPlugin::GetScope2() {	return sidebar->scope2->GetSelection(); }
bool CamShiftPlugin::IsPreviewOn(){ return sidebar->preview->GetValue(); }
void CamShiftPlugin::DoPreview()
{
	if (!IsPreviewOn())
		return;
	cm->ReloadCurrentFrameContours(false);
	ProcessImage(&cm->img, cm->GetPos(), cm->GetZPos(), cm->viewFluorescence);
	cm->Redraw(false);
}
void CamShiftPlugin::FetchParams()
{
	for (int p=0; p<3; p++)
		hsizes[p] = sidebar->hsize->GetValue();
	criteria = cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, sidebar->max_iter->GetValue(), sidebar->epsilon->GetValue());
	useFirst = sidebar->useFirst->GetValue();
}

void CamShiftPlugin::ProcessImage( ImagePlus* img, int pos, int zPos, bool fluorescence )
{
	if (pos==0)
		return;
	FetchParams();
	ImagePlus *oimg = cm->Access(useFirst ? 0 : pos-1, zPos, fluorescence);
	CvRect orect, searchwin;
	CvPoint ocenter;

	img->CloneContours(oimg);
	int numContours = (int) oimg->contourArray.size();
	for (int i=0; i<numContours; i++)
	{
		ProcessStatic(i, img, oimg, hsizes, criteria, planes, hist, backproject, orect, ocenter, searchwin, rotation, shift, false);
	}
	cm->Release(useFirst ? 0 : pos-1, zPos, fluorescence);
}
void CamShiftPlugin::OnOK()
{
	wxBeginBusyCursor();
	if (GetScope() == 0) // single
	{
		if (GetScope2() != 1) // both and normal
        {
            ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), false), cm->GetPos(), cm->GetZPos(), false);
            cm->Release(cm->GetPos(), cm->GetZPos(), false);
        }
        if (GetScope2() != 0) // both and fluorescence
        {
            ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), true), cm->GetPos(), cm->GetZPos(), true);
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
                    ProcessStatic(j, cm->Access(i,cm->GetZPos(),false,false,GetScope()), cm->Access(useFirst ? 0 : i-1, cm->GetZPos()), hsizes, criteria,
                                    planes, hist, backproject, orect, ocenter, searchwin, rotation, shift, i>1);
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
                    ProcessStatic(j, cm->Access(i,cm->GetZPos(),true,false,GetScope()), cm->Access(useFirst ? 0 : i-1, cm->GetZPos()), hsizes, criteria,
                                    planes, hist, backproject, orect, ocenter, searchwin, rotation, shift, i>1);
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
		CreateProgressDlg(slideCount);
        for (int i=0; i<slideCount && progressDlg->Update(i, wxString::Format(_T("Slide %d of %d"), i+1, slideCount)); i++)
        {
            if (GetScope2() != 1) // both and normal
            {
                oimg = cm->Access(0,i);
                int numContours = (int) oimg->contourArray.size();
                cm->Access(cm->GetPos(),i,false,true)->CloneContours(oimg);
                for (int j=0; j<numContours; j++)
                {
                    ProcessStatic(j, cm->Access(cm->GetPos(),i,false,false,GetScope()), cm->Access(useFirst ? 0 : cm->GetPos()-1, i), hsizes, criteria,
                                    planes, hist, backproject, orect, ocenter, searchwin, rotation, shift, i>1);
                }
                cm->Release(cm->GetPos(),i,false);
                cm->Release(cm->GetPos()-1,i,false);
                cm->Release(0,i,false);
            }
            if (GetScope2() != 0) // both and fluorescence
            {
                oimg = cm->Access(0,i, true);
                int numContours = (int) oimg->contourArray.size();
                cm->Access(cm->GetPos(),i,true,true)->CloneContours(oimg);

                for (int j=0; j<numContours; j++)
                {
                    ProcessStatic(j, cm->Access(cm->GetPos(),i,true,false,GetScope()), cm->Access(useFirst ? 0 : cm->GetPos()-1, i), hsizes, criteria,
                                    planes, hist, backproject, orect, ocenter, searchwin, rotation, shift, i>1);
                }
                cm->Release(cm->GetPos(),i,true);
                cm->Release(cm->GetPos()-1,i,true);
                cm->Release(0,i,true);
            }
        }
        DestroyProgressDlg();
	}
	else // all
	{
		FetchParams();
		ImagePlus *oimg;
		CvRect orect, searchwin;
		CvPoint ocenter;
		int frameCount = cm->GetFrameCount();
		int slideCount = cm->slideCount;
		CreateProgressDlg(frameCount*slideCount*(GetScope2()==2?2:1));
		bool cont=true;
		for (int slide = 0; slide < slideCount && cont; slide++)
		{
		    if (GetScope2() != 1) // both and normal
            {
                oimg = cm->Access(0,slide,false);
                for (int i=1; i<frameCount; i++)
                    cm->Access(i,slide,false,true)->CloneContours(oimg);
                int numContours = (int) oimg->contourArray.size();
                for (int i=1; i<frameCount && (cont=progressDlg->Update(slide*frameCount+i, wxString::Format(_T("Frame %d of %d, Slide %d of %d"), i+1, frameCount, slide+1, slideCount))); i++)
                {
                    for (int j=0; j<numContours; j++)
                    {
                        ProcessStatic(j, cm->Access(i,slide,false,false,GetScope()), cm->Access(useFirst ? 0 : i-1, slide), hsizes, criteria,
                                    planes, hist, backproject, orect, ocenter, searchwin, rotation, shift, i>1);
                    }
                    cm->Release(i-1,slide,false);
                }
                cm->Release(frameCount-1,slide,false);
                cm->Release(0,slide,false);
            }
            if (GetScope2() != 0) // both and fluorescence
            {
                oimg = cm->Access(0,slide, true);
                int numContours = (int) oimg->contourArray.size();
                for (int i=1; i<frameCount; i++)
                    cm->Access(i,slide,true,true)->CloneContours(oimg);
                bool cont=true;
                for (int i=1; i<frameCount && cont; i++)
                {
                    for (int j=0; j<numContours && (cont=progressDlg->Update(slide*frameCount+i+(GetScope2()==2?frameCount*slideCount:0), wxString::Format(_T("Frame %d of %d, Slide %d of %d"), i+1, frameCount, slide+1, slideCount))); j++)
                    {
                        if (GetScope2() != 0) // both and fluorescence
                        {
                            ProcessStatic(j, cm->Access(i,slide,true,false,GetScope()), cm->Access(useFirst ? 0 : i-1, slide, true), hsizes, criteria,
                                        planes, hist, backproject, orect, ocenter, searchwin, rotation, shift, i>1);
                        }
                    }
                    cm->Release(i-1,slide,false);
                }
                cm->Release(frameCount-1,slide,false);
                cm->Release(0,slide,false);

            }
		}
		DestroyProgressDlg();
	}
	cm->ReloadCurrentFrameContours(true, false);
	wxEndBusyCursor();
}

void CamShiftPlugin::OnFluorescence()
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

void CamShiftPlugin::ProcessStatic
( int i, ImagePlus *img, ImagePlus *oimg, int *hsizes, CvTermCriteria criteria,
IplImage** &planes, CvHistogram* &hist, IplImage* &backproject, CvRect &orect, CvPoint &ocenter, CvRect &searchwin, CvMat* &rotation, CvMat* &shift, bool oready){
	if (hist && hist->mat.dim[0].size!=hsizes[0])
		cvReleaseHist(&hist);
	if( !hist )
        hist = cvCreateHist( 3, hsizes, CV_HIST_ARRAY, NULL, 0);
    if( !backproject )
		backproject = cvCreateImage( cvGetSize(img->orig), IPL_DEPTH_8U, 1 );
	if( !planes ){
	    planes = (IplImage**) malloc(3 * sizeof(IplImage*));
        for (int p=0; p<3; p++)
			planes[p] = cvCreateImage( cvGetSize(img->orig), 8, 1 );
	}
	if (!rotation)
		rotation = cvCreateMat(2,3,CV_32FC1);
	if (!shift)
		shift = cvCreateMat(2,1,CV_32FC1);

	if (!oready)
	{
		orect = cvBoundingRect(oimg->contourArray[i],1);
		cvCvtPixToPlane( oimg->orig, planes[0], planes[1], planes[2], 0 );
        for (int p=0; p<3; p++)
            cvSetImageROI(planes[p],orect);
        cvCalcHist( planes, hist, 0, NULL );
		cvNormalizeHist(hist, 255);
        for (int p=0; p<3; p++)
            cvResetImageROI(planes[p]);
		searchwin = orect; //cvRect(0,0,img->orig->width, img->orig->height);
		ocenter = cvPoint(orect.x+orect.width/2, orect.y+orect.height/2);
	}
	//The following checks shouldn't be needed.
	RestrictRect(searchwin, cvRect(0,0,backproject->width,backproject->height));

	cvCvtPixToPlane( img->orig, planes[0], planes[1], planes[2], 0 );
    cvCalcBackProject( planes, backproject, hist );
	CvBox2D track_box;
	CvConnectedComp track_comp;
    cvCamShift( backproject, searchwin,
                criteria,
                &track_comp, &track_box );
	searchwin = track_comp.rect;
	cvmSet(shift,0,0,track_box.center.x - ocenter.x);
	cvmSet(shift,1,0,track_box.center.y - ocenter.y);
//	shift->data.fl[0] = track_box.center.x - ocenter.x;
//	shift->data.fl[1] = track_box.center.y - ocenter.y;
	cv2DRotationMatrix(track_box.center, track_box.angle, 1.0, rotation);
	cvTransform(oimg->contourArray[i],img->contourArray[i],rotation,shift);
//	CvMat *ofm = FeatPointsToMat(oimg->feats[i]);
//	Cvmat *fm  = FeatPointsToMat(img->feats[i]);
//	cvTransform(ofm,img->contourArray[i],rotation,shift);
	TransformFeatPoints(oimg->feats[i], img->feats[i], rotation, shift);
}
