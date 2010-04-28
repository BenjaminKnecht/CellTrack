#include "Preferences.h"
#include "Util.h"
#include "Gui.h"
#include <highgui.h>

wxConfigBase * Preferences::config;
bool Preferences::initialized = false;

#define DECLARE_PREF_GET(TYPE,NAME)	\
	TYPE Preferences::NAME; \
	TYPE Preferences::Get##NAME () { \
		InitializeOnDemand(); \
		return NAME; \
	}
DECLARE_PREF_GET(bool,      SavingCodecUseCombo)
DECLARE_PREF_GET(int,       SavingCodecCombo)
DECLARE_PREF_GET(bool,      SavingCodecUseFOURCC)
DECLARE_PREF_GET(wxString,  SavingCodecFOURCC)
DECLARE_PREF_GET(bool,      SavingSizeOverride)
DECLARE_PREF_GET(wxString,  SavingSizeWidth)
DECLARE_PREF_GET(wxString,  SavingSizeHeight)
DECLARE_PREF_GET(int,       SavingFpsDefault)
DECLARE_PREF_GET(bool,      SavingFpsOverride)
DECLARE_PREF_GET(wxString,  ColorContourBorderColor)
DECLARE_PREF_GET(wxString,  ColorContourPointColor)
DECLARE_PREF_GET(int,       ColorContourBorderWidth)
DECLARE_PREF_GET(wxString,  ColorFContourBorderColor)
DECLARE_PREF_GET(wxString,  ColorFContourPointColor)
DECLARE_PREF_GET(int,       ColorFContourBorderWidth)
DECLARE_PREF_GET(wxString,  ColorTContourBorderColor)
DECLARE_PREF_GET(wxString,  ColorTContourPointColor)
DECLARE_PREF_GET(int,       ColorTContourBorderWidth)
DECLARE_PREF_GET(wxString,  ColorBContourBorderColor)
DECLARE_PREF_GET(wxString,  ColorBContourPointColor)
DECLARE_PREF_GET(int,       ColorBContourBorderWidth)
DECLARE_PREF_GET(bool,      ColorContourPolygonFill)
DECLARE_PREF_GET(wxString,  ColorContourPolygonColor)
DECLARE_PREF_GET(int,       Notebook)
DECLARE_PREF_GET(wxString,  ColorContourSelectedColor)
DECLARE_PREF_GET(wxString,  ColorFeatureColor)


#define SET_CONTROL_VALUE(key)	c_##key->SetValue(key);
#define SET_CONTROL_VALUE_DEFAULT(key)	c_##key->SetValue(d.c_##key->GetValue());
#define GET_CONTROL_VALUE(key)	(key = c_##key->GetValue());
#define SET_CONTROL_SELECTION(key)	c_##key->SetSelection(key);
#define SET_CONTROL_SELECTION_DEFAULT(key)	c_##key->SetSelection(d.c_##key->GetSelection());
#define GET_CONTROL_SELECTION(key)	(key = c_##key->GetSelection());
#define SET_CONTROL_COLOUR(key)	c_##key->SetColour(key);
#define SET_CONTROL_COLOUR_DEFAULT(key)	c_##key->SetColour(d.c_##key->GetColour());
#define GET_CONTROL_COLOUR(key)	(key = c_##key->GetColour().GetAsString(wxC2S_HTML_SYNTAX));

