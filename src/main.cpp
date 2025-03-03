
// Internal includes
#include "Globals.hpp"
#include "AppLogic.hpp"

// Common main function called from the entry point
void commonMain()
{
    // Instantiate application logic and view
    auto appLogic = std::make_unique<AppLogic>();
    appLogic->init();

    while(1){
        appLogic->update();
    };

    appLogic.reset();

    // Exit successfully
    LOG_INFO("Done!");
}

// Console application entry point
int main(int argc, char** argv)
{
    // Call common main function
    commonMain();

    // Application finished
    return EXIT_SUCCESS;
}

// Windows application entry point
int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    int argc = 0;
    LPWSTR* args = CommandLineToArgvW(pCmdLine, &argc);

    bool console = false;
    for (int i = 0; i < argc; i++) {
        if (std::wstring(args[i]) == (L"--console")) {
            console = true;
        }
    }

    if (console) {
        if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
            errno_t err;
            FILE* stream = nullptr;
            err = freopen_s(&stream, "CONOUT$", "w", stdout);
            err = freopen_s(&stream, "CONOUT$", "w", stderr);
        }
    }

    // Call common main function
    commonMain();

    // Application finished
    return EXIT_SUCCESS;
}
