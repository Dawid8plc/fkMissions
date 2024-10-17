typedef struct IUnknown IUnknown;

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions

#include <filesystem>

#ifdef _X86_
extern "C" { int _afxForceUSRDLL; }
#else
extern "C" { int __afxForceUSRDLL; }
#endif

#include <ShellScalingAPI.h>
#pragma comment(lib, "Shcore.lib")

#include "include/MinHook.h"
#include "Hooks.h"

#include <sstream>
#include <fstream>
#include "CDButton.h"
#include "MissionLevel.h"

#include "include/WndImage.h"

#include "MissionPasswords.hpp";

#pragma comment(lib,"user32.lib") 
#pragma comment(lib,"libs\\libMinHook.x86.lib")

std::string lang;
LPCTSTR missionSelectText;
LPCTSTR missionText;

std::string mainPath;

//Mission Modal

CButton prevLvl;
CButton prev5Lvl;
CButton nextLvl;
CButton next5Lvl;

CStatic LevelNameLabel;

WNDPROC ogMissionModalWndProc = nullptr;

CWnd* passwordDialog;
CWnd* missionTab;

CStatic LevelPreview;
CWndImage LevelPreviewImg;

HBITMAP LevelPreviewBitmap = NULL;

int selectedLevel = 1;

int completedLevels = 0;

double GetDpiScaleFactor(HWND hwnd)
{
    HDC hdc = GetDC(hwnd);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return dpi / 96.0; // 96 is the default DPI
}

int readReg()
{
    CHAR buffer[256];
    DWORD dwSize = _countof(buffer);

    if (ERROR_SUCCESS == RegGetValueA(HKEY_CURRENT_USER, "Software\\Team17SoftwareLTD\\frontend\\Worms2", "Mission", RRF_RT_REG_SZ, NULL, &buffer, &dwSize)) {

        string keyValue = string(buffer);

        size_t index = keyValue.find_last_of(' ');

        if (index == -1)
        {
            completedLevels = 0;
        }
        else
        {
            completedLevels = index + 1;
        }

        if (completedLevels > 45)
            completedLevels = 45;

        return 0;
    }
    else {
        completedLevels = 0;

        return 1;
    }
}

void LoadLevel()
{
    DeleteObject(LevelPreviewBitmap);

    std::ostringstream s;

    s << mainPath << "\\Data\\MISSION\\" << selectedLevel << "\\MISSION.LEV";

    MissionLevel level(s.str());

    std::wostringstream s2;

    s2 << missionText << " " << selectedLevel;

    LevelPreviewBitmap = level.ToBitmap();

    LevelPreviewImg.SetImg(LevelPreviewBitmap);

    LevelNameLabel.SetWindowTextW(s2.str().c_str());

    CWnd* passwordBox = passwordDialog->GetDlgItem(2135);

    passwordBox->SetWindowTextW(Passwords[selectedLevel - 1].c_str());

    CRect rect;
    LevelNameLabel.GetWindowRect(&rect);
    passwordDialog->ScreenToClient(&rect);

    passwordDialog->InvalidateRect(&rect);
    passwordDialog->UpdateWindow();
}

