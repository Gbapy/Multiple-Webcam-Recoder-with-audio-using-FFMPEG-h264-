// WR.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "WR.h"


// This is an example of an exported variable
WR_API int nWR=0;
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

int			nSelectedWC = -1;
HWAVEIN     hWaveIn;
bool		bInitialized = false;
// Forward declarations of functions included in this code module:
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


void CloseRender(void *threadArg) {
	int n = (int)threadArg;
	Sleep(2000);
	wCam[n].CloseMedia();
	SetWindowText(GetDlgItem(hMainDlg, IDC_START_REC), "Start Record");
	EnableWindow(GetDlgItem(hMainDlg, IDC_START_REC), TRUE);
	_endthread();
}

// This is an example of an exported function.
WR_API int fnWR(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see WR.h for the class definition
CWR::CWR()
{
	return;
}
// Mp4Render.cpp : Defines the entry point for the application.
//
void GetWorkDir() {
	char exe[MAX_PATH * 2] = { 0 };

	GetModuleFileName(NULL, exe, sizeof(exe));
	char *tmp = NULL;

	if (tmp = strrchr(exe, '\\'))
	{
		tmp[1] = 0;
	}
	strcpy(szDir, exe);
}

static DWORD WINAPI dummy_window_thread(LPVOID *unused)
{
	static const char dummy_window_class[] = "temp_d3d_window_4039785";
	WNDCLASS wc;
	MSG msg;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_OWNDC;
	wc.hInstance = dll_inst;
	//wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.lpszClassName = dummy_window_class;

	GetWorkDir();
	nResized_Height = 480;
	nResized_Width = 600;
	
	marginPosX = 0; marginPosY = 0; marginWidth = 1.0f; marginHeight = 1.0f;
	wmPosX = 0.7f; wmPosY = 0.8f; wmWidth = 0.2f; wmHeight = 0.1f; wmTransparency = 0.3f;

	bInitialized = false;
	for (int i = 0; i < MAX_WEBCAM; i++) {
		source[i] = VideoCapture(i);
		wCam[i] = WEBCAM(i, nResized_Width, nResized_Height, true, 400000, 64000);
		
		wStream[i] = NULL;
	}
	HACCEL hAccelTable;

	hAccelTable = LoadAccelerators(dll_inst, MAKEINTRESOURCE(IDD_DLG_MAIN));
	hMainDlg = CreateDialog(dll_inst, MAKEINTRESOURCE(IDD_DLG_MAIN), NULL, (DLGPROC)WndProc);
	ShowWindow(hMainDlg, SW_HIDE);
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DWORD dwWaitResult = WaitForMultipleObjects(
		MAX_WEBCAM,   // number of handles in array
		ghRenderThreads,     // array of thread handles
		TRUE,          // wait until all are signaled
		INFINITE);
	dwWaitResult = WaitForMultipleObjects(
		MAX_WEBCAM,   // number of handles in array
		ghCaptureThreads,     // array of thread handles
		TRUE,          // wait until all are signaled
		INFINITE);

	for (int i = 0; i < MAX_WEBCAM; i++) {
		source[i].release();
		wCam[i].CloseMedia();
		CloseHandle(wCam[i].capturing);
	}
	(void)unused;
	return 0;
}

void WINAPI RenderMedia(LPVOID threadArg) {
	int nWebCamNo = (int)threadArg;

	while (!bStopFlag) {
		Mat		frm;
		DWORD	ct = GetTickCount();
		int		mn = 1e+10;
		int		mnI = -1;
		int		ret = 0;

		if (!source[nWebCamNo].isOpened()) { Sleep(30); continue; }
		if (wStream[nWebCamNo] == NULL) { Sleep(30); continue; }

		DWORD dwWaitResult = WaitForSingleObject(wCam[nWebCamNo].capturing, INFINITE);
		if (dwWaitResult != WAIT_OBJECT_0)
			continue;
		for (int i = 0; i < MAX_WSTREAM_COUNT; i++) {
			if (wStream[nWebCamNo]->frm[i].empty()) break;
			if (abs((long)wStream[nWebCamNo]->tick[i] - (long)ct) < mn) {
				mn = abs((long)wStream[nWebCamNo]->tick[i] - (long)ct);
				mnI = i;
			}
		}
		if (mnI >= 0 && mn < 300) {
			wStream[nWebCamNo]->frm[mnI].copyTo(frm);
		}
		else{ Sleep(30); continue; }

		ret = wCam[nWebCamNo].WriteMedia(nWebCamNo, &frm);
		ct = GetTickCount() - ct;

		if (ct < 40) {
			Sleep(40 - ct);
		}
		frm.release();
	}
	_beginthread(CloseRender, 0, (void *)nWebCamNo);
	_endthread();
}

void WINAPI WCapture(LPVOID threadArg) {
	int nFrmCnt = 0;
	WEBCAM_STREAM ws;
	int nWebCamNo = (int)threadArg;


	while (!bStopFlag) {
		Mat frm;
		if (!source[nWebCamNo].isOpened()) {
			source[nWebCamNo].open(nWebCamNo);
			Sleep(300);
			nFrmCnt = 0;
			continue;
		}

		wStream[nWebCamNo] = &ws;
		source[nWebCamNo] >> frm;
		if (frm.empty()) continue;
		nFrmCnt++;
		ResetEvent(wCam[nWebCamNo].capturing);
		if (nFrmCnt > MAX_WSTREAM_COUNT) {
			wStream[nWebCamNo]->frm[0].release();
			for (int i = 0; i < MAX_WSTREAM_COUNT - 1; i++) {
				wStream[nWebCamNo]->frm[i + 1].copyTo(wStream[nWebCamNo]->frm[i]);
				wStream[nWebCamNo]->tick[i] = wStream[nWebCamNo]->tick[i + 1];
			}
			nFrmCnt = MAX_WSTREAM_COUNT;
		}
		frm.copyTo(wStream[nWebCamNo]->frm[nFrmCnt - 1]);
		wStream[nWebCamNo]->tick[nFrmCnt - 1] = GetTickCount();
		frm.release();
		SetEvent(wCam[nWebCamNo].capturing);
		Sleep(20);
	}

	if (wStream[nWebCamNo] != NULL) {
		source[nWebCamNo].release();
		for (int i = 0; i < MAX_WSTREAM_COUNT; i++) {
			if (!wStream[nWebCamNo]->frm[i].empty()) wStream[nWebCamNo]->frm[i].release();
		}
		wStream[nWebCamNo] = NULL;
	}
	_endthread();
}

static inline void init_dummy_window_thread(void)
{
	HANDLE thread =
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)dummy_window_thread, NULL, 0, NULL);
	if (!thread) {
		return;
	}

	CloseHandle(thread);
}

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
{
	if (reason == DLL_PROCESS_ATTACH) {
		dll_inst = hinst;
		init_dummy_window_thread();
	}
	(void)unused1;
	return true;
}

