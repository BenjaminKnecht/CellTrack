#include "CaptureManager.h"
#include "Preferences.h"
#include "Util.h"
#include <iostream>
#include <fstream>
#include <set>

CaptureManager::CaptureManager(void)
: book(NULL), canvas(NULL), RedrawListener(NULL), BookChangeListener(NULL), ReloadListener(NULL)
{ Reset(); }
CaptureManager::~CaptureManager(void){
	Reset();
}
void CaptureManager::Reset(){
	int i;
	if (book) {
		for (i=0; i<totalFrameCount; i++){
			delete book[i];
		}
		delete[] book;
	}
	frameCount = 0;
	slideCount = 1;
	hasFluorescence = false;
	viewFluorescence = false;
	totalFrameCount = 0;
	offset = 1;
	fps = 0;
	pos = 0;
	zPos = 0;
	loadedImgs = 0;
	size = cvSize(0, 0);
	img.ReleaseAll();
	loadRadius = 4;
	drawFluorescence = false;
	drawTopBorder = false;
	drawBottomBorder = false;
}
void CaptureManager::SetCanvas(MyCanvas *canvas_){
	canvas = canvas_;
	canvas->SetCM(this);
}
MyCanvas* CaptureManager::GetCanvas()
{
	return canvas;
}

bool CaptureManager::OpenMovie(const char* avi){
	Reset();
	MyCapture_Movie capture(avi);
	return OpenMovie_initialize(capture);
}
bool CaptureManager::OpenMovie( const wxArrayString& files )
{
	Reset();
	MyCapture_Files capture(files);
	return OpenMovie_initialize(capture);
}
bool CaptureManager::OpenConfocal(const wxArrayString &files, int zSlides, bool fluorescence)
{
    Reset();
    m_capture = new MyCapture_Confocal(files, zSlides, fluorescence);
    return OpenConfocal_initialize();
}

void CaptureManager::SetQueue(ImageJobQueue* queue)
{
    m_queue = queue;
}

#include <wx/progdlg.h>
bool CaptureManager::OpenMovie_initialize(MyCapture &capture)
{
	if (!capture.IsOK())
		return false;
	frameCount = capture.frameCount;
	totalFrameCount = frameCount;
	slideCount = 1;
	fps = capture.fps ? capture.fps : Preferences::GetSavingFpsDefault();
	book = new ImagePlus*[frameCount];
	wxProgressDialog progressDlg(_T("Loading movie..."), wxString::Format(_T("Frame 0 of %d"), frameCount),frameCount, NULL, wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE);
	for (int i=0; i<totalFrameCount; i++){
		progressDlg.Update(i+1, wxString::Format(_T("Frame %d of %d"), i+1, totalFrameCount));
		IplImage *newframe = capture.queryFrame(i);
		IplImage *resized;
		if (i == 0){
			size = cvSize(newframe->width, newframe->height);
			book[i] = new ImagePlus(newframe);
		}
		else if(!cvSizeEquals(newframe, size)){
			resized = cvCloneImage(book[0]->orig);
			cvResize(newframe, resized);
			book[i] = new ImagePlus(resized);
			cvReleaseImage(&resized);
		}
		else
			book[i] = new ImagePlus(newframe);
	}
	if (BookChangeListener)
		BookChangeListener->OnBookChange();
	SetPos(0);
	return true;
}

bool CaptureManager::OpenConfocal_initialize()
{
	if (!m_capture->IsOK())
		return false;
	slideCount = m_capture->getSlideNumber();
	hasFluorescence = m_capture->getFluorescence();
	frameCount = m_capture->frameCount/(hasFluorescence?2:1)/slideCount;
	totalFrameCount = frameCount*(hasFluorescence?2:1)*slideCount;
	offset = (hasFluorescence?2:1)*slideCount;
	fps = Preferences::GetSavingFpsDefault();
	book = new ImagePlus*[totalFrameCount];
	IplImage* testImg = NULL;
	if (testImg = cvLoadImage((m_capture->getFilename(0)).mb_str()))
    {
        size = cvSize(testImg->width, testImg->height);
        cvReleaseImage(&testImg);
        testImg = NULL;
    }
	/*wxProgressDialog progressDlg(_T("Loading movie..."), wxString::Format(_T("Frame 0 of %d"), totalFrameCount),totalFrameCount, NULL, wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE);
	for (int i=0; i<totalFrameCount; i++){
        if (i == 31)
            std::cout << "loading frame .. " << i << std::endl;
		progressDlg.Update(i+1, wxString::Format(_T("Frame %d of %d"), i+1, totalFrameCount));
		IplImage *newframe = capture.queryFrame(i);
		IplImage *resized;
		if (i == 0){
			size = cvSize(newframe->width, newframe->height);
			book[1] = new ImagePlus(newframe);
		}
		else if(!cvSizeEquals(newframe, size)){
			resized = cvCloneImage(book[1]->orig);
			cvResize(newframe, resized);
			book[i+(i%2 == 1 ? 1 : -1)] = new ImagePlus(resized);
			cvReleaseImage(&resized);
		}
		else
			book[i+(i%2 == 1 ? 1 : -1)] = new ImagePlus(newframe);
	}*/
	for (int i=0; i<totalFrameCount; i++)
	{
	    book[i] = new ImagePlus(i%2==0?hasFluorescence:false);
	}
	if (BookChangeListener)
		BookChangeListener->OnBookChange();
	return true;
}


