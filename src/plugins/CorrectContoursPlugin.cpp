#include "CorrectContoursPlugin.h"
//#include "Util.h"

CorrectContoursPlugin::CorrectContoursPlugin( wxWindow* parent_, MyFrame *win_ ): PluginBase(GetStaticName(), parent_, win_, true, true)
{
	sidebar =  new CorrectContoursSidebar(parent_, this);
	sidebarw = sidebar;
	DoPreview();
}

int CorrectContoursPlugin::GetScope() {	return sidebar->scope->GetSelection(); }
int CorrectContoursPlugin::GetScope2() {	return sidebar->scope2->GetSelection(); }
bool CorrectContoursPlugin::IsPreviewOn(){ return sidebar->preview->GetValue(); }
void CorrectContoursPlugin::DoPreview()
{
	if (!IsPreviewOn())
		return;
	cm->ReloadCurrentFrameContours(false);
	ProcessImage(&cm->img, cm->GetPos(), cm->GetZPos(), cm->viewFluorescence);
	cm->Redraw(false);
	ShowProcessInfo();
}

void CorrectContoursPlugin::OnOK()
{
	if (doProcessImageOnOK && GetScope()>=0 && GetScope2()>=0)
	{
		wxBeginBusyCursor();
		if (GetScope() == 1)
		{
			CreateProgressDlg();
			for (int j = 0; j<cm->slideCount; j++)
            {
                for (int i=0; i<cm->GetFrameCount() && UpdateProgressDlg(i+j*cm->GetFrameCount()); i++)
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
            for (int i=0; i<cm->GetFrameCount() && UpdateProgressDlg(i); i++)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage(cm->Access(i,cm->GetZPos(),false, true), i, cm->GetZPos(), false);
                    cm->Release(i,cm->GetZPos(),false);
                }
                if (GetScope2() != 0)
                {
                    ProcessImage(cm->Access(i,cm->GetZPos(),true, true), i, cm->GetZPos(), true);
                    cm->Release(i,cm->GetZPos(),true);
                }
			}
			DestroyProgressDlg();
		}
		else if (GetScope() == 3)
		{
		    CreateProgressDlg(cm->slideCount);
            for (int i=0; i<cm->slideCount && UpdateProgressDlg(i); i++)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage(cm->Access(cm->GetPos(),i,false, true), cm->GetPos(), i, false);
                    cm->Release(cm->GetPos(),i,false);
                }
                if (GetScope2() != 0)
                {
                    ProcessImage(cm->Access(cm->GetPos(),i,true, true), cm->GetPos(), i, true);
                    cm->Release(cm->GetPos(),i,true);
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
		//if (!IsPreviewOn())
        cm->ReloadCurrentFrame(true, false);
		wxEndBusyCursor();
	}
}

void CorrectContoursPlugin::ProcessImage( ImagePlus *img, int pos, int zPos, bool fluorescence )
{

}

void CorrectContoursPlugin::ProcessImage_static( ImagePlus *source, ImagePlus* target )
{
	target->CloneContours(source);
}