bool WR_API GetModuleState() {
	return bInitialized;
}

int WR_API SetMargin(float nPosX, float nPosY, float nWidth, float nHeight) {
	marginPosX = nPosX; marginPosY = nPosY;
	marginWidth = nWidth; marginHeight = nHeight;
	return 0;
}

int WR_API SetOutputFile(int nCamNo, char *szFile) {
	if (!bInitialized) return 3;
	if (wStream[nCamNo] == NULL) return 1;
	if (wCam[nCamNo].bStatus) return 2;
	strcpy(wCam[nCamNo].szFile, szFile);
	return 0;
}

BOOL WR_API	TerminateModule() {
	bStopFlag = true;
	SendMessage(hMainDlg, WM_DESTROY, NULL, NULL);
	return SendMessage(hMainDlg, WM_QUIT, NULL, NULL);
}

int WR_API	SetDisplayWindow(int nCamNo, HWND hwnd) {
	if (!bInitialized) return 3;
	if (wStream[nCamNo] == NULL) return 1;
	if (wCam[nCamNo].bStatus) return 2;
	wCam[nCamNo].hWnd = hwnd;
	return 0;
}

bool WR_API SetWaterMark(char *szFile) {
	if (FileExists(szFile)) {
		waterMark = imread(szFile);
		return true;
	}
	return false;
}

bool WR_API SetWatermarkGeometry(float posX, float posY, float width, float height, float transparency) {
	wmPosX = posX; wmPosY = posY; wmWidth = width; wmHeight = height; wmTransparency = transparency;
	return true;
}