bool CaptureManager::SaveMovie(const char* avi){
	bool resize = false;
	CvSize newsize = size;
	if ( Preferences::GetSavingSizeOverride() && !cvSizeEquals(Preferences::GetSavingSize(), size) ){
		resize = true;
		newsize = Preferences::GetSavingSize();
	}
	CvVideoWriter* writer = cvCreateVideoWriter(avi,
		Preferences::GetSavingCodec(),
		Preferences::GetSavingFpsOverride() || !fps ? Preferences::GetSavingFpsDefault() : fps,
		newsize, 1);
	IplImage *resized;
	if (resize)
		resized = cvCreateImage(newsize,8,3);
	IplImage *frame_flip = cvCreateImage(newsize,8,3);
	wxProgressDialog progressDlg(_T("Saving movie..."), wxString::Format(_T("Frame 0 of %d"), frameCount),frameCount, NULL, wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE);
	for (int i=0; i<frameCount; i++) {
		progressDlg.Update(i+1, wxString::Format(_T("Frame %d of %d"), i+1, frameCount));
		if (resize)
			cvResize(book[i*offset]->ToIplImage(), resized);
		else
			resized = book[i*offset]->ToIplImage();
		cvConvertImage( resized, frame_flip, CV_CVTIMG_SWAP_RB );
		cvWriteFrame(writer, frame_flip);
	}
	cvReleaseVideoWriter(&writer);
	cvReleaseImage(&frame_flip);
	frame_flip = NULL;
	if (resize)
		cvReleaseImage(&resized);
	return true;
}
//TODO: Check who uses this and how to change accordingly
int CaptureManager::GetFrameCount(){
	return frameCount;
}
CvSize CaptureManager::GetSize(){
	return size;
}
int CaptureManager::GetPos(){
	return pos;
}
int CaptureManager::GetZPos(){
	return zPos;
}
int CaptureManager::GetTotalPos()
{
	return CalculateDirect(pos, zPos, viewFluorescence);
}

int CaptureManager::CalculateDirect(int p, int z, bool fluorescence)
{
    return p*offset+z*(hasFluorescence?2:1) + ((hasFluorescence&&fluorescence)?0:1);
}

void CaptureManager::Release(int p, int z, bool fluorescence)
{
    // unload previous loaded image
    if (!IsInNeighborhood(p, z))
    {

        int direct = CalculateDirect(p, z, fluorescence);
        if (book[direct]->isLoading)
            book[direct]->isLoading = false;
        else
            m_queue->AddJob(Job(Job::thread_delete, direct, book[direct]->orig), 1);
        book[direct]->orig = NULL;
    }
}

ImagePlus* CaptureManager::Access(int p, int z, bool fluorescence, bool noImage, bool preload)
{
    if (p < 0 || p >= frameCount || z < 0 || z >= slideCount)
        return NULL;

    // preload next three pictures
    if (preload)
    {
        for (int i = 1; i <= 3; i++)
        {
            int newpos = i + p;
            int newZPos = z + newpos/frameCount;
            if (newZPos <= slideCount)
            {
                if (newpos >= frameCount)
                    newpos = newpos % frameCount;
                int direct = CalculateDirect(newpos, newZPos, fluorescence);
                if (!book[direct]->orig && !book[direct]->isLoading)
                {
                    m_queue->AddJob(Job(Job::thread_load, direct, m_capture->getFilename(direct)), 1);
                    book[direct]->isLoading = true;
                }
            }
        }
    }
    int direct = CalculateDirect(p, z, fluorescence);
    ImagePlus* temp = book[direct];
    if (!temp->orig && !noImage)
        temp->orig = cvLoadImage(m_capture->getFilename(direct).mb_str());
    return temp;
}

ImagePlus* CaptureManager::DirectAccess(int x)
{
    return book[x];
}

void CaptureManager::ReloadCurrentFrame(bool redraw, bool callPlugin)
{
	if (callPlugin && ReloadListener)
		ReloadListener->OnReload();
	img = *book[GetTotalPos()];
	if (redraw && img.orig)
		Redraw(callPlugin);
}

bool CaptureManager::IsInNeighborhood(int testPos, int testZPos) const
{
    return (testPos >= 0 && testPos < frameCount && testZPos >= 0 && testZPos < slideCount && abs(testPos - pos) + abs(testZPos - zPos) <= loadRadius);
}

