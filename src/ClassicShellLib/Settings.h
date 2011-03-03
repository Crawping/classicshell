// Classic Shell (c) 2009-2011, Ivo Beltchev
// The sources for Classic Shell are distributed under the MIT open source license

#pragma once

// 
class ISettingsPanel
{
public:
	virtual HWND Create( HWND parent )=0;
	virtual HWND Activate( struct CSetting *pGroup, const RECT &rect, bool bReset )=0;
	virtual bool Validate( HWND parent )=0;
};

struct CSetting
{
	enum Type
	{
		TYPE_GROUP=-2,
		TYPE_RADIO=-1,
		TYPE_BOOL=1,
		TYPE_INT,
		TYPE_HOTKEY,

		// string types
		TYPE_STRING,
		TYPE_ICON,
		TYPE_SOUND,
		TYPE_MULTISTRING,
	};

	enum
	{
		// initial settings
		FLAG_WARM=1,
		FLAG_COLD=2,
		FLAG_BASIC=4,
		FLAG_HIDDEN=8,

		// for run-time use only
		FLAG_DEFAULT=1024,
		FLAG_LOCKED_REG=2048, // locked by HKLM registry setting
		FLAG_LOCKED_GP=4096, // locked by a group policy
		FLAG_LOCKED_MASK=FLAG_LOCKED_REG|FLAG_LOCKED_GP,
		FLAG_WARNING=8192, // show a warning icon
	};

	const wchar_t *name;
	Type type;
	int nameID, tipID; // resource strings
	CComVariant defValue; // default value
	unsigned int flags;
	const wchar_t *depend;
	ISettingsPanel *pPanel; // custom panel for editing this group of settings

	// runtime use only
	CComVariant value; // current value
	CComVariant tempValue; // the value is stored here when editing begins and restored if the editing is canceled
	unsigned int tempFlags;
};

// Images in the tree image list
enum {
	SETTING_STATE_NONE=1,
	SETTING_STATE_SETTING=2,
	SETTING_STATE_CHECKBOX=4,
	SETTING_STATE_RADIO=8,

	// additional flags
	SETTING_STATE_DISABLED=1,
	SETTING_STATE_CHECKED=2,
};

void InitSettings( CSetting *pSettings, bool bMenu );
void LoadSettings( void );
void UpdateDefaultSettings( void );
void EditSettings( const wchar_t *title, bool bModal );
void CloseSettings( void );
void UpdateSettings( void ); // implemented by the user
void ClosingSettings( HWND hWnd, int flags, int command ); // implemented by the user
const wchar_t *GetDocRelativePath( void ); // implemented by the user
bool IsSettingsMessage( MSG *msg );
const CSetting *GetAllSettings( void );

// Updates the setting with a new default value, and locked/hidden flags
void UpdateSetting( const wchar_t *name, const CComVariant &defValue, bool bLockedGP, bool bHidden=false );
// Updates the setting with a new tooltip and a warning flag
void UpdateSetting( const wchar_t *name, int tipID, bool bWarning );

bool GetSettingBool( const wchar_t *name );
int GetSettingInt( const wchar_t *name );
CString GetSettingString( const wchar_t *name );

// In some cases the default can change dynamically, so the setting may be out of date. Use bDef to detect if the default value should be used
int GetSettingInt( const wchar_t *name, bool &bDef );
bool GetSettingBool( const wchar_t *name, bool &bDef );

struct CSettingsLockRead
{
	CSettingsLockRead( void );
	~CSettingsLockRead( void );
};

struct CSettingsLockWrite
{
	CSettingsLockWrite( void );
	~CSettingsLockWrite( void );
};
