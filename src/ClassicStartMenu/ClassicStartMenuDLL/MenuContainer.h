// Classic Shell (c) 2009-2010, Ivo Beltchev
// The sources for Classic Shell are distributed under the MIT open source license

#pragma once

#include "SkinManager.h"
#include <vector>
#include <map>

enum TMenuID
{
	MENU_NO=0,
	MENU_LAST=0,
	MENU_SEPARATOR,
	MENU_EMPTY,

	// standard menu items
	MENU_PROGRAMS,
	MENU_FAVORITES,
	MENU_DOCUMENTS,
		MENU_USERFILES,
		MENU_USERDOCUMENTS,
		MENU_USERPICTURES,
	MENU_SETTINGS,
		MENU_CONTROLPANEL,
		MENU_NETWORK,
		MENU_PRINTERS,
		MENU_TASKBAR,
		MENU_FEATURES,
		MENU_CLASSIC_SETTINGS,
	MENU_SEARCH,
		MENU_SEARCH_FILES,
		MENU_SEARCH_PRINTER,
		MENU_SEARCH_COMPUTERS,
		MENU_SEARCH_PEOPLE,
	MENU_HELP,
	MENU_RUN,
	MENU_LOGOFF,
	MENU_DISCONNECT,
	MENU_UNDOCK,
	MENU_SHUTDOWN_BOX,

	// additional commands
	MENU_CUSTOM, // used for any custom item
	MENU_SLEEP,
	MENU_HIBERNATE,
	MENU_RESTART,
	MENU_SHUTDOWN,
	MENU_SWITCHUSER,
};

struct StdMenuItem
{
	TMenuID id;
	const char *key; // localization key
	const wchar_t *name; // default name
	int icon; // index in shell32.dll
	TMenuID submenuID; // MENU_NO if no submenu
	const KNOWNFOLDERID *folder1; // NULL if not used
	const KNOWNFOLDERID *folder2; // NULL if not used
	const char *tipKey; // localization key for the tooltip
	const wchar_t *tip; // default tooltip
	const StdMenuItem *submenu;
	const wchar_t *link;
	const wchar_t *command;
	const wchar_t *iconPath;
};

class CMenuAccessible;

// CMenuContainer - implementation of a single menu box. Contains one or more vertical toolbars
class CMenuContainer: public CWindowImpl<CMenuContainer>, public IDropTarget
{
public:
	DECLARE_WND_CLASS_EX(L"ClassicShell.CMenuContainer",CS_DROPSHADOW,COLOR_MENU)