int WR_API SetCameraState(int nCamNo, bool bCamState) {
	if (!bInitialized) return 3;
	if (wStream[nCamNo] == NULL) return 1;
	if (wCam[nCamNo].bStatus && !bCamState) {
		_beginthread(CloseRender, 0, (void *)nCamNo);
	}
	if (!wCam[nCamNo].bStatus && bCamState) {
		wCam[nCamNo].InitMedia();
		wCam[nCamNo].bStatus = bCamState;
	}
	
	return 0;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	static WAVEFORMATEX waveform;
	static PWAVEHDR     pWaveHdr1, pWaveHdr2;
	static PBYTE        pBuffer1, pBuffer2;

	switch (message)
	{
	case MM_WIM_OPEN:
	{
		pBuffer1 = reinterpret_cast <PBYTE> (malloc(INP_BUFFER_SIZE));
		pBuffer2 = reinterpret_cast <PBYTE> (malloc(INP_BUFFER_SIZE));

		if (!pBuffer1 || !pBuffer2)
		{
			if (pBuffer1) free(pBuffer1);
			if (pBuffer2) free(pBuffer2);
			waveInClose(hWaveIn);
			Warn(hWnd, "Fail to allocate memory for record!");
			bFoundRecDevice = false;
			break;
		}
		pWaveHdr1->lpData = reinterpret_cast <CHAR*>(pBuffer1);
		pWaveHdr1->dwBufferLength = INP_BUFFER_SIZE;
		pWaveHdr1->dwBytesRecorded = 0;
		pWaveHdr1->dwUser = 0;
		pWaveHdr1->dwFlags = 0;
		pWaveHdr1->dwLoops = 1;
		pWaveHdr1->lpNext = NULL;
		pWaveHdr1->reserved = 0;

		waveInPrepareHeader(hWaveIn, pWaveHdr1, sizeof(WAVEHDR));

		pWaveHdr2->lpData = reinterpret_cast <CHAR*>(pBuffer2);
		pWaveHdr2->dwBufferLength = INP_BUFFER_SIZE;
		pWaveHdr2->dwBytesRecorded = 0;
		pWaveHdr2->dwUser = 0;
		pWaveHdr2->dwFlags = 0;
		pWaveHdr2->dwLoops = 1;
		pWaveHdr2->lpNext = NULL;
		pWaveHdr2->reserved = 0;

		waveInPrepareHeader(hWaveIn, pWaveHdr2, sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, pWaveHdr1, sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, pWaveHdr2, sizeof(WAVEHDR));
		waveInStart(hWaveIn);
	}
		break;
	case MM_WIM_DATA:
	{
		static int n = 0;

		n++;
		if (((PWAVEHDR)lParam)->dwBytesRecorded != INP_BUFFER_SIZE) {
			n = 0;
			Warn(hWnd, "Invalid Audio Buffer");
		}
		if (n > 25) {
			for (int i = 0; i < 24; i++) {
				CopyMemory(lpAudioData + INP_BUFFER_SIZE * i, lpAudioData + INP_BUFFER_SIZE * (i + 1), INP_BUFFER_SIZE);
			}
			n = 25;
			nTimeBase = GetTickCount() - 1000;
		}
		if (n == 1) {
			nTimeBase = GetTickCount();
		}
		CopyMemory(lpAudioData + (n - 1) * INP_BUFFER_SIZE, ((PWAVEHDR)lParam)->lpData, ((PWAVEHDR)lParam)->dwBytesRecorded);
		waveInAddBuffer(hWaveIn, (PWAVEHDR)lParam, sizeof(WAVEHDR));
	}
		break;
	case WM_INITDIALOG:
	{
		pWaveHdr1 = reinterpret_cast <PWAVEHDR> (malloc(sizeof(WAVEHDR)));
		pWaveHdr2 = reinterpret_cast <PWAVEHDR> (malloc(sizeof(WAVEHDR)));

		waveInReset(hWaveIn);
		waveform.wFormatTag = WAVE_FORMAT_PCM;
		waveform.nChannels = 1;
		waveform.nSamplesPerSec = 11025;
		waveform.nAvgBytesPerSec = 11025;
		waveform.nBlockAlign = 1;
		waveform.wBitsPerSample = 8;
		waveform.cbSize = 0;

		if (waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD)hWnd, 0, CALLBACK_WINDOW))
		{
			Warn(hWnd, "No record device found!");
			bFoundRecDevice = false;
		}
		else{
			bFoundRecDevice = true;
		}

		SetWindowText(GetDlgItem(hWnd, IDC_OUT_DIR), szDir);
		LVCOLUMNW LvCol;
		HWND hList = GetDlgItem(hWnd, IDC_LIST_CAM);

		SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
			LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_TWOCLICKACTIVATE | LVS_EX_LABELTIP
			);

		memset(&LvCol, 0, sizeof(LvCol));
		LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		LvCol.pszText = L"No";
		LvCol.cx = CompensateXDPI(93);
		LvCol.fmt = LVCFMT_LEFT;
		SendMessage(hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&LvCol);

		LvCol.pszText = L"Capture Status";
		LvCol.cx = CompensateXDPI(186);
		LvCol.fmt = LVCFMT_LEFT;
		SendMessage(hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&LvCol);

		LvCol.pszText = L"Speed";
		LvCol.cx = CompensateXDPI(230);
		LvCol.fmt = LVCFMT_LEFT;
		SendMessage(hList, LVM_INSERTCOLUMNW, 2, (LPARAM)&LvCol);

		LvCol.pszText = L"Status";
		LvCol.cx = CompensateXDPI(230);
		LvCol.fmt = LVCFMT_LEFT;
		SendMessage(hList, LVM_INSERTCOLUMNW, 3, (LPARAM)&LvCol);

		bool flag = false;

		for (int i = 0; i < MAX_WEBCAM; i++) {
			if (!source[i].isOpened()) continue;
			ListItemAdd(hList, i, wCam[i].name);
			ListSubItemSet(hList, i, 1, "Not-Capturing");
			flag = true;
		}

		if (flag) {
			ListView_SetSelectionMark(GetDlgItem(hWnd, IDC_LIST_CAM), 0);
			nSelectedWC = 0;
			wCam[nSelectedWC].bShow = true;
		}
		for (int i = 0; i < MAX_WEBCAM; i++) {
			DWORD dwThreadID;
			ghRenderThreads[i] = CreateThread(
				NULL,              // default security
				0,                 // default stack size
				(LPTHREAD_START_ROUTINE)RenderMedia,        // name of the thread function
				(LPVOID)i,              // no thread parameters
				0,                 // default startup flags
				&dwThreadID);
			ghCaptureThreads[i] = CreateThread(
				NULL,              // default security
				0,                 // default stack size
				(LPTHREAD_START_ROUTINE)WCapture,        // name of the thread function
				(LPVOID)i,              // no thread parameters
				0,                 // default startup flags
				&dwThreadID);
		}
		bInitialized = true;
	}
		break;
	case WM_NOTIFY:
		nSelectedWC = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_LIST_CAM));
		if (nSelectedWC != -1) {
			for (int i = 0; i < MAX_WEBCAM; i++) {
				if (i != nSelectedWC)
					wCam[i].bShow = false;
			}
			wCam[nSelectedWC].bShow = true;
			if (wCam[nSelectedWC].bStatus) {
				SetWindowText(GetDlgItem(hWnd, IDC_START_REC), "Stop Record");
			}
			else
			{
				SetWindowText(GetDlgItem(hWnd, IDC_START_REC), "Start Record");
			}
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		switch (wmId)
		{
		case IDC_START_REC:
			if (nSelectedWC == -1) break;
			if (wCam[nSelectedWC].bStatus){
				EnableWindow(GetDlgItem(hWnd, IDC_START_REC), FALSE);
				_beginthread(CloseRender, 0, (void *)nSelectedWC);
			}
			else{
				wCam[nSelectedWC].InitMedia();
				SetWindowText(GetDlgItem(hWnd, IDC_START_REC), "Stop Record");
			}
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDC_BROWSE_WM:
		{
			OPENFILENAME ofn;
			char szFile[MAX_PATH];

			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Jpg\0*.Jpg\0Jpeg\0 *.jpeg\0Bmp\0*.bmp\0Png\0*.png";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			GetOpenFileName(&ofn);
			if (!FileExists(szFile)) break;
			if (!waterMark.empty()) waterMark.release();
			waterMark = imread(szFile);
			if (!waterMark.empty()) {
				resize(waterMark, waterMark, Size(nResized_Width / 5, nResized_Height / 10));
				ShowImage(GetDlgItem(hWnd, IDC_WATER_MARK), &waterMark);
				SetWindowText(GetDlgItem(hWnd, IDC_WM_DIR), szFile);
			}
		}
			break;
		case IDCANCEL:
			bStopFlag = true;
			waveInClose(hWaveIn);
			EndDialog(hWnd, 0);
			break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
//		if (!waterMark.empty())
			//ShowImage(GetDlgItem(hWnd, IDC_WATER_MARK), &waterMark);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		waveInClose(hWaveIn);
		PostQuitMessage(0);
		break;
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
