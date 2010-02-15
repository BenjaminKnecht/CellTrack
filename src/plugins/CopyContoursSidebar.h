#ifndef __CopyContoursSidebar__
#define __CopyContoursSidebar__

#include "Gui.h"
#include <wx/msgdlg.h>
#include "PluginDefinitions.h"
#include "CopyContoursPlugin.h"
class CopyContoursPlugin;

class CopyContoursSidebar : public CopyContoursSidebar_
{
	DEFINE_PLUGIN_SIDEBAR(CopyContours)
	DECLARE_DYNAMIC_CLASS(CopyContoursSidebar)
};

#endif // __CopyContoursSidebar__
