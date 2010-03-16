#include "ImproveContoursPlugin.h"
#include "Util.h"

ImproveContoursPlugin::ImproveContoursPlugin( wxWindow* parent_, MyFrame *win_ ): PluginBase(GetStaticName(), parent_, win_, true, true), gray(NULL)
{
	sidebar =  new ImproveContoursSidebar(parent_, this);
	sidebarw = sidebar;
	if (cm->viewFluorescence)
        sidebar->scope2->SetSelection(1);
	DoPreview();
}

int ImproveContoursPlugin::GetScope() {	return sidebar->scope->GetSelection(); }
int ImproveContoursPlugin::GetScope2() { return sidebar->scope2->GetSelection(); }
bool ImproveContoursPlugin::IsPreviewOn(){ return sidebar->preview->GetValue(); }

void ImproveContoursPlugin::OnFluorescence()
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

void ImproveContoursPlugin::DoPreview()
{
	if (!IsPreviewOn())
		return;
	cm->ReloadCurrentFrameContours(false);
	ProcessImage(&cm->img);
	cm->Redraw(false);
}

void ImproveContoursPlugin::ProcessImage( ImagePlus *img )
{
	float alpha=sidebar->alpha->GetValue();
	float beta= sidebar->beta->GetValue();
	float gamma=sidebar->gamma->GetValue();
	CvSize win=cvSize(2*sidebar->width->GetValue()-1, 2*sidebar->height->GetValue()-1);
	int scheme = sidebar->gradient->GetValue();
    ProcessImage_static(img, gray, alpha, beta, gamma, win, scheme, sidebar->use_blur->GetValue(), sidebar->max_iter->GetValue(), sidebar->epsilon->GetValue());

}

void ImproveContoursPlugin::ProcessImage_static(ImagePlus* img, IplImage* &gray, float alpha, float beta, float gamma, CvSize win, int scheme, bool useBlur, int max_iter, float epsilon)
{
    CvSeq *seq;
	CvPoint* ps;
    if (!gray)
		gray = cvCreateImage( cvSize(img->orig->width, img->orig->height), IPL_DEPTH_8U, 1 );
	cvCvtColor(img->orig, gray, CV_BGR2GRAY);
	if (useBlur)
    {
        IplImage* temp = cvCreateImage( cvSize(gray->width, gray->height), IPL_DEPTH_8U, 1 );
        cvCopyImage(gray, temp);
        cvSmooth(temp, gray, CV_MEDIAN, 3);
        cvReleaseImage(&temp);
    }
	for (int i=(int)img->contourArray.size()-1; i>=0; i--)
	{
		seq = img->contourArray[i];
		int np = seq->total;
		ps = (CvPoint*)malloc( np*sizeof(CvPoint) );
		cvCvtSeqToArray(seq, ps);

		uchar *data;
        CvSize size;
        int step;
		cvGetRawData( gray, &data, &step, &size );
        if( gray == NULL )
            std::cout << "Err1" << std::endl;
        if( (size.height <= 0) || (size.width <= 0) )
            std::cout << "Err2" << std::endl;
        if( step < size.width )
            std::cout << "Err3" << std::endl;
        if( ps == NULL )
            std::cout << "Err4" << std::endl;
        if( np < 3 )
        {
            std::cout << "Err5" << std::endl;
            cm->SaveContours("debugContours.txt");
        }
        if( &alpha == NULL )
            std::cout << "Err6" << std::endl;
        if( &beta == NULL )
            std::cout << "Err7" << std::endl;
        if( &gamma == NULL )
            std::cout << "Err8" << std::endl;
        if( CV_VALUE != CV_VALUE && CV_VALUE != CV_ARRAY )
            std::cout << "Err9" << std::endl;
        if( (win.height <= 0) || (!(win.height & 1)))
            std::cout << "Err10 " << win.height << std::endl;
        if( (win.width <= 0) || (!(win.width & 1)))
            std::cout << "Err11" << std::endl;


		CvTermCriteria term=cvTermCriteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, max_iter, epsilon*np);
		cvSnakeImage( gray, ps, np, &alpha, &beta, &gamma, CV_VALUE, win, term, scheme );
		img->ReplaceContour(i, ps, np);
		free(ps); ps=NULL;
	}
}
