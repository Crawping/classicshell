// Classic Shell (c) 2009-2010, Ivo Beltchev
// The sources for Classic Shell are distributed under the MIT open source license

// ExplorerBand.cpp : Implementation of CExplorerBand

#include "stdafx.h"
#include "ExplorerBand.h"
#include "resource.h"
#include "ExplorerBHO.h"
#include "ResourceHelper.h"
#include "SettingsUI.h"
#include "Settings.h"
#include "SettingsParser.h"
#include "Translations.h"
#include "dllmain.h"
#include "SettingsUIHelper.h"
#include <shdeprecated.h>

///////////////////////////////////////////////////////////////////////////////

// CBandWindow - the parent window of the toolbar

static struct
{
	const wchar_t *name;
	int id;
} g_StdCommands[]={
	{L"up",CBandWindow::ID_GOUP},
	{L"cut",CBandWindow::ID_CUT},
	{L"copy",CBandWindow::ID_COPY},
	{L"paste",CBandWindow::ID_PASTE},
	{L"delete",CBandWindow::ID_DELETE},
	{L"properties",CBandWindow::ID_PROPERTIES},
	{L"email",CBandWindow::ID_EMAIL},
	{L"settings",CBandWindow::ID_SETTINGS},
	{L"moveto",CBandWindow::ID_MOVETO},
	{L"copyto",CBandWindow::ID_COPYTO},
	{L"undo",CBandWindow::ID_UNDO},
	{L"redo",CBandWindow::ID_REDO},
	{L"selectall",CBandWindow::ID_SELECTALL},
	{L"deselect",CBandWindow::ID_DESELECT},
	{L"invertselection",CBandWindow::ID_INVERT},
	{L"back",CBandWindow::ID_GOBACK},
	{L"forward",CBandWindow::ID_GOFORWARD},
	{L"refresh",CBandWindow::ID_REFRESH},
	{L"stop",CBandWindow::ID_STOP},
	{L"rename",CBandWindow::ID_RENAME},
	{L"viewtiles",CBandWindow::ID_VIEW_TILES},
	{L"viewdetails",CBandWindow::ID_VIEW_DETAILS},
	{L"viewlist",CBandWindow::ID_VIEW_LIST},
	{L"viewcontent",CBandWindow::ID_VIEW_CONTENT},
	{L"viewicons_small",CBandWindow::ID_VIEW_ICONS1},
	{L"viewicons_medium",CBandWindow::ID_VIEW_ICONS2},
	{L"viewicons_large",CBandWindow::ID_VIEW_ICONS3},
	{L"viewicons_extralarge",CBandWindow::ID_VIEW_ICONS4},
};

void CBandWindow::ParseToolbarItem( const wchar_t *name, StdToolbarItem &item )
{
	wchar_t text[256];
	Sprintf(text,_countof(text),L"%s.Command",name);
	const wchar_t *str=m_Parser.FindSetting(text);

	item.id=ID_CUSTOM;
	if (str)
	{
		for (int i=0;i<_countof(g_StdCommands);i++)
			if (_wcsicmp(str,g_StdCommands[i].name)==0)
			{
				item.id=g_StdCommands[i].id;
				break;
			}
	}

	if (item.id==ID_CUSTOM)
	{
		item.command=str;
		item.regName=name;
	}

	Sprintf(text,_countof(text),L"%s.Link",name);
	item.link=m_Parser.FindSetting(text);
	if (item.link)
	{
		const wchar_t *c=wcschr(item.link,'|');
		if (c)
		{
			for (c++;*c==' ';)
				c++;
			item.link=c;
		}
	}

	Sprintf(text,_countof(text),L"%s.Icon",name);
	item.iconPath=m_Parser.FindSetting(text);

	Sprintf(text,_countof(text),L"%s.IconDisabled",name);
	item.iconPathD=m_Parser.FindSetting(text);

	Sprintf(text,_countof(text),L"%s.Label",name);
	str=m_Parser.FindSetting(text);
	if (str)
	{
		if (str[0]=='$')
			item.label=FindTranslation(str+1,NULL);
		else
			item.label=str;
	}

	Sprintf(text,_countof(text),L"%s.Tip",name);
	str=m_Parser.FindSetting(text);
	if (str)
	{
		if (str[0]=='$')
			item.tip=FindTranslation(str+1,NULL);
		else
			item.tip=str;
	}

}

void CBandWindow::ParseToolbar( void )
{
	m_Items.clear();
	CString setting=GetSettingString(L"ToolbarItems");
	m_Parser.LoadText(setting,setting.GetLength());
	m_Parser.ParseText();
	std::vector<CSettingsParser::TreeItem> items;
	m_Parser.ParseTree(L"Items",items);
	m_Items.resize(items.size());
	for (int i=0;i<(int)items.size();i++)
	{
		const wchar_t *name=items[i].name;
		StdToolbarItem &item=m_Items[i];
		{
			// can't use memset here because item is not a POD (there is a CString inside)
			item.id=0;
			item.tip=NULL;
			item.label=NULL;
			item.command=NULL;
			item.link=NULL;
			item.iconPath=NULL;
			item.iconPathD=NULL;
			item.submenu=NULL;
			item.menuIcon=NULL;
			item.menuIconD=NULL;
			item.bIconLoaded=false;
			item.bDisabled=false;
			item.bChecked=false;
		}

		// handle special names
		if (!*name)
		{
			item.id=ID_LAST;
			continue;
		}
		if (_wcsicmp(name,L"SEPARATOR")==0)
		{
			item.id=ID_SEPARATOR;
			continue;
		}

		ParseToolbarItem(name,item);
		int idx=items[i].children;
		if (idx>=0)
			item.submenu=&m_Items[idx];
	}
	if (m_Items.size()==1)
	{
		m_Items.resize(2);
		m_Items[1]=m_Items[0];
		m_Items[0].id=ID_SETTINGS;
		m_Items[0].tip=FindTranslation(L"$Toolbar.Settings",NULL);
		m_Items[0].command=L"settings";
		m_Items[0].iconPath=L",1";
	}
}