Preferences::Preferences( wxWindow* parent, bool keepDefaults )
:
PreferencesDialog_( parent )
{
	if ( keepDefaults )
		return;
	SET_CONTROL_VALUE(SavingCodecUseCombo)
	SET_CONTROL_SELECTION(SavingCodecCombo)
	SET_CONTROL_VALUE(SavingCodecUseFOURCC)
	SET_CONTROL_VALUE(SavingCodecFOURCC)
	SET_CONTROL_VALUE(SavingSizeOverride)
	SET_CONTROL_VALUE(SavingSizeWidth)
	SET_CONTROL_VALUE(SavingSizeHeight)
	SET_CONTROL_VALUE(SavingFpsOverride)
	SET_CONTROL_VALUE(SavingFpsDefault)
	SET_CONTROL_COLOUR(ColorContourBorderColor)
	SET_CONTROL_COLOUR(ColorContourPointColor)
	SET_CONTROL_VALUE(ColorContourBorderWidth)
	SET_CONTROL_COLOUR(ColorFContourBorderColor)
	SET_CONTROL_COLOUR(ColorFContourPointColor)
	SET_CONTROL_VALUE(ColorFContourBorderWidth)
	SET_CONTROL_COLOUR(ColorTContourBorderColor)
	SET_CONTROL_COLOUR(ColorTContourPointColor)
	SET_CONTROL_VALUE(ColorTContourBorderWidth)
	SET_CONTROL_COLOUR(ColorBContourBorderColor)
	SET_CONTROL_COLOUR(ColorBContourPointColor)
	SET_CONTROL_VALUE(ColorBContourBorderWidth)
	SET_CONTROL_VALUE(ColorContourPolygonFill)
	SET_CONTROL_COLOUR(ColorContourPolygonColor)
	SET_CONTROL_SELECTION(Notebook)
	SET_CONTROL_COLOUR(ColorContourSelectedColor)
	SET_CONTROL_COLOUR(ColorFeatureColor)

}
void Preferences::OnRestoreDefaults( wxCommandEvent& e )
{
	Preferences d(NULL, true);
	SET_CONTROL_VALUE_DEFAULT(SavingCodecUseCombo)
	SET_CONTROL_SELECTION_DEFAULT(SavingCodecCombo)
	SET_CONTROL_VALUE_DEFAULT(SavingCodecUseFOURCC)
	SET_CONTROL_VALUE_DEFAULT(SavingCodecFOURCC)
	SET_CONTROL_VALUE_DEFAULT(SavingSizeOverride)
	SET_CONTROL_VALUE_DEFAULT(SavingSizeWidth)
	SET_CONTROL_VALUE_DEFAULT(SavingSizeHeight)
	SET_CONTROL_VALUE_DEFAULT(SavingFpsOverride)
	SET_CONTROL_VALUE_DEFAULT(SavingFpsDefault)
	SET_CONTROL_COLOUR_DEFAULT(ColorContourBorderColor)
    SET_CONTROL_COLOUR_DEFAULT(ColorContourPointColor)
	SET_CONTROL_VALUE_DEFAULT(ColorContourBorderWidth)
	SET_CONTROL_COLOUR_DEFAULT(ColorFContourBorderColor)
	SET_CONTROL_COLOUR_DEFAULT(ColorFContourPointColor)
	SET_CONTROL_VALUE_DEFAULT(ColorFContourBorderWidth)
	SET_CONTROL_COLOUR_DEFAULT(ColorTContourBorderColor)
	SET_CONTROL_COLOUR_DEFAULT(ColorTContourPointColor)
	SET_CONTROL_VALUE_DEFAULT(ColorTContourBorderWidth)
	SET_CONTROL_COLOUR_DEFAULT(ColorBContourBorderColor)
	SET_CONTROL_COLOUR_DEFAULT(ColorBContourPointColor)
	SET_CONTROL_VALUE_DEFAULT(ColorBContourBorderWidth)
	SET_CONTROL_VALUE_DEFAULT(ColorContourPolygonFill)
	SET_CONTROL_COLOUR_DEFAULT(ColorContourPolygonColor)
//	SET_CONTROL_SELECTION_DEFAULT(Notebook)
	SET_CONTROL_COLOUR_DEFAULT(ColorContourSelectedColor)
	SET_CONTROL_COLOUR_DEFAULT(ColorFeatureColor)

}

