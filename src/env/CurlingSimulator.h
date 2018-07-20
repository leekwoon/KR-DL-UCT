
#include <deque>


namespace curling_simulator {

	// 긒??륃뺪
	typedef struct _GAMESTATE {
		int		ShotNum;		// Number of current shots
								// ShotNum Is n, the next shot is the n + 1 th shot

		int		CurEnd;			// Current end number
		int		LastEnd;		// Final end number
		int		Score[10];		// Score from 1st to 10th end / 선공기준
		bool	WhiteToMove;	// Information on turn
								// 0 -> 선공차례, 1-> 후공차례

		float	body[16][2];	// 스톤좌표


	} GAMESTATE, *PGAMESTATE;

	typedef struct _ShotPos {
		float x;
		float y;
		bool angle;

	} SHOTPOS, PSHOTPOS;

	typedef struct _ShotVec {
		float x;
		float y;
		bool angle;

	} SHOTVEC, PSHOTVEC;

	typedef struct _ShotVecTheta {
		float v;
		float theta;
	} SHOTVECTHETA;


	// 不要な場合は以下コメントにしてください

	/* シミュレーション関数 */
	// GAMESTATE	*pGameState	- シミュレーション前の局面情報
	// SHOTVEC		Shot		- シミュレーションを行うショットベクトル
	// float		Rand		- 乱数の大きさ
	// SHOTVEC		*lpResShot	- 実際にシミュレーションで使われたショットベクトル（乱数が加えられたショットベクトルの値）
	// int			LoopCount	- 何フレームまでシミュレートを行うか（-1を指定すると最後までシミュレーションを行う）
	// 戻り値	- Simulation関数が失敗すると0が返ります。成功するとそれ以外の値が返ります
	int Simulation(GAMESTATE *pGameState, SHOTVEC Shot, float Rand, SHOTVEC *lpResShot, int LoopCount);

	/* シミュレーション関数（拡張版） */
	// GAMESTATE	*pGameState	- シミュレーション前の局面情報
	// SHOTVEC		Shot		- シミュレーションを行うショットベクトル
	// float		RandX		- 横方向の乱数の大きさ
	// float		RandY		- 縦方向の乱数の大きさ
	// SHOTVEC		*lpResShot	- 実際にシミュレーションで使われたショットベクトル（乱数が加えられたショットベクトルの値）
	// float		*pLoci		- シミュレーション結果（軌跡）を受け取る配列
	//							  1フレーム毎のストーンの位置座標（X,Y合わせ32個セット）が一次元配列で返る
	// int			ResLociSize	- シミュレーション結果（軌跡）を受け取る配列の最大サイズ（シミュレーション結果がこのサイズを超える場合には、このサイズまで結果が格納されます）
	// 戻り値	- SimulationEx関数が失敗すると0が返ります。成功した場合にはシミュレーションに掛ったフレーム数が返ります
	int SimulationEx(GAMESTATE *pGameState, SHOTVEC Shot, float RandX, float RandY, SHOTVEC *lpResShot, float *pLoci, int ResLociSize);

	/* Shot generation function (draw shot) */
	// SHOTPOS ShotPos - Specify the coordinates. A shot that stops at the coordinates specified here will be generated
	// SHOTVEC * lpResShotVec - Specify the address to receive the generated shot
	// Return value - 0 will be returned if the CreateShot function fails. Other values ​​will be returned if it succeeds
	int CreateShot(SHOTPOS ShotPos, SHOTVEC *lpResShotVec);

	/* Shot generation function (take shot) */
	// SHOTPOS ShotPos - Specify the coordinates. A shot passing through the coordinates specified here will be generated
	// float Power - Specify the strength of the shot. A shot of the strength specified here will be generated
	// SHOTVEC * lpResShotVec - Specify the address to receive the generated shot
	// Return value - 0 will be returned if the CreateShot function fails. Other values ​​will be returned if it succeeds
	int CreateHitShot(SHOTPOS Shot, float Power, SHOTVEC *lpResShot);

	//typedef int (*SIMULATION_FUNC)( GAMESTATE *pGameState, SHOTVEC Shot, float Rand, SHOTVEC *lpResShot, int LoopCount );
	//typedef int (*SIMULATIONEX_FUNC)( GAMESTATE *pGameState, SHOTVEC Shot, float RandX, float RandY, SHOTVEC *lpResShot, float *pLoci, size_t ResLociSize );
	//typedef int (*CREATESHOT_FUNC)( SHOTPOS ShotPos, SHOTVEC *lpResShotVec );
	//typedef int (*CREATEHITSHOT_FUNC)( SHOTPOS Shot, float Power, SHOTVEC *lpResShot );

	int GetScore(GAMESTATE *pGameState);

} // namespace curling_simulator