void CaptureManager::LoadNeighborhood(int newpos, int newZPos)
{
    //std::cout << "Queue Size: " << m_queue->GetLength() << std::endl;
    //std::cout << "Loading: " << newpos << " " << newZPos << std::endl;
    std::set<std::pair<int,int> > toLoad;
    std::set<int> toUnload;
    for (int i = pos - loadRadius; i<loadRadius+pos+1; i++)
    {
        for (int j = zPos - (loadRadius - abs(i - pos)); j < zPos + (loadRadius - abs(i - pos)) + 1; j++)
        {
            if (i >= 0 && i < frameCount && j >= 0 && j < slideCount)
            {
                toUnload.insert(i*offset + j*(hasFluorescence?2:1));
            }
        }
    }

    for (int i = newpos - loadRadius; i<loadRadius+newpos+1; i++)
    {
        for (int j = newZPos - (loadRadius - abs(i - newpos)); j < newZPos + (loadRadius - abs(i - newpos)) + 1; j++)
        {
            if (i >= 0 && i < frameCount && j >= 0 && j < slideCount)
            {
                std::set<int>::iterator it = toUnload.find(i*offset + j*(hasFluorescence?2:1));
                if (it != toUnload.end())
                    toUnload.erase(it);
                // No else because of initial state when nothing is loaded yet.
                // Later I will catch the case of an already loaded image anyway.
                toLoad.insert(std::pair<int,int>(i*offset + j*(hasFluorescence?2:1), 2 + abs(newpos - i) + abs(newZPos - j)));
            }
        }
    }

    for (std::set<int>::iterator it = toUnload.begin(); it != toUnload.end(); it++)
    {
        if (book[*it]->isLoading)
            book[*it]->isLoading = false;
        else
            m_queue->AddJob(Job(Job::thread_delete, *it, book[*it]->orig), 2);
        book[*it]->orig = NULL;

        if (book[*it + 1]->isLoading)
            book[*it + 1]->isLoading = false;
        else
            m_queue->AddJob(Job(Job::thread_delete, *it + 1, book[*it + 1]->orig), 2 + (viewFluorescence?1:-1));
        book[*it + 1]->orig = NULL;
    }

    for (std::set<std::pair<int,int> >::iterator it = toLoad.begin(); it != toLoad.end(); it++)
    {
        if (!book[it->first]->orig && !book[it->first]->isLoading)
        {
            m_queue->AddJob(Job(Job::thread_load, it->first, m_capture->getFilename(it->first)), it->second);
            book[it->first]->isLoading = true;
        }
        if (!book[it->first + 1]->orig && !book[it->first + 1]->isLoading)
        {
            m_queue->AddJob(Job(Job::thread_load, it->first + 1, m_capture->getFilename(it->first + 1)), it->second);
            book[it->first + 1]->isLoading = true;
        }
    }

    loadedImgs + (toLoad.size() - toUnload.size());

}

void CaptureManager::ReloadCurrentFrameContours(bool redraw, bool callPlugin){
	if (callPlugin && ReloadListener)
		ReloadListener->OnReload();
	img.CloneContours(book[GetTotalPos()]);
	if (redraw)
		Redraw(callPlugin);
}
void CaptureManager::PushbackCurrentFrame()
{
	*(book[GetTotalPos()]) = img;
}
bool CaptureManager::SetPos(int newpos, bool reload){
	if (newpos<0 || newpos>=frameCount || (newpos==pos && img.orig && !reload))
		return false;
    LoadNeighborhood(newpos, zPos);
	pos = newpos;
	ReloadCurrentFrame();
	return true;
}
bool CaptureManager::SetZPos(int newpos, bool reload){
	if (newpos<0 || newpos>=slideCount || (newpos==zPos && img.orig && !reload))
		return false;
    LoadNeighborhood(pos, newpos);
	zPos = newpos;
	ReloadCurrentFrame();
	return true;
}
bool CaptureManager::SetPos(int newpos, int newZPos, bool reload){
	if (newpos<0 || newpos>=frameCount || newZPos<0 || newZPos>=slideCount
        || (newZPos==zPos && newpos==pos && img.orig && !reload))
		return false;
    LoadNeighborhood(newpos, newZPos);
	pos = newpos;
	zPos = newZPos;
	ReloadCurrentFrame();
	return true;
}
bool CaptureManager::OnDeleteBefore()
{
	if (!pos)
		return false;
	for (int i=0; i<pos*offset; i++)
		delete book[i];
	for (int i=0; i<totalFrameCount-pos*offset; i++)
		book[i] = book[i+pos*offset];
	frameCount-=pos;
	totalFrameCount-=pos*offset;
	SetPos(0,true);
	if (BookChangeListener)
		BookChangeListener->OnBookChange();
	return true;
}
bool CaptureManager::OnDeleteAfter()
{
	for (int i=(pos+1)*offset; i<totalFrameCount; i++)
		delete book[i];
	frameCount=pos+1;
	totalFrameCount=(pos+1)*offset;
	if (BookChangeListener)
		BookChangeListener->OnBookChange();
	return true;
}
bool CaptureManager::OnDelete()
{
	if (!frameCount)
		return false;
	if (frameCount == 1){
		wxLogError(_T("Cannot delete the last and only frame."));
		return false;
	}
	ImagePlus* todelete[offset];
	for (int i = pos*offset; i<(pos+1)*offset; i++)
	{
	    todelete[i/pos] = book[i];
	}
	int i;
	for (i=pos*offset; i<totalFrameCount-offset; i++)
		book[i]=book[i+1];
    for (;i<totalFrameCount;i++)
        book[i] = NULL;
	frameCount--;
	totalFrameCount-=offset;
	if (BookChangeListener)
		BookChangeListener->OnBookChange();
	SetPos(pos==frameCount ? frameCount-1 : pos, true);
	for (int j = 0; j<offset; i++)
        delete todelete[i];
	return true;
}

