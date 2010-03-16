#include "MyFrame.h"
#include "Util.h"
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include "CustomEvents.h"
#include "Preferences.h"
#include "IncludePlugins.h"
#include "ImageLoader.h"

#define WX_SET_ENABLED(win, to)	if(to) win->Enable(); else win->Disable();

extern const wxEventType wxEVT_IMGTHREAD;

IMPLEMENT_DYNAMIC_CLASS( MyFrame, wxFrame )
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_CUSTOM(EVT_PLUGIN_END, wxID_ANY, MyFrame::OnPluginEnd)
EVT_TIMER(wxID_ANY, MyFrame::OnTimer)
EVT_COMMAND(wxID_ANY, wxEVT_IMGTHREAD, MyFrame::OnThread)
// 	EVT_CLOSE(MyFrame::OnClose)
//	EVT_BUTTON(wxID_OK, MyFrame::OnOK)
//	EVT_CANNY( wxID_ANY, MyFrame::OnCanny )
//	EVT_SPINCTRL(wxID_ANY, MyFrame::OnTemp)
// EVT_MOUSE_EVENTS(MyFrame::OnTemp)
END_EVENT_TABLE()

MyFrame::MyFrame(): MyFrame_(NULL), cm(NULL) {}
MyFrame::~MyFrame()
{
	if ( hotplug ) {
		hotplug->OnCancel();
		wxCommandEvent dummy = wxCommandEvent();
		OnPluginEnd(dummy);
	}
	if (cm)
		delete cm;
	if (timer)
		delete timer;
    if (queue)
        delete queue;
}

#define SPLITTER_UNSPLIT_DESTROY(splitter, i)	\
	if ( splitter->IsSplit() ){ \
		wxWindow *toremove = splitter->GetWindow##i(); \
		splitter->Unsplit(toremove);  \
		toremove->Destroy(); \
	}

