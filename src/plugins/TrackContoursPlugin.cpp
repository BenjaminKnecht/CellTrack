#include "TrackContoursPlugin.h"
#include "Util.h"

TrackContoursPlugin::TrackContoursPlugin( wxWindow* parent_, MyFrame *win_ )
: PluginBase(GetStaticName(), parent_, win_, true, true,true),
gray(NULL), ogray(NULL), avgiterations(0)
{
	sidebar =  new TrackContoursSidebar(parent_, this);
	sidebarw = sidebar;
	DoPreview();
}
void TrackContoursPlugin::ReleaseTemps()
{
	if (gray) cvReleaseImage(&gray);
	if (ogray) cvReleaseImage(&ogray);
}
int TrackContoursPlugin::GetScope() {	return sidebar->scope->GetSelection(); }
bool TrackContoursPlugin::IsPreviewOn(){ return sidebar->preview->GetValue(); }
void TrackContoursPlugin::DoPreview()
{
	if (!IsPreviewOn())
		return;
	cm->ReloadCurrentFrameContours(false);
	ProcessImage(&cm->img, cm->GetPos(), cm->GetZPos());
	cm->Redraw(false);
	ShowProcessInfo();
}
void TrackContoursPlugin::ShowProcessInfo()
{
	win->SetStatusText(WXFORMAT(_T("avg iterations: %.1f"), avgiterations));
}
void TrackContoursPlugin::FetchParams()
{
	alpha = sidebar->alpha->GetValue();
	beta = sidebar->beta->GetValue();
	gamma = sidebar->gamma->GetValue();
	oalpha = sidebar->oalpha->GetValue();
	obeta = sidebar->obeta->GetValue();
	ogamma = sidebar->ogamma->GetValue();
	oteta = sidebar->oteta->GetValue();
	ozeta = sidebar->ozeta->GetValue();
	oomega = sidebar->oomega->GetValue();
	useAvailable = sidebar->useAvailable->GetValue();
	winsize = cvSize(2*sidebar->width->GetValue()-1, 2*sidebar->height->GetValue()-1);
	scheme = sidebar->gradient->GetValue();
	criteria = cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, sidebar->max_iter->GetValue(), sidebar->epsilon->GetValue());
}
void TrackContoursPlugin::OnOK()
{
	wxBeginBusyCursor();
	if (!GetScope()){
		ProcessImage(cm->Access(cm->GetPos(), cm->GetZPos()), cm->GetPos(), cm->GetZPos());
		ShowProcessInfo();
	}
	else
	{
		FetchParams();
		ImagePlus *oimg, *img;
		IplImage *swap;
		CvPoint *ps, *ops, *swap_ps;
		float *oEarc=NULL;
		int frameCount = cm->GetFrameCount();
        int slideCount = cm->slideCount;
		avgiterations = new float[slideCount];
		for (int slide = 0; slide < slideCount; slide++)
		{
		    avgiterations[slide]=0;
            int numContours = (int) cm->Access(0,slide,false,true)->contourArray.size();
            for (int i=1; i<cm->GetFrameCount(); i++)
            {
                if (useAvailable)
                    cm->Access(i,slide,false,true)->RemoveContours(numContours, cm->Access(i,slide,false,true)->contourArray.size()-1);
                else
                    cm->Access(i,slide,false,true)->RemoveAllContours();
            }
            CreateProgressDlg(numContours*frameCount*slideCount);
            bool cont=true;

			oimg = cm->Access(0,slide, false, false, true);
			for (int i=1; i<frameCount && cont; i++)
			{
                for (int j=0; j<numContours && (cont=progressDlg->Update(slide*slideCount+j*frameCount+i, wxString::Format(_T("Cell %d of %d, Frame %d of %d, Slide %d of %d"), j+1,numContours, i+1, frameCount, slide+1, slideCount))); j++)
                {
                    img = cm->Access(i,slide, false, false, true);
                    int iterations=0;
                    TrackContoursPlugin::ProcessStatic(j, img, oimg, gray, ogray, ps, ops, alpha, beta, gamma, oalpha, obeta, ogamma, oteta, ozeta, oomega, winsize, scheme, criteria, useAvailable, iterations,  i>1, i==cm->GetFrameCount()-1, oEarc, true);
                    avgiterations[slide]+=iterations;
                    cm->Release(i,slide,false);
                    CV_SWAP( ogray, gray, swap );
                    CV_SWAP( ops, ps, swap_ps);
                }
                if (oEarc){
                    free(oEarc);
                    oEarc = NULL;
                }
			}
			cm->Release(0,slide,false);
			if (numContours)
                avgiterations[slide]/=numContours;
		}
		DestroyProgressDlg();

	}
	cm->ReloadCurrentFrameContours(true, false);
	wxEndBusyCursor();
}