bool CaptureManager::OnPrev()
{
	return SetPos(pos-1);
}

bool CaptureManager::OnNext()
{
	return SetPos(pos+1);
}

bool CaptureManager::ShowFluorescence(bool show)
{
    viewFluorescence = show;
    ReloadCurrentFrame();
}

void CaptureManager::Redraw(bool callPlugin)
{
	if (!canvas)
		return;
//	canvas->SetImage(img.ToWxImage());
	if (callPlugin && RedrawListener)
		RedrawListener->OnRedraw();
	canvas->SetImage(img);
}

void CaptureManager::SetRedrawListener( PluginBase* RedrawListener_ )
{
	if (RedrawListener && RedrawListener_)
		wxLogError(_T("canvas already had a redraw listener."));
	RedrawListener = RedrawListener_;
}
void CaptureManager::SetReloadListener( PluginBase* ReloadListener_ )
{
	if (ReloadListener && ReloadListener_)
		wxLogError(_T("canvas already had a Reload listener."));
	ReloadListener = ReloadListener_;
}
void CaptureManager::SetBookChangeListener( PluginBase* BookChangeListener_ )
{
	if (BookChangeListener && BookChangeListener_)
		wxLogError(_T("canvas already had a BookChange listener."));
	BookChangeListener = BookChangeListener_;
}


#include "MyPoint.h"
std::vector<float> CaptureManager::GetSpeeds(int c, float &totalDisp, float &avgSpeed)
{
	std::vector<CvPoint> traj = GetTrajectory(c);
	std::vector<float> s(frameCount-1, 0.0);
	totalDisp = 0;
	int goodSteps = 0;
	for (int i=0; i<frameCount-1; i++){
		if (!(MyPoint(-1,-1)==traj[i] || MyPoint(-1,-1)==traj[i+1]))
		{
			goodSteps++;
			totalDisp += (s[i] = MyPoint(traj[i]).Euclidean(MyPoint(traj[i+1])));
		}
	}
	avgSpeed = (goodSteps ? totalDisp/goodSteps : 0);
	return s;
}

std::vector<float> CaptureManager::GetAreas(int c, float &avgArea)
{
	std::vector<float> a(frameCount, 0.0);
	int goodSteps = 0;
	float totalArea = 0.0;
	for (int i=0; i<frameCount; i++)
	{
		if (book[i*offset]->contourArray.size() > c)
		{
			goodSteps++;
			totalArea += (a[i]=fabs(cvContourArea(book[i*offset]->contourArray[c])));
		}
	}
	avgArea = (goodSteps ? totalArea/goodSteps : 0);
	return a;
}

