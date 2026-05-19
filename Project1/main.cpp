#include <stdio.h>
#include <windows.h>
#include <process.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <list>
#include <cstdlib>
#include <ctime>


#pragma comment(lib, "winmm.lib")

using namespace std;
list <int> lst;

HANDLE g_hWorkerEvent;
HANDLE g_hDeleteEvent;
HANDLE g_hPrintEvent;
HANDLE g_hSaveEvent;
SRWLOCK g_srwLock;


unsigned __stdcall PrintThread(void* arg) // 읽기
{
	while (1) {

		WaitForSingleObject(g_hPrintEvent, INFINITE);
		wprintf(L"PrintThread :");


		AcquireSRWLockShared(&g_srwLock);
		for (list<int>::iterator it = lst.begin(); it != lst.end(); ++it) {
			wprintf(L"%d ", *it);
		}
		ReleaseSRWLockShared(&g_srwLock);


		wprintf(L"\n");

	}
	return 0;
}

unsigned __stdcall DeleteThread(void* arg) // 읽기 쓰기 ?
{
	while (1) {

		WaitForSingleObject(g_hDeleteEvent, INFINITE);
		wprintf(L"DeleteThread!\n");

		AcquireSRWLockExclusive(&g_srwLock);

		if (!lst.empty()) {

			lst.pop_back();

		}
		ReleaseSRWLockExclusive(&g_srwLock);


	}
	return 0;
}
unsigned __stdcall WorkerThread(void* arg) // 쓰기
{
	srand((unsigned int)time(NULL) + GetCurrentThreadId());

	// 스레드 마다 다른 값을 하기 위해 스레드 id 값 연산 추가
	while (1) {

		WaitForSingleObject(g_hWorkerEvent, INFINITE);
		ResetEvent(g_hWorkerEvent);
		wprintf(L"WorkerThread!\n");

		AcquireSRWLockExclusive(&g_srwLock);
		lst.push_back(rand() % 100 + 1);

		ReleaseSRWLockExclusive(&g_srwLock);
	}

	return 0;
}
unsigned __stdcall SaveThread(void* arg) // 읽기 
{


	while (1) {

		WaitForSingleObject(g_hSaveEvent, INFINITE);
		wprintf(L"SaveThread!\n");
		// 텍스트 모드
		FILE* pFile = nullptr;
		_wfopen_s(&pFile, L"test.txt", L"w");  // 쓰기 모드 (덮어쓰기)

		if (pFile != nullptr) {


			AcquireSRWLockShared(&g_srwLock);
			for (int value : lst) {
				fwprintf(pFile, L"%d-", value);
			}
			ReleaseSRWLockShared(&g_srwLock);

			fclose(pFile);
		}
	}

	return 0;
}


int main()
{

	InitializeSRWLock(&g_srwLock);

	// 유니코드 출력 초기화
	int tmp = _setmode(_fileno(stdout), _O_U16TEXT);
	timeBeginPeriod(1);


	// 이벤트 생성
	g_hPrintEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	g_hDeleteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	g_hWorkerEvent = CreateEvent(NULL, TRUE, FALSE, NULL);   // 수동
	g_hSaveEvent = CreateEvent(NULL, FALSE, FALSE, NULL);


	HANDLE hThreads[6]{};
	hThreads[0] = (HANDLE)_beginthreadex(nullptr, 0, PrintThread, nullptr, 0, nullptr);
	hThreads[1] = (HANDLE)_beginthreadex(nullptr, 0, DeleteThread, nullptr, 0, nullptr);
	hThreads[2] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	hThreads[3] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	hThreads[4] = (HANDLE)_beginthreadex(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
	hThreads[5] = (HANDLE)_beginthreadex(nullptr, 0, SaveThread, nullptr, 0, nullptr);

	while (1) {
		Sleep(333);
		SetEvent(g_hDeleteEvent);   // Worker: 즉시

		Sleep(666);
		SetEvent(g_hWorkerEvent);   // Worker: 즉시
		SetEvent(g_hPrintEvent);    // Print: 0.5초 후
		// 키 눌렀을 시 파일 저장
		if (_kbhit()) {
			int key = _getch();
			if (key == ' ') {  // 스페이스바
				SetEvent(g_hSaveEvent);
			}
		}


	}
	WaitForMultipleObjects(6, hThreads, TRUE, INFINITE);

	for (int i = 0; i < 6; i++) CloseHandle(hThreads[i]);

	timeEndPeriod(1);
	return 0;
}