// icons
#include "all_icons.xpm"
#define SET_ICON_IMAGES(key)	\
	(m_##key)->SetBitmapLabel(xpm_##key); \
	(m_##key)->SetBitmapSelected(xpm_##key); \
	(m_##key)->SetBitmapDisabled(xpm_disabled_##key); \
	(m_##key)->SetBitmapHover(xpm_hover_##key);


////////////////////////////////////////////////////////////////////
// Method:	Constructor
// Purpose:
// Input:
// Output:	NONE
////////////////////////////////////////////////////////////////////
MyFrame::MyFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
: MyFrame_(NULL, -1, title, pos, size, style), hotplug(NULL), isplaying(false), loaded(false)
{
	config = wxConfig::Get();
	cm = new CaptureManager();
	queue = new ImageJobQueue(this);

	cm->SetCanvas(canvas);
    canvas->SetCM(cm);
    canvas2->SetCM(cm);
	//	  SetIcon(wxIcon(wxT("celltrack.xpm")));
	SPLITTER_UNSPLIT_DESTROY( splitterSidebar, 1 )
	SPLITTER_UNSPLIT_DESTROY( splitterBottombar, 2 )
	EnableMenus( false );

	SET_ICON_IMAGES( delete )
	SET_ICON_IMAGES( stop )
	SET_ICON_IMAGES( play )
	SET_ICON_IMAGES( prev )
	SET_ICON_IMAGES( next )
	SET_ICON_IMAGES( fluorescence )

	timer = new wxTimer(this);
	int widths[] = {-1,200};
	sbar->SetStatusWidths(2, widths);

	if(Config_ReadBool(config,_T("Frame/maximized"), false))
		Maximize();
	else if(Config_ReadBool(config,_T("Frame/fullscreen"), false))
		ShowFullScreen(true);
	ShowCanvas2(false);
	getCWD();
	imgDir = config->Read(_T("LastFilepath"),cwd);
	m_zslides = config->Read(_T("SliceNumber"), 33);
    config->Read(_T("SliceDistance"), &m_deltaZ, 0.25);
	config->Read(_T("Calibration"), &m_calibration, 0.213);
	loadFluorescence = Config_ReadBool(config,_T("LoadFluorescence"), true);
}

void MyFrame::myShow(bool askLoad)
{
	Show();
	ImageLoader* imgLoader = new ImageLoader(queue);
	imgLoader->SetPriority(70);
	imgLoader->Run();
	cm->SetQueue(queue);
	if (!loaded && askLoad){
		LoadImagesDialog_ w(this);
		if(w.ShowModal() == wxID_OK){
			if (w.filetype->GetSelection()==0)
			{
			    wxCommandEvent dummy = wxCommandEvent();
				OnOpenMovie(dummy);
			}
			else if (w.filetype->GetSelection()==1)
			{
			    wxCommandEvent dummy = wxCommandEvent();
				OnOpenImages(dummy);
			}
			else
			{
			    wxCommandEvent dummy = wxCommandEvent();
				OnOpenConfocal(dummy);
			}
		}
//		else OpenMovie(config->Read("AppDir")+"sampleMovie.avi");
	}
	loaded = true;
}
void MyFrame::EnableMenus( bool enable )
{
	EnableMenu(menu_go, enable);
	EnableMenu(menu_image, enable);
	EnableMenu(menu_contours, enable);
	EnableMenu(menu_contour_views, enable);
	EnableMenu(menu_track, enable);
	EnableMenu(menu_analyze, enable);
	if(enable)
		EnableMenu(menu_file);
//	EnableMenu(menu_help, enable);
//	EnableMenuItem(menu_file,wxID_MENU_SAVE,enable);
}

void MyFrame::OnPreferences( wxCommandEvent &event )
{
	Preferences d(this);
	if (d.ShowModal() == wxID_OK)
	{
	    wxSizeEvent dummy = wxSizeEvent();
		canvas->OnSize(dummy);
	}

}
void MyFrame::OnOpenMovie( wxCommandEvent &event )
{
	wxString filename = wxFileSelector(_T("Select a movie file to load"), cwd, _T(""), _T(""), _T("Movie Files (*.avi)|*.avi"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);
	if (!filename.empty()) {
		OpenMovie(filename);
	}
}
void MyFrame::OpenMovie( const wxString& filename )
{
	wxBeginBusyCursor();
		setCWD(wxPathOnly(filename));
		if(cm->OpenMovie(filename.mb_str()))
			OnNewMovieOpened();
	wxEndBusyCursor();
}


void MyFrame::OnOpenImages( wxCommandEvent& event )
{
	wxFileDialog d(this, _T("Select image files to load"), cwd, _T(""), _T("Windows bitmaps (*.bmp, *.dib)|*.bmp;*.dib|JPEG files (*.jpeg,*.jpg,*.jpe)|*.jpeg;*.jpg;*.jpe|Portable Network Graphics (*.png)|*.png|Portable image format (*.pbm,*.pgm,*.ppm)|*.pbm;*.pgm;*.ppm|Sun rasters (*.sr,*.ras)|*.sr;*.ras|TIFF files (*.tiff,*.tif)|*.tiff;*.tif|OpenEXR HDR images (.exr)|*.exr|JPEG 2000 images (.jp2)|*.jp2|All files (*.*)|*.*"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST | wxFD_MULTIPLE);
	d.SetFilterIndex(config->Read(_T("OpenImages/filterIndex"),1l));
	if (d.ShowModal() == wxID_OK)
	{
		config->Write(_T("OpenImages/filterIndex"),(long)d.GetFilterIndex());
		wxArrayString files;
		d.GetFilenames(files);
		if (!files.GetCount())
			return;
        wxBeginBusyCursor();
		if(cm->OpenMovie(files))
			OnNewMovieOpened();
        wxEndBusyCursor();
	}
}

void MyFrame::OnOpenConfocal( wxCommandEvent& event )
{
    wxFileDialog d(this, _T("Select image files to load"), imgDir, _T(""), _T("All files (*.*)|*.*|TIFF files (*.tiff,*.tif)|*.tiff;*.tif"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST | wxFD_MULTIPLE);
    if (d.ShowModal() == wxID_OK)
    {
        config->Write(_T("OpenImages/filterIndex"),(long)d.GetFilterIndex());
        wxArrayString files;
        d.GetPaths(files);
        imgDir = d.GetDirectory();
        config->Write(_T("LastFilepath"), imgDir);
        if (!files.GetCount())
            return;
        ConfocalDialog_ w(this);
        w.m_zslides->SetValue(m_zslides);
        w.m_deltaZ->SetValue(wxString::Format(_T("%.3f"), m_deltaZ));
        w.m_calibration->SetValue(wxString::Format(_T("%.3f"), m_calibration));
        w.m_fluorescence->SetValue(loadFluorescence);
        if(w.ShowModal() == wxID_OK)
        {
            m_zslides = w.m_zslides->GetValue();
            m_deltaZ = wxStringToDouble(w.m_deltaZ->GetValue());
            m_calibration = wxStringToDouble(w.m_calibration->GetValue());
            loadFluorescence = w.m_fluorescence->GetValue();
        }
        else
            return;
        while (files.GetCount()%((loadFluorescence?2:1)*m_zslides) != 0)
        {
            ConfocalDialog_ v(this);
            v.m_fluorescence->SetValue(loadFluorescence);
            v.m_deltaZ->SetValue(wxString::Format(_T("%.3f"), m_deltaZ));
            v.m_calibration->SetValue(wxString::Format(_T("%.3f"), m_calibration));
            v.m_zslides->SetValue(m_zslides);

            if(v.ShowModal() == wxID_OK)
            {
                m_zslides = v.m_zslides->GetValue();
                m_deltaZ = wxStringToDouble(v.m_deltaZ->GetValue());
                m_calibration = wxStringToDouble(v.m_calibration->GetValue());
                loadFluorescence = v.m_fluorescence->GetValue();
            }
            else
                return;
        }
        config->Write(_T("SliceNumber"), m_zslides);
        config->Write(_T("SliceDistance"), m_deltaZ);
        config->Write(_T("Calibration"), m_calibration);
        config->Write(_T("LoadFluorescence"), loadFluorescence);
        wxBeginBusyCursor();
        if(cm->OpenConfocal(files, m_zslides, loadFluorescence, m_deltaZ, m_calibration))
        {
            m_slider2->Show();
            Layout();
            if (loadFluorescence)
                m_fluorescence->Enable();
            OnNewMovieOpened();
        }
        wxEndBusyCursor();
    }
}

void MyFrame::OnThread( wxCommandEvent& event )
{
    ImagePlus* img = NULL;
    switch(event.GetId())
    {
        case Job::thread_started:
            //std::cout << "Thread started" << std::endl;
            break;
        case Job::thread_loaded:
            //std::cout << "Thread loaded img " << event.GetInt() << std::endl;
            img = cm->DirectAccess(event.GetInt());
            if(img->orig || !img->isLoading)
            {
                queue->AddJob(Job(Job::thread_delete, -1, (IplImage*)event.GetClientData()), 1);
                cm->loadedImgs--;
                break;
            }
            img->isLoading = false;
            img->orig = (IplImage*)(event.GetClientData());
            if(cm->GetTotalPos() == event.GetInt())
            {
                cm->ReloadCurrentFrame();
            }
            break;
        case Job::thread_deleted:
            //std::cout << "Thread freed img " << event.GetInt() << std::endl;
            if (event.GetInt() >= 0)
                cm->DirectAccess(event.GetInt())->orig = NULL;
            break;
        default: break;
    }
}

#include <wx/sound.h>
#include "FilterContoursSidebar.h"
void MyFrame::OnNewMovieOpened()
{
	cm->SetPos(0,0);
	OnNavigate();
	m_delete->Enable();
	m_stop->Enable();
	EnableMenus( true );

	return;
	wxCommandEvent dummy = wxCommandEvent();
	OnFindContours(dummy );
	hotplug->OnOK(); OnPluginEnd(dummy);
	OnFilterContours(dummy);
	FilterContoursSidebar *s = (FilterContoursSidebar *) hotplug->GetSidebar();
	s->isMinArea->SetValue(true);
	s->minArea->SetValue(1800);
	s->scope->SetSelection(1);
	s->scope2->SetSelection(0);
	s->preview->SetValue(false);
	hotplug->OnOK(); OnPluginEnd(dummy);
	OnImproveContours(dummy );
	hotplug->OnOK(); OnPluginEnd(dummy);
//	OnLKContours(dummy );
	OnCombinedTracking(dummy);
	((CombinedTrackingSidebar *) hotplug->GetSidebar())->useNC->SetValue(false);
	hotplug->OnOK(); OnPluginEnd(dummy);
//	OnExportTrackData(dummy);
	OnPlotDeformation(dummy);
}

void MyFrame::OnSaveMovieAs( wxCommandEvent &event )
{
	wxString filename = wxFileSelector(_T("Save movie file as..."), cwd, _T(""), _T(""), _T("Movie Files (*.avi)|*.avi"), wxSAVE | wxCHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
	if (!filename.empty()){
		setCWD(wxPathOnly(filename));
	wxBeginBusyCursor();
		cm->SaveMovie(filename.mb_str());
	wxEndBusyCursor();
	}
}

void MyFrame::OnSaveFrameAs( wxCommandEvent &event )
{
	wxString filename = wxFileSelector(_T("Save current frame as..."), cwd, _T(""), _T(""), _T("Image Files|*.png;*.bmp;*.jpg;*.gif"), wxSAVE | wxCHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
	if (!filename.empty()){
		setCWD(wxPathOnly(filename));
		wxBeginBusyCursor();
		canvas->drawn.ConvertToImage().SaveFile(filename);
		wxEndBusyCursor();
	}
}

void MyFrame::OnClose(wxCloseEvent &event)
{
    queue->AddJob(Job(Job::thread_exit),0);
	config->Write(_T("Frame/maximized"), IsMaximized());
	config->Write(_T("Frame/fullscreen"), IsFullScreen());
	config->Write(_T("Frame/iconized"), IsIconized());
	if (!IsMaximized() && !IsFullScreen() && !IsIconized()){
		wxRect frameDim = GetRect();
		config->Write(wxT("Frame/width"), frameDim.width);
		config->Write(wxT("Frame/height"), frameDim.height);
		config->Write(wxT("Frame/x"), frameDim.x);
		config->Write(wxT("Frame/y"), frameDim.y);
	}
	Show(false);
	if(hotplug){
	    wxCommandEvent dummy = wxCommandEvent();
		OnPluginEnd(dummy);
	}
	event.Skip();
}

wxString MyFrame::getCWD()
{
	config->Read(_T("CWD"), &cwd);
	return cwd;
}

void MyFrame::setCWD(const wxString& path)
{
	cwd = path;
	config->Write(_T("CWD"), cwd);
}

void MyFrame::OnDelete( wxCommandEvent& event )
{
	if(cm->OnDelete())
		OnNavigate();
}

void MyFrame::OnDeleteBefore( wxCommandEvent& event )
{
	if(cm->OnDeleteBefore())
		OnNavigate();
}
void MyFrame::OnDeleteAfter( wxCommandEvent& event )
{
	if(cm->OnDeleteAfter())
		OnNavigate();
}
void MyFrame::OnPlay( wxCommandEvent& event )
{
	m_play->Disable();
	isplaying = true;
	timer->Start(1000/(cm->fps ? cm->fps : 1), wxTIMER_CONTINUOUS );
}

void MyFrame::OnStop( wxCommandEvent& event )
{
	if (isplaying) {
		m_play->Enable();
		isplaying = false;
		timer->Stop();
	}
	else if(cm->SetPos(0))
		OnNavigate();
}
void MyFrame::OnTogglePlay( wxCommandEvent& event )
{
    wxCommandEvent dummy = wxCommandEvent();
	if (isplaying)
		OnStop(dummy);
	else
		OnPlay(dummy);
}

void MyFrame::OnTimer( wxTimerEvent &e )
{
	if (isplaying) {
        wxCommandEvent dummy = wxCommandEvent();
		if(m_next->IsEnabled())
			OnNext(dummy);
		else {
			OnStop(dummy);
			if(cm->SetPos(0))
				OnNavigate();
		}
	}
	else {
		return;
	}
}

void MyFrame::OnPrev( wxCommandEvent &event )
{
	if(cm->OnPrev())
		OnNavigate();
}
void MyFrame::OnNext( wxCommandEvent &event )
{
	if(cm->OnNext())
		OnNavigate();
}
void MyFrame::OnScroll( wxScrollEvent &event )
{
	if (cm->SetPos(m_slider->GetValue()))
		OnNavigate();
}
void MyFrame::OnVScroll( wxScrollEvent &event )
{
	if (cm->SetZPos(m_slider2->GetValue()))
		OnNavigate();
}

void MyFrame::OnFirst( wxCommandEvent& event )
{
	if (cm->SetPos(0))
		OnNavigate();
}

void MyFrame::OnFluorescence( wxCommandEvent &event )
{
    cm->ShowFluorescence(!(cm->viewFluorescence));
    if(hotplug)
        hotplug->OnFluorescence();
    OnNavigate();
}

void MyFrame::OnLast( wxCommandEvent& event )
{
	if (cm->SetPos(cm->GetFrameCount()-1))
		OnNavigate();
}

void MyFrame::UpdateControls()
{
	WX_SET_ENABLED ( m_prev, cm->GetPos() > 0 );
	WX_SET_ENABLED ( m_next, cm->GetPos() < cm->GetFrameCount() -1 );
	m_slider->SetRange( 0, cm->GetFrameCount() - 1 );
	m_slider->SetValue( cm->GetPos() );
	m_slider2->SetRange( 0, cm->slideCount - 1 );
	m_slider2->SetValue( cm->GetZPos() );
	WX_SET_ENABLED ( m_slider, cm->GetFrameCount() > 1 );
	WX_SET_ENABLED ( m_slider2, cm->slideCount > 1 );
	WX_SET_ENABLED ( m_play, cm->GetFrameCount() > 1 );
	sbar->SetStatusText(wxString::Format(_T("%d of %d | Slice %d of %d"), cm->pos+1, cm->GetFrameCount(), cm->zPos+1, cm->slideCount),SBAR_POS);
}

void MyFrame::OnNavigate()
{
	UpdateControls();
	if ( hotplug )
		hotplug->OnNavigate();
}

bool MyFrame::CheckPluginAlreadyHot(string pluginname)
{
	if ( pluginname != "" && hotplug && hotplug->GetName() == pluginname  )
	{
//		wxMessageBox(wxString("There is already an active ") + pluginname + " plugin.");
		return true;
	}
	return false;
}

template <class Plugin>
void MyFrame::StartPlugin(Plugin *newplugin, wxWindow *parentwin)
{
	if (CheckPluginAlreadyHot(Plugin::GetStaticName()))
		return;
	if ( hotplug )
	{
//		if (wxMessageBox(wxString("There is already an active plugin named \"")+hotplug->GetStaticName()+"\". Do you want to continue starting new plugin named \""+Plugin::GetStaticName()+"\"?", "Cancel active plugin?", wxOK|wxCANCEL, this) != wxOK)
//			return;
		hotplug->OnCancel();
		wxCommandEvent dummy = wxCommandEvent();
		OnPluginEnd(dummy);
	}
	hotplug = new Plugin(parentwin, this);
	wxWindow *sidebar = hotplug->GetSidebar();
	if ( sidebar )
		splitterSidebar->SplitVertically(sidebar, splitterSidebar->GetWindow1(), sidebar->GetSize().GetWidth());
	sidebar = hotplug->GetBottombar();
	if ( sidebar )
	{
		splitterBottombar->SplitHorizontally(splitterBottombar->GetWindow1(), sidebar, splitterBottombar->GetSize().GetHeight() - sidebar->GetSize().GetHeight());
	}
	if (hotplug->ShowsCanvas2())
		ShowCanvas2(true);
}

template <class Plugin>
void MyFrame::MemTestPlugin()
{
	for (int i=0; i<100; i++)
	{
		Plugin *p=NULL;
		StartPlugin(p, splitterSidebar);
		hotplug->OnCancel();
		OnPluginEnd(wxCommandEvent());
	}
}
#define DEFINE_ONPLUGIN(name)	\
	void MyFrame::On##name(wxCommandEvent &event) { \
	name##Plugin *p=NULL;	StartPlugin(p, splitterSidebar); }
DEFINE_ONPLUGIN(FindContours)
DEFINE_ONPLUGIN(FilterContours)
DEFINE_ONPLUGIN(EditContours)
DEFINE_ONPLUGIN( ImproveContours )
DEFINE_ONPLUGIN( SubtractBackground )
DEFINE_ONPLUGIN( Smooth )
DEFINE_ONPLUGIN( TrackContours )
DEFINE_ONPLUGIN( LKContours )
DEFINE_ONPLUGIN( CamShift )
DEFINE_ONPLUGIN( MatchTemplate )
DEFINE_ONPLUGIN( CombinedTracking )
DEFINE_ONPLUGIN( FindFeatures )
DEFINE_ONPLUGIN( NormalizeContours )
DEFINE_ONPLUGIN( ResizeMovie )
DEFINE_ONPLUGIN( Crop )
DEFINE_ONPLUGIN( CopyContours )
DEFINE_ONPLUGIN( CorrectContours )

void MyFrame::OnPluginEnd(wxEvent &event)
{
	wxWindow *sidebar=hotplug->GetSidebar(), *bottombar=hotplug->GetBottombar();
	if ( sidebar )
		splitterSidebar->Unsplit(sidebar);
	if ( bottombar )
		splitterBottombar->Unsplit(bottombar);
	ShowCanvas2(false);
	delete hotplug;
	hotplug = NULL;
}

void MyFrame::OnKeyDown( wxKeyEvent& e )
{
    wxCommandEvent dummy = wxCommandEvent();
	if (e.GetKeyCode() == WXK_LEFT)
		OnPrev(dummy);
	else if (e.GetKeyCode() == WXK_RIGHT)
		OnNext(dummy);
}

void MyFrame::OnMouse( wxMouseEvent& event )
{
		event.Skip();
		return;
		if (event.GetEventObject() != canvas)
			return;
		if (!event.AltDown())
			return;
		if (event.GetEventObject() == canvas && event.m_leftDown)
			sbar->SetStatusText(wxString::Format(_T("x=%d, y=%d"), event.GetX(), event.GetY()));
}

void MyFrame::SetStatusText( const wxString& text, int i )
{
	sbar->SetStatusText(text, i);
}

void MyFrame::ShowCanvas2(bool show)
{
	if (show)
	{
		if (!splitterCanvas->IsSplit())
		{
			canvas2->Show(true);
			splitterCanvas->SplitVertically(canvas2,canvas);
		}
	}
	else
	{
		if (splitterCanvas->IsSplit())
		{
			splitterCanvas->Unsplit(canvas2);
			canvas2->Show(false);
		}
	}
}
//---------------------- Analysis ------------------------------
bool MyFrame::SetupCellPlot(wxString title, wxString ytitle, PlotDialog* &pd, mpWindow* &p, int &numCells)
{
	numCells=cm->Access(0,0,false,true)->contourArray.size();
	if (!numCells)
	{
		wxLogError(_T("No cells in the first frame. Detect/draw boundaries in the first frame and apply tracking first."));
		return false;
	}
	pd = new PlotDialog(this, wxID_ANY, title);
	p = pd->plot;
    p->AddLayer(     new mpScaleX(wxT("frame"), mpALIGN_BOTTOM, true) );
    p->AddLayer(     new mpScaleY(ytitle, mpALIGN_LEFT, true) );
	return true;
}

#include "mpFXVector.h"
void MyFrame::OnPlotSpeed( wxCommandEvent& e )
{
	PlotDialog *pd; mpWindow *p; int numCells;
	if(!SetupCellPlot(_T("Cell Speed"), _T("speed (pixel/frame)"), pd, p, numCells)) return;
	for (int i=0; i<numCells; i++){
		float totalDisp, avgSpeed;
		std::vector<float> speeds = cm->GetSpeeds(i, totalDisp, avgSpeed);
		wxString name = wxString::Format(_T("Cell-%d (distance=%.1f, speed=%.1f)"), i+1, totalDisp, avgSpeed);
		mpFXVector *v = new mpFXVector(name);
		v->SetData( speeds );
		v->SetContinuity(true);
		//wxPen mypen(*wxRED, 5, wxSOLID); v->SetPen( mypen);
		p->AddLayer(   v   );
	}
	p->Fit();
	pd->ShowModal();
	delete pd;
}

void MyFrame::OnPlotArea( wxCommandEvent& e )
{
	PlotDialog *pd; mpWindow *p; int numCells;
	if(!SetupCellPlot(_T("Cell Area"), _T("area (pixel^2)"), pd, p, numCells)) return;
	for (int i=0; i<numCells; i++)
	{
		float avgArea;
		std::vector<float> areas = cm->GetAreas(i, avgArea);
		wxString name = wxString::Format(_T("Cell-%d (avgArea=%.1f)"), i+1, avgArea);
		mpFXVector *v = new mpFXVector(name);
		v->SetData( areas );
		v->SetContinuity(true);
		p->AddLayer(   v   );
	}
	p->Fit();
	pd->ShowModal();
	delete pd;
}

void MyFrame::OnPlotAreaDiff( wxCommandEvent& e )
{
	PlotDialog *pd; mpWindow *p; int numCells;
	if(!SetupCellPlot(_T("Change in Cell Area"), _T("d-area (pixel^2/frame)"), pd, p, numCells)) return;
	for (int i=0; i<numCells; i++){
		float avgArea;
		std::vector<float> areas = cm->GetAreaDiff(i, avgArea);
		wxString name = wxString::Format(_T("Cell-%d (avgChange=%.1f)"), i+1, avgArea);
		mpFXVector *v = new mpFXVector(name);
		v->SetData( areas );
		v->SetContinuity(true);
		p->AddLayer(   v   );
	}
	p->Fit();
	pd->ShowModal();
	delete pd;
}

void MyFrame::OnPlotVolume( wxCommandEvent& e )
{
	PlotDialog *pd; mpWindow *p; int numCells;
	if(!SetupCellPlot(_T("Cell Volume"), _T("volume (μm^3)"), pd, p, numCells)) return;
	for (int i=0; i<numCells; i++)
	{
		float avgVolume;
		std::vector<float> volumes = cm->GetVolumes(i, avgVolume);
		wxString name = wxString::Format(_T("Cell-%d (avgVolume=%.1f)"), i+1, avgVolume);
		mpFXVector *v = new mpFXVector(name);
		v->SetData( volumes );
		v->SetContinuity(true);
		p->AddLayer(   v   );
	}
	p->Fit();
	pd->ShowModal();
	delete pd;
}

void MyFrame::OnPlotVolumeDiff( wxCommandEvent& e )
{
	PlotDialog *pd; mpWindow *p; int numCells;
	if(!SetupCellPlot(_T("Change in Cell Volume"), _T("d-volume (μm^3/frame)"), pd, p, numCells)) return;
	for (int i=0; i<numCells; i++)
	{
		float avgVolume;
		std::vector<float> volumes = cm->GetVolumeDiff(i, avgVolume);
		wxString name = wxString::Format(_T("Cell-%d (avgChange=%.1f)"), i+1, avgVolume);
		mpFXVector *v = new mpFXVector(name);
		v->SetData( volumes );
		v->SetContinuity(true);
		p->AddLayer(   v   );
	}
	p->Fit();
	pd->ShowModal();
	delete pd;
}

void MyFrame::OnPlotDeformation( wxCommandEvent& e )
{
	PlotDialog *pd; mpWindow *p; int numCells;
	if(!SetupCellPlot(_T("Cell Deformation"), _T("deformation (pixel^2/frame)"), pd, p, numCells)) return;
	for (int i=0; i<numCells; i++){
		float avgDef;
		std::vector<float> defs = cm->GetDeformation(i, avgDef);
		wxString name = wxString::Format(_T("Cell-%d (avgDeform=%.1f)"), i+1, avgDef);
		mpFXVector *v = new mpFXVector(name);
		v->SetData( defs );
		v->SetContinuity(true);
		p->AddLayer(   v   );
	}
	p->Fit();
	pd->ShowModal();
	delete pd;
}

#include "PictureCanvas.h"
void MyFrame::OnViewImage(wxString title, bool (CaptureManager::*func)(wxBitmap &))
{
	wxBitmap bmp;
	if((cm->*func)(bmp))
	{
		ShowImageDialog d(this,wxID_ANY,title);
		d.canvas->SetImage(bmp);
		d.ShowModal();
	}
}

void MyFrame::OnViewImage(wxString title, bool (CaptureManager::*func)(wxBitmap &, int, int))
{
	wxBitmap bmp;
	if((cm->*func)(bmp, 0, cm->frameCount))
	{
		ShowImageDialog d(this,wxID_ANY,title);
		d.canvas->SetImage(bmp);
		d.ShowModal();
	}
}

void MyFrame::OnExportImage(wxString title, bool (CaptureManager::*func)(wxBitmap &))
{
	wxString filename = wxFileSelector(title, cwd, _T(""), _T(""), _T("Image Files|*.png;*.jpg;*.gif"), wxSAVE | wxCHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
    if (!filename.empty())
	{
        setCWD(wxPathOnly(filename));
		wxBeginBusyCursor();
		wxBitmap bmp;
		if((cm->*func)(bmp))
			bmp.ConvertToImage().SaveFile(filename);
		wxEndBusyCursor();
	}
}

void MyFrame::OnExportImage(wxString title, bool (CaptureManager::*func)(wxBitmap &, int, int))
{
    wxString filename = wxFileSelector(title, cwd, _T(""), _T(""), _T("Image Files|*.png;*.jpg;*.gif"), wxSAVE | wxCHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
    if (!filename.empty())
	{
        setCWD(wxPathOnly(filename));
		wxBeginBusyCursor();
		wxBitmap bmp;
		if((cm->*func)(bmp, 0, cm->frameCount))
			bmp.ConvertToImage().SaveFile(filename);
		wxEndBusyCursor();
	}
}

void MyFrame::OnViewTrackImage( wxCommandEvent& e )
{
	OnViewImage(_T("Cell Tracking"), &CaptureManager::SaveTrackImage);
}

void MyFrame::OnViewTrajectoryImage( wxCommandEvent& e )
{
	OnViewImage(_T("Cell Trajectory"), &CaptureManager::SaveTrajectoryImage);
}

void MyFrame::OnExportTrackImage( wxCommandEvent& e )
{
	OnExportImage(_T("Save tracking image as..."), &CaptureManager::SaveTrackImage);
}

void MyFrame::OnExportSingleTrackImages( wxCommandEvent& e )
{
    wxString filename = wxFileSelector(_T("Save tracking images as..."), cwd, _T(""), _T(""), _T("Image Files|*.png;*.jpg;*.gif"), wxSAVE | wxCHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
    if (!filename.empty())
    {
        setCWD(wxPathOnly(filename));
        wxString name = filename.BeforeLast('.');
        wxString extension = filename.AfterLast('.');
        wxBeginBusyCursor();
        wxProgressDialog* progressDlg = new wxProgressDialog(_T("Saving individual images..."), wxString::Format(_T("Processing image of frame 0 and 1 of %d..."), cm->GetFrameCount()), cm->GetFrameCount()-1, this, wxPD_CAN_ABORT|wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE);
        for (int i = 1; i < cm->frameCount; i++)
        {
            progressDlg->Update(i-1, wxString::Format(_T("Processing image of frame %d and %d of %d..."), i, i+1, cm->GetFrameCount()));
            wxBitmap bmp;
            if(cm->SaveTrackImage(bmp, i-1, i+1))
                bmp.ConvertToImage().SaveFile(name + wxString::Format(_T("%d_%d."), i, i+1) + extension);
        }
        progressDlg->Destroy();
		progressDlg = NULL;
        wxEndBusyCursor();
    }
}

void MyFrame::OnExportTrajectoryImage( wxCommandEvent& e )
{
	OnExportImage(_T("Save trajectory image as..."), &CaptureManager::SaveTrajectoryImage);
}

void MyFrame::OnExportData(wxString title, bool (CaptureManager::*func)(const char*))
{
	wxString filename = wxFileSelector(title, cwd, _T(""), _T(""), _T("Text Files|*.txt"), wxSAVE | wxCHANGE_DIR | wxFD_OVERWRITE_PROMPT, this);
	if (!filename.empty())
	{
		setCWD(wxPathOnly(filename));
		wxBeginBusyCursor();
		(cm->*func)(filename.mb_str());
		wxEndBusyCursor();
	}
}

void MyFrame::OnExportTrajectoryData(wxCommandEvent& e )
{
	OnExportData(_T("Save Trajectory data as..."), &CaptureManager::SaveTrajectoryData);
}

void MyFrame::OnExportTrackData(wxCommandEvent& e )
{
	OnExportData(_T("Save Tracking data as..."), &CaptureManager::SaveTrackData);
}

void MyFrame::OnExportSpeedData(wxCommandEvent& e )
{
	OnExportData(_T("Save Cell Speed data as..."), &CaptureManager::SaveSpeedData);
}

void MyFrame::OnExportAreaData(wxCommandEvent& e )
{
	OnExportData(_T("Save Cell Area data as..."), &CaptureManager::SaveAreaData);
}

void MyFrame::OnExportVolumeData(wxCommandEvent& e )
{
	OnExportData(_T("Save Cell Volume data as..."), &CaptureManager::SaveVolumeData);
}

void MyFrame::OnExportDeformationData(wxCommandEvent& e )
{
	OnExportData(_T("Save Cell Area data as..."), &CaptureManager::SaveDeformationData);
}

void MyFrame::OnExportContours(wxCommandEvent& e )
{
	OnExportData(_T("Save Contours as..."), &CaptureManager::SaveContours);
}

void MyFrame::OnImportTrackData(wxCommandEvent& e )
{
	wxString filename = wxFileSelector(_T("Import tracking data..."), cwd, _T(""), _T(""), _T("Text Files|*.txt"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);
	if (!filename.empty())
	{
		setCWD(wxPathOnly(filename));
		wxBeginBusyCursor();
		cm->ImportTrackData(filename.mb_str());
//TODO: is this necessary?
        wxSizeEvent dummy = wxSizeEvent();
		canvas->OnSize(dummy);
		wxEndBusyCursor();
	}
}

void MyFrame::OnImportContours(wxCommandEvent& e )
{
	wxString filename = wxFileSelector(_T("Import contours..."), cwd, _T(""), _T(""), _T("Text Files|*.txt"), wxOPEN | wxCHANGE_DIR | wxFILE_MUST_EXIST, this);
	if (!filename.empty())
	{
		setCWD(wxPathOnly(filename));
		wxBeginBusyCursor();
		cm->ImportContours(filename.mb_str());
//TODO: is this necessary?
        wxSizeEvent dummy = wxSizeEvent();
		canvas->OnSize(dummy);
		wxEndBusyCursor();
	}
}

#include <wx/aboutdlg.h>
void MyFrame::OnAbout(wxCommandEvent &e)
{
	wxAboutDialogInfo i;
	i.SetName(_T("Confocal CellTrack"));
	i.SetVersion(_T("0.2-1.1"));
	i.SetDescription(_T("An Open-Source Software for Cell Tracking and Motility Analysis on Confocal Data"));
	i.SetCopyright(_T("(C) 2008 Ahmet Sacan <sacan@cse.ohio-state.edu>, extended by Benjamin Knecht <bknecht@ethz.ch>"));
	i.SetWebSite(_T("http://db.cse.ohio-state.edu/CellTrack"));
	wxAboutBox(i);
}

void MyFrame::OnHelp(wxCommandEvent &e)
{
	wxLaunchDefaultBrowser(_T("http://db.cse.ohio-state.edu/CellTrack"));
}

void MyFrame::OnBorder( wxCommandEvent& event )
{
    cm->drawBorder = menu_contour_views->FindItemByPosition(0)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnPoints( wxCommandEvent& event )
{
    cm->drawPoints = menu_contour_views->FindItemByPosition(1)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnOtherBorder( wxCommandEvent& event )
{
    cm->drawOtherBorder = menu_contour_views->FindItemByPosition(3)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnOtherPoints( wxCommandEvent& event )
{
    cm->drawOtherPoints = menu_contour_views->FindItemByPosition(4)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnTopBorder( wxCommandEvent& event )
{
    cm->drawTopBorder = menu_contour_views->FindItemByPosition(6)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnTopPoints( wxCommandEvent& event )
{
    cm->drawTopPoints = menu_contour_views->FindItemByPosition(7)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnBottomBorder( wxCommandEvent& event )
{
    cm->drawBottomBorder = menu_contour_views->FindItemByPosition(9)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnBottomPoints( wxCommandEvent& event )
{
    cm->drawBottomPoints = menu_contour_views->FindItemByPosition(10)->IsChecked();
    canvas->ResetCanvas();
	canvas->RefreshContours();
}

void MyFrame::OnNewApproach(wxCommandEvent &e)
{
    /*wxProgressDialog* progressDlg = new wxProgressDialog(_T("New Approach :: Finding Contours"), wxString::Format(_T("Frame 0 of %d..."), cm->GetFrameCount()), cm->GetFrameCount(), this, wxPD_CAN_ABORT|wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE);
    for (int i = 0; i<cm->GetFrameCount() && progressDlg->Update(i, wxString::Format(_T("Frame %d of %d..."), i+1, cm->GetFrameCount())); i++)
    {
        IplImage* gray = NULL;
        IplImage* edge = NULL;
        FindContoursPlugin::ProcessImage_static(cm->Access(i,0,true,false,2), gray, edge, 0, 0, 3, 1, 0, true, false, true, true);
        FilterContoursPlugin::ProcessImage_static(cm->Access(i,0,true,true), 1,0,3000,10000, 0, 3, cm->GetSize());
        std::cout << cm->Access(i,0,true,true)->contourArray.size() << std::endl;
        cm->Release(i,0,true);
        cvReleaseImage(&gray);
        cvReleaseImage(&edge);
    }
    progressDlg->Destroy();
    progressDlg = NULL;*/

    wxProgressDialog* progressDlg = new wxProgressDialog(_T("New Approach :: Calculating Contours"), wxString::Format(_T("Frame 0 of %d..."), cm->GetFrameCount()*cm->slideCount), cm->GetFrameCount()*cm->slideCount, this, wxPD_CAN_ABORT|wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE);
    for (int j = 0; j<cm->slideCount; j++)
    {

        int winsize = 1.0f/8.0f*((float)j)+3.0f;
        CvSize win = cvSize(2*winsize-1, 2*winsize-1);
        int alpha = 1.0f/4.0f*((float)j)+1.0f;
        int beta = 1.0f/2.0f*((float)j)+1.0f;
        int gamma = 1.0f/8.0f*((float)j)+1.0f;
        int max = 1.0f/4.0f*((float)j)+3.0f;;
        IplImage* gray = NULL;
        IplImage* edge = NULL;
        for (int i = 0; i<cm->GetFrameCount() && progressDlg->Update(j*cm->GetFrameCount()+i, wxString::Format(_T("Frame %d of %d..."), j*cm->GetFrameCount()+i+1, cm->GetFrameCount()*cm->slideCount)); i++)
        {
            cm->SaveContours("debugContours.txt");
            FindContoursPlugin::ProcessImage_static(cm->Access(i,j,true,false,2), gray, edge, 0, 0, 3, 1, 0, true, false, true, true);
            FilterContoursPlugin::ProcessImage_static(cm->Access(i,j,true,true), 1,0,3000,10000, 0, 3, cm->GetSize());
            //std::cout << cm->Access(i,0,true,true)->contourArray.size() << std::endl;
            ImproveContoursPlugin::ProcessImage_static(cm->Access(i,j,true,false,1), gray, alpha, beta, gamma, win, 1, true, 10, 0.1f);
            cm->Release(i,j,true);
            NormalizeContoursPlugin::ProcessStatic(cm->Access(i,j,true,true), 1, max);
            /*if (j==cm->slideCount-1)
                break;
            CopyContoursPlugin::ProcessImage_static(cm->Access(i,j,true,true), cm->Access(i,j+1,true,true));*/
        }
        cvReleaseImage(&edge);
        cvReleaseImage(&gray);
    }
    progressDlg->Destroy();
    progressDlg = NULL;
}