std::vector<float> CaptureManager::GetAreaDiff(int c, float &avgDiff){
	std::vector<float> a = GetAreas(c,avgDiff);
	std::vector<float> diff(frameCount-1, 0.0);
	float totalDiff = 0;
	int goodSteps = 0;
	for (int i=0; i<frameCount-1; i++){
		if (book[i*offset]->contourArray.size() > c){
			goodSteps++;
			totalDiff += (diff[i] = a[i+1]-a[i]);
		}
	}
	avgDiff = (goodSteps ? totalDiff/goodSteps : 0);
	return diff;
}
#include "FindContoursPlugin.h"
std::vector<float> CaptureManager::GetDeformation(int c, float &avgDef){
	std::vector<CvPoint> traj = GetTrajectory(c);
	std::vector<float> areas = GetAreas(c,avgDef);
	std::vector<float> defs(frameCount-1, 0.0);
	float totalDef = 0;
	int goodSteps = 0;
	CvSeq *h_next;
	ImagePlus *img_ = new ImagePlus(img);
	IplImage *gray = cvCreateImage(cvGetSize(img.orig), IPL_DEPTH_8U, 1);
	IplImage *edge = cvCreateImage(cvGetSize(img.orig), IPL_DEPTH_8U, 1);
	for (int i=0; i<frameCount-1; i++)
	{
		if (!(MyPoint(-1,-1)==traj[i] || MyPoint(-1,-1)==traj[i+1]))
		{
			wxPoint *ps = ContourToPointArray(book[i]->contourArray[c], MyPoint(traj[i+1])-MyPoint(traj[i]).ToWxPoint());
			img_->RemoveAllContours();
			img_->AddContour(ps,book[i*offset]->contourArray[c]->total);
			delete[] ps;

			CvSeq *seq = book[i+1]->contourArray[c];
			CvSeq *oseq = img_->contourArray[0];
			//Draw both contours on the temporary image
			cvZero(img_->orig);
			h_next = seq->h_next; seq->h_next = NULL;
			cvDrawContours(img_->orig, seq, CV_RGB(255,255,255), CV_RGB(0,0,0), 1, CV_FILLED, CV_AA, cvPoint(0,0));
			seq->h_next = h_next;
			cvDrawContours(img_->orig, oseq, CV_RGB(255,255,200), CV_RGB(0,0,0), 1, CV_FILLED, CV_AA, cvPoint(0,0));

			//detect contours on the drawn image:
			FindContoursPlugin::ProcessImage_static(img_,gray,edge,150,50,3,1);
			float unionArea = 0;
			for (int j=0; j<img_->contourArray.size(); j++){
				unionArea += fabs(cvContourArea(img_->contourArray[j]));
			}

			goodSteps++;
			totalDef += (defs[i] = 2*unionArea - areas[i] - areas[i+1]);
		}
	}
	cvReleaseImage(&gray);
	cvReleaseImage(&edge);
	delete img_;
	avgDef = (goodSteps ? totalDef/goodSteps : 0);
	return defs;
}
std::vector<CvPoint> CaptureManager::GetTrajectory(int c){
	std::vector<CvPoint> traj(frameCount);
	for (int i=0; i<frameCount; i++){
		if (book[i*offset]->contourArray.size() <= c)
			traj[i]=cvPoint(-1,-1);
		else{
			CvMoments m;
			double m00,m10,m01;
			cvMoments(book[i*offset]->contourArray[c], &m);
			m00 = cvGetSpatialMoment(&m,0,0);
			m01 = cvGetSpatialMoment(&m,0,1);
			m10 = cvGetSpatialMoment(&m,1,0);
			traj[i] = cvPoint(cvRound(m.m10/m.m00), cvRound(m.m01/m.m00));
		}
	}
	return traj;
}


#include <stdio.h>
bool CaptureManager::SaveData_setup(const char* file, FILE* &fp){
	if (!book[0]->contourArray.size()){
		wxLogError(_T("No objects in the first frame. Detect/draw boundaries in the first frame and apply tracking first."));
		return false;
	}
	if (!(fp = fopen(file,"w"))){
		wxLogError(_T("Unable to open file %s"), file);
		return false;
	}
	return true;
}

bool CaptureManager::SaveTrackData(const char* file) {
	FILE *fp;
	if(!SaveData_setup(file,fp)) return false;

	int numContours=book[0]->contourArray.size();
	fprintf(fp, "#width: %d, height: %d, frameCount: %d, fps: %d\n", size.width, size.height, frameCount, numContours, fps);
	fprintf(fp, "#cellCount: %d\n", numContours);
	for (int c=0; c<numContours; c++){
		CvSeq *oseq = book[0]->contourArray[c];
		int np = oseq->total;
		fprintf(fp, "#Cell: %d, pointCount: %d\n", c+1, np);
		for (int j=0; j<frameCount; j++){
			for (int i=0; i<np; i++){
				if(book[j*offset]->contourArray.size() <= c || book[j*offset]->contourArray[c]->total <= i)
					fprintf(fp,"-1 -1 ");
				else{
					CvPoint *p = (CvPoint*) cvGetSeqElem(book[j*offset]->contourArray[c], i);
					fprintf(fp, "%d\t%d\t", p->x, p->y);
				}
			}
			fprintf(fp,"\n");
		}
	}
	fclose(fp);
	return true;
}
bool CaptureManager::ImportTrackData(const char* file) {
	FILE *fp = fopen(file,"r");
	if (!fp){
		wxLogError(_T("Unable to open file %s"), file);
		return false;
	}

	//check if there's already some detected cells.
	bool hasCells = false;
	for( int i=0; i<frameCount; i++ ){
		if (book[i*offset]->contourArray.size()){
			hasCells = true;
			break;
		}
	}
	if (hasCells){
		int reply = wxMessageBox(_T("There are already detected cells. Do you want to remove existing cells before importing tracking data?"), _T("Remove existing cells?"), wxYES_NO | wxCANCEL, NULL);
		if (reply == wxCANCEL)
			return false;
		else if(reply == wxYES){
			for( int i=0; i<frameCount; i++ )
				book[i*offset]->RemoveAllContours();
		}
	}
	//TODO: give notice if the movie info of the track-data does not match with current movie.
	int NumCells;
	int Width, Height, FrameCount, FSP;
	if( fscanf(fp, "#width: %d, height: %d, frameCount: %d, fps: %d\n#cellCount: %d\n", &Width, &Height, &FrameCount, &FSP, &NumCells) !=5 ) {
		wxLogError(_T("Unable to parse file header %s"), file);	return false;
	}
	for (int c=0; c<NumCells; c++){
		int cellid,np;
		if( fscanf(fp, "#Cell: %d, pointCount: %d\n", &cellid, &np) !=2 ) {
			wxLogError(_T("Unable to parse file %s for cell %d header %s"), file, c+1);	return false;
		}
		for (int j=0; j<frameCount && j<FrameCount; j++) {
			std::vector<wxPoint> roi(np);
			for (int i=0; i<np; i++){
				if( fscanf(fp, "%d %d", &roi[i].x, &roi[i].y) !=2 ) {
					wxLogError(_T("Unable to parse file %s for cell %d coordinates"), file, c+1);	return false;
				}
			}
			book[j*offset]->AddRoi(roi);
		}
	}
	fclose(fp);
	this->ReloadCurrentFrameContours();
	return true;
}