LRESULT CBandWindow::OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	ParseToolbar();

	bool bLabels=false;
	bool bList=GetSettingBool(L"ToolbarListMode");
	int mainCount=0; // number of the main items
	for (std::vector<StdToolbarItem>::const_iterator it=m_Items.begin();it->id!=ID_LAST;++it,mainCount++)
		if (it->label)
			bLabels=true;

	// create the toolbar
	if (bLabels && !bList)
		m_Toolbar=CreateWindow(TOOLBARCLASSNAME,L"",WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT|CCS_NODIVIDER|CCS_NOPARENTALIGN|CCS_NORESIZE,0,0,10,10,m_hWnd,(HMENU)101,g_Instance,NULL);
	else
		m_Toolbar=CreateWindow(TOOLBARCLASSNAME,L"",WS_CHILD|TBSTYLE_TOOLTIPS|TBSTYLE_FLAT|TBSTYLE_LIST|CCS_NODIVIDER|CCS_NOPARENTALIGN|CCS_NORESIZE,0,0,10,10,m_hWnd,(HMENU)101,g_Instance,NULL);

	m_Toolbar.SendMessage(TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DRAWDDARROWS|TBSTYLE_EX_HIDECLIPPEDBUTTONS);
	m_Toolbar.SendMessage(TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON));
	m_Toolbar.SendMessage(TB_SETMAXTEXTROWS,1);

	int iconSize=GetSettingInt(GetSettingBool(L"UseBigButtons")?L"LargeIconSize":L"SmallIconSize");
	if (iconSize<8) iconSize=8;
	else if (iconSize>128) iconSize=128;

	m_MenuIconSize=GetSettingInt(L"MenuIconSize");
	if (m_MenuIconSize<=0) m_MenuIconSize=0;
	else if (m_MenuIconSize<8) m_MenuIconSize=8;
	if (m_MenuIconSize>32) m_MenuIconSize=32;

	m_ImgEnabled=ImageList_Create(iconSize,iconSize,ILC_COLOR32|ILC_MASK|(IsLanguageRTL()?ILC_MIRROR:0),0,mainCount);
	m_ImgDisabled=ImageList_Create(iconSize,iconSize,ILC_COLOR32|ILC_MASK|(IsLanguageRTL()?ILC_MIRROR:0),0,mainCount);

	HMODULE hShell32=GetModuleHandle(L"Shell32.dll");
	std::vector<HMODULE> modules;

	bool bSame=GetSettingBool(L"SameSizeButtons");

	// create buttons
	std::vector<TBBUTTON> buttons(mainCount);
	for (int i=0;i<mainCount;i++)
	{
		const StdToolbarItem &item=m_Items[i];
		TBBUTTON &button=buttons[i];

		button.idCommand=i+1;
		button.dwData=i;

		if (item.id==ID_SEPARATOR)
			button.fsStyle=BTNS_SEP;
		else
		{
			button.iBitmap=I_IMAGENONE;
			if (item.iconPath)
			{
				if (_wcsicmp(item.iconPath,L"NONE")!=0)
				{
					HICON hIcon=LoadIcon(iconSize,item.iconPath,modules);
					if (!hIcon)
						hIcon=(HICON)LoadImage(hShell32,MAKEINTRESOURCE(1),IMAGE_ICON,iconSize,iconSize,LR_DEFAULTCOLOR);
					if (hIcon)
					{
						button.iBitmap=ImageList_AddIcon(m_ImgEnabled,hIcon);
						HICON hIcon2=item.iconPathD?LoadIcon(iconSize,item.iconPathD,modules):NULL;
						if (!hIcon2)
							hIcon2=CreateDisabledIcon(hIcon,iconSize);
						int idx=ImageList_AddIcon(m_ImgDisabled,hIcon2);
						ATLASSERT(button.iBitmap==idx);
						DestroyIcon(hIcon);
						DestroyIcon(hIcon2);
					}
				}
			}
			else if (item.link)
			{
				HICON hIcon=NULL;
				PIDLIST_ABSOLUTE pidl=NULL;
				SFGAOF flags=0;
				wchar_t path[_MAX_PATH];
				Strcpy(path,_countof(path),item.link);
				DoEnvironmentSubst(path,_countof(path));
				if (SUCCEEDED(SHParseDisplayName(path,NULL,&pidl,0,&flags)) && pidl)
				{
					hIcon=LoadIcon(iconSize,pidl);
					ILFree(pidl);
				}
				if (hIcon)
				{
					button.iBitmap=ImageList_AddIcon(m_ImgEnabled,hIcon);
					HICON hIcon2=item.iconPathD?LoadIcon(iconSize,item.iconPathD,modules):NULL;
					if (!hIcon2)
						hIcon2=CreateDisabledIcon(hIcon,iconSize);
					int idx=ImageList_AddIcon(m_ImgDisabled,hIcon2);
					ATLASSERT(button.iBitmap==idx);
					DestroyIcon(hIcon2);
					DestroyIcon(hIcon);
				}
			}

			button.fsState=(item.id!=ID_SETTINGS || GetSettingBool(L"EnableSettings"))?TBSTATE_ENABLED:0;
			button.fsStyle=BTNS_BUTTON|BTNS_NOPREFIX;
			if (!bSame)
				button.fsStyle|=BTNS_AUTOSIZE;
			if (item.label)
			{
				button.fsStyle|=BTNS_SHOWTEXT;
				button.iString=(INT_PTR)item.label;
			}

			bool bFolder=false;
			if (item.link)
			{
				wchar_t path[_MAX_PATH];
				Strcpy(path,_countof(path),item.link);
				DoEnvironmentSubst(path,_countof(path));
				PIDLIST_RELATIVE pidl;
				SFGAOF flags=0;
				if (SUCCEEDED(SHParseDisplayName(path,NULL,&pidl,SFGAO_FOLDER,&flags)))
				{
					ILFree(pidl);
					if (flags&SFGAO_FOLDER)
						bFolder=true;
				}
			}

			if (item.submenu || bFolder)
				button.fsStyle|=(item.id<ID_CUSTOM || item.command || (item.link && item.submenu))?BTNS_DROPDOWN:BTNS_WHOLEDROPDOWN;
		}
	}

	for (std::vector<HMODULE>::const_iterator it=modules.begin();it!=modules.end();++it)
		FreeLibrary(*it);

	// add buttons
	HIMAGELIST old=(HIMAGELIST)m_Toolbar.SendMessage(TB_SETIMAGELIST,0,(LPARAM)m_ImgEnabled);
	if (old) ImageList_Destroy(old);
	old=(HIMAGELIST)m_Toolbar.SendMessage(TB_SETDISABLEDIMAGELIST,0,(LPARAM)m_ImgDisabled);
	if (old) ImageList_Destroy(old);
	if (!buttons.empty())
		m_Toolbar.SendMessage(TB_ADDBUTTONS,buttons.size(),(LPARAM)&buttons[0]);
	SendMessage(WM_CLEAR);
	return 0;
}

LRESULT CBandWindow::OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	if (m_ImgEnabled) ImageList_Destroy(m_ImgEnabled); m_ImgEnabled=NULL;
	if (m_ImgDisabled) ImageList_Destroy(m_ImgDisabled); m_ImgDisabled=NULL;
	for (std::vector<StdToolbarItem>::iterator it=m_Items.begin();it!=m_Items.end();++it)
	{
		if (it->menuIcon) DeleteObject(it->menuIcon);
		if (it->menuIconD) DeleteObject(it->menuIconD);
	}
	m_Items.clear();
	bHandled=FALSE;
	return 0;
}

LRESULT CBandWindow::OnUpdateUI( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	// update the state of the custom buttons based on the registry settings
	CRegKey regSettings;
	if (regSettings.Open(HKEY_CURRENT_USER,L"Software\\IvoSoft\\ClassicExplorer")==ERROR_SUCCESS)
	{
		bool bMain=true;
		for (int idx=0;idx<(int)m_Items.size();idx++)
		{
			if (m_Items[idx].id==ID_LAST)
				bMain=false;
			if (!m_Items[idx].regName.IsEmpty())
			{
				DWORD val;
				if (regSettings.QueryDWORDValue(m_Items[idx].regName,val)!=ERROR_SUCCESS)
					val=0;
				bool bDisabled=(val&1)!=0;
				bool bChecked=(val&2)!=0;
				if (bMain)
				{
					if (bDisabled!=m_Items[idx].bDisabled) m_Toolbar.SendMessage(TB_ENABLEBUTTON,idx+1,bDisabled?0:1);
					if (bChecked!=m_Items[idx].bChecked) m_Toolbar.SendMessage(TB_CHECKBUTTON,idx+1,bChecked?1:0);
				}
				m_Items[idx].bDisabled=bDisabled;
				m_Items[idx].bChecked=bChecked;
			}
		}
	}
	return 0;
}

