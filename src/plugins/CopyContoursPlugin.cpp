#include "CopyContoursPlugin.h"
//#include "Util.h"

CopyContoursPlugin::CopyContoursPlugin( wxWindow* parent_, MyFrame *win_ ): PluginBase(GetStaticName(), parent_, win_, true, true)
{
	sidebar =  new CopyContoursSidebar(parent_, this);
	sidebarw = sidebar;
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
	ProcessImage(&cm->img, cm->GetPos(), cm->GetZPos());
	cm->Redraw(false);
	ShowProcessInfo();
}

void CopyContoursPlugin::ProcessImage( ImagePlus *img, int pos, int zPos )
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
            targetFluorescence = img->isFluorescence;
        }
        else return;
    }
    else if (mode == 1)
    {
        if (zPos > 0)
        {
            targetPos = pos;
            targetZPos = zPos-1;
            targetFluorescence = img->isFluorescence;
        }
        else return;
    }
    else
    {
        targetPos = pos;
        targetZPos = zPos;
        targetFluorescence = !(img->isFluorescence);
        if(mode == 3 && img->isFluorescence)
            return;
        if(mode == 2 && !(img->isFluorescence))
            return;
    }
    ImagePlus* target = cm->Access(targetPos, targetZPos, targetFluorescence);
    ProcessImage_static(img, target);
    cm->Release(targetPos, targetZPos, targetFluorescence);
}

void CopyContoursPlugin::ProcessImage_static( ImagePlus *source, ImagePlus* target )
{
	target->CloneContours(source);
}