bool CaptureManager::SaveContours(const char* file) {
	std::ofstream outfile;
	outfile.open(file, std::ofstream::out);
	if (!outfile)
	{
		wxLogError(_T("Unable to open file %s"), file);
		return false;
	}

	//int numContours=book[0]->contourArray.size();
	outfile << "#width: " << size.width << ", height: " << size.height << ", frameCount: " << frameCount << ", slideCount: " << slideCount << ", fluorescence: " << (hasFluorescence?"yes":"no") << ", fps: " << fps << std::endl;
	for (int frame=0; frame < frameCount; frame++)
	{
	    for (int slide=0; slide < slideCount; slide++)
        {
            for (int fluor=0; fluor < (hasFluorescence?2:1); fluor++)
            {
                int direct = CalculateDirect(frame, slide, (fluor==0?true:false));
                int numContours = book[direct]->contourArray.size();

                outfile << "#Frame: " << frame << ", Slide: " << slide << ", Fluorescence: " << (hasFluorescence&&fluor==0?"yes":"no") << ", Cell count: " << numContours << std::endl;
                for (int cell = 0; cell < numContours; cell++)
                {
                    CvSeq *seq = book[direct]->contourArray[cell];
                    int numPoints = seq->total;
                    outfile << "#Cell: " << cell+1 << ", Point Count: " << numPoints << std::endl;
                    for (int i=0; i<numPoints; i++)
                    {
                        CvPoint* point = (CvPoint*) cvGetSeqElem(seq, i);
                        outfile << point->x << " " << point->y << " ";
                    }
                    outfile << std::endl;
                }
            }
        }
	}
	outfile.flush();
	outfile.close();
	return true;
}

bool CaptureManager::ImportContours(const char* file)
{
	std::ifstream infile;
	//std::cout << "opening: " << file << std::endl;
	infile.open(file, std::ifstream::in);
	if (!infile)
	{
		wxLogError(_T("Unable to open file %s"), file);
		return false;
	}


	//check if there's already some detected cells.
	bool hasCells = false;
	for( int j=0; j<slideCount; j++)
	{
        for( int i=0; i<frameCount; i++ )
        {
            for( int k=0; k<2; k++ )
            {
                if (book[CalculateDirect(i,j,(k==0?false:true))]->contourArray.size())
                {
                    hasCells = true;
                    break;
                }
            }
        }
	}
	if (hasCells)
	{
		int reply = wxMessageBox(_T("There are already detected cells. Do you want to remove existing cells before importing tracking data?"), _T("Remove existing cells?"), wxYES_NO | wxCANCEL, NULL);
		if (reply == wxCANCEL)
			return false;
		else if(reply == wxYES)
		{
			for( int j=0; j<slideCount; j++)
                for( int i=0; i<frameCount; i++ )
                    for( int k=0; k<2; k++ )
                        book[CalculateDirect(i,j,(k==0?false:true))]->RemoveAllContours();
		}
	}
	//TODO: give notice if the movie info of the track-data does not match with current movie.
	int NumCells;
	int Width, Height, FrameCount, SlideCount, FPS;
	std::string ignStr, flString;
	infile >> ignStr >> Width >> ignStr >> ignStr >> Height >> ignStr >> ignStr >> FrameCount >> ignStr >> ignStr >> SlideCount >> ignStr >> ignStr >> flString >> ignStr >> FPS;
	//std::cout << Width << " " << Height << " " << FrameCount << " " << SlideCount << " " << flString << " " << FPS << std::endl;

	bool fluor = (flString.compare("yes,") == 0);

	if (Width != size.width || Height != size.height || FrameCount != frameCount || SlideCount != slideCount || fluor != hasFluorescence || FPS != fps)
	{
        int reply = wxMessageBox(_T("The contours to be loaded do not fit the current image series. Proceed?"), _T("Proceed?"), wxYES_NO | wxCANCEL, NULL);
        if (reply == wxCANCEL)
                return false;
	}
	int frame = -1, slide = -1, numContours = -1;
	bool noErrors = true;
	while (noErrors && !infile.eof())
	{
	    flString.clear();
	    infile >> ignStr >> frame >> ignStr >> ignStr >> slide >> ignStr >> ignStr >> flString >> ignStr >> ignStr >> numContours;
	    //std::cout << frame << " " << slide << " " << flString << " " << numContours << " " << std::endl;
        if (frame >= 0 && frame < frameCount && slide >= 0 && slide < slideCount && noErrors)
        {
            for (int cell=0; cell<numContours && noErrors; cell++)
            {
                int cellid,numPoints;
                /*if(cell >= 1)
                {
                    char* buffer = (char*) malloc (sizeof(char)*1);
                    fread (buffer,1,1,fp);
                    std::cout << cell << " -- " << buffer << std::endl;
                }*/
                infile >> ignStr >> cellid >> ignStr >> ignStr >> ignStr >> numPoints;
                std::vector<wxPoint> roi(numPoints);
                for (int i=0; i<numPoints && noErrors; i++)
                {
                    infile >> roi[i].x >> roi[i].y;
                    //std::cout << roi[i].x << " " << roi[i].y << std::endl;
                }

                if (noErrors)
                    book[CalculateDirect(frame, slide, (flString.compare("yes,") == 0))]->AddRoi(roi);
            }
        }
	}
	infile.close();
	this->ReloadCurrentFrameContours();
	return true;
}

