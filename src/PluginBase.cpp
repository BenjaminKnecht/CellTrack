#include "PluginBase.h"

PluginBase::PluginBase( string name_, wxWindow* parent_, MyFrame *win_, bool hasPreview_, bool doProcessImageOnOK_, bool showsCanvas2_)
: name(name_), parent(parent_), win(win_), cm(win->cm), sidebarw(NULL), bottombarw(NULL), hasPreview(hasPreview_), doProcessImageOnOK(doProcessImageOnOK_), canvas(win->canvas), canvas2(win->canvas2), showsCanvas2(showsCanvas2_), progressDlg(NULL)
{
	if (hasPreview)
		cm->SetRedrawListener(this);
	cm->SetBookChangeListener(this);
	if (showsCanvas2)
		OnNavigate();
}

PluginBase::~PluginBase(void)
{
	if (hasPreview)
		cm->SetRedrawListener(NULL);
	cm->SetBookChangeListener(NULL);
	ReleaseTemps();
	if (sidebarw)
		sidebarw->Destroy();
	if(progressDlg)
		progressDlg->Destroy();
}

void PluginBase::OnFluorescence() {}

void PluginBase::OnNavigate()
{
	if (showsCanvas2)
	{
		if(cm->GetPos())
		{
			canvas2->SetImage(*(cm->Access(cm->GetPos()-1, cm->GetZPos(), cm->viewFluorescence)));
		}
		else
			canvas2->SetImage(ImagePlus());
	}
	if (IsPreviewOn())
		ShowProcessInfo();
}

void PluginBase::OnEndPreview()
{
	if ( cm && cm->GetFrameCount() )
		cm->ReloadCurrentFrame(true,false);
	/*
	if (showsCanvas2){
		win->ShowCanvas2(false);
		canvas2->SetImage(ImagePlus());
	}
	*/
}
void PluginBase::OnCancel()
{
	if ( IsPreviewOn() )
		OnEndPreview();
}
void PluginBase::OnRedraw(  )
{
	if ( IsPreviewOn() && cm && cm->GetFrameCount())
		ProcessImage(&cm->img, cm->GetPos(), cm->GetZPos(), cm->viewFluorescence);
}
void PluginBase::OnOK()
{
	if (doProcessImageOnOK && GetScope()>=0 && GetScope2()>=0)
	{
		wxBeginBusyCursor();
		if (GetScope() == 1) // all
		{
			CreateProgressDlg();
			for (int j = 0; j<cm->slideCount; j++)
            {
                for (int i=0; i<cm->GetFrameCount() && UpdateProgressDlg(i+j*cm->GetFrameCount(), cm->GetFrameCount()*cm->slideCount); i++)
                {
                    if (GetScope2() != 1)
                    {
                        ProcessImage(cm->Access(i,j,false, false, GetScope()), i, j, false);
                        cm->Release(i,j,false);
                    }
                    if (GetScope2() != 0)
                    {
                        ProcessImage(cm->Access(i,j,true, false, GetScope()), i, j, true);
                        cm->Release(i,j,true);
                    }
                }
			}
			DestroyProgressDlg();
		}
		else if (GetScope() == 2) // t-direction
		{
		    CreateProgressDlg(cm->GetFrameCount());
            for (int i=0; i<cm->GetFrameCount() && UpdateProgressDlg(i, cm->GetFrameCount()); i++)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage(cm->Access(i,cm->GetZPos(),false, false, GetScope()), i, cm->GetZPos(), false);
                    cm->Release(i,cm->GetZPos(),false);
                }
                if (GetScope2() != 0)
                {
                    ProcessImage(cm->Access(i,cm->GetZPos(),true, false, GetScope()), i, cm->GetZPos(), true);
                    cm->Release(i,cm->GetZPos(),true);
                }
			}
			DestroyProgressDlg();
		}
		else if (GetScope() == 3) // z-direction
		{
		    CreateProgressDlg(cm->slideCount);
            for (int i=0; i<cm->slideCount && UpdateProgressDlg(i, cm->slideCount); i++)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage(cm->Access(cm->GetPos(),i,false, false, GetScope()), cm->GetPos(), i, false);
                    cm->Release(cm->GetPos(),i,false);
                }
                if (GetScope2() != 0)
                {
                    ProcessImage(cm->Access(cm->GetPos(),i,true, false, GetScope()), cm->GetPos(), i, true);
                    cm->Release(cm->GetPos(),i,true);
                }
			}
			DestroyProgressDlg();
		}
		else // single
		{
		    if (GetScope2() != 1)
            {
                ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), false), cm->GetPos(), cm->GetZPos(), false);
                cm->Release(cm->GetPos(), cm->GetZPos(), false);
			}
            if (GetScope2() != 0)
            {
                ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), true), cm->GetPos(), cm->GetZPos(), true);
                cm->Release(cm->GetPos(), cm->GetZPos(), true);
            }
		}
		//if (!IsPreviewOn())
        cm->ReloadCurrentFrame(true, false);
		wxEndBusyCursor();
	}
}

void PluginBase::CreateProgressDlg(int maxFrames)
{
	DestroyProgressDlg();
	progressDlg = new wxProgressDialog(wxString::FromAscii((name+string(" processing...")).c_str()), wxString::Format(_T("Frame 0 of %d..."), maxFrames>=0 ? maxFrames : cm->GetFrameCount()*cm->slideCount), maxFrames>=0 ? maxFrames : cm->GetFrameCount()*cm->slideCount, win, wxPD_CAN_ABORT|wxPD_APP_MODAL|wxPD_ELAPSED_TIME|wxPD_REMAINING_TIME|wxPD_AUTO_HIDE);
}

bool PluginBase::UpdateProgressDlg(int frame, int maximum)
{
	if (!progressDlg)
		CreateProgressDlg(maximum);
	return progressDlg->Update(frame+1, wxString::Format(_T("Frame %d of %d..."), frame+1, maximum));
}

void PluginBase::DestroyProgressDlg()
{
	if (progressDlg)
	{
		if (!progressDlg->Update(0))
			cm->ReloadCurrentFrame();
		progressDlg->Destroy();
		progressDlg = NULL;
	}
}