void CBandWindow::SendEmail( void )
{
	const IID CLSID_SendMail={0x9E56BE60,0xC50F,0x11CF,{0x9A,0x2C,0x00,0xA0,0xC9,0x0A,0x90,0xCE}};

	CComPtr<IShellView> pView;
	if (FAILED(m_pBrowser->QueryActiveShellView(&pView))) return;

	// check if there is anything selected
	CComQIPtr<IFolderView> pView2=pView;
	int count;
	if (pView2 && SUCCEEDED(pView2->ItemCount(SVGIO_SELECTION,&count)) && count==0)
		return;

	// get the data object
	CComPtr<IDataObject> pDataObj;
	if (FAILED(pView->GetItemObject(SVGIO_SELECTION,IID_IDataObject,(void**)&pDataObj)))
		return;
	CComQIPtr<IAsyncOperation> pAsync=pDataObj;
	if (pAsync)
		pAsync->SetAsyncMode(FALSE);

	// drop into the SendMail handler
	CComPtr<IDropTarget> pDropTarget;
	if (SUCCEEDED(CoCreateInstance(CLSID_SendMail,NULL,CLSCTX_ALL,IID_IDropTarget,(void **)&pDropTarget)))
	{
		POINTL pt={0,0};
		DWORD dwEffect=0;
		pDropTarget->DragEnter(pDataObj,MK_LBUTTON,pt,&dwEffect);
		pDropTarget->Drop(pDataObj,0,pt,&dwEffect);
	}
}

void CBandWindow::SendShellTabCommand( int command )
{
	// sends a command to the ShellTabWindowClass window
	for (CWindow parent=GetParent();parent.m_hWnd;parent=parent.GetParent())
	{
		// find a parent window with class ShellTabWindowClass
		wchar_t name[256];
		GetClassName(parent.m_hWnd,name,_countof(name));
		if (_wcsicmp(name,L"ShellTabWindowClass")==0)
		{
			parent.SendMessage(WM_COMMAND,command);
			break;
		}
	}
}

