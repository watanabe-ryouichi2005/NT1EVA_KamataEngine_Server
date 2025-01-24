
#include <Novice.h>

#include <math.h>
#include <process.h>
#include <mmsystem.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")


DWORD  WINAPI Threadfunc(void* );
SOCKET sWait;
bool bSocket = false;
HWND hwMain;

const char kWindowTitle[] = "KAMATA ENGINEサーバ";

typedef struct {
	float x;
	float y;
}Vector2;

typedef struct {
	Vector2 center;
	float radius;
}Circle;

// キー入力結果を受け取る箱
Circle a, b;
Vector2 center = { 100,100 };
char keys[256] = { 0 };
char preKeys[256] = { 0 };
int color = RED;
int slimeTexutre = Novice::LoadTexture("./NT1_Slime.png");
// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	WSADATA wdData;
	static HANDLE hThread;
	static DWORD dwID;


	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, 1280, 720);

	hwMain = GetDesktopWindow();


	a.center.x = 400;
	a.center.y = 400;
	a.radius = 100;

	b.center.x = 200;
	b.center.y = 200;
	b.radius = 50;

	// winsock初期化
	WSAStartup(MAKEWORD(2, 0), &wdData);

	// データを送受信処理をスレッド（WinMainの流れに関係なく動作する処理の流れ）として生成。
	// データ送受信をスレッドにしないと何かデータを受信するまでRECV関数で止まってしまう。
	hThread = (HANDLE)CreateThread(NULL, 0, &Threadfunc, (LPVOID)&a, 0, &dwID);

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0) {
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		if (keys[DIK_UP] != 0) {
			b.center.y -= 5;
		}
		if (keys[DIK_DOWN] != 0) {
			b.center.y += 5;
		}
		if (keys[DIK_RIGHT] != 0) {
			b.center.x += 5;
		}
		if (keys[DIK_LEFT] != 0) {
			b.center.x -= 5;
		}

		///
		/// ↓更新処理ここから
		///

		float distance =
			sqrtf((float)pow((double)a.center.x - (double)b.center.x, 2) +
				  (float)pow((double)a.center.y - (double)b.center.y, 2));

		if (distance <= a.radius+b.radius) {
			color = BLUE;
		}
		else color = RED;
		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///
		Novice::DrawEllipse((int)a.center.x, (int)a.center.y, (int)a.radius, (int)a.radius, 0.0f, WHITE, kFillModeSolid);
		Novice::DrawEllipse((int)b.center.x, (int)b.center.y, (int)b.radius, (int)b.radius,0.0f, color, kFillModeSolid);
		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0) {
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();

	// winsock終了
	WSACleanup();

	return 0;
}