	// message handlers
	BEGIN_MSG_MAP( CMenuContainer )
		// forward all messages to m_pMenu2 and m_pMenu3 to ensure the context menu functions properly
		if (m_pMenu3)
		{
			if (SUCCEEDED(m_pMenu3->HandleMenuMsg2(uMsg,wParam,lParam,&lResult)))
				return TRUE;
		}
		else if (m_pMenu2)
		{
			if (SUCCEEDED(m_pMenu2->HandleMenuMsg(uMsg,wParam,lParam)))
			{
				lResult=0;
				return TRUE;
			}
		}
		MESSAGE_HANDLER( WM_CREATE, OnCreate )
		MESSAGE_HANDLER( WM_DESTROY, OnDestroy )
		MESSAGE_HANDLER( MCM_REFRESH, OnRefresh )
		MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBkgnd )
		MESSAGE_HANDLER( WM_ACTIVATE, OnActivate )
		MESSAGE_HANDLER( WM_MOUSEACTIVATE, OnMouseActivate )
		MESSAGE_HANDLER( WM_CONTEXTMENU, OnContextMenu )
		MESSAGE_HANDLER( WM_TIMER, OnTimer )
		MESSAGE_HANDLER( WM_SYSCOMMAND, OnSysCommand )
		MESSAGE_HANDLER( WM_GETOBJECT, OnGetAccObject )
		MESSAGE_HANDLER( MCM_SETHOTITEM, OnSetHotItem )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, NM_CUSTOMDRAW, OnCustomDraw )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, NM_CLICK, OnClick )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, NM_KEYDOWN, OnKeyDown )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, NM_CHAR, OnChar )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, TBN_HOTITEMCHANGE, OnHotItemChange )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, TBN_GETINFOTIP, OnGetInfoTip )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, TBN_DRAGOUT, OnDragOut )
		NOTIFY_RANGE_CODE_HANDLER( ID_TOOLBAR_FIRST, ID_TOOLBAR_LAST, TBN_GETOBJECT, OnGetObject )
		NOTIFY_HANDLER( ID_PAGER, PGN_CALCSIZE, OnPagerCalcSize )
		NOTIFY_HANDLER( ID_PAGER, PGN_SCROLL, OnPagerScroll )
	END_MSG_MAP()

	// options when creating a container
	enum
	{
		CONTAINER_LARGE        = 0x0001, // use large icons
		CONTAINER_MULTICOLUMN  = 0x0002, // use multiple columns instead of a pager
		CONTAINER_NOSUBFOLDERS = 0x0004, // don't go into subfolders (for control panel)
		CONTAINER_PROGRAMS     = 0x0008, // this is a folder from the Start Menu hierarchy (drop operations prefer link over move)
		CONTAINER_DOCUMENTS    = 0x0010, // sort by time, limit the count (for recent documents)
		CONTAINER_LINK         = 0x0020, // this is an expanded link to a folder (always in a pager)
		CONTAINER_ADDTOP       = 0x0040, // put standard items at the top
		CONTAINER_DRAG         = 0x0080, // allow items to be dragged out
		CONTAINER_DROP         = 0x0100, // allow dropping of items
		CONTAINER_LEFT         = 0x0200, // the window is aligned on the left
		CONTAINER_TOP          = 0x0400, // the window is aligned on the top
		CONTAINER_AUTOSORT     = 0x0800, // the menu is always in alphabetical order
	};

	CMenuContainer( CMenuContainer *pParent, int index, int options, const StdMenuItem *pStdItem, PIDLIST_ABSOLUTE path1, PIDLIST_ABSOLUTE path2, const CString &regName );
	~CMenuContainer( void );

	void InitItems( void );
	void InitToolbars( void );

	static bool CloseStartMenu( void );
	static void HideStartMenu( void );
	static bool IsMenuOpened( void ) { return !s_Menus.empty(); }
	static bool IgnoreTaskbarTimers( void ) { return !s_Menus.empty() && (s_TaskbarState&ABS_AUTOHIDE); }
	static HWND ToggleStartMenu( HWND startButton, bool bKeyboard );

	// IUnknown
	virtual STDMETHODIMP QueryInterface( REFIID riid, void **ppvObject )
	{
		*ppvObject=NULL;
		if (IID_IUnknown==riid || IID_IDropTarget==riid)
		{
			AddRef();
			*ppvObject=static_cast<IDropTarget*>(this);
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef( void ) 
	{ 
		return InterlockedIncrement(&m_RefCount);
	}

	virtual ULONG STDMETHODCALLTYPE Release( void )
	{
		long nTemp=InterlockedDecrement(&m_RefCount);
		if (!nTemp) delete this;
		return nTemp;
	}

	// IDropTarget
	virtual HRESULT STDMETHODCALLTYPE DragEnter( IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect );
	virtual HRESULT STDMETHODCALLTYPE DragOver( DWORD grfKeyState, POINTL pt, DWORD *pdwEffect );
	virtual HRESULT STDMETHODCALLTYPE DragLeave( void );
	virtual HRESULT STDMETHODCALLTYPE Drop( IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect );

protected:
	LRESULT OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnRefresh( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnEraseBkgnd( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnActivate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnMouseActivate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnContextMenu( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnSysCommand( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnGetAccObject( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnSetHotItem( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnCustomDraw( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnKeyDown( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnChar( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnHotItemChange( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnGetInfoTip( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnDragOut( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnGetObject( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnPagerCalcSize( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	LRESULT OnPagerScroll( int idCtrl, LPNMHDR pnmh, BOOL& bHandled );
	virtual void OnFinalMessage( HWND ) { Release(); }

private:
	// description of a menu item
	struct MenuItem
	{
		TMenuID id; // if pStdItem!=NULL, this is pStdItem->id. otherwise it can only be MENU_NO, MENU_SEPARATOR or MENU_EMPTY
		const StdMenuItem *pStdItem; // NULL if not a standard menu item
		CString name;
		unsigned int nameHash;
		int icon;
		int column; // index in m_Toolbars
		int btnIndex; // button index in the toolbar
		bool bFolder; // this is a folder - draw arrow
		bool bLink; // this is a link (if a link to a folder is expanded it is always in a pager)
		bool bPrograms; // this item is part of the Start Menu folder hierarchy

		// pair of shell items. 2 items are used to combine a user folder with a common folder (I.E. user programs/common programs)
		PIDLIST_ABSOLUTE pItem1;
		PIDLIST_ABSOLUTE pItem2;
		union
		{
			UINT accelerator; // accelerator character, 0 if none
			FILETIME time; // timestamp of the file (for sorting recent documents)
		};

		bool operator<( const MenuItem &x ) const
		{
			if (btnIndex<x.btnIndex) return true;
			if (btnIndex>x.btnIndex) return false;
			if (bFolder && !x.bFolder) return true;
			if (!bFolder && x.bFolder) return false;
			if (bFolder)
			{
				const wchar_t *drive1=name.IsEmpty()?NULL:wcschr((const wchar_t*)name+1,':');
				const wchar_t *drive2=x.name.IsEmpty()?NULL:wcschr((const wchar_t*)x.name+1,':');
				if (drive1 && !drive2) return true;
				if (!drive1 && drive2) return false;
				if (drive1)
					return drive1[-1]<drive2[-1];
			}
			return CompareString(LOCALE_USER_DEFAULT,LINGUISTIC_IGNORECASE,name,-1,x.name,-1)==CSTR_LESS_THAN;
		}
	};

	struct SortMenuItem
	{
		CString name;
		unsigned int nameHash;
		bool bFolder;

		bool operator<( const SortMenuItem &x ) const
		{
			if (bFolder && !x.bFolder) return true;
			if (!bFolder && x.bFolder) return false;
			if (bFolder)
			{
				const wchar_t *drive1=name.IsEmpty()?NULL:wcschr((const wchar_t*)name+1,':');
				const wchar_t *drive2=x.name.IsEmpty()?NULL:wcschr((const wchar_t*)x.name+1,':');
				if (drive1 && !drive2) return true;
				if (!drive1 && drive2) return false;
				if (drive1)
					return drive1[-1]<drive2[-1];
			}
			return CompareString(LOCALE_USER_DEFAULT,LINGUISTIC_IGNORECASE,name,-1,x.name,-1)==CSTR_LESS_THAN;
		}
	};

	LONG m_RefCount;
	bool m_bRefreshPosted;
	bool m_bDestroyed; // the menu is destroyed but not yet deleted
	int m_Options;
	const StdMenuItem *m_pStdItem; // the first item
	CMenuContainer *m_pParent; // parent menu
	int m_Submenu; // the item index of the opened submenu
	int m_ParentIndex; // the index of this menu in the parent (usually matches m_pParent->m_Submenu)
	CString m_RegName; // name of the registry key to store the item order
	PIDLIST_ABSOLUTE m_Path1;
	PIDLIST_ABSOLUTE m_Path2;
	CComPtr<IShellFolder> m_pDropFolder; // the primary folder (used only as a drop target)

	std::vector<MenuItem> m_Items; // all items in the menu (including separators)
	std::vector<CWindow> m_Toolbars; // one toolbar for each menu column
	CWindow m_Pager; // pager control if needed
	CComQIPtr<IContextMenu2> m_pMenu2; // additional interfaces used when a context menu is displayed
	CComQIPtr<IContextMenu3> m_pMenu3;

	CWindow m_DropToolbar;
	int m_DragHoverTime;
	int m_DragHoverItem;
	int m_DragIndex; // the index of the item being dragged
	CComPtr<IDropTargetHelper> m_pDropTargetHelper; // to show images while dragging
	CComPtr<IDataObject> m_pDragObject;

	LONG m_ClickTime; // last click time (for detecting double clicks)
	POINT m_ClickPos; // last click position (for detecting double clicks)
	DWORD m_HotPos; // last mouse position over a hot item (used to ignore TBN_HOTITEMCHANGE when the mouse didn't really move)
	int m_HoverItem; // item under the mouse (used for opening a submenu when the mouse hovers over an item)
	int m_ContextItem; // force this to be the hot item while a context menu is up
	HBITMAP m_Bitmap; // the background bitmap
	HRGN m_Region; // the outline region
	RECT m_rContent;
	CMenuAccessible *m_pAccessible;

	// additional commands for the context menu
	enum
	{
		CMD_OPEN_ALL=1,
		CMD_SORT,
		CMD_AUTOSORT,
		CMD_NEWFOLDER,

		CMD_LAST
	};

	// ways to activate a menu item
	enum TActivateType
	{
		ACTIVATE_SELECT, // just selects the item
		ACTIVATE_OPEN, // opens the submenu or selects if not a menu
		ACTIVATE_OPEN_KBD, // same as above, but when done with a keyboard
		ACTIVATE_EXECUTE, // executes the item
		ACTIVATE_MENU, // shows context menu
	};

	enum
	{
		// control IDs
		ID_PAGER=1,
		ID_TOOLBAR_FIRST=2,
		ID_TOOLBAR_LAST=1000,

		// added to the m_Item index to get the button user data. ensures the user data is used correctly
		ID_OFFSET=1000,

		// timer ID
		TIMER_HOVER=1,

		MCM_REFRESH=WM_USER+10, // posted to force the container to refresh its contents
		MCM_SETHOTITEM=WM_USER+11, // sets the hot item. wParam is the nameHash of the item
	};

	CWindow CreateToolbar( int id );
	// pPt - optional point in screen space (used only by ACTIVATE_EXECUTE and ACTIVATE_MENU)
	void ActivateItem( int index, TActivateType type, const POINT *pPt );
	void ShowKeyboardCues( void );
	void SetActiveWindow( void );
	void CreateBackground( int width, int height ); // width, height - the content area
	void PostRefreshMessage( void );
	void SaveItemOrder( const std::vector<SortMenuItem> &items );
	void LoadItemOrder( void );
	void FadeOutItem( int index );

	static int s_MaxRecentDocuments; // limit for the number of recent documents
	static bool s_bScrollMenus; // global scroll menus setting
	static bool s_bRTL; // RTL layout
	static bool s_bKeyboardCues; // show keyboard cues
	static bool s_bExpandRight; // prefer expanding submenus to the right
	static bool s_bBehindTaskbar; // the main menu is behind the taskbar (when the taskbar is horizontal)
	static bool s_bShowTopEmpty; // shows the empty item on the top menu so the user can drag items there
	static bool s_bNoDragDrop; // disables drag/drop
	static bool s_bNoContextMenu; // disables the context menu
	static bool s_bExpandLinks; // expand links to folders
	static char s_bActiveDirectory; // the Active Directory services are available (-1 - uninitialized)
	static CMenuContainer *s_pDragSource; // the source of the current drag operation
	static bool s_bRightDrag; // dragging with the right mouse button
	static RECT s_MainRect; // area of the main monitor
	static DWORD s_TaskbarState; // the state of the taskbar (ABS_AUTOHIDE and ABS_ALWAYSONTOP)
	static DWORD s_HoverTime;
	static DWORD s_SubmenuStyle;
	static CLIPFORMAT s_ShellFormat; // CFSTR_SHELLIDLIST
	static CComPtr<IShellFolder> s_pDesktop; // cached pointer of the desktop object
	static HWND s_LastFGWindow; // stores the foreground window to restore later when the menu closes

	static std::vector<CMenuContainer*> s_Menus; // all menus, in cascading order
	static std::map<unsigned int,int> s_PagerScrolls; // scroll offset for each sub menu

	static MenuSkin s_Skin;

	static LRESULT CALLBACK ToolbarSubclassProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData );
	static LRESULT CALLBACK PagerSubclassProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData );

	friend class COwnerWindow;
	friend class CMenuAccessible;
};

class CMenuFader: public CWindowImpl<CMenuFader>
{
public:
	CMenuFader( HBITMAP bmp, HRGN region, int duration, RECT &rect );
	~CMenuFader( void );
	DECLARE_WND_CLASS_EX(L"ClassicShell.CMenuFader",0,COLOR_MENU)

	// message handlers
	BEGIN_MSG_MAP( CMenuFader )
		MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBkgnd )
		MESSAGE_HANDLER( WM_TIMER, OnTimer )
	END_MSG_MAP()

	void Create( void );

	static void ClearAll( void );

protected:
	LRESULT OnEraseBkgnd( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	LRESULT OnTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled );
	virtual void OnFinalMessage( HWND ) { PostQuitMessage(0); delete this; }

private:
	int m_Time0;
	int m_Duration;
	int m_LastTime;
	HBITMAP m_Bitmap;
	HRGN m_Region;
	RECT m_Rect;

	static std::vector<CMenuFader*> s_Faders;
};