void Preferences::OnCancel( wxCommandEvent& e )
{
	EndModal(wxID_CANCEL);
}
void Preferences::OnSave( wxCommandEvent& e )
{
	GET_CONTROL_VALUE(SavingCodecUseCombo)
	GET_CONTROL_SELECTION(SavingCodecCombo)
	GET_CONTROL_VALUE(SavingCodecUseFOURCC)
	GET_CONTROL_VALUE(SavingCodecFOURCC)
	GET_CONTROL_VALUE(SavingSizeOverride)
	GET_CONTROL_VALUE(SavingSizeWidth)
	GET_CONTROL_VALUE(SavingSizeHeight)
	GET_CONTROL_VALUE(SavingFpsOverride)
	GET_CONTROL_VALUE(SavingFpsDefault)
	GET_CONTROL_COLOUR(ColorContourBorderColor)
	GET_CONTROL_COLOUR(ColorContourPointColor)
	GET_CONTROL_VALUE(ColorContourBorderWidth)
	GET_CONTROL_COLOUR(ColorFContourBorderColor)
	GET_CONTROL_COLOUR(ColorFContourPointColor)
	GET_CONTROL_VALUE(ColorFContourBorderWidth)
	GET_CONTROL_COLOUR(ColorTContourBorderColor)
	GET_CONTROL_COLOUR(ColorTContourPointColor)
	GET_CONTROL_VALUE(ColorTContourBorderWidth)
	GET_CONTROL_COLOUR(ColorBContourBorderColor)
	GET_CONTROL_COLOUR(ColorBContourPointColor)
	GET_CONTROL_VALUE(ColorBContourBorderWidth)
	GET_CONTROL_VALUE(ColorContourPolygonFill)
	GET_CONTROL_COLOUR(ColorContourPolygonColor)
	GET_CONTROL_SELECTION(Notebook)
	GET_CONTROL_COLOUR(ColorContourSelectedColor)
	GET_CONTROL_COLOUR(ColorFeatureColor)


#define PREF_WRITE_VALUE(NAME)	\
	config->Write(_T(#NAME), NAME);
#define PREF_WRITE_VALUE_LONG(NAME)	\
	config->Write(_T(#NAME), (long) NAME);

	config->SetPath(_T("/Saving/Codec"));
	PREF_WRITE_VALUE(SavingCodecUseCombo)
	PREF_WRITE_VALUE_LONG(SavingCodecCombo)
	PREF_WRITE_VALUE(SavingCodecUseFOURCC)
	PREF_WRITE_VALUE(SavingCodecFOURCC)
	config->SetPath(_T("/Saving/Size"));
	PREF_WRITE_VALUE(SavingSizeOverride)
	PREF_WRITE_VALUE(SavingSizeWidth)
	PREF_WRITE_VALUE(SavingSizeHeight)
	config->SetPath(_T("/Saving/Fps"));
	PREF_WRITE_VALUE_LONG(SavingFpsDefault)
	PREF_WRITE_VALUE(SavingFpsOverride)
	config->SetPath(_T("/Color"));
	PREF_WRITE_VALUE(ColorContourBorderColor)
	PREF_WRITE_VALUE(ColorContourPointColor)
	PREF_WRITE_VALUE_LONG(ColorContourBorderWidth)
	PREF_WRITE_VALUE(ColorFContourBorderColor)
	PREF_WRITE_VALUE(ColorFContourPointColor)
	PREF_WRITE_VALUE_LONG(ColorFContourBorderWidth)
	PREF_WRITE_VALUE(ColorTContourBorderColor)
	PREF_WRITE_VALUE(ColorTContourPointColor)
	PREF_WRITE_VALUE_LONG(ColorTContourBorderWidth)
	PREF_WRITE_VALUE(ColorBContourBorderColor)
	PREF_WRITE_VALUE(ColorBContourPointColor)
	PREF_WRITE_VALUE_LONG(ColorBContourBorderWidth)
	PREF_WRITE_VALUE(ColorContourPolygonFill)
	PREF_WRITE_VALUE(ColorContourPolygonColor)
	PREF_WRITE_VALUE_LONG(Notebook)
	PREF_WRITE_VALUE(ColorContourSelectedColor)
	PREF_WRITE_VALUE(ColorFeatureColor)

	config->SetPath(_T("/"));
	EndModal(wxID_OK);
}

void Preferences::InitializeOnDemand()
{
	if (initialized)
		return;
	config = wxConfig::Get();
	Preferences d(NULL, true);


#define PREF_INIT_VALUE(NAME)	\
	(NAME) = config->Read(_T(#NAME), d.c_##NAME->GetValue());
#define PREF_INIT_VALUE_LONG(NAME)	\
	(NAME) = config->Read(_T(#NAME), (long) d.c_##NAME->GetValue());
#define PREF_INIT_BOOL(NAME)	\
	(NAME) = Config_ReadBool(config, _T(#NAME), d.c_##NAME->GetValue());
#define PREF_INIT_SELECTION(NAME)	\
	(NAME) = config->Read(_T(#NAME), (long) d.c_##NAME->GetSelection());
#define PREF_INIT_COLOUR(NAME)	\
	(NAME) = config->Read(_T(#NAME), d.c_##NAME->GetColour().GetAsString(wxC2S_HTML_SYNTAX));

	config->SetPath(_T("/Saving/Codec"));
	PREF_INIT_BOOL(SavingCodecUseCombo)
	PREF_INIT_SELECTION(SavingCodecCombo)
	PREF_INIT_BOOL(SavingCodecUseFOURCC)
	PREF_INIT_VALUE(SavingCodecFOURCC)
	config->SetPath(_T("/Saving/Size"));
	PREF_INIT_BOOL(SavingSizeOverride)
	PREF_INIT_VALUE(SavingSizeWidth)
	PREF_INIT_VALUE(SavingSizeHeight)
	config->SetPath(_T("/Saving/Fps"));
	PREF_INIT_VALUE_LONG(SavingFpsDefault)
	PREF_INIT_BOOL(SavingFpsOverride)
	config->SetPath(_T("/Color"));
	PREF_INIT_COLOUR(ColorContourBorderColor)
	PREF_INIT_COLOUR(ColorContourPointColor)
	PREF_INIT_VALUE_LONG(ColorContourBorderWidth)
	PREF_INIT_COLOUR(ColorFContourBorderColor)
	PREF_INIT_COLOUR(ColorFContourPointColor)
	PREF_INIT_VALUE_LONG(ColorFContourBorderWidth)
	PREF_INIT_COLOUR(ColorTContourBorderColor)
	PREF_INIT_COLOUR(ColorTContourPointColor)
	PREF_INIT_VALUE_LONG(ColorTContourBorderWidth)
	PREF_INIT_COLOUR(ColorBContourBorderColor)
	PREF_INIT_COLOUR(ColorBContourPointColor)
	PREF_INIT_VALUE_LONG(ColorBContourBorderWidth)
	PREF_INIT_BOOL(ColorContourPolygonFill)
	PREF_INIT_COLOUR(ColorContourPolygonColor)
	PREF_INIT_SELECTION(Notebook)
	PREF_INIT_COLOUR(ColorContourSelectedColor)
	PREF_INIT_COLOUR(ColorFeatureColor)

	config->SetPath(_T("/"));
	initialized = true;
}

long Preferences::GetSavingCodec()
{
	InitializeOnDemand();
	if (SavingCodecUseFOURCC && SavingCodecFOURCC.length()==4)
		return CV_FOURCC(SavingCodecFOURCC[0],SavingCodecFOURCC[1],SavingCodecFOURCC[2],SavingCodecFOURCC[3]);
	else {
		switch(SavingCodecCombo){
			case 0: return CV_FOURCC('I','Y','U','V');
			case 1: return CV_FOURCC('C','V','I','D');
			case 2: return CV_FOURCC('D','I','V','X');
			case 3: return -1;
		}
	}
	return -1;
}

CvSize Preferences::GetSavingSize()
{
	InitializeOnDemand();
	return cvSize(wxStringToLong(SavingSizeWidth), wxStringToLong(SavingSizeHeight));
}