bool CaptureManager::SaveTrajectoryData(const char* file) {
	FILE *fp;
	if(!SaveData_setup(file,fp)) return false;

	int numContours=book[0]->contourArray.size();
	fprintf(fp, "#width: %d, height: %d, frameCount: %d, fps: %d\n", size.width, size.height, frameCount, numContours, fps);
	fprintf(fp, "#cellCount: %d\n", numContours);
	for (int c=0; c<numContours; c++){
		CvSeq *oseq = book[0]->contourArray[c];
		int np = oseq->total;
		fprintf(fp, "#Cell: %d, pointCount: %d\n", c+1, np);
		std::vector<CvPoint> traj = GetTrajectory(c);
		for (int i=0; i<traj.size(); i++){
			fprintf(fp, "%d\t%d\n", traj[i].x, traj[i].y);
		}
	}
	fclose(fp);
	return true;
}

bool CaptureManager::SaveSpeedData(const char* file)
{
	FILE *fp;
	if(!SaveData_setup(file,fp)) return false;

	int numContours=book[0]->contourArray.size();
	fprintf(fp, "#width: %d, height: %d, frameCount: %d, fps: %d\n", size.width, size.height, frameCount, numContours, fps);
	fprintf(fp, "#cellCount: %d\n", numContours);
	for (int c=0; c<numContours; c++)
	{
		CvSeq *oseq = book[0]->contourArray[c];
		int np = oseq->total;
		float totalDisp, avgSpeed;
		std::vector<float> speeds = GetSpeeds(c, totalDisp, avgSpeed);

		fprintf(fp, "#Cell: %d, pointCount: %d, totalDistance: %f, avgSped: %f\n", c+1, np, totalDisp, avgSpeed);
		for (int i=0; i<speeds.size(); i++){
			fprintf(fp, "%f\n", speeds[i]);
		}
	}
	fclose(fp);
	return true;
}

bool CaptureManager::SaveAreaData(const char* file)
{
	FILE *fp;
	if(!SaveData_setup(file,fp)) return false;

	int numContours=book[0]->contourArray.size();
	fprintf(fp, "#width: %d, height: %d, frameCount: %d, fps: %d\n", size.width, size.height, frameCount, numContours, fps);
	fprintf(fp, "#cellCount: %d\n", numContours);
	for (int c=0; c<numContours; c++){
		CvSeq *oseq = book[0]->contourArray[c];
		int np = oseq->total;
		float avgArea;
		std::vector<float> areas = GetAreas(c, avgArea);

		fprintf(fp, "#Cell: %d, pointCount: %d, avgArea: %f\n", c+1, np, avgArea);
		for (int i=0; i<areas.size(); i++){
			fprintf(fp, "%f\n", areas[i]);
		}
	}
	fclose(fp);
	return true;
}
bool CaptureManager::SaveDeformationData(const char* file) {
	FILE *fp;
	if(!SaveData_setup(file,fp)) return false;

	int numContours=book[0]->contourArray.size();
	fprintf(fp, "#width: %d, height: %d, frameCount: %d, fps: %d\n", size.width, size.height, frameCount, numContours, fps);
	fprintf(fp, "#cellCount: %d\n", numContours);
	for (int c=0; c<numContours; c++){
		CvSeq *oseq = book[0]->contourArray[c];
		int np = oseq->total;
		float avgDef;
		std::vector<float> defs = GetDeformation(c, avgDef);

		fprintf(fp, "#Cell: %d, pointCount: %d, avgDeformation: %f\n", c+1, np, avgDef);
		for (int i=0; i<defs.size(); i++){
			fprintf(fp, "%f\n", defs[i]);
		}
	}
	fclose(fp);
	return true;
}

