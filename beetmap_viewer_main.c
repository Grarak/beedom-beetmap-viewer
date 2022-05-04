#include <MinHook.h>
#include <Windows.h>

#define BEETMAP_VIEWER_WIDTH 84
#define BEETMAP_VIEWER_HEIGHT 48

unsigned char* beetmap_viewer_pixels = NULL;
HANDLE beetmap_viewer_pixels_mutex;

void beetmap_viewer_pixels_lock()
{
    WaitForSingleObject(beetmap_viewer_pixels_mutex, INFINITE);
}

void beetmap_viewer_pixels_release()
{
    ReleaseMutex(beetmap_viewer_pixels_mutex);
}

void beetmap_viewer_doom_loop();

DWORD WINAPI DoomThread(LPVOID lpParam)
{
    beetmap_viewer_doom_loop();
    return 0;
}

HANDLE doom_thread_handle;

struct Color {
	unsigned int padding;
	unsigned int flags;
	unsigned short red;
	unsigned short green;
	unsigned short blue;
	unsigned int padding2;
};

typedef void(__stdcall* get_color_t)(PVOID, struct Color*);
get_color_t og_get_color = NULL;

void __stdcall get_color(void* instance, struct Color* color) {
	og_get_color(instance, color);

	if ((color->red >> 8) == 0x7b && (color->green >> 15) == 1 && (color->blue >> 14) == 3) {
		char x = (color->green >> 8) & ~(1 << 7);
		char y = (color->blue >> 8) & ~(3 << 6);

		unsigned char* doom_color = beetmap_viewer_pixels + (3 * (y * BEETMAP_VIEWER_WIDTH + x));
		color->red = ((unsigned short) doom_color[0]) << 8;
		color->green = ((unsigned short) doom_color[1]) << 8;
		color->blue = ((unsigned short) doom_color[2]) << 8;
	}
}

BOOL in_deselected_all_sub = FALSE;
BOOL force_selected = FALSE;

typedef void(__fastcall* deselect_all_t)(void*, void*, void*, BOOL, BOOL);
deselect_all_t og_deselect_all = NULL;

void __fastcall deselect_all(void* instance, void* edx, void* object, BOOL arg1, BOOL arg2) {
	in_deselected_all_sub = TRUE;
	beetmap_viewer_pixels_lock();
	force_selected = TRUE;
	og_deselect_all(instance, edx, object, arg1, arg2);
	beetmap_viewer_pixels_release();
	in_deselected_all_sub = FALSE;
}

typedef BOOL(__fastcall* is_selected_t)(void*, void*, void*);
is_selected_t og_is_selected = NULL;

BOOL __fastcall is_selected(void* instance, void* edx, void* object) {
	if (in_deselected_all_sub && force_selected) {
		return TRUE;
	}
	return og_is_selected(instance, edx, object);
}

typedef void(__fastcall* set_selected_t)(void*, void*, void*, BOOL);
set_selected_t og_set_selected = NULL;

void __fastcall set_selected(void* instance, void* edx, void* object, BOOL selected) {
	if (in_deselected_all_sub) {
		force_selected = FALSE;
		og_set_selected(instance, edx, object, TRUE);
	}
	else {
		og_set_selected(instance, edx, object, selected);
	}
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
	case DLL_PROCESS_ATTACH: {
		srand((unsigned int)time(NULL));

		unsigned int aleo_base_addr = (unsigned int)GetModuleHandleA("aleo51.dll");
		unsigned int amoded_base_addr = (unsigned int)GetModuleHandleA("amoded51.dll");
		unsigned int adraw_base_addr = (unsigned int)GetModuleHandleA("adraw51.dll");

		unsigned int get_color_addr = aleo_base_addr + 0x3495;
		unsigned int deselect_all_addr = amoded_base_addr + 0x65f0b;
		unsigned int is_selected_addr = adraw_base_addr + 0x199c3;
		unsigned int set_selected_addr = adraw_base_addr + 0x19e05;

		beetmap_viewer_pixels_mutex = CreateMutex(NULL, FALSE, NULL);

		doom_thread_handle = CreateThread(
			NULL,
			0,
			DoomThread,
			NULL,
			0,
			NULL);

		MH_Initialize();
		DisableThreadLibraryCalls(hModule);

		if (MH_CreateHook((LPVOID)get_color_addr, &get_color, (LPVOID*)&og_get_color) == MH_OK) {
			MH_EnableHook((LPVOID)get_color_addr);
		}

		if (MH_CreateHook((LPVOID)deselect_all_addr, &deselect_all, (LPVOID*)&og_deselect_all) == MH_OK) {
			MH_EnableHook((LPVOID)deselect_all_addr);
		}

		if (MH_CreateHook((LPVOID)is_selected_addr, &is_selected, (LPVOID*)&og_is_selected) == MH_OK) {
			MH_EnableHook((LPVOID)is_selected_addr);
		}

		if (MH_CreateHook((LPVOID)set_selected_addr, &set_selected, (LPVOID*)&og_set_selected) == MH_OK) {
			MH_EnableHook((LPVOID)set_selected_addr);
		}
	}
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
		MH_Uninitialize();

		CloseHandle(doom_thread_handle);
        break;
    }
    return TRUE;
}
