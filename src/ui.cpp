/*
 *	This file is a part of the "Nullboard Backup Agent" source
 *	code and it is distributed under the terms of 2-clause BSD
 *	license.
 *
 *	Copyright (c) 2022 Alexander Pankratov, ap@swapped.ch.
 *	All rights reserved.
 */
#include "ui.h"
#include "trace.h"
#include "config.h"
#include "console.h"
#include "utils.h"

#include "_version.h"

#include "res/resource.h"
#include "sfc/window.h"
#include "sfc/dialog.h"
#include "sfc/bitmap.h"
#include "sfc/icon.h"
#include "sfc/menu.h"
#include "sfc/font.h"
#include "sfc/systray.h"

#include "xfc/sprite.h"
#include "xfc/editbox.h"
#include "xfc/shake_control.h"

//
#define APP_TITLE  L"Nullboard agent"
#define WMX_TRAY   (WM_USER + 1)
#define WMX_EXIT   (WM_USER + 2)

/*
 *	imports
 */
struct api_error_cb;

bool copy_to_clipboard(HWND hwnd, const wstring & text, api_error_cb * err = NULL);
bool shell_execute(const wstring & what, const wstring & args = L"", bool elevated = false, api_error_cb * err = NULL, HANDLE * proc = NULL);

//
struct dialog_add_area : sfc_dialog
{
	void on_initialize()
	{
		name.attach(hwnd, IDC_NAME);
		token.attach(hwnd, IDC_TOKEN);
		copy_token.hwnd = get_dlg_item(IDC_COPY);

		generate_token();
		token.set_text( to_wstr(token_val) );
		name.set_focus();
	}

	void on_wm_command(uint cmd_id)
	{
		if (cmd_id == IDC_COPY)
		{
			if (! copy_to_clipboard( hwnd, token.get_text() ))
			{
				shake_control(copy_token);
			}
			else
			{
				copy_token.enable(false);
				copy_token.set_text("Done");
				set_timer(0x01, 450);
			}
		}
	}

	void on_wm_timer(uint timer_id)
	{
		if (timer_id == 0x01)
		{
			copy_token.enable(true);
			copy_token.set_text(L"Copy");
			kill_timer(timer_id);
		}
	}

	void on_ok()
	{
		if (! get_name_val())
		{
			shake_control(name, true);
			return;
		}

		conf.areas[token_val] = { name_val, L"" };
		save_ini();

		make_path(conf.path + L"\\" + name_val);
		copy_to_clipboard(hwnd, to_wstr(token_val));

		end_modal(IDOK);
	}

	uint run_modal(HWND parent)
	{
		return sfc_dialog::run_modal(IDD_ADD_AREA, parent);
	}

	void generate_token()
	{
		for (;;)
		{
			token_val.clear();

			srand(GetTickCount());

			for (int i=0; i<4; i++)
			{
				const char soup1[] = "ABCEFHJKLMNPQRSTUVWXY";
				const char soup2[] = "0123456789";
				if (token_val.size()) token_val += L'-';

				token_val += soup1[ rand() % (sizeof(soup1)-1) ];
				token_val += soup2[ rand() % (sizeof(soup2)-1) ];
				token_val += soup2[ rand() % (sizeof(soup2)-1) ];
				token_val += soup2[ rand() % (sizeof(soup2)-1) ];
			}

			if (conf.areas.find(token_val) == conf.areas.end())
				break;
		}
	}

	bool get_name_val()
	{
		name_val = name.get_text();

		while (name_val.size() && name_val.front() == ' ')
			name_val = name_val.substr(1);

		while (name_val.size() && name_val.back() == ' ')
			name_val.pop_back();

		if (name_val.empty())
			return false;

		for (auto & ch : name_val)
		{
			static const wchar_t * reserved = L"<>:\"/\\|?*";

			if (wcschr(reserved, ch))
			{
				static const wchar_t * hmmm =
					L"The name cannot contain < > : \\ / | ? * characters, because\ninternally it is used as a name of the backup folder.";

				MessageBox(hwnd, hmmm, APP_TITLE, MB_OK | MB_ICONINFORMATION);
				return false;
			}
		}

		return true;
	}

	//
	sfc_font        segoe_sym;
	xfc_vc_editbox  name;
	xfc_vc_editbox  token;
	sfc_hwnd        copy_token;
	string          token_val;
	wstring         name_val;
};

struct the_ui : sfc_window
{
	the_ui()
	{
		self = NULL;
		tray_phase = 0;
	}

	//
	bool create()
	{
		sfc_rect rc;

		rc.set(100, 100, 400, 300);

		if (! sfc_window::create(APP_TITLE, WS_OVERLAPPED, 0, NULL, NULL, &rc, 0))
		{
			return false;
		}

		set_icons();

		init_systray();

		if (conf.say_hello)
			systray.show_balloon(L"... is now up and running.", APP_TITLE L"     ", NIIF_INFO);

		return true;
	}

