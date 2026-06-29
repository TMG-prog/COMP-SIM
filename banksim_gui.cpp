
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "comctl32.lib")

// ---------- Control IDs ----------
#define ID_EDIT_COUNT   101
#define ID_BTN_RUN      102
#define ID_LABEL        103
#define ID_OUTPUT_EDIT  201

HWND g_hInputWnd  = nullptr;
HWND g_hOutputWnd = nullptr;
HWND g_hEditCount  = nullptr;
HWND g_hOutputEdit = nullptr;

// ---------- Simulation data structures ----------
struct Customer {
    int    id;
    double iat, arrivalTime, serviceTime, serviceStart, serviceEnd;
    double waitTime, timeInSystem;
};

// ---------- Run the simulation and build the report text ----------
std::wstring runSimulationToText(int numCustomers) {
    if (numCustomers <= 0) numCustomers = 100;

    std::vector<Customer> customers(numCustomers);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> iatDist(1.0, 8.0);
    std::uniform_real_distribution<double> stDist(1.0, 6.0);

    double clock = 0.0, serverFreeAt = 0.0;
    for (int i = 0; i < numCustomers; ++i) {
        Customer& c = customers[i];
        c.id = i + 1;
        c.iat = iatDist(gen);
        clock += c.iat;
        c.arrivalTime = clock;
        c.serviceTime = stDist(gen);
        c.serviceStart = (c.arrivalTime > serverFreeAt) ? c.arrivalTime : serverFreeAt;
        c.waitTime = c.serviceStart - c.arrivalTime;
        c.serviceEnd = c.serviceStart + c.serviceTime;
        c.timeInSystem = c.waitTime + c.serviceTime;
        serverFreeAt = c.serviceEnd;
    }

    std::wostringstream out;
    out << std::fixed << std::setprecision(2);
    out << L"Cust   IAT       ArrTime     SvcTime   SvcStart    SvcEnd      Wait      InSystem\r\n";
    out << L"--------------------------------------------------------------------------------\r\n";

    double totalWait = 0, totalService = 0, totalInSystem = 0, idleTime = 0, lastEnd = 0;
    int numWaited = 0;

    for (auto& c : customers) {
        out << std::setw(5) << std::left << c.id << L"  "
            << std::setw(8) << c.iat << L"  "
            << std::setw(10) << c.arrivalTime << L"  "
            << std::setw(8) << c.serviceTime << L"  "
            << std::setw(10) << c.serviceStart << L"  "
            << std::setw(10) << c.serviceEnd << L"  "
            << std::setw(8) << c.waitTime << L"  "
            << std::setw(8) << c.timeInSystem << L"\r\n";

        totalWait += c.waitTime;
        totalService += c.serviceTime;
        totalInSystem += c.timeInSystem;
        if (c.waitTime > 0.0001) numWaited++;

        double gap = c.serviceStart - lastEnd;
        if (gap > 0) idleTime += gap;
        lastEnd = c.serviceEnd;
    }

    int n = numCustomers;
    double totalTime = lastEnd;
    double avgWait = totalWait / n;
    double avgService = totalService / n;
    double avgInSystem = totalInSystem / n;
    double probWait = (double)numWaited / n * 100.0;
    double serverUtil = (totalTime > 0) ? (totalService / totalTime) * 100.0 : 0;
    double idlePercent = 100.0 - serverUtil;

    out << L"\r\n================ QUEUE STATISTICS ================\r\n";
    out << L"Total customers served        : " << n << L"\r\n";
    out << L"Total simulation time         : " << totalTime << L" minutes\r\n";
    out << L"Average inter-arrival time    : " << (totalTime / n) << L" minutes\r\n";
    out << L"Average service time          : " << avgService << L" minutes\r\n";
    out << L"Average waiting time in queue : " << avgWait << L" minutes\r\n";
    out << L"Average time in system        : " << avgInSystem << L" minutes\r\n";
    out << L"Customers who waited          : " << numWaited << L" (" << probWait << L"%)\r\n";
    out << L"Server idle time              : " << idleTime << L" minutes\r\n";
    out << L"Server utilization            : " << serverUtil << L" %\r\n";
    out << L"Server idle percentage        : " << idlePercent << L" %\r\n";

    return out.str();
}

// ---------- OUTPUT WINDOW procedure ----------
LRESULT CALLBACK OutputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE: {
            RECT rc; GetClientRect(hWnd, &rc);
            MoveWindow(g_hOutputEdit, 0, 0, rc.right, rc.bottom, TRUE);
            return 0;
        }
        case WM_CLOSE:
            ShowWindow(hWnd, SW_HIDE);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void createOutputWindow(HINSTANCE hInst) {
    if (g_hOutputWnd) return;

    WNDCLASS wc = {};
    wc.lpfnWndProc = OutputWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"BankSimOutputWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    g_hOutputWnd = CreateWindowEx(0, L"BankSimOutputWindow", L"Output Window - Simulation Results",
        WS_OVERLAPPEDWINDOW,
        520, 100, 720, 600,
        nullptr, nullptr, hInst, nullptr);

    g_hOutputEdit = CreateWindowEx(0, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        0, 0, 720, 600,
        g_hOutputWnd, (HMENU)ID_OUTPUT_EDIT, hInst, nullptr);

    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH, L"Consolas");
    SendMessage(g_hOutputEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// ---------- INPUT WINDOW procedure ----------
LRESULT CALLBACK InputWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;

            CreateWindowEx(0, L"STATIC", L"Number of customers:",
                WS_CHILD | WS_VISIBLE, 20, 20, 160, 20,
                hWnd, (HMENU)ID_LABEL, hInst, nullptr);

            g_hEditCount = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"100",
                WS_CHILD | WS_VISIBLE | ES_NUMBER,
                190, 18, 80, 24,
                hWnd, (HMENU)ID_EDIT_COUNT, hInst, nullptr);

            CreateWindowEx(0, L"BUTTON", L"Run Simulation",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                20, 60, 250, 32,
                hWnd, (HMENU)ID_BTN_RUN, hInst, nullptr);

            return 0;
        }
        case WM_COMMAND: {
            if (LOWORD(wParam) == ID_BTN_RUN) {
                wchar_t buf[16];
                GetWindowText(g_hEditCount, buf, 16);
                int numCustomers = _wtoi(buf);
                if (numCustomers <= 0) numCustomers = 100;

                std::wstring report = runSimulationToText(numCustomers);

                createOutputWindow((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE));
                SetWindowText(g_hOutputEdit, report.c_str());
                ShowWindow(g_hOutputWnd, SW_SHOW);
                UpdateWindow(g_hOutputWnd);
                SetForegroundWindow(g_hOutputWnd);
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = InputWndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"BankSimInputWindow";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    RegisterClass(&wc);

    g_hInputWnd = CreateWindowEx(0, L"BankSimInputWindow", L"Input Window - Bank Queue Simulation",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        100, 100, 320, 150,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(g_hInputWnd, nCmdShow);
    UpdateWindow(g_hInputWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}