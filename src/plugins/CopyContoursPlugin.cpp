#include "CopyContoursPlugin.h"
//#include "Util.h"

CopyContoursPlugin::CopyContoursPlugin( wxWindow* parent_, MyFrame *win_ ): PluginBase(GetStaticName(), parent_, win_, true, true)
{
	sidebar =  new CopyContoursSidebar(parent_, this);
	sidebarw = sidebar;
	if (cm->viewFluorescence)
        sidebar->scope2->SetSelection(1);
	DoPreview();
}

int CopyContoursPlugin::GetScope() {	return sidebar->scope->GetSelection(); }
int CopyContoursPlugin::GetScope2() {	return sidebar->scope2->GetSelection(); }
bool CopyContoursPlugin::IsPreviewOn(){ return sidebar->preview->GetValue(); }
void CopyContoursPlugin::DoPreview()
{
	if (!IsPreviewOn())
		return;
	cm->ReloadCurrentFrameContours(false);
	//ProcessImage(&cm->img, cm->GetPos(), cm->GetZPos());
	cm->Redraw(false);
	ShowProcessInfo();
}

void CopyContoursPlugin::OnOK()
{
	if (doProcessImageOnOK && GetScope()>=0 && GetScope2()>=0)
	{
		wxBeginBusyCursor();
		if (GetScope() == 1)
		{
			CreateProgressDlg();
			for (int j = 0; j<cm->slideCount; j++)
            {
                for (int i=0; i<cm->GetFrameCount() && UpdateProgressDlg(i+j*cm->GetFrameCount(), cm->GetFrameCount()*cm->slideCount); i++)
                {
                    if (GetScope2() != 1)
                    {
                        ProcessImage(cm->Access(i,j,false, true), i, j, false);
                    }
                    if (GetScope2() != 0)
                    {
                        ProcessImage(cm->Access(i,j,true, true), i, j, true);
                    }
                }
			}
			DestroyProgressDlg();
		}
		else if (GetScope() == 2)
		{
		    CreateProgressDlg(cm->GetFrameCount());
            for (int i=0; i<cm->GetFrameCount() && UpdateProgressDlg(i, cm->GetFrameCount()); i++)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage(cm->Access(i,cm->GetZPos(),false, true), i, cm->GetZPos(), false);
                }
                if (GetScope2() != 0)
                {
                    ProcessImage(cm->Access(i,cm->GetZPos(),true, true), i, cm->GetZPos(), true);
                }
			}
			DestroyProgressDlg();
		}
		else if (GetScope() == 3)
		{
		    CreateProgressDlg(cm->slideCount);
            for (int i=0; i<cm->slideCount && UpdateProgressDlg(i, cm->slideCount); i++)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage(cm->Access(cm->GetPos(),i,false, true), cm->GetPos(), i, false);
                }
                if (GetScope2() != 0)
                {
                    ProcessImage(cm->Access(cm->GetPos(),i,true, true), cm->GetPos(), i, true);
                }
			}
			DestroyProgressDlg();
		}
		else
		{
		    if (GetScope2() != 1)
            {
                ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), false, true), cm->GetPos(), cm->GetZPos(), false);
			}
            if (GetScope2() != 0)
            {
                ProcessImage( cm->Access(cm->GetPos(),cm->GetZPos(), true, true), cm->GetPos(), cm->GetZPos(), true);
            }
		}
		wxEndBusyCursor();
	}
}

void CopyContoursPlugin::ProcessImage( ImagePlus *img, int pos, int zPos, bool fluor )
{
    int targetPos, targetZPos;
    bool targetFluorescence;
    int mode = sidebar->direction->GetSelection();
    if (mode == 0)
    {
        if (zPos < cm->slideCount)
        {
            targetPos = pos;
            targetZPos = zPos+1;
            targetFluorescence = fluor;
        }
        else return;
    }
    else if (mode == 1)
    {
        if (zPos > 0)
        {
            targetPos = pos;
            targetZPos = zPos-1;
            targetFluorescence = fluor;
        }
        else return;
    }
    else
    {
        if (!cm->hasFluorescence)
            return;
        targetPos = pos;
        targetZPos = zPos;
        targetFluorescence = !fluor;
        if(mode == 3 && fluor)
            return;
        if(mode == 2 && !fluor)
            return;
    }
    ImagePlus* target = cm->Access(targetPos, targetZPos, targetFluorescence, true);
    ProcessImage_static(img, target);
}

void CopyContoursPlugin::ProcessImage_static( ImagePlus *source, ImagePlus* target )
{
	target->CloneContours(source);
}

void CopyContoursPlugin::OnFluorescence()
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