#include <wx/bitmap.h>
#include <wx/dc.h>
bool CaptureManager::SaveTrackImage(wxBitmap &bmp)
{
	if (!book[0]->contourArray.size())
	{
		wxLogError(_T("No objects in the first frame. Detect/draw boundaries in the first frame and apply tracking first."));
		return false;
	}
	wxRealPoint scale(6,6);
	bmp.Create(scale.x*size.width,scale.y*size.height);
	wxMemoryDC dc(bmp);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();
	for (int c=0; c<book[0]->contourArray.size(); c++){
		CvSeq *oseq = book[0]->contourArray[c];
		MyCanvas::DrawContour_static(&dc, oseq,wxPoint(0,0),scale);
		for (int i=0; i<oseq->total; i++){
			CvPoint *lastLoc = (CvPoint*) cvGetSeqElem(oseq, i);
			dc.SetPen(wxPen(wxColour(Preferences::GetColorContourCornerColor())));
			dc.SetBrush(*wxTRANSPARENT_BRUSH);
			for (int j=1; j<frameCount; j++){
				if(book[j]->contourArray.size() <= c || book[j*offset]->contourArray[c]->total <= i)
					continue;
				CvPoint *p = (CvPoint*) cvGetSeqElem(book[j*offset]->contourArray[c], i);
				dc.DrawLine(scale.x*lastLoc->x,scale.y*lastLoc->y, scale.x*p->x,scale.y*p->y);
				lastLoc = p;
			}
			dc.DrawCircle(MyPoint((CvPoint*)cvGetSeqElem(book[0]->contourArray[c],i))*scale, Preferences::GetColorContourCornerWidth());
			dc.SetBrush(*wxRED_BRUSH);
			dc.SetPen(wxPen(*wxRED));
			for (int j=1; j<frameCount; j++){
				if(book[j*offset]->contourArray.size() <= c || book[j*offset]->contourArray[c]->total <= i)
					continue;
				CvPoint *p = (CvPoint*) cvGetSeqElem(book[j*offset]->contourArray[c], i);
				dc.DrawCircle(MyPoint(*p)*scale, Preferences::GetColorContourCornerWidth());
			}
		}
		if(c < book[(frameCount-1)*offset]->contourArray.size())
			MyCanvas::DrawContour_static(&dc, book[(frameCount-1)*offset]->contourArray[c],wxPoint(0,0),scale, true, wxRED);
	}
	return true;
}
bool CaptureManager::SaveTrajectoryImage(wxBitmap &bmp) {
	if (!book[0]->contourArray.size()){
		wxLogError(_T("No objects in the first frame. Detect/draw boundaries in the first frame and apply tracking first."));
		return false;
	}
	wxRealPoint scale(6,6);
	CvPoint lastLoc;
	bmp.Create(scale.x*size.width,scale.y*size.height);
	wxMemoryDC dc(bmp);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();
	for (int c=0; c<book[0]->contourArray.size(); c++){
		std::vector<CvPoint> traj = GetTrajectory(c);
		lastLoc = traj[0];
		MyCanvas::DrawContour_static(&dc, book[0]->contourArray[c],wxPoint(0,0),scale);
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		for (int i=1; i<traj.size(); i++){
			if(traj[i].x<0)
				continue;
			if (i>0){
				dc.SetPen(wxPen(wxColour(Preferences::GetColorContourBorderColor()), Preferences::GetColorContourBorderWidth()));
				dc.DrawLine(scale.x*lastLoc.x,scale.y*lastLoc.y, scale.x*traj[i].x,scale.y*traj[i].y);
			}
			lastLoc = traj[i];
		}
		dc.SetBrush(wxBrush(wxColour(Preferences::GetColorContourCornerColor())));
		dc.SetPen(wxPen(wxColour(Preferences::GetColorContourCornerColor())));
		for (int i=0; i<traj.size(); i++){
			if(traj[i].x<0)
				continue;
			dc.DrawCircle(MyPoint(traj[i])*scale, 2*Preferences::GetColorContourCornerWidth());
		}
		dc.SetBrush(*wxRED_BRUSH);
		dc.SetPen(wxPen(*wxRED));
		if(traj[frameCount-1].x>=0){
			dc.DrawCircle(MyPoint(traj[frameCount-1])*scale, 2*Preferences::GetColorContourCornerWidth());
			MyCanvas::DrawContour_static(&dc, book[(frameCount-1)*offset]->contourArray[c],wxPoint(0,0),scale, true, wxRED);
		}
	}
	return true;
}
void CaptureManager::Resize(int width, int height, int method){
	if (MyPoint(width,height) == MyPoint(size))
		return;
	size = cvSize(width,height);
	for (int i=0; i<totalFrameCount; i++){
		book[i]->Resize(width, height, method);
	}
	ReloadCurrentFrame();
}
void CaptureManager::Crop(CvPoint topleft, CvSize size_){
	if (size.width == 0)
		return;
	for (int i=0; i<totalFrameCount; i++){
		book[i]->Crop(topleft, size_);
	}
	size = cvSize(size_.width, size_.height);
	ReloadCurrentFrame();
}
