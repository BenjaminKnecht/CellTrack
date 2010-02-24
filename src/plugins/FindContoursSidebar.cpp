#include "FindContoursSidebar.h"

IMPLEMENT_DYNAMIC_CLASS( FindContoursSidebar, wxPanel )
DECLARE_PLUGIN_SIDEBAR(FindContours)

void FindContoursSidebar::OnChooseAuto( wxCommandEvent& event )
{
    if (autoThresh->GetValue())
    {
        thresh1->Disable();
        thresh2->Disable();
    }
    else
    {
        thresh1->Enable();
        thresh2->Enable();
    }
    OnChange();
}
