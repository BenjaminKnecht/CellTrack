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
            for (int i=0; i<cm->GetFrameCount(); i++)
            {
                for (int j=0; j<cm->slideCount-1 && UpdateProgressDlg(i*cm->slideCount+j); j++)
                {
                    if (GetScope2() != 1)
                    {
                        ProcessImage_static(cm->Access(i,j,false, true), cm->Access(i,j+1,false, true));
                    }
                    if (GetScope2() != 0)
                    {
                        ProcessImage_static(cm->Access(i,j,true, true), cm->Access(i,j+1,true, true));
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
                if (cm->GetZPos() == cm->slideCount-1)
                {
                    if (GetScope2() != 1)
                    {
                        ProcessImage_static(cm->Access(i,cm->GetZPos()-1,false, true), cm->Access(i,cm->GetZPos(),false, true));
                    }
                    if (GetScope2() != 0)
                    {
                        ProcessImage_static(cm->Access(i,cm->GetZPos()-1,true, true), cm->Access(i,cm->GetZPos(),true, true));
                    }
                }
                else
                {
                    if (GetScope2() != 1)
                    {
                        ProcessImage_static(cm->Access(i,cm->GetZPos(),false, true), cm->Access(i,cm->GetZPos()+1,false, true));
                    }
                    if (GetScope2() != 0)
                    {
                        ProcessImage_static(cm->Access(i,cm->GetZPos(),true, true), cm->Access(i,cm->GetZPos()+1,true, true));
                    }
                }
			}
			DestroyProgressDlg();
		}
		else if (GetScope() == 3)
		{
		    CreateProgressDlg(cm->slideCount);
            for (int i=0; i<cm->slideCount-1 && UpdateProgressDlg(i); i++)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage_static(cm->Access(cm->GetPos(),i,false, true), cm->Access(cm->GetPos(),i+1,false, true));
                }
                if (GetScope2() != 0)
                {
                    ProcessImage_static(cm->Access(cm->GetPos(),i,true, true), cm->Access(cm->GetPos(),i+1,true, true));
                }
			}
			DestroyProgressDlg();
		}
		else
		{
		    if (cm->GetZPos() == cm->slideCount-1)
            {
                if (GetScope2() != 1)
                {
                    ProcessImage_static(cm->Access(cm->GetPos(),cm->GetZPos()-1,false, true), cm->Access(cm->GetPos(),cm->GetZPos(),false, true));
                }
                if (GetScope2() != 0)
                {
                    ProcessImage_static(cm->Access(cm->GetPos(),cm->GetZPos()-1,true, true), cm->Access(cm->GetPos(),cm->GetZPos(),true, true));
                }
            }
            else
            {
                if (GetScope2() != 1)
                {
                    ProcessImage_static(cm->Access(cm->GetPos(),cm->GetZPos(),false, true), cm->Access(cm->GetPos(),cm->GetZPos()+1,false, true));
                }
                if (GetScope2() != 0)
                {
                    ProcessImage_static(cm->Access(cm->GetPos(),cm->GetZPos(),true, true), cm->Access(cm->GetPos(),cm->GetZPos()+1,true, true));
                }
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

void CorrectContoursPlugin::ProcessImage_static( ImagePlus* bottom, ImagePlus* top )
{
    // finding the largest contours in the image
    // (in case the contours have not been cleaned up properly)
    int numBottomCells = bottom->contourArray.size();
    int numTopCells = top->contourArray.size();
    if (numBottomCells == 0 || numTopCells == 0)
        return;
    CvSeq* bottomCell;
    CvSeq* topCell;
    bool dirty = false;
    float maxArea = 0.0f;
    // finding largest cell contour at lower image
    for (int i = 0; i<numBottomCells; i++)
    {
        float area = fabs(cvContourArea(bottom->contourArray[i]));
        if (area > maxArea)
        {
            maxArea = area;
            bottomCell = bottom->contourArray[i];
        }
    }
    int biggestCellIndex = 0;
    // finding largest cell contour at upper image
    maxArea = 0.0f;
    for (int i = 0; i<numTopCells; i++)
    {
        float area = fabs(cvContourArea(top->contourArray[i]));
        if (area > maxArea)
        {
            maxArea = area;
            biggestCellIndex = i;
            topCell = top->contourArray[i];
        }
    }
    // the top must be smaller than the bottom
    // so we check for every point of the top cell if it is inside the bottom cell

    //CvPoint* topPointsInt = (CvPoint*)malloc( topCell->total*sizeof(CvPoint) );
    CvPoint2D32f* topPoints = (CvPoint2D32f*)malloc( topCell->total*sizeof(CvPoint2D32f) );
    cvCvtSeqToArray(topCell, topPoints);
    for (int i=0; i<topCell->total; i++)
    {
        // this part is from the cvPointPolygonTest
        CvSeqReader reader;
        int counter = 0;
        int total = bottomCell->total;
        cvStartReadSeq( bottomCell, &reader, -1 );
        CvPoint2D32f v0, v, min0, min, pt = topPoints[i];
        CV_READ_SEQ_ELEM( v, reader );
        double distance = 0;
        double min_dist_num = FLT_MAX, min_dist_denom = 1;
        for( int j = 0; j < total; j++ )
        {
            double dx, dy, dx1, dy1, dx2, dy2, dist_num, dist_denom = 1;

            v0 = v;
            CV_READ_SEQ_ELEM( v, reader );

            dx = v.x - v0.x; dy = v.y - v0.y;
            dx1 = pt.x - v0.x; dy1 = pt.y - v0.y;
            dx2 = pt.x - v.x; dy2 = pt.y - v.y;

            if( dx1*dx + dy1*dy <= 0 )
                dist_num = dx1*dx1 + dy1*dy1;
            else if( dx2*dx + dy2*dy >= 0 )
                dist_num = dx2*dx2 + dy2*dy2;
            else
            {
                dist_num = (dy1*dx - dx1*dy);
                dist_num *= dist_num;
                dist_denom = dx*dx + dy*dy;
            }

            if( dist_num*min_dist_denom < min_dist_num*dist_denom )
            {
                min_dist_num = dist_num;
                min_dist_denom = dist_denom;
                min0 = v0;
                min = v;
                if( min_dist_num == 0 )
                    break;
            }

            if( (v0.y <= pt.y && v.y <= pt.y) ||
                (v0.y > pt.y && v.y > pt.y) ||
                (v0.x < pt.x && v.x < pt.x) )
                continue;

            dist_num = dy1*dx - dx1*dy;
            if( dy < 0 )
                dist_num = -dist_num;
            counter += dist_num > 0;
        }
        distance = sqrt(min_dist_num/min_dist_denom);
        if( counter % 2 == 0 )
            distance = -distance;

        // point outside of contour!
        if (distance < 0)
        {
            double a = ((min0.x - min.x)/(min0.y - min.y));
            double b = topPoints[i].y + 1/((min0.x - min.x)/(min0.y - min.y))*topPoints[i].x;
            double c = min.y - ((min0.x - min.x)/(min0.y - min.y))*min.x;
            double dnewx = (b - c)/(a+1/a);
            double dnewy = -(1/a)*dnewx+b;
            int newy = ((int)dnewy);
            int newx = ((int)dnewx);
            if (dnewx > topPoints[i].x)
                newx += 1;
            if (dnewy > topPoints[i].y)
                newy += 1;
            topPoints[i].x = newx;
            topPoints[i].y = newy;
            dirty = true;
        }
    }
    top->ReplaceContour(biggestCellIndex, topPoints, topCell->total);
}