	//
	bool wnd_proc(sfc_message & msg)
	{
		if (msg.id == WM_ERASEBKGND)
		{
			sfc_hdc   dc( (HDC)msg.wp );
			sfc_rect  rc;

			get_client_rect(rc);
			dc.fill_rect(rc, 0xFF);

			msg.res = true;
			return true;
		}

		if (msg.id == WMX_TRAY)
		{
			on_wmx_systray(msg.lp);
			return true;
		}

		if (msg.id == wm_taskbar_created)
		{
			systray.show(true);
			return true;
		}

		if (msg.id == WM_TIMER && msg.wp == 0x01)
		{
			tray_phase = (++tray_phase) % 8;
			systray.set_icon( tray_icons[tray_phase].h );
			if (tray_phase == 0) kill_timer(msg.wp);
			return true;
		}

		if (msg.id == WMX_EXIT)
		{
			destroy();
			return true;
		}

		if (msg.id == WM_DESTROY)
		{
			systray.hide();
			PostQuitMessage(0);
		}

		return sfc_window::wnd_proc(msg);
	}

	bool on_wm_command(uint ctrl_id)
	{
		switch (ctrl_id)
		{
		case IDC_EXIT:
			destroy();
			return true;

		case IDC_SAY_HELLO:
			conf.say_hello = ! conf.say_hello;
			trace_v("conf.say_hello: %u\n", conf.say_hello);
			save_ini();
			return true;

		case IDC_SHOW_CONSOLE:
			conf.console = ! conf.console;
			show_console(conf.console);
			save_ini();
			return true;

		case IDC_VERBOSE_TRACE:
			conf.trace = (conf.trace < 3) ? 3 : 2;
			trace_i("conf.trace: level %u\n", conf.trace);
			save_ini();
			return true;

		case IDC_ADD_AREA:
			dialog_add_area().run_modal(hwnd);
			return true;

		case IDC_ABOUT:
			on_about();
			return true;
		}

		{
			for (auto & a : conf.areas)
			{
				if (ctrl_id == IDC_COPY_AREA_TOKEN)
				{
					copy_to_clipboard(hwnd, to_wstr(a.first));
					systray.show_balloon(L"Access token copied to the clipboard.", APP_TITLE, NIIF_INFO);
					return true;
				}

				if (ctrl_id == IDC_OPEN_AREA_FOLDER)
				{
					wstring path = conf.path + L"\\" + a.second.folder;

					if (! folder_exists(path))
					{
						MessageBox(hwnd, L"Backup folder doesn't yet exist", APP_TITLE, MB_OK | MB_ICONINFORMATION);
						return true;
					}

					shell_execute(path);
					return true;
				}

				if (ctrl_id == IDC_OPEN_AREA_PAGE)
				{
					auto const & url = a.second.url;

					if (url.empty())
					{
						MessageBox(hwnd, L"Web page location is not yet known", APP_TITLE, MB_OK | MB_ICONINFORMATION);
						return true;
					}

					shell_execute(url);
					return true;
				}

				if (ctrl_id == IDC_REMOVE_AREA)
				{
					wstring mesg =
						L"About to remove the following backup from configuration,\nbut without removing any on-disk files -\n\n        "
						+ a.second.folder
						+ L"\n\nProceed?";

					if (MessageBox(hwnd, mesg.c_str(), APP_TITLE, MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						conf.areas.erase(a.first);
						save_ini();
					}

					return true;
				}

				if (ctrl_id < 1000)
					break;

				ctrl_id -= 1000;
			}
		}

		return false;
	}

	void on_wmx_systray(uint msg)
	{
		sfc_hwnd popup;

		if (msg != WM_LBUTTONUP && msg != WM_RBUTTONUP)
			return;

		popup.hwnd = GetLastActivePopup(hwnd);

		if (popup.hwnd != hwnd)
		{
			popup.set_foreground(true);
			return;
		}

		//
		vector<HMENU>  menus;
		sfc_hmenu      sub;
		sfc_menu       foo;

		int        area_i = 0;
		int        pos = 0;
		POINT      pt;
		int        ret;

		foo.load(IDR_MENU);
		sub = foo.get_sub_menu(0);
		menus.push_back(foo.h);
		foo.h = NULL;

		for (auto & a : conf.areas)
		{
			wstring       text;
			sfc_hmenu     sub_area;
			MENUITEMINFO  mii = { sizeof mii };

			foo.load(IDR_MENU);
			sub_area = foo.get_sub_menu(1);
			menus.push_back(foo.h);
			foo.h = NULL;

			mii.fMask = MIIM_ID;

			mii.wID = IDC_OPEN_AREA_PAGE + 1000*area_i;
			sub_area.set_item_info(IDC_OPEN_AREA_PAGE, false, mii);

			mii.wID = IDC_OPEN_AREA_FOLDER + 1000*area_i;
			sub_area.set_item_info(IDC_OPEN_AREA_FOLDER, false, mii);

			mii.wID = IDC_COPY_AREA_TOKEN + 1000*area_i;
			sub_area.set_item_info(IDC_COPY_AREA_TOKEN, false, mii);

			mii.wID = IDC_REMOVE_AREA + 1000*area_i;
			sub_area.set_item_info(IDC_REMOVE_AREA, false, mii);

			sub.insert_submenu((L"[ " + a.second.folder + L" ]").c_str(), sub_area.h, pos++, true);
			area_i++;
		}

		if (area_i)
			sub.insert_separator(pos++, true);

		if (conf.say_hello)
			sub.check_item(IDC_SAY_HELLO, false, true, false);

		if (conf.console)
			sub.check_item(IDC_SHOW_CONSOLE, false, true, false);

		if (conf.trace > 2)
			sub.check_item(IDC_VERBOSE_TRACE, false, true, false);

		set_foreground(false);
		pt = msg_copy.pt;
		ret = TrackPopupMenu(sub.h, TPM_CENTERALIGN | TPM_BOTTOMALIGN | TPM_RETURNCMD | TPM_VERNEGANIMATION, pt.x, pt.y, 0, hwnd, NULL);
		if (ret) on_wm_command(ret);

		for (auto & m : menus)
			DestroyMenu(m);
	}

	void on_about()
	{
		MSGBOXPARAMS mbp = { sizeof mbp };
		
		mbp.hwndOwner = hwnd;
		mbp.hInstance = sfc_instance;
		mbp.lpszText = L"   " APP_TITLE L", version " CURRENT_VERSION_STR L"\n   Alexander Pankratov / swapped.ch\n\n";
		mbp.lpszCaption = L"About";
		mbp.dwStyle = MB_OK | MB_USERICON | MB_SETFOREGROUND;
		mbp.lpszIcon = MAKEINTRESOURCE(IDI_MAIN);
    
		MessageBoxIndirect(&mbp);
	}

	void on_engine_activity()
	{
		tray_phase = 0;
		set_timer(0x1, 50);
	}

	//
	void set_icons()
	{
		HMODULE self = GetModuleHandle(NULL);
		set_icon(LoadIcon(self, _RES(IDI_MAIN)), false);
		set_icon(LoadIcon(self, _RES(IDI_MAIN)), true);
	}

	void init_systray()
	{
		sfc_rect rc;
		uint dpi;

		dpi = get_current_dpi();
		if (dpi < 120) { rc.set(0,0,16,16); } else
		if (dpi < 144) { rc.set(0,0,20,20); rc.offset(16, 0); } else
			       { rc.set(0,0,24,24); rc.offset(40, 0); }

		sprites.load_png(_RES(IDR_SPRITES), _RES(1000));

		for (int i=0; i<8; i++)
		{
			tray_icons[i].h = extract_ico( sprites.h, rc );
			rc.offset(0, 24);
		}

		//
		systray.init(hwnd, WMX_TRAY);
		systray.set_tooltip(APP_TITLE);
		systray.set_icon(tray_icons[0].h);
		systray.show(true);

		//
		wm_taskbar_created = RegisterWindowMessage(L"TaskbarCreated");
		ChangeWindowMessageFilter(wm_taskbar_created, MSGFLT_ADD);
	}

	//
	void run()
	{
		int rc;
		MSG msg;

		while (! destroyed)
		{
			rc = GetMessage(&msg, NULL, 0, 0);
			if (! rc)
				break;

			if (rc == -1)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	HANDLE         self;
	sfc_bitmap     sprites;
	sfc_icon       tray_icons[8];
	int            tray_phase;
	sfc_tray_icon  systray;
	uint_t         wm_taskbar_created;
};

//
static the_ui ui;

static dword __stdcall ui_thread(void * p)
{
	if (! ui.create())
	{
		trace_e("UI initialization failed\n");
		return -1;
	}

	((the_ui*)p)->run();
	return 0;
}

//
HANDLE start_ui()
{
	ui.self = CreateThread(NULL, 0, ui_thread, &ui, 0, NULL);
	return ui.self;
}

void stop_ui()
{
	if (! ui.hwnd)
		return;

	ui.post_message(WMX_EXIT);
	WaitForSingleObject(ui.self, -1);
}

// a callback from the engine

void on_engine_activity()
{
	ui.on_engine_activity();
}