// Executes a cut/copy/paste/delete command
LRESULT CBandWindow::OnCommand( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled )
{
	int idx=LOWORD(wParam)-1;
	int id=m_Items[idx].id;
	if (id>=ID_CUSTOM)
	{
		if (m_Items[idx].command)
		{
			wchar_t buf[2048];
			Strcpy(buf,_countof(buf),m_Items[idx].command);
			DoEnvironmentSubst(buf,_countof(buf));
			wchar_t *pBuf=buf;
			bool bArg1=wcsstr(buf,L"%1")!=NULL;
			bool bArg2=wcsstr(buf,L"%2")!=NULL;
			wchar_t path[_MAX_PATH];
			wchar_t file[_MAX_PATH];
			path[0]=file[0]=0;

			CComPtr<IShellView> pView;
			if (SUCCEEDED(m_pBrowser->QueryActiveShellView(&pView)))
			{
				CComPtr<IPersistFolder2> pFolder;
				LPITEMIDLIST pidl;
				CComQIPtr<IFolderView> pView2=pView;
				if (pView2 && SUCCEEDED(pView2->GetFolder(IID_IPersistFolder2,(void**)&pFolder)) && SUCCEEDED(pFolder->GetCurFolder(&pidl)))
				{
					// get current path
					SHGetPathFromIDList(pidl,path);
					if (bArg2)
					{
						CComPtr<IEnumIDList> pEnum;
						int count;
						// if only one file is selected get the file name (%2)
						if (SUCCEEDED(pView2->ItemCount(SVGIO_SELECTION,&count)) && count==1 && SUCCEEDED(pView2->Items(SVGIO_SELECTION,IID_IEnumIDList,(void**)&pEnum)) && pEnum)
						{
							PITEMID_CHILD child;
							if (pEnum->Next(1,&child,NULL)==S_OK)
							{
								LPITEMIDLIST full=ILCombine(pidl,child);
								SHGetPathFromIDList(full,file);
								ILFree(child);
								ILFree(full);
							}
						}
					}
					ILFree(pidl);
				}

				if (bArg1 || bArg2)
				{
					// expand environment variables, %1, %2
					DWORD_PTR args[100]={(DWORD_PTR)path,(DWORD_PTR)file};
					FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_FROM_STRING,buf,0,0,(LPWSTR)&pBuf,0,(va_list*)args);
				}

				wchar_t exe[_MAX_PATH];
				const wchar_t *params=SeparateArguments(pBuf,exe);
				if (_wcsicmp(exe,L"open")==0)
				{
					PIDLIST_RELATIVE pidl;
					SFGAOF flags=0;
					if (m_pBrowser && SUCCEEDED(SHParseDisplayName((LPWSTR)params,NULL,&pidl,0,&flags)))
					{
						UINT flags=(GetKeyState(VK_CONTROL)<0?SBSP_NEWBROWSER:SBSP_SAMEBROWSER);
						m_pBrowser->BrowseObject(pidl,flags|SBSP_ABSOLUTE);
						ILFree(pidl);
					}
				}
				else
					ShellExecute(NULL,NULL,exe,params,path,SW_SHOWNORMAL);
				if (pBuf!=buf)
					LocalFree(pBuf);
			}
		}
		else if (m_Items[idx].link)
		{
			wchar_t path[_MAX_PATH];
			Strcpy(path,_countof(path),m_Items[idx].link);
			DoEnvironmentSubst(path,_countof(path));
			PIDLIST_RELATIVE pidl;
			SFGAOF flags=0;
			if (m_pBrowser && SUCCEEDED(SHParseDisplayName(path,NULL,&pidl,0,&flags)))
			{
				UINT flags=(GetKeyState(VK_CONTROL)<0?SBSP_NEWBROWSER:SBSP_SAMEBROWSER);
				m_pBrowser->BrowseObject(pidl,flags|SBSP_ABSOLUTE);
				ILFree(pidl);
			}
		}

		return TRUE;
	}

	if (id==ID_SETTINGS)
	{
#ifndef BUILD_SETUP
		if (GetKeyState(VK_SHIFT)<0)
			*(int*)0=0; // force a crash if Shift is pressed. Makes it easy to restart explorer.exe
#endif
		EditSettings();
		return TRUE;
	}

	if (id==ID_GOUP || id==ID_GOBACK || id==ID_GOFORWARD)
	{
		if (m_pBrowser)
		{
			UINT flags=(GetKeyState(VK_CONTROL)<0?SBSP_NEWBROWSER:SBSP_SAMEBROWSER);
			if (id==ID_GOUP)
				m_pBrowser->BrowseObject(NULL,flags|SBSP_PARENT);
			if (id==ID_GOBACK)
				m_pBrowser->BrowseObject(NULL,flags|SBSP_NAVIGATEBACK);
			if (id==ID_GOFORWARD)
				m_pBrowser->BrowseObject(NULL,flags|SBSP_NAVIGATEFORWARD);
		}
		return TRUE;
	}
	if (id==ID_EMAIL)
	{
		SendEmail();
		return TRUE;
	}
	if (id==ID_RENAME)
	{
		CComPtr<IShellView> pView;
		if (SUCCEEDED(m_pBrowser->QueryActiveShellView(&pView)))
		{
			CComQIPtr<IFolderView2> pView2=pView;
			if (pView2) pView2->DoRename();
		}
		return TRUE;
	}

	// check if the focus is on the tree side or on the list side
	CWindow focus=GetFocus();
	wchar_t name[256];
	GetClassName(focus,name,_countof(name));
	CWindow parent=focus.GetParent();
	if (_wcsicmp(name,WC_TREEVIEW)==0)
	{
		// send these commands to the parent of the tree view
		if (id==ID_CUT)
			parent.SendMessage(WM_COMMAND,41025);
		if (id==ID_COPY)
			parent.SendMessage(WM_COMMAND,41026);
		if (id==ID_PASTE)
			parent.SendMessage(WM_COMMAND,41027);
		if (id==ID_DELETE)
			parent.SendMessage(WM_COMMAND,40995);
		if (id==ID_PROPERTIES)
			ShowTreeProperties(focus.m_hWnd);
	}
	else
	{
		GetClassName(parent,name,_countof(name));
		if (_wcsicmp(name,L"SHELLDLL_DefView")==0)
		{
			// send these commands to the SHELLDLL_DefView window
			if (id==ID_CUT)
			{
				parent.SendMessage(WM_COMMAND,28696);
				focus.InvalidateRect(NULL);
			}
			if (id==ID_COPY)
				parent.SendMessage(WM_COMMAND,28697);
			if (id==ID_PASTE)
				parent.SendMessage(WM_COMMAND,28698);
			if (id==ID_DELETE)
				parent.SendMessage(WM_COMMAND,28689);
			if (id==ID_PROPERTIES)
				parent.SendMessage(WM_COMMAND,28691);
			if (id==ID_COPYTO)
				parent.SendMessage(WM_COMMAND,28702);
			if (id==ID_MOVETO)
				parent.SendMessage(WM_COMMAND,28703);
		}
	}

	if (id==ID_UNDO)
		SendShellTabCommand(28699);
	if (id==ID_REDO)
		SendShellTabCommand(28704);

	if (id==ID_VIEW_TILES)
		SendShellTabCommand(28748);
	if (id==ID_VIEW_DETAILS)
		SendShellTabCommand(28747);
	if (id==ID_VIEW_LIST)
		SendShellTabCommand(28753);
	if (id==ID_VIEW_CONTENT)
		SendShellTabCommand(28754);
	if (id==ID_VIEW_ICONS1)
		SendShellTabCommand(28752);
	if (id==ID_VIEW_ICONS2)
		SendShellTabCommand(28750);
	if (id==ID_VIEW_ICONS3)
		SendShellTabCommand(28751);
	if (id==ID_VIEW_ICONS4)
		SendShellTabCommand(28749);

	if (id==ID_SELECTALL || id==ID_INVERT || id==ID_DESELECT)
	{
		// handle selection commands the hard way (instead of sending commands with SendShellTabCommand).
		// some folders don't support selection and they crash if they get selection commands
		CComPtr<IShellView> pView;
		if (FAILED(m_pBrowser->QueryActiveShellView(&pView)))
			return TRUE;

		CComQIPtr<IFolderView2> pView2=pView;
		if (!pView2) return TRUE;

		// ID_DESELECT
		if (id==ID_DESELECT)
		{
			pView2->SelectItem(-1,SVSI_DESELECTOTHERS);
			return TRUE;
		}

		int count;
		if (FAILED(pView2->ItemCount(SVGIO_ALLVIEW,&count)))
			return TRUE;

		// ID_SELECTALL
		if (id==ID_SELECTALL)
		{
			for (int i=0;i<count;i++)
				pView2->SelectItem(i,SVSI_SELECT);
			return TRUE;
		}

		// ID_INVERT
		// we can't use IFolderView2::GetSelectedItem to enumerate the selected items. there is a bug in it on Windows 7. when called
		// with 0 or 1 it returns 0 for the next item, causing an infinite loop. we have to use Item + GetSelectionState instead.
		// it allocates a PIDLs, so it is not ideal, but what can you do. stupid bugs
		for (int i=0;i<count;i++)
		{
			PITEMID_CHILD child;
			if (SUCCEEDED(pView2->Item(i,&child)) && child)
			{
				DWORD state;
				if (SUCCEEDED(pView2->GetSelectionState(child,&state)))
					pView2->SelectItem(i,(state&SVSI_SELECT)?SVSI_DESELECT:SVSI_SELECT);
				ILFree(child);
			}
		}
		return TRUE;
	}
	if (id==ID_REFRESH)
		SendShellTabCommand(41504);
	if (id==ID_STOP && m_pWebBrowser)
		m_pWebBrowser->Stop();

	return TRUE;
}

LRESULT CBandWindow::OnRClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
	NMMOUSE *pInfo=(NMMOUSE*)pnmh;
	POINT pt=pInfo->pt;
	{
		RECT rc;
		int count=(int)m_Toolbar.SendMessage(TB_BUTTONCOUNT);
		m_Toolbar.SendMessage(TB_GETITEMRECT,count-1,(LPARAM)&rc);
		if (pt.x>rc.right)
			return 0;
	}
	m_Toolbar.ClientToScreen(&pt);
	ShowSettingsMenu(m_hWnd,pt.x,pt.y);
	return 1;
}

LRESULT CBandWindow::OnGetInfoTip( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
	NMTBGETINFOTIP *pTip=(NMTBGETINFOTIP*)pnmh;
	const StdToolbarItem &item=m_Items[pTip->lParam];
	if (item.tip)
	{
		// show the tip for the standard item
		Strcpy(pTip->pszText,pTip->cchTextMax,item.tip);
	}
	return 0;
}

// Callback for IShellMenu. Executes the selected command
class CMenuCallback: public IShellMenuCallback
{
public:
	CMenuCallback( IShellBrowser *pBrowser ) { m_bExecuted=false; m_pBrowser=pBrowser; }

	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void **ppvObject );
	virtual ULONG STDMETHODCALLTYPE AddRef( void ) { return 1; }
	virtual ULONG STDMETHODCALLTYPE Release( void ) { return 1; }
	STDMETHOD(CallbackSM)(LPSMDATA psmd,UINT uMsg,WPARAM wParam,LPARAM lParam);

private:
	bool m_bExecuted;
	IShellBrowser *m_pBrowser;
};

HRESULT STDMETHODCALLTYPE CMenuCallback::QueryInterface( REFIID riid, void **ppvObject )
{
	if (riid==IID_IUnknown || riid==IID_IShellMenuCallback)
	{
		*ppvObject=this;
		return S_OK;
	}
	else
	{
		*ppvObject=NULL;
		return E_NOINTERFACE;
	}
}

