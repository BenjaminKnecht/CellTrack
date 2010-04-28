#ifndef __CorrectContoursSidebar__
#define __CorrectContoursSidebar__

#include "Gui.h"
#include <wx/msgdlg.h>
#include "PluginDefinitions.h"
#include "CorrectContoursPlugin.h"
class CorrectContoursPlugin;

class CorrectContoursSidebar : public CorrectContoursSidebar_
{
	DEFINE_PLUGIN_SIDEBAR(CorrectContours)
	DECLARE_DYNAMIC_CLASS(CorrectContoursSidebar)
};

#endif /*__CorrectContoursSidebar__*/