#include "cvSnakeTrack.h"
void TrackContoursPlugin::ProcessImage( ImagePlus *img, int pos, int zPos ){
	if (pos==0)
		return;
	FetchParams();
	ImagePlus *oimg = cm->Access(pos-1, zPos);
	avgiterations[zPos]=0;
	int numContours = (int) oimg->contourArray.size();
	if (useAvailable)
		img->RemoveContours(numContours, img->contourArray.size()-1);
	else
		img->RemoveAllContours();
	for (int i=0; i<numContours; i++){
		CvPoint *ps, *ops;
		int iterations=0;
		float *nullfloat=NULL;
		TrackContoursPlugin::ProcessStatic(i, img, oimg, gray, ogray, ps, ops, alpha, beta, gamma, oalpha, obeta, ogamma, oteta, ozeta, oomega, winsize, scheme, criteria, useAvailable, iterations,  false, true, nullfloat);
		avgiterations[zPos]+=iterations;
	}
	if (numContours)
		avgiterations[zPos]/=numContours;
    cm->Release(pos-1, zPos, false);
}
// is is the index in oimg.contourArray
void TrackContoursPlugin::ProcessStatic
( int i, ImagePlus *img, ImagePlus *oimg, IplImage* &gray, IplImage* &ogray, CvPoint* &ps, CvPoint* &ops,
 float alpha, float beta, float gamma,
 float oalpha, float obeta, float ogamma, float oteta, float ozeta, float oomega,
 CvSize winsize, int scheme, CvTermCriteria criteria,
 bool useAvailable, int &iterations, bool oready, bool freetemps, float* &oEarc, bool oEarc_ready){
	if (!gray)
		gray = cvCreateImage( cvSize(img->orig->width, img->orig->height), IPL_DEPTH_8U, 1 );
	if (!ogray)
		ogray = cvCreateImage( cvSize(img->orig->width, img->orig->height), IPL_DEPTH_8U, 1 );
	cvCvtColor(img->orig, gray, CV_BGR2GRAY);
	int numContours = (int) oimg->contourArray.size();
	int np = oimg->contourArray[i]->total;

	if (!oready){
		CvSeq *oseq = oimg->contourArray[i];
		cvCvtColor(oimg->orig, ogray, CV_BGR2GRAY);
		ops = (CvPoint*)malloc( np*sizeof(CvPoint) );
		cvCvtSeqToArray(oseq,ops);
		if (oEarc_ready && !oEarc){
			oEarc = (float *) malloc(np*sizeof(float));
			oEarc_ready  = false;
		}
	}
	CvSeq *seq = i<img->contourArray.size() ? img->contourArray[i] : NULL;
	bool usingAvailable = useAvailable && seq && seq->total == np;
	if (usingAvailable){
		usingAvailable = true;
		if(!oready)
			ps = (CvPoint*)malloc( np*sizeof(CvPoint) );
		cvCvtSeqToArray(seq, ps);
	}
	else
		ps = ops;
	criteria.epsilon *= np;
	iterations=0;
	cvSnakeImageTrack( gray, ps, np,
		&alpha, &beta, &gamma,
		ogray, ops, &oalpha, &obeta, &ogamma, &oteta, &ozeta, &oomega,
		CV_VALUE, winsize, criteria, scheme, oEarc, oEarc_ready, &iterations
		);
	if (useAvailable && i<img->contourArray.size())
		img->ReplaceContour(i,ps, np);
	else
		img->AddContour(ps,np);
	if (!useAvailable || img->feats[i].size() != oimg->feats[i].size())
		img->feats[i] = oimg->feats[i];
	if (freetemps){
		if (ps!=ops){
			free(ps); ps=NULL;
		}
		free(ops); ops = NULL;
	}

}