HRESULT STDMETHODCALLTYPE CMenuCallback::CallbackSM( LPSMDATA psmd,UINT uMsg,WPARAM wParam,LPARAM lParam )
{
	switch(uMsg)
	{
		case SMC_GETINFO:
			{
				SMINFO *pSmInfo=(SMINFO*)lParam;

				if (pSmInfo->dwMask&SMIM_FLAGS)
					pSmInfo->dwFlags|=SMIF_DROPCASCADE|SMIF_TRACKPOPUP;

				if (pSmInfo->dwMask&SMIM_ICON)
					pSmInfo->iIcon=-1;
			}
			return S_OK;

		case SMC_SFEXEC:
			{
				if (m_bExecuted)
					return S_OK;
				m_bExecuted=true;
				SFGAOF flags=SFGAO_FOLDER|SFGAO_LINK;

				if (SUCCEEDED(psmd->psf->GetAttributesOf(1,(LPCITEMIDLIST*)&psmd->pidlItem,&flags)))
				{
					PIDLIST_ABSOLUTE pidl=NULL;
					if (flags&SFGAO_LINK)
					{
						flags=0;
						// resolve link
						CComPtr<IShellLink> pLink;
						if (SUCCEEDED(psmd->psf->GetUIObjectOf(NULL,1,(LPCITEMIDLIST*)&psmd->pidlItem,IID_IShellLink,NULL,(void**)&pLink)) && pLink)
							pLink->GetIDList(&pidl);
						if (pidl)
						{
							CComPtr<IShellFolder> pFolder;
							PCUITEMID_CHILD child;
							SHBindToParent(pidl,IID_IShellFolder,(void**)&pFolder,&child);
							SFGAOF flags2=SFGAO_FOLDER;
							if (pFolder && SUCCEEDED(pFolder->GetAttributesOf(1,&child,&flags2)) && (flags2&SFGAO_FOLDER))
								flags=SFGAO_FOLDER;
							else
							{
								ILFree(pidl);
								pidl=NULL;
							}
						}
					}

					if (!pidl)
						pidl=ILCombine(psmd->pidlFolder,psmd->pidlItem);

					if (flags&SFGAO_FOLDER)
					{
						// navigate to folder
						if (m_pBrowser)
						{
							UINT flags=(GetKeyState(VK_CONTROL)<0?SBSP_NEWBROWSER:SBSP_SAMEBROWSER);
							m_pBrowser->BrowseObject(pidl,flags);
						}
					}
					else
					{
						// execute file
						SHELLEXECUTEINFO execute={sizeof(execute),SEE_MASK_IDLIST};
						execute.lpIDList=pidl;
						execute.nShow=SW_SHOWNORMAL;
						ShellExecuteEx(&execute);
					}
					ILFree(pidl);
				}
				return S_OK;
			}

		case SMC_PROMOTE:
		case SMC_EXITMENU:
		case SMC_GETSFINFO:
		case SMC_SFSELECTITEM:
		case SMC_REFRESH:
		case SMC_DEMOTE:
		case SMC_DEFAULTICON:
		case SMC_NEWITEM:
		case SMC_CHEVRONEXPAND:
		case SMC_DISPLAYCHEVRONTIP:
		case SMC_SETSFOBJECT:
		case SMC_SHCHANGENOTIFY:
		case SMC_CHEVRONGETTIP:
		case SMC_SFDDRESTRICTED:
			return S_OK;

		default:
			return S_FALSE;
	}
}