//Process the Mission Password modal's incoming messages
LRESULT CALLBACK MissionModalWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        if (wmEvent == BN_CLICKED)
        {
            if (wmId == 1 || wmId == 2) {
                DestroyWindow(LevelPreviewImg);
                DeleteObject(LevelPreviewBitmap);
            }

            //Previous Level
            if (wmId == 10)
            {
                if (selectedLevel > 1)
                {
                    selectedLevel -= 1;

                    LoadLevel();
                }
            }
            //Next Level
            else if (wmId == 11)
            {
                if (selectedLevel < 45 && selectedLevel < completedLevels + 1)
                {
                    selectedLevel += 1;

                    LoadLevel();
                }
            }
            //Previous 5 Level
            else if (wmId == 13)
            {
                bool load = selectedLevel != 1;

                selectedLevel -= 5;

                if (selectedLevel < 1)
                    selectedLevel = 1;

                if (load)
                    LoadLevel();


            }
            //Next 5 Level
            else if (wmId == 14)
            {
                bool load = selectedLevel != 45;

                selectedLevel += 5;

                if (selectedLevel > 45 || selectedLevel > completedLevels)
                    selectedLevel = completedLevels >= 45 ? 45 : completedLevels + 1;

                if (load)
                    LoadLevel();
            }
            else {
                return CallWindowProc(ogMissionModalWndProc, hWnd, message, wParam, lParam);
            }
        }
    }
    break;
    case WM_CLOSE:
    {
        DestroyWindow(LevelPreviewImg);
        DeleteObject(LevelPreviewBitmap);
        return CallWindowProc(ogMissionModalWndProc, hWnd, message, wParam, lParam);
    }
    break;
    default:
        return CallWindowProc(ogMissionModalWndProc, hWnd, message, wParam, lParam);
    }

    return CallWindowProc(ogMissionModalWndProc, hWnd, message, wParam, lParam);
    //return 0;
}
//Mission Modal