/* 通信スレッド関数 */
DWORD WINAPI Threadfunc(void* ) {

	SOCKET  sConnect;
	WORD wPort = 8000;
	SOCKADDR_IN saConnect,  saLocal;
	int iLen, iRecv;

	// リスンソケット
	sWait = socket(PF_INET, SOCK_STREAM, 0);

	ZeroMemory(&saLocal, sizeof(saLocal));

	// 8000番に接続待機用ソケット作成
	saLocal.sin_family = AF_INET;
	saLocal.sin_addr.s_addr = INADDR_ANY;
	saLocal.sin_port = htons(wPort);

	if (bind(sWait, (LPSOCKADDR)&saLocal, sizeof(saLocal)) == SOCKET_ERROR) {

		closesocket(sWait);
		SetWindowText(hwMain, L"接続待機ソケット失敗");
		return 1;
	}

	if (listen(sWait, 2) == SOCKET_ERROR) {

		closesocket(sWait);
		SetWindowText(hwMain, L"接続待機ソケット失敗");
		return 1;
	}

	SetWindowText(hwMain, L"接続待機ソケット成功");

	iLen = sizeof(saConnect);

	// sConnectに接続受け入れ
	sConnect = accept(sWait, (LPSOCKADDR)(&saConnect), &iLen);

	// 接続待ち用ソケット解放
	closesocket(sWait);

	if (sConnect == INVALID_SOCKET) {

		shutdown(sConnect, 2);
		closesocket(sConnect);
		shutdown(sWait, 2);
		closesocket(sWait);

		SetWindowText(hwMain, L"ソケット接続失敗");

		return 1;
	}

	SetWindowText(hwMain, L"ソケット接続成功");

	iRecv = 0;

	while (1)
	{
		int     nRcv;

		//old_pos2P = pos2P;

		// データ受け取り
		nRcv = recv(sConnect, (char*)&a, sizeof(Circle), 0);

		if (nRcv == SOCKET_ERROR)break;

		// メッセージ送信
		send(sConnect, (const char*)&b, sizeof(Circle), 0);

		//// 受信したクライアントが操作するキャラの座標が更新されていたら、更新領域を作って
		//// InvalidateRect関数でWM_PAINTメッセージを発行、キャラを再描画する
		//if (old_pos2P.x != pos2P.x || old_pos2P.y != pos2P.y)
		//{
		//	rect.left = old_pos2P.x - 10;
		//	rect.top = old_pos2P.y - 10;
		//	rect.right = old_pos2P.x + 42;
		//	rect.bottom = old_pos2P.y + 42;
		//	InvalidateRect(hwMain, &rect, TRUE);
		//}
	}

	shutdown(sConnect, 2);
	closesocket(sConnect);

	return 0;
}

#if 0
// 通信スレッド関数
static DWORD WINAPI threadfunc(void* px) {

	SOCKET sConnect;
	WORD wPort = 8000;
	int iLen, iRecv;
	struct sockaddr_in saConnect, saLocal;

	// リスンソケット
	sWait = socket(PF_INET, SOCK_STREAM, 0);

	ZeroMemory(&saLocal, sizeof(saLocal));

	// 8000番に接続待機用ソケット作成
	saLocal.sin_family = AF_INET;
	saLocal.sin_addr.s_addr = INADDR_ANY;
	saLocal.sin_port = htons(wPort);

	if (bind(sWait, (LPSOCKADDR)&saLocal, sizeof(saLocal)) == SOCKET_ERROR) {

		closesocket(sWait);

		SetWindowText(hwMain, (LPCWSTR)"接続待機ソケット失敗");

		return 1;
	}

	if (listen(sWait, 2) == SOCKET_ERROR) {

		closesocket(sWait);

		SetWindowText(hwMain, (LPCWSTR)"接続待機ソケット失敗");

		return 1;
	}

	SetWindowText(hwMain, (LPCWSTR)"接続待機ソケット成功");

	iLen = sizeof(saConnect);

	// sConnectに接続受け入れ
	sConnect = accept(sWait, (LPSOCKADDR)(&saConnect), &iLen);

	if (sConnect == INVALID_SOCKET) {

		shutdown(sConnect, 2);
		closesocket(sConnect);

		shutdown(sWait, 2);
		closesocket(sWait);

		SetWindowText(hwMain, (LPCWSTR)"ソケット接続失敗");

		return 1;
	}

	// ソケット作成フラグセット
	bSocket = true;

	SetWindowText(hwMain, (LPCWSTR)"ソケット接続成功");

	iRecv = 0;

	while (1)
	{
		int     nRcv;

		// データ受け取り
		nRcv = recv(sConnect, (char*)&a, sizeof(Circle), 0);

		if (nRcv == SOCKET_ERROR)
		{
			break;
		}

		// メッセージ送信
		send(sConnect, (const char*)&b, sizeof(Circle), 0);
	}

	// ソケットを閉じる
	timeEndPeriod(1);

	shutdown(sConnect, 2);
	closesocket(sConnect);
	closesocket(sWait);

	return 0;
}
#endif