LRESULT CBandWindow::OnDropDown( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
	NMTOOLBAR *pButton=(NMTOOLBAR*)pnmh;
	int idx=0;
	const StdToolbarItem *pItem=NULL;
	for (std::vector<StdToolbarItem>::const_iterator it=m_Items.begin();it->id!=ID_LAST;++it,idx++)
	{
		RECT rc;
		if (m_Toolbar.SendMessage(TB_GETITEMRECT,idx,(LPARAM)&rc) && memcmp(&rc,&pButton->rcButton,sizeof(RECT))==0)
		{
			pItem=&*it;
			break;
		}
	}
	if (pItem && (pItem->submenu || pItem->link))
	{
		if (pItem->submenu)
		{
			TPMPARAMS params={sizeof(params),pButton->rcButton};
			m_Toolbar.ClientToScreen(&params.rcExclude); // must not use MapWindowPoints because it produces wrong results in RTL cases
			HMENU menu=CreateDropMenu(pItem->submenu);
			int res=TrackPopupMenuEx(menu,TPM_RETURNCMD|TPM_VERTICAL,params.rcExclude.left,params.rcExclude.bottom,m_hWnd,&params);
			DestroyMenu(menu);
			if (res>0)
				PostMessage(WM_COMMAND,res);
		}
		else if (pItem->link)
		{
			TPMPARAMS params={sizeof(params),pButton->rcButton};
			m_Toolbar.MapWindowPoints(NULL,&params.rcExclude); // must use MapWindowPoints to handle RTL correctly
			CMenuCallback callback(m_pBrowser);

			LPITEMIDLIST pidl;
			CComPtr<IShellFolder> pFolder;
			wchar_t buf[1024];
			Strcpy(buf,_countof(buf),pItem->link);
			DoEnvironmentSubst(buf,_countof(buf));
			SFGAOF flags=0;
			if (SUCCEEDED(SHParseDisplayName(buf,NULL,&pidl,0,&flags)))
				SHBindToObject(NULL,pidl,NULL,IID_IShellFolder,(void **)&pFolder);
			if (pFolder)
			{
				HRESULT hr;
				CComPtr<ITrackShellMenu> pMenu;

				CoCreateInstance(CLSID_TrackShellMenu,NULL,CLSCTX_INPROC_SERVER,IID_ITrackShellMenu,(void**)&pMenu);

				if (pMenu)
				{
					CMenuCallback callback(m_pBrowser);
					hr=pMenu->Initialize(&callback,-1,ANCESTORDEFAULT,SMINIT_TOPLEVEL|SMINIT_VERTICAL|SMINIT_RESTRICT_DRAGDROP);
					if (SUCCEEDED(hr))
					{
						CRegKey cRegOrder;

						wchar_t *pFavs;
						if (FAILED(SHGetKnownFolderPath(FOLDERID_Favorites,0,NULL,&pFavs)))
							pFavs=NULL;
						if (pFavs && SUCCEEDED(SHGetPathFromIDList(pidl,buf)))
						{
							// must compare strings and not pidls. sometimes pidls can be different but point to the same folder
							CharUpper(pFavs);
							CharUpper(buf);
							if (wcscmp(pFavs,buf)==0)
								cRegOrder.Open(HKEY_CURRENT_USER,_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Favorites"));
						}
						if (pFavs) CoTaskMemFree(pFavs);

						hr=pMenu->SetShellFolder(pFolder,pidl,cRegOrder.m_hKey,SMSET_BOTTOM|0x00000008); // SMSET_USEBKICONEXTRACTION=0x00000008
						if (SUCCEEDED(hr))
						{
							cRegOrder.Detach();
							POINTL ptl={params.rcExclude.left,params.rcExclude.bottom};
							RECTL rcl={params.rcExclude.left,params.rcExclude.top,params.rcExclude.right,params.rcExclude.bottom};
							pMenu->Popup(NULL,&ptl,&rcl,MPPF_FORCEZORDER|MPPF_BOTTOM);
						}
					}
				}

				ILFree(pidl);
			}
		}

		// remove the next mouse click if it is on the same button
		MSG msg;
		RECT rc=pButton->rcButton;
		m_Toolbar.ClientToScreen(&rc);
		if (PeekMessage(&msg,NULL,WM_LBUTTONDOWN,WM_LBUTTONDBLCLK,PM_NOREMOVE) && PtInRect(&rc,msg.pt))
			PeekMessage(&msg,NULL,WM_LBUTTONDOWN,WM_LBUTTONDBLCLK,PM_REMOVE);
	}
	return TBDDRET_DEFAULT;
}

LRESULT CBandWindow::OnChevron( int idCtrl, LPNMHDR pnmh, BOOL& bHandled )
{
	NMREBARCHEVRON *pChevron=(NMREBARCHEVRON*)pnmh;
	REBARBANDINFO info={sizeof(info),RBBIM_CHILD};
	if (::SendMessage(pnmh->hwndFrom,RB_GETBANDINFO,pChevron->uBand,(LPARAM)&info) && info.hwndChild==m_Toolbar.m_hWnd)
	{
		RECT clientRect;
		m_Toolbar.GetClientRect(&clientRect);
		int idx=0;
		for (std::vector<StdToolbarItem>::const_iterator it=m_Items.begin();it->id!=ID_LAST;++it,idx++)
		{
			RECT rc;
			m_Toolbar.SendMessage(TB_GETITEMRECT,idx,(LPARAM)&rc);
			if (rc.right>clientRect.right)
				break;
		}
		while (m_Items[idx].id==ID_SEPARATOR)
			idx++;
		if (m_Items[idx].id==ID_LAST) return 1;
		HMENU menu=CreateDropMenu(&m_Items[idx]);
		TPMPARAMS params={sizeof(params),pChevron->rc};
		// use ClientToScreen instead of MapWindowPoints because it works better in RTL mode
		::ClientToScreen(pnmh->hwndFrom,(POINT*)&params.rcExclude);
		::ClientToScreen(pnmh->hwndFrom,((POINT*)&params.rcExclude)+1);
		int res=TrackPopupMenuEx(menu,TPM_RETURNCMD|TPM_VERTICAL,params.rcExclude.left,params.rcExclude.bottom,m_hWnd,&params);
		DestroyMenu(menu);

		if (res>0)
			PostMessage(WM_COMMAND,res);

		// remove the next mouse click if it is on the chevron
		MSG msg;
		if (PeekMessage(&msg,NULL,WM_LBUTTONDOWN,WM_LBUTTONDBLCLK,PM_NOREMOVE) && PtInRect(&params.rcExclude,msg.pt))
			PeekMessage(&msg,NULL,WM_LBUTTONDOWN,WM_LBUTTONDBLCLK,PM_REMOVE);

		return 1;
	}
	return 0;
}

HMENU CBandWindow::CreateDropMenu( const StdToolbarItem *pItem )
{
	HMODULE hShell32=GetModuleHandle(L"Shell32.dll");
	std::vector<HMODULE> modules;
	HMENU menu=CreateDropMenuRec(pItem,modules,hShell32);
	for (std::vector<HMODULE>::const_iterator it=modules.begin();it!=modules.end();++it)
		FreeLibrary(*it);
	MENUINFO info={sizeof(info),MIM_STYLE,MNS_CHECKORBMP};
	SetMenuInfo(menu,&info);
	return menu;
}

HMENU CBandWindow::CreateDropMenuRec( const StdToolbarItem *pItem, std::vector<HMODULE> &modules, HMODULE hShell32 )
{
	HMENU menu=CreatePopupMenu();
	for (int idx=0;pItem->id!=ID_LAST;pItem++,idx++)
	{
		if (pItem->id==ID_SEPARATOR)
		{
			AppendMenu(menu,MF_SEPARATOR,0,0);
			continue;
		}
		const wchar_t *name=pItem->label;
		if (!name) name=pItem->tip;

		if (!pItem->bIconLoaded)
		{
			pItem->bIconLoaded=true;
			if (m_MenuIconSize>0)
			{
				if (pItem->iconPath)
				{
					if (_wcsicmp(pItem->iconPath,L"NONE")!=0)
					{
						HICON hIcon=LoadIcon(m_MenuIconSize,pItem->iconPath,modules);
						if (!hIcon)
							hIcon=(HICON)LoadImage(hShell32,MAKEINTRESOURCE(1),IMAGE_ICON,m_MenuIconSize,m_MenuIconSize,LR_DEFAULTCOLOR);
						if (hIcon)
						{
							HICON hIcon2=pItem->iconPathD?LoadIcon(m_MenuIconSize,pItem->iconPathD,modules):NULL;
							if (!hIcon2)
								hIcon2=CreateDisabledIcon(hIcon,m_MenuIconSize);
							pItem->menuIcon=BitmapFromIcon(hIcon,m_MenuIconSize,NULL,true);
							pItem->menuIconD=BitmapFromIcon(hIcon2,m_MenuIconSize,NULL,true);
						}
					}
				}
				else if (pItem->link)
				{
					HICON hIcon=NULL;
					PIDLIST_ABSOLUTE pidl=NULL;
					SFGAOF flags=0;
					wchar_t path[_MAX_PATH];
					Strcpy(path,_countof(path),pItem->link);
					DoEnvironmentSubst(path,_countof(path));
					if (SUCCEEDED(SHParseDisplayName(path,NULL,&pidl,0,&flags)) && pidl)
					{
						if (!name)
						{
							wchar_t *pName=NULL;
							if (SUCCEEDED(SHGetNameFromIDList(pidl,SIGDN_PARENTRELATIVEEDITING,&pName)) && pName)
							{
								pItem->menuText=pName;
								CoTaskMemFree(pName);
							}
						}
						hIcon=LoadIcon(m_MenuIconSize,pidl);
						ILFree(pidl);
					}
					if (hIcon)
					{
						HICON hIcon2=pItem->iconPathD?LoadIcon(m_MenuIconSize,pItem->iconPathD,modules):NULL;
						if (!hIcon2)
							hIcon2=CreateDisabledIcon(hIcon,m_MenuIconSize);
						pItem->menuIcon=BitmapFromIcon(hIcon,m_MenuIconSize,NULL,true);
						pItem->menuIconD=BitmapFromIcon(hIcon2,m_MenuIconSize,NULL,true);
					}
				}
			}
		}

		if (!name) name=pItem->menuText;

		if (pItem->submenu)
		{
			HMENU menu2=CreateDropMenu(pItem->submenu);
			AppendMenu(menu,MF_POPUP,(UINT_PTR)menu2,name);
		}
		else
		{
			int cmd=(int)(pItem-&m_Items[0]+1);
			AppendMenu(menu,MF_STRING,cmd,name);
		}

		if (pItem->menuIcon)
		{
			MENUITEMINFO mii={sizeof(mii)};
			mii.fMask=MIIM_BITMAP;
			mii.hbmpItem=pItem->bDisabled?pItem->menuIconD:pItem->menuIcon;
			SetMenuItemInfo(menu,idx,TRUE,&mii);
		}

		if (pItem->bDisabled)
			EnableMenuItem(menu,idx,MF_BYPOSITION|MF_GRAYED);
		if (pItem->bChecked)
			CheckMenuItem(menu,idx,MF_BYPOSITION|MF_CHECKED);
	}
	return menu;
}

void CBandWindow::UpdateToolbar( void )
{
	// disable the Up button if we are at the top level
	bool bDesktop=false;
	if (m_pBrowser)
	{
		CComPtr<IShellView> pView;
		m_pBrowser->QueryActiveShellView(&pView);
		if (pView)
		{
			CComQIPtr<IFolderView> pView2=pView;
			if (pView2)
			{
				CComPtr<IPersistFolder2> pFolder;
				pView2->GetFolder(IID_IPersistFolder2,(void**)&pFolder);
				if (pFolder)
				{
					LPITEMIDLIST pidl;
					pFolder->GetCurFolder(&pidl);
					if (ILIsEmpty(pidl))
						bDesktop=true; // only the top level has empty PIDL
					CoTaskMemFree(pidl);
				}
			}
		}
	}
	EnableButton(ID_GOUP,!bDesktop);
}

void CBandWindow::EnableButton( int cmd, bool bEnable )
{
	bool bMain=true;
	bool bDisabled=!bEnable;
	for (int idx=0;idx<(int)m_Items.size();idx++)
	{
		if (m_Items[idx].id==ID_LAST)
			bMain=false;
		if (m_Items[idx].id==cmd && m_Items[idx].bDisabled!=bDisabled)
		{
			m_Items[idx].bDisabled=bDisabled;
			if (bMain)
				m_Toolbar.SendMessage(TB_ENABLEBUTTON,idx+1,bDisabled?0:1);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

// CExplorerBand - adds a toolbar band to Windows Explorer with 2 buttons - "Up" and "Settings"

CExplorerBand::CExplorerBand( void )
{
	m_bSubclassRebar=(LOWORD(GetVersion())==0x0106); // Windows 7
	m_bSubclassedRebar=false;
	m_TopWindow=NULL;
}

// Subclasses the rebar control on Windows 7. Makes sure the RBBS_BREAK style is properly set. Windows 7 has a bug
// that forces RBBS_BREAK for every rebar band
LRESULT CALLBACK CExplorerBand::RebarSubclassProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	if (uMsg==RB_SETBANDINFO && ((CExplorerBand*)uIdSubclass)->m_bHandleSetInfo)
	{
		REBARBANDINFO *pInfo=(REBARBANDINFO*)lParam;
		if ((pInfo->hwndChild==(HWND)dwRefData) && (pInfo->fMask&RBBIM_STYLE))
		{
			if (((CExplorerBand*)uIdSubclass)->m_bBandNewLine)
				pInfo->fStyle|=RBBS_BREAK;
			else
				pInfo->fStyle&=~RBBS_BREAK;
		}
	}

	if (uMsg==WM_CLEAR)
		((CExplorerBand*)uIdSubclass)->m_bHandleSetInfo=false;

	if (uMsg==RB_DELETEBAND)
	{
		int n=(int)SendMessage(hWnd,RB_GETBANDCOUNT,0,0);
		CExplorerBand *pThis=(CExplorerBand*)uIdSubclass;
		for (int i=0;i<n;i++)
		{
			// when ANY band but ours is being deleted, make sure we preserve OUR break style (on Win7 all bands get a break style when any band is deleted)
			REBARBANDINFO info={sizeof(info),RBBIM_STYLE|RBBIM_CHILD};
			SendMessage(hWnd,RB_GETBANDINFO,i,(LPARAM)&info);
			if (info.hwndChild==(HWND)dwRefData)
			{
				LRESULT res=DefSubclassProc(hWnd,uMsg,wParam,lParam);
				if (i!=(int)wParam)
				{
					bool old=pThis->m_bHandleSetInfo;
					pThis->m_bHandleSetInfo=false;
					SendMessage(hWnd,RB_SETBANDINFO,i,(LPARAM)&info);
					pThis->m_bHandleSetInfo=old;
				}
				return res;
			}
		}
	}

	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

// Subclasses the rebar's parent to catch RBN_CHEVRONPUSHED
LRESULT CALLBACK CExplorerBand::ParentSubclassProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	if (uMsg==WM_NOTIFY && ((NMHDR*)lParam)->code==RBN_CHEVRONPUSHED)
	{
		if (SendMessage((HWND)dwRefData,uMsg,wParam,lParam))
			return 0;
	}
	return DefSubclassProc(hWnd,uMsg,wParam,lParam);
}

// IDeskBand
STDMETHODIMP CExplorerBand::GetBandInfo( DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi )
{
	// initializes the band
	if (!m_bSubclassedRebar)
	{
		HWND rebar=GetParent(m_BandWindow.GetToolbar());
		wchar_t className[256];
		GetClassName(rebar,className,_countof(className));
		if (_wcsicmp(className,REBARCLASSNAME)==0)
		{
			if (m_bSubclassRebar)
			{
				CRegKey regSettings;
				if (regSettings.Open(HKEY_CURRENT_USER,L"Software\\IvoSoft\\ClassicExplorer")!=ERROR_SUCCESS)
					regSettings.Create(HKEY_CURRENT_USER,L"Software\\IvoSoft\\ClassicExplorer");

				DWORD NewLine;
				if (regSettings.QueryDWORDValue(L"NewLine",NewLine)!=ERROR_SUCCESS)
					NewLine=0;
				m_bBandNewLine=NewLine!=0;

				SetWindowSubclass(rebar,RebarSubclassProc,(UINT_PTR)this,(DWORD_PTR)m_BandWindow.GetToolbar());
			}
			SetWindowSubclass(GetParent(rebar),ParentSubclassProc,(UINT_PTR)this,(DWORD_PTR)m_BandWindow.m_hWnd);
			m_bSubclassedRebar=true;
		}
	}
	RECT rc;
	SendMessage(m_BandWindow.GetToolbar(),TB_GETITEMRECT,0,(LPARAM)&rc);
	int minSize=rc.right;
	int count=(int)SendMessage(m_BandWindow.GetToolbar(),TB_BUTTONCOUNT,0,0);
	SendMessage(m_BandWindow.GetToolbar(),TB_GETITEMRECT,count-1,(LPARAM)&rc);
	bool bChevron=GetSettingBool(L"ResizeableToolbar");

	if (pdbi)
	{
		if (pdbi->dwMask&DBIM_MINSIZE)
		{
			pdbi->ptMinSize.x=bChevron?minSize:rc.right;
			pdbi->ptMinSize.y=rc.bottom;
		}
		if (pdbi->dwMask&DBIM_MAXSIZE)
		{
			pdbi->ptMaxSize.x=0; // ignored
			pdbi->ptMaxSize.y=-1;	// unlimited
		}
		if (pdbi->dwMask&DBIM_INTEGRAL)
		{
			pdbi->ptIntegral.x=0; // not sizeable
			pdbi->ptIntegral.y=0; // not sizeable
		}
		if (pdbi->dwMask&DBIM_ACTUAL)
		{
			pdbi->ptActual.x=rc.right;
			pdbi->ptActual.y=rc.bottom;
		}
		if (pdbi->dwMask&DBIM_TITLE)
		{
			*pdbi->wszTitle=0; // no title
		}
		if (pdbi->dwMask&DBIM_BKCOLOR)
		{
			//Use the default background color by removing this flag.
			pdbi->dwMask&=~DBIM_BKCOLOR;
		}
		if (pdbi->dwMask&DBIM_MODEFLAGS)
		{
			if (bChevron)
				pdbi->dwModeFlags|=DBIMF_USECHEVRON;
		}
	}
	return S_OK;
}

// IOleWindow
STDMETHODIMP CExplorerBand::GetWindow( HWND* phwnd )
{
	if (!phwnd)
		return E_INVALIDARG;
	*phwnd=m_BandWindow.GetToolbar();
	return S_OK;
}

STDMETHODIMP CExplorerBand::ContextSensitiveHelp( BOOL fEnterMode )
{
	return S_OK;
}

// IDockingWindow
STDMETHODIMP CExplorerBand::CloseDW( unsigned long dwReserved )
{
	ShowDW(FALSE);
	return S_OK;
}

STDMETHODIMP CExplorerBand::ResizeBorderDW( const RECT* prcBorder, IUnknown* punkToolbarSite, BOOL fReserved )
{
	// Not used by any band object.
	return E_NOTIMPL;
}

STDMETHODIMP CExplorerBand::ShowDW( BOOL fShow )
{
	if (m_bSubclassRebar && m_bSubclassedRebar)
	{
		// on Windows 7 get the current RBBS_BREAK state and save it in the registry to be restored later
		HWND rebar=GetParent(m_BandWindow.GetToolbar());
		m_bHandleSetInfo=true;
		if (!fShow)
		{
			int n=(int)SendMessage(rebar,RB_GETBANDCOUNT,0,0);
			for (int i=0;i<n;i++)
			{
				REBARBANDINFO info={sizeof(info),RBBIM_STYLE|RBBIM_CHILD};
				SendMessage(rebar,RB_GETBANDINFO,i,(LPARAM)&info);
				if (info.hwndChild==m_BandWindow.GetToolbar())
				{
					m_bBandNewLine=(info.fStyle&RBBS_BREAK)!=0;
					CRegKey regSettings;
					if (regSettings.Open(HKEY_CURRENT_USER,L"Software\\IvoSoft\\ClassicExplorer")!=ERROR_SUCCESS)
						regSettings.Create(HKEY_CURRENT_USER,L"Software\\IvoSoft\\ClassicExplorer");

					regSettings.SetDWORDValue(L"NewLine",m_bBandNewLine?1:0);
					break;
				}
			}
		}
		ShowWindow(m_BandWindow.GetToolbar(),fShow?SW_SHOW:SW_HIDE);
		PostMessage(rebar,WM_CLEAR,0,0);
	}
	else
		ShowWindow(m_BandWindow.GetToolbar(),fShow?SW_SHOW:SW_HIDE);
	return S_OK;
}

// IObjectWithSite
STDMETHODIMP CExplorerBand::SetSite( IUnknown* pUnkSite )
{
	IObjectWithSiteImpl<CExplorerBand>::SetSite(pUnkSite);

	if (m_BandWindow.IsWindow())
		m_BandWindow.DestroyWindow();
	m_BandWindow.SetBrowsers(NULL,NULL);
	if (m_bSubclassedRebar)
	{
		HWND hwnd=GetParent(m_BandWindow.GetToolbar());
		if (m_bSubclassRebar)
			RemoveWindowSubclass(hwnd,RebarSubclassProc,(UINT_PTR)this);
		RemoveWindowSubclass(GetParent(hwnd),ParentSubclassProc,(UINT_PTR)this);
	}
	m_bSubclassedRebar=false;
	m_bHandleSetInfo=true;

	if (m_pWebBrowser && m_dwEventCookie!=0xFEFEFEFE)
		DispEventUnadvise(m_pWebBrowser,&DIID_DWebBrowserEvents2);
	m_pWebBrowser=NULL;

	//If punkSite is not NULL, a new site is being set.
	if (pUnkSite)
	{
		//Get the parent window.
		HWND hWndParent=NULL;

		CComQIPtr<IOleWindow> pOleWindow=pUnkSite;
		if (pOleWindow)
			pOleWindow->GetWindow(&hWndParent);

		if (!IsWindow(hWndParent))
			return E_FAIL;

		m_TopWindow=GetAncestor(hWndParent,GA_ROOT);
		if (!GetProp(m_TopWindow,g_LoadedSettingsAtom))
		{
			SetProp(m_TopWindow,g_LoadedSettingsAtom,(HANDLE)1);
			LoadSettings();
		}

		m_BandWindow.Create(hWndParent,NULL,NULL,WS_CHILD);
		if (!m_BandWindow.IsWindow())
			return E_FAIL;

		CComQIPtr<IServiceProvider> pProvider=pUnkSite;

		if (pProvider)
		{
			CComPtr<IShellBrowser> pBrowser;
			pProvider->QueryService(SID_SShellBrowser,IID_IShellBrowser,(void**)&pBrowser);

			// listen for web browser notifications. we only care about DISPID_NAVIGATECOMPLETE2 and DISPID_ONQUIT
			pProvider->QueryService(SID_SWebBrowserApp,IID_IWebBrowser2,(void**)&m_pWebBrowser);
			if (m_pWebBrowser)
			{
				if (m_dwEventCookie==0xFEFEFEFE) // ATL's event cookie is 0xFEFEFEFE when the sink is not advised
					DispEventAdvise(m_pWebBrowser,&DIID_DWebBrowserEvents2);
			}
			m_BandWindow.SetBrowsers(pBrowser,m_pWebBrowser);
		}
	}
	else
	{
		if (m_TopWindow) RemoveProp(m_TopWindow,g_LoadedSettingsAtom);
		m_TopWindow=NULL;
	}
	return S_OK;
}

STDMETHODIMP CExplorerBand::OnNavigateComplete( IDispatch *pDisp, VARIANT *URL )
{
	// this is called when the current folder changes. disable the Up button if this is the desktop folder
	m_BandWindow.UpdateToolbar();
	return S_OK;
}

STDMETHODIMP CExplorerBand::OnCommandStateChange( long Command, VARIANT_BOOL Enable )
{
	if (Command==CSC_NAVIGATEFORWARD)
	{
		m_BandWindow.EnableButton(CBandWindow::ID_GOFORWARD,Enable?true:false);
	}
	if (Command==CSC_NAVIGATEBACK)
	{
		m_BandWindow.EnableButton(CBandWindow::ID_GOBACK,Enable?true:false);
	}
	return S_OK;
}

STDMETHODIMP CExplorerBand::OnQuit( void )
{
	if (m_pWebBrowser && m_dwEventCookie!=0xFEFEFEFE) // ATL's event cookie is 0xFEFEFEFE, when the sink is not advised
		return DispEventUnadvise(m_pWebBrowser,&DIID_DWebBrowserEvents2);
	return S_OK;
}