void CenterWindowOnParent(CWnd* pParentWnd, CWnd* pChildWnd)
{
    // Get the parent window's rectangle
    CRect rectParent;
    pParentWnd->GetWindowRect(&rectParent);

    // Calculate the center of the parent window
    int parentWidth = rectParent.Width();
    int parentHeight = rectParent.Height();
    int parentCenterX = rectParent.left + parentWidth / 2;
    int parentCenterY = rectParent.top + parentHeight / 2;

    // Get the child window's rectangle
    CRect rectChild;
    pChildWnd->GetWindowRect(&rectChild);

    // Calculate the position to center the child window
    int childWidth = rectChild.Width();
    int childHeight = rectChild.Height();
    int childLeft = parentCenterX - childWidth / 2;
    int childTop = parentCenterY - childHeight / 2;

    // Set the child window's position
    pChildWnd->SetWindowPos(NULL, childLeft, childTop, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

//Function that hooks to the CreateDialogIndirectParamA method
typedef HWND(WINAPI* CreateDialogIndirectParamAType)(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
CreateDialogIndirectParamAType pCreateDialogIndirectParamA = nullptr; //original function pointer after hook
CreateDialogIndirectParamAType pCreateDialogIndirectParamATarget; //original function pointer BEFORE hook do not call this!
HWND WINAPI detourCreateDialogIndirectParamA(HINSTANCE hInstance, LPCDLGTEMPLATEA lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
    auto returnVal = pCreateDialogIndirectParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);

    CWnd* pWnd = CWnd::FromHandle(returnVal);

    CString title;
    pWnd->GetWindowTextW(title);

    if (returnVal != NULL && !(LevelNameLabel.GetSafeHwnd() && ::IsWindow(LevelNameLabel.GetSafeHwnd()))) {
        if (title == L"Enter password to continue." || title == L"By kontynuowac wprowadz haslo." || title == L"Mit Paßwort fortsetzen" || title == L"Introducir clave para seguir" ||
            title == L"Introduce contraseña para continuar" || title == L"Entrer un mot de passe pour continuer" || title == L"Per continuare, invia parola d'ordine." ||
            title == L"Voer wachtwoord in om verder te gaan" || title == L"By kontynuować wprowadź hasło." || title == "Digite a senha para continuar" ||
            title == L"Для продолжения введите пароль." || title == L"Ange lösenord för att fortsätta" || title == L"Introduz a palvra-passe para continuar")
        {
            readReg();
            selectedLevel = completedLevels + 1;

            if (selectedLevel > 45)
                selectedLevel = 45;

            // Calculate DPI scaling factor
            double dpiScaleFactor = GetDpiScaleFactor(pWnd->GetSafeHwnd());

            passwordDialog = pWnd;

            //Get all default controls and fetch the used font
            CWnd* okBtn = passwordDialog->GetDlgItem(1);
            CWnd* cancelBtn = passwordDialog->GetDlgItem(2);
            CEdit* passwordBox = (CEdit*)passwordDialog->GetDlgItem(2135);
            

            CDC *okBtnDC = okBtn->GetDC();
            CFont *font = okBtn->GetFont();
            CFont *old = okBtnDC->SelectObject(font);

            okBtnDC->SelectObject(old);
            //Get all default controls and fetch the used font

            //Move, resize and rename window
            CRect rect;
            pWnd->GetWindowRect(&rect);
            rect.left = rect.left;
            rect.top = rect.top - 89;
            rect.right = rect.left + static_cast<int>(278 * dpiScaleFactor);
            rect.bottom = rect.top + static_cast<int>(263 * dpiScaleFactor);
            pWnd->MoveWindow(rect);

            CenterWindowOnParent(CWnd::FromHandle(hWndParent), pWnd);

            //passwordDialog->SetWindowTextW(L"Mission Select");
            passwordDialog->SetWindowTextW(missionSelectText);
            //Move, resize and rename window

            //Level Preview
            LevelPreview.Create(L"", WS_CHILD | WS_VISIBLE | SS_BITMAP, CRect(
                static_cast<int>(11 * dpiScaleFactor),
                static_cast<int>(11 * dpiScaleFactor),
                static_cast<int>(11 * dpiScaleFactor + 250 * dpiScaleFactor),
                static_cast<int>(11 * dpiScaleFactor + 122 * dpiScaleFactor)),
                pWnd, 9);

            LevelPreview.SetFont(font);

            LevelPreviewImg.CreateFromStatic(&LevelPreview);

            LevelPreviewImg.SetBltMode(CWndImage::bltStretch);
            //Level Preview

            //Next and Prev level buttons
            RECT prevLvlRect;
            prevLvlRect.left = static_cast<int>(11 * dpiScaleFactor);
            prevLvlRect.right = static_cast<int>(31 * dpiScaleFactor);
            prevLvlRect.top = static_cast<int>(141 * dpiScaleFactor);
            prevLvlRect.bottom = static_cast<int>(161 * dpiScaleFactor);

            RECT prev5LvlRect;
            prev5LvlRect.left = static_cast<int>(36 * dpiScaleFactor);
            prev5LvlRect.right = static_cast<int>(56 * dpiScaleFactor);
            prev5LvlRect.top = static_cast<int>(141 * dpiScaleFactor);
            prev5LvlRect.bottom = static_cast<int>(161 * dpiScaleFactor);

            RECT nextLvlRect;
            nextLvlRect.left = static_cast<int>(241 * dpiScaleFactor);
            nextLvlRect.right = static_cast<int>(261 * dpiScaleFactor);
            nextLvlRect.top = static_cast<int>(141 * dpiScaleFactor);
            nextLvlRect.bottom = static_cast<int>(161 * dpiScaleFactor);

            RECT next5LvlRect;
            next5LvlRect.left = static_cast<int>(216 * dpiScaleFactor);
            next5LvlRect.right = static_cast<int>(236 * dpiScaleFactor);
            next5LvlRect.top = static_cast<int>(141 * dpiScaleFactor);
            next5LvlRect.bottom = static_cast<int>(161 * dpiScaleFactor);

            prevLvl.Create(L"<", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER, prevLvlRect, pWnd, 10);
            prev5Lvl.Create(L"<<", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER, prev5LvlRect, pWnd, 13);
            nextLvl.Create(L">", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER, nextLvlRect, pWnd, 11);
            next5Lvl.Create(L">>", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_CENTER, next5LvlRect, pWnd, 14);
            //Next and Prev level buttons

            //Level Name label
            LevelNameLabel.Create(L"Mission 1", WS_CHILD | WS_VISIBLE | SS_CENTER, CRect(
                static_cast<int>(57 * dpiScaleFactor),  
                static_cast<int>(141 * dpiScaleFactor),
                static_cast<int>(218 * dpiScaleFactor),
                static_cast<int>(161 * dpiScaleFactor)),
                pWnd, 12);
            LevelNameLabel.SetFont(font);
            //Level Name label

            //Move original ui controls
            passwordBox->SetWindowPos(NULL, static_cast<int>(11 * dpiScaleFactor), static_cast<int>(170 * dpiScaleFactor), static_cast<int>(250 * dpiScaleFactor), static_cast<int>(23 * dpiScaleFactor), SWP_NOZORDER);
            okBtn->SetWindowPos(NULL, static_cast<int>(150 * dpiScaleFactor), static_cast<int>(201 * dpiScaleFactor), static_cast<int>(111 * dpiScaleFactor), static_cast<int>(23 * dpiScaleFactor), SWP_NOZORDER);
            cancelBtn->SetWindowPos(NULL, static_cast<int>(11 * dpiScaleFactor), static_cast<int>(201 * dpiScaleFactor), static_cast<int>(111 * dpiScaleFactor), static_cast<int>(23 * dpiScaleFactor), SWP_NOZORDER);
            //Move original ui controls

            LoadLevel();

            passwordBox->SetSel(-1);

            ogMissionModalWndProc = (WNDPROC)SetWindowLongPtr(returnVal, GWLP_WNDPROC, (LONG_PTR)MissionModalWndProc);
        }
    }
    return returnVal;
}

void AssignLabels() 
{
    if (lang == "en")
    {
        missionSelectText = _TEXT("Mission Select");
        missionText = _TEXT("Mission");
    }
    else if (lang == "pl")
    {
        missionSelectText = _TEXT("Wybór misji");
        missionText = _TEXT("Misja");
    }
    else if (lang == "de")
    {
        missionSelectText = _TEXT("Mission auswählen");
        missionText = _TEXT("Mission");
    }
    else if (lang == "es" || lang == "es-419")
    {
        missionSelectText = _TEXT("Misión Seleccionar");
        missionText = _TEXT("Misión");
    }
    else if (lang == "fr")
    {
        missionSelectText = _TEXT("Sélection de la mission");
        missionText = _TEXT("Mission");
    }
    else if (lang == "it") 
    {
        missionSelectText = _TEXT("Selezione della missione");
        missionText = _TEXT("Missione");
    }
    else if (lang == "nl") 
    {
        missionSelectText = _TEXT("Missie selecteren");
        missionText = _TEXT("Missie");
    }
    else if (lang == "pt" || lang == "pt-br")
    {
        missionSelectText = _TEXT("Seleção da missão");
        missionText = _TEXT("Missão");
    }
    else if (lang == "ru")
    {
        missionSelectText = _TEXT("Выбор миссии");
        missionText = _TEXT("Миссия");
    }
    else if (lang == "sv")
    {
        missionSelectText = _TEXT("Uppdrag Välj");
        missionText = _TEXT("Uppdrag");
    }else
    {
        missionSelectText = _TEXT("Mission Select");
        missionText = _TEXT("Mission");
    }
}

void shutdown() {

    MH_Uninitialize();
}

bool Initialized = false;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        std::filesystem::path a = std::filesystem::current_path();
        mainPath = a.generic_string();

        MH_STATUS status = MH_Initialize();

        if (status != MH_OK)
        {
            std::string sStatus = MH_StatusToString(status);
            shutdown();
            return 0;
        }

        if (MH_CreateHookApiEx(L"user32", "CreateDialogIndirectParamA", &detourCreateDialogIndirectParamA, reinterpret_cast<void**>(&pCreateDialogIndirectParamA), reinterpret_cast<void**>(&pCreateDialogIndirectParamATarget)) != MH_OK) {
            shutdown();
            return 1;
        }

        if (MH_EnableHook(reinterpret_cast<void**>(pCreateDialogIndirectParamATarget)) != MH_OK) {
            shutdown();
            return 1;
        }

        Initialized = true;

        if (std::filesystem::exists("language.txt")) 
        {
            std::ifstream t("language.txt");
            std::stringstream buffer;
            buffer << t.rdbuf();
            lang = buffer.str();
        }
        else {
            lang = "en";
        }

        AssignLabels();
    }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        if(Initialized && lpReserved)
            shutdown();
        break;
    }
    return TRUE;
}

