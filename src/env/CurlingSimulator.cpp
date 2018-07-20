
#include	<Box2D/Box2D.h>
#include	<time.h>
#include	<random>
#include    <iostream>

#include	"CurlingSimulator.h"


namespace curling_simulator {

	/*
	#ifdef _DEBUG
		#pragma comment( lib, "Box2D_d.lib" )
	#endif
	#ifndef _DEBUG
		#pragma comment( lib, "Box2D.lib" )
	#endif
	*/

	// �X�g?�����
#define STONEINFO_SIZE 0.145f
#define STONEINFO_ANGLE 0.0f
#define STONEINFO_DYNAMIC true
#define STONEINFO_ISBALL true
#define STONEINFO_DENSITY 10.0f
#define STONEINFO_RESTITUTION 1.0f
#define STONEINFO_FRICTION 12.009216f
//#define STONEINFO_FRICTION 0.12009216f
#define STONEINFO_CATEGORYBITS 3
#define STONEINFO_MASKBITS 3

#define RINKINFO_X 0.0f
#define RINKINFO_Y 3.05f
#define RINKINFO_W 4.75f
#define RINKINFO_H 42.5f

#define TEEPOSITION_X 2.375f
#define TEEPOSITION_Y 4.88f

#define PLAYAREA_X 0.0f
#define PLAYAREA_Y 3.05f
#define PLAYAREA_W 4.75f
#define PLAYAREA_H 8.23f

// �x�N�g���̍ő�l
#define SHOTVEC_Y_MIN -48.79f

	enum {
		STATE_RINK = 0x0001,					// �����N��
		STATE_PLAYAREA = STATE_RINK << 1,			// �v���C�G���A��
		STATE_FREEGUARD = STATE_PLAYAREA << 1,		// �t��?�K?�h??��
		STATE_HOUSE = STATE_FREEGUARD << 1		// �n�E�X��
	};

	// �n??����
	typedef struct _GROUND {
		b2Body *body;
	} GROUND, *LPGROUND;

	// �Q??���
	typedef struct _SUBGAMESTATE {
		int		ShotNum;
		b2Body	*body[16];

	} SUBGAMESTATE, *PSUBGAMESTATE;

	// �X�g?�����
	typedef struct _StoneInfo {
		float	RangeWithError_x;
		float	RangeWithError_y;

	} STONEINFO, *PSTONEINFO;

	// ?���T�C�Y
	float	VIWSC = 32.0f;

	b2Vec2 gravity(0.0f, 0.0f);
	bool doSleep = true;

	// FPS
	//float g_timeStep = 1.0f / 600.0f;
	float g_timeStep = 1.0f / 1000.0f;  // �����I�ɕύX 2015/11/26
	// �V?����?�V�������x
	int velocityIterations = 10;
	int positionIterations = 10;

	// �����N�E�v��?�G���A
	STONEINFO StoneInfo;

	// ��?�����x
#define STANDARD_ANGLE 0.066696f

// �s�N�Z�� �� ��?�g��
	float ptom(int x)
	{
		return	(float)(x / VIWSC);
	}

	// ��?�g�� �� �s�N�Z��
	int mtop(float x)
	{
		return	(int)(x*VIWSC);
	}

	std::mt19937 dice;

	// ?�f�B?�̍쐬
	//=====================================================================================
	b2Body *CreateBody(float x, float y, float w,
		float h, float angle, bool dynamic, int isBall,
		float density, float restitution, float friction, int categoryBits, int maskBits, b2World *world)
	{
		b2BodyDef bodyDef;

		// ���I?�ÓI
		if (dynamic) {
			bodyDef.type = b2_dynamicBody;
		}
		else {
			bodyDef.type = b2_staticBody;
		}

		// ?���ʒu
		bodyDef.position.Set(x, y);

		// �p�x
		bodyDef.angle = angle;

		// ?�f�B�̐���
		b2Body *body = world->CreateBody(&bodyDef);


		// �t�B�N�X?���̍쐬
		//-------------------------------------------------------
		b2FixtureDef fixtureDef;

		b2CircleShape dynamicBall;
		dynamicBall.m_radius = w;

		b2PolygonShape staticBox;
		staticBox.SetAsBox(w, h);

		b2PolygonShape staticTri;
		staticTri.SetAsBox(w, h);
		staticTri.m_vertices[0].Set(0, 0);
		staticTri.m_vertices[1].Set(w, 0);
		staticTri.m_vertices[2].Set(w / 2, h*sqrt(3.0f) / 2);


		if (isBall == 1) {
			fixtureDef.shape = &dynamicBall;
		}
		else if (isBall == 0) {
			fixtureDef.shape = &staticBox;
		}
		else {
			fixtureDef.shape = &staticTri;
		}

		// ���x
		fixtureDef.density = density;

		// ������
		fixtureDef.restitution = restitution;

		// ?�C��
		fixtureDef.friction = friction;

		fixtureDef.filter.categoryBits = categoryBits;
		fixtureDef.filter.maskBits = maskBits;

		body->CreateFixture(&fixtureDef);

		return body;
	}

	bool SetShot(SHOTVEC Shot, SUBGAMESTATE *pSubGameState, b2World *world, float RandX, float RandY, SHOTVEC *lpResShot)
	{
		bool bRet = false;

		if (pSubGameState->ShotNum < 16) {
			pSubGameState->body[pSubGameState->ShotNum] = CreateBody(RINKINFO_W / 2, 41.28f, STONEINFO_SIZE, STONEINFO_SIZE, STONEINFO_ANGLE, STONEINFO_DYNAMIC, STONEINFO_ISBALL, STONEINFO_DENSITY, STONEINFO_RESTITUTION, STONEINFO_FRICTION, STONEINFO_CATEGORYBITS, STONEINFO_MASKBITS, world);

			lpResShot->x = Shot.x + RandX;
			lpResShot->y = Shot.y + RandY;
			lpResShot->angle = Shot.angle;

			pSubGameState->body[pSubGameState->ShotNum]->SetLinearVelocity(b2Vec2(lpResShot->x, lpResShot->y));
			if (lpResShot->angle == 0) {
				pSubGameState->body[pSubGameState->ShotNum]->SetAngularVelocity(STANDARD_ANGLE);
			}
			else {
				pSubGameState->body[pSubGameState->ShotNum]->SetAngularVelocity(STANDARD_ANGLE * -1);
			}
			pSubGameState->ShotNum++;

			bRet = true;
		}

		return bRet;
	}

	// ?�C�͂̌v�Z
	//=====================================================================================
	b2Vec2 FrictionStep(float friction, b2Vec2 vec, float angle)
	{
		b2Vec2	velocity = vec;
		float length = velocity.Length();

		if (length > friction) {
			b2Vec2 normalize, normalize2;
			normalize = velocity;

			normalize.Normalize();
			normalize2.y = normalize.x;
			normalize2.x = -1 * normalize.y;

			normalize *= friction;
			velocity -= normalize;

			// ��?�ɂ��e��
			if (angle != 0.0f) {
				normalize2 *= ((angle > 0) ? STANDARD_ANGLE * friction : -STANDARD_ANGLE * friction);
				length = velocity.Length();
				velocity += normalize2;
				velocity.Normalize();
				velocity *= length;
			}

			return velocity;
		}
		else {
			return b2Vec2(0.0f, 0.0f);
		}
	}

	void Friction(float friction, SUBGAMESTATE *pSubGameState)
	{
		b2Vec2 vec;

		for (int i = 0; i < pSubGameState->ShotNum; i++) {
			if (pSubGameState->body[i] != nullptr) {
				vec = FrictionStep(friction, pSubGameState->body[i]->GetLinearVelocity(), pSubGameState->body[i]->GetAngularVelocity());
				pSubGameState->body[i]->SetLinearVelocity(vec);
				if (vec.Length() == 0) {
					pSubGameState->body[i]->SetAngularVelocity(0.0f);
				}
			}
		}
	}

	// �X�g?���̈ʒu
	int GetStoneState(float x, float y)
	{
		int iRet = 0;
		b2Vec2 WorkVec = b2Vec2(x - 2.375f, y - 4.88f);

		// �����N��
		if ((x > RINKINFO_X + STONEINFO_SIZE && x < RINKINFO_X + RINKINFO_W - STONEINFO_SIZE)
			&& (y > RINKINFO_Y + STONEINFO_SIZE && y < RINKINFO_Y + RINKINFO_H - STONEINFO_SIZE))
		{
			iRet |= STATE_RINK;
		}

		// �v���C�G���A��
		if ((x > PLAYAREA_X + STONEINFO_SIZE && x < PLAYAREA_X + PLAYAREA_W - STONEINFO_SIZE)
			&& (y > PLAYAREA_Y - STONEINFO_SIZE && y < PLAYAREA_Y + PLAYAREA_H - STONEINFO_SIZE))
		{
			iRet |= STATE_PLAYAREA;
		}

		// �n�E�X��
		if (WorkVec.Length() < 1.83f + STONEINFO_SIZE) {
			iRet |= STATE_HOUSE;
		}

		// �t��?�K?�h??��
		if (!(iRet & STATE_HOUSE)
			&& (x > PLAYAREA_X + STONEINFO_SIZE && x < PLAYAREA_X + PLAYAREA_W - STONEINFO_SIZE)
			&& (y > 4.88 + STONEINFO_SIZE && y < PLAYAREA_Y + PLAYAREA_H - STONEINFO_SIZE))
		{
			iRet |= STATE_FREEGUARD;
		}

		return iRet;
	}

	// �V?����?�V���� - ���C����?�v
	//=====================================================================================
	int MainLoop(SUBGAMESTATE *pSubGameState, b2World *world, float timeStep, float *pLoci, size_t ResLociSize, int LoopCount = -1)
	{
		int iRet;
		b2Vec2 vec;

		int stoneState;

		Friction(STONEINFO_FRICTION*timeStep*0.5f, pSubGameState);
		for (iRet = 0; iRet < LoopCount || LoopCount == -1; iRet++) {
			// ?�C�͂̌v�Z
			world->Step(timeStep, velocityIterations, positionIterations);
			Friction(STONEINFO_FRICTION*timeStep, pSubGameState);

			// �V���b�g�̋O�Ղ�z��Ɋi?����
			if (pLoci != nullptr) {
				for (int i = 0; i < 16 && ((iRet * 32 + ((i + i) * 2)) * sizeof(float) < ResLociSize); i++) {
					if (i < pSubGameState->ShotNum && pSubGameState->body[i] != nullptr) {
						vec = pSubGameState->body[i]->GetPosition();
						pLoci[iRet * 32 + i * 2] = vec.x;
						pLoci[iRet * 32 + i * 2 + 1] = vec.y;
					}
					else {
						pLoci[iRet * 32 + i * 2] = 0.0f;
						pLoci[iRet * 32 + i * 2 + 1] = 0.0f;
					}
				}
			}


			/*
				�S�ẴX�g?������?������ true ��Ԃ�
				�X�g?���������N����o���ꍇ�ɂ͂��̃X�g?�������O����
			*/
			for (int i = 0; i < pSubGameState->ShotNum; i++) {
				if (pSubGameState->body[i] != nullptr) {
					// �X�g?���������N�O�ɏo���珜�O����
					vec = pSubGameState->body[i]->GetPosition();
					stoneState = GetStoneState(vec.x, vec.y);
					if (stoneState == 0) {
						world->DestroyBody(pSubGameState->body[i]);
						pSubGameState->body[i] = nullptr;
					}
					else if (pSubGameState->body[i]->IsAwake() == true) {
						break;
					}
				}

				if (i == pSubGameState->ShotNum - 1) {
					if (LoopCount > iRet) {
						iRet = -1;
					}
					goto LOOP_END;
				}
			}
		}
	LOOP_END:

		// �X�g?�����v���C�G���A���ɂ��邩�ǂ���?�F�b�N
		// �v���C�G���A�O�ł���Ώ��O����
		if (pSubGameState->body[pSubGameState->ShotNum - 1] != nullptr) {
			vec = pSubGameState->body[pSubGameState->ShotNum - 1]->GetPosition();
			stoneState = GetStoneState(vec.x, vec.y);
			if (LoopCount == -1 && !(stoneState & STATE_PLAYAREA)) {
				pSubGameState->body[pSubGameState->ShotNum - 1] = nullptr;
			}
		}

		return iRet;
	}

	// ?�����v�Z����
	//=====================================================================================
	int GetScore(GAMESTATE *pGameState)
	{
		/*
			��葤�̓�?����?�ɃX�R�A��Ԃ��B
			��肪��?�����ꍇ�ɂ͐��̐����A��肪��?�����ꍇ�ɂ͕��̐���Ԃ��B
		*/

		int iScore = 0;
		float NearestLength_r, NearestLength_y, House;
		b2Vec2 WorkVec, TeePos;

		TeePos.Set(2.375f, 4.88f);
		NearestLength_r = NearestLength_y = House = (1.83f + 0.15f);

		// ����No.1�X�g?��
		for (int i = 0; i <= pGameState->ShotNum; i += 2) {
			if (pGameState->body[i] != nullptr) {
				WorkVec = b2Vec2(pGameState->body[i][0], pGameState->body[i][1]);
				WorkVec.operator -=(TeePos);
				if (WorkVec.Length() <= NearestLength_r) {
					NearestLength_r = WorkVec.Length();
				}
			}
		}

		// ����No.1�X�g?��
		for (int i = 1; i <= pGameState->ShotNum; i += 2) {
			if (pGameState->body[i] != nullptr) {
				WorkVec = b2Vec2(pGameState->body[i][0], pGameState->body[i][1]);
				WorkVec.operator -=(TeePos);
				if (WorkVec.Length() <= NearestLength_y) {
					NearestLength_y = WorkVec.Length();
				}
			}
		}

		// ��?�̌v�Z
		if (NearestLength_r < NearestLength_y) {
			for (int i = 0; i <= pGameState->ShotNum; i += 2) {
				if (pGameState->body[i] != nullptr) {
					WorkVec = b2Vec2(pGameState->body[i][0], pGameState->body[i][1]);
					WorkVec.operator -=(TeePos);
					if (WorkVec.Length() < NearestLength_y) {
						iScore++;
					}
				}
			}
		}
		else if (NearestLength_y < NearestLength_r) {
			for (int i = 1; i <= pGameState->ShotNum; i += 2) {
				if (pGameState->body[i] != nullptr) {
					WorkVec = b2Vec2(pGameState->body[i][0], pGameState->body[i][1]);
					WorkVec.operator -=(TeePos);
					if (WorkVec.Length() < NearestLength_r) {
						iScore--;
					}
				}
			}
		}

		if ((pGameState->ShotNum % 2 == 0 && pGameState->WhiteToMove != 0)
			|| (pGameState->ShotNum % 2 == 1 && pGameState->WhiteToMove == 0))
		{
			iScore *= -1;
		}

		return iScore;
	}

	// �V���b�g�̐���
	//=====================================================================================
	b2Vec2 CreateShot(float x, float y)			/* �V���b�g�̋�����?�C�͂��l�������l�ɒ��� */
	{
		b2Vec2 Shot;
		float len;

		Shot.Set(x - 2.375f, y - 41.28f);
		len = Shot.Length();
		Shot.Normalize();
		Shot.operator *=(sqrt(len*2.0f*STONEINFO_FRICTION));

		return Shot;
	}

	// �V���b�g�̒�?�ʒu�̍��W����V���b�g�𐶐�
	//-------------------------------------------------------
	int CreateShot(SHOTPOS Shot, SHOTVEC *lpResShot)
	{
		float tt = 0.0335f;
		b2Vec2 vec, TeePos;
		TeePos.Set(2.375f, 4.88f);

		if (Shot.angle == true) {
			vec = CreateShot(1.22f + Shot.x - tt * (Shot.y - TeePos.y), tt*(Shot.x - TeePos.x) + Shot.y);
		}
		else {
			vec = CreateShot(-1.22f + Shot.x + tt * (Shot.y - TeePos.y), -tt * (Shot.x - TeePos.x) + Shot.y);
		}

		lpResShot->x = vec.x;
		lpResShot->y = vec.y;
		lpResShot->angle = Shot.angle;

		return true;
	}

	// �V���b�g�̕����E�����E��?��������V���b�g�𐶐�
	//-------------------------------------------------------
	bool CreateShot(SHOTPOS Shot, float Power, SHOTVEC *lpResShot)
	{
		bool bRet = false;
		SHOTPOS tmpShot;
		float StY;

		if (Power >= 0.0f) {
			if (Power >= 0.0f && Power <= 1.0f) {
				StY = 11.28f + ((0.0f - Power) * 0.76f);
			}
			else if (Power > 1.0f && Power <= 3.0f) {
				StY = 10.52f + ((1.0f - Power) * 1.52f);
			}
			else if (Power > 3.0f && Power <= 4.0f) {
				StY = 7.47f + ((3.0f - Power) * 0.76f);
			}
			else if (Power > 4.0f && Power <= 10.0f) {
				StY = 6.71f + ((4.0f - Power) * 0.61f);
			}
			else if (Power > 10.0f) {
				if (Power > 50) {
					Power = 50;
				}
				StY = 3.05f + ((10.0f - Power) * 1.52f);
			}

			tmpShot.x = ((Shot.x - 2.375f) * (StY - 41.28f) / (Shot.y - 41.28f)) + 2.375f;
			tmpShot.y = StY;
			tmpShot.angle = Shot.angle;

			CreateShot(tmpShot, lpResShot);
			bRet = true;
		}
		else {
			CreateShot(Shot, lpResShot);
			bRet = true;
		}

		return bRet;
	}

	// b2Body �̔j��
	//=====================================================================================
	void DestroyBody(SUBGAMESTATE *pSubGameState, b2World *world)
	{
		for (int i = 0; i < pSubGameState->ShotNum; i++) {
			if (pSubGameState->body[i] != nullptr) {
				world->DestroyBody(pSubGameState->body[i]);
				pSubGameState->body[i] = nullptr;
			}
		}
	}

	// �Q??���̏�����
	//-------------------------------------------------------
	bool InitSUBGAMESTATE(SUBGAMESTATE *pSubGameState, GAMESTATE *pGameState, b2World *world)
	{
		memset(pSubGameState, 0x00, sizeof(SUBGAMESTATE));
		pSubGameState->ShotNum = pGameState->ShotNum;

		if (pGameState->ShotNum > 15) {
			return false;
		}

		for (int i = 0; i < pGameState->ShotNum; i++) {
			if (GetStoneState(pGameState->body[i][0], pGameState->body[i][1]) & STATE_PLAYAREA) {
				pSubGameState->body[i] = CreateBody(pGameState->body[i][0], pGameState->body[i][1], STONEINFO_SIZE, STONEINFO_SIZE, STONEINFO_ANGLE, STONEINFO_DYNAMIC, STONEINFO_ISBALL, STONEINFO_DENSITY, STONEINFO_RESTITUTION, STONEINFO_FRICTION, STONEINFO_CATEGORYBITS, STONEINFO_MASKBITS, world);
			}
		}

		return true;
	}

	// �V���b�g�̑傫����?�F�b�N�i�ő�l�𒴂��Ă���ꍇ�̓C��?�K���V���b�g�Ƃ��ăx�N�g����0.0000 0.0000 0�ɂ���j
	bool isLegalVec(SHOTVEC *vec)
	{
		bool bRet = true;

		if (vec->y < SHOTVEC_Y_MIN) {
			vec->x = 0.000f;
			vec->y = 0.000f;
			vec->angle = 0;
			bRet = false;
		}

		return bRet;
	}


	// �ʉ߈ʒu�E�����E��?��������q�b�g�V���b�g�𐶐�
	//-------------------------------------------------------
	int CreateHitShot(SHOTPOS Shot, float Power, SHOTVEC *lpResShot)
	{
		int iRet = 0;

		// SUBGAMESTATE�̏�����
		SUBGAMESTATE SubGameState;
		b2World world(gravity);
		memset(&SubGameState, 0x00, sizeof(SUBGAMESTATE));
		SubGameState.ShotNum = 0;

		// �V���b�g�̃Z�b�g
		SHOTVEC tShot;
		CreateShot(Shot, Power, &tShot);

		SetShot(tShot, &SubGameState, &world, 0.0f, 0.0f, lpResShot);

		b2Vec2 tar, s, t;
		tar.Set(Shot.x - 2.375f, 41.28f - Shot.y);
		Friction(STONEINFO_FRICTION*g_timeStep*0.5f, &SubGameState);
		for (int i = 0; ; i++) {

			// ?�C�͂̌v�Z
			world.Step(g_timeStep, velocityIterations, positionIterations);
			Friction(STONEINFO_FRICTION*g_timeStep, &SubGameState);

			t = s;
			s = SubGameState.body[SubGameState.ShotNum - 1]->GetPosition();
			s = b2Vec2(s.x - 2.375f, 41.28f - s.y);
			if (s.Length() > tar.Length()) {
				b2Vec2 u = s - t;
				float a = s.Length() - t.Length();
				float b = tar.Length() - t.Length();

				u *= b / a;
				t += u;

				t.Length();
				float n = atan2(tar.y, tar.x);
				float m = atan2(t.y, t.x);

				n = m - n;

				u = b2Vec2(tShot.x, tShot.y);
				lpResShot->x = cos(n)*u.x - sin(n)*u.y;
				lpResShot->y = sin(n)*u.x + cos(n)*u.y;
				lpResShot->angle = Shot.angle;

				iRet = 1;
				goto EndOfCreateHitShot;
			}

			// �S�ẴX�g?������?������ true ��Ԃ�
			for (int i = 0; i < SubGameState.ShotNum; i++) {
				if (SubGameState.body[i] != nullptr && SubGameState.body[i]->IsAwake() == true) {
					break;
				}

				if (i == SubGameState.ShotNum - 1) {
					goto EndOfCreateHitShot;
				}
			}
		}

	EndOfCreateHitShot:

		return iRet;
	}

	// �V?����?�V����?��
	//=====================================================================================
	int Simulation(GAMESTATE *pGameState, SHOTVEC Shot, float Rand = STONEINFO_SIZE, SHOTVEC *lpResShot = nullptr, int LoopCount = -1)
	{
		SUBGAMESTATE SubGameState;
		b2Vec2 vec;
		bool bFreeGuardRule = false;
		SHOTVEC WorkVec;

		isLegalVec(&Shot);

		// �Q??���̏�����
		b2World world(gravity);
		if (InitSUBGAMESTATE(&SubGameState, pGameState, &world) == false) {
			return false;
		}

		// ���� 0, �W?�΍����X�g?�����a�ŕ��z������
		if (Rand != 0.0f) {
			//std::normal_distribution<float> dist(0, Rand);
			// GAT�p��x�����̗����𔼕��ɕύX 2015/12/04
			//        y�����̗�����?�ɕύX   2015/12/08
			std::normal_distribution<float> distX(0, Rand / 2);
			std::normal_distribution<float> distY(0, Rand * 2);

			SHOTPOS TeePos, ShotPos;
			SHOTVEC TeeShot, ShotVec;

			TeePos.x = 2.375f;
			TeePos.y = 4.88f;
			TeePos.angle = Shot.angle;
			CreateShot(TeePos, &TeeShot);

			ShotPos.x = 2.375f + distX(dice);
			ShotPos.y = 4.88f + distY(dice);
			ShotPos.angle = Shot.angle;
			CreateShot(ShotPos, &ShotVec);

			// �V���b�g�̃Z�b�g
			if (SetShot(Shot, &SubGameState, &world, TeeShot.x - ShotVec.x, TeeShot.y - ShotVec.y, &WorkVec) == false) {
				return false;
			}
		}
		else {
			// �V���b�g�̃Z�b�g
			if (SetShot(Shot, &SubGameState, &world, 0.0f, 0.0f, &WorkVec) == false) {
				return false;
			}
		}

		// 
		if (lpResShot != nullptr) {
			memcpy(lpResShot, &WorkVec, sizeof(SHOTVEC));
		}

		// �������Z
		if (MainLoop(&SubGameState, &world, g_timeStep, nullptr, 0x00, LoopCount) == -1) {
			return false;
		}

		// �t��?�K?�h??���̓K�p
		if (SubGameState.ShotNum < 5) {
			for (int i = (pGameState->ShotNum + 1) % 2; i < pGameState->ShotNum; i += 2) {
				if (GetStoneState(pGameState->body[i][0], pGameState->body[i][1]) & STATE_FREEGUARD) {
					if (SubGameState.body[i] == nullptr) {
						bFreeGuardRule = true;
						break;
					}
					else {
						vec = SubGameState.body[i]->GetPosition();
						if (!(GetStoneState(vec.x, vec.y) & STATE_PLAYAREA)) {
							bFreeGuardRule = true;
							break;
						}
					}
				}
			}
		}

		// �V?����?�V�������ʂ�߂�
		if (bFreeGuardRule == true) {
			pGameState->body[pGameState->ShotNum][0] = 0.0f;
			pGameState->body[pGameState->ShotNum][1] = 0.0f;
		}
		else {
			for (int i = 0; i < 16; i++) {
				if (SubGameState.body[i] != nullptr) {
					vec = SubGameState.body[i]->GetPosition();
				}
				else {
					vec = b2Vec2(0.0f, 0.0f);
				}
				pGameState->body[i][0] = vec.x;
				pGameState->body[i][1] = vec.y;
			}
		}

		// �V���b�g���Ǝ�Ԃ̍X�V
		if (SubGameState.ShotNum == 16) {
			pGameState->Score[pGameState->CurEnd] = GetScore(pGameState);

			if (pGameState->Score[pGameState->CurEnd] > 0) {
				pGameState->WhiteToMove = 0;
			}
			else if (pGameState->Score[pGameState->CurEnd] < 0) {
				pGameState->WhiteToMove = 1;
			}
			else {
				pGameState->WhiteToMove ^= 1;
			}

			pGameState->ShotNum = 0;
			pGameState->CurEnd++;
		}
		else {
			pGameState->ShotNum = SubGameState.ShotNum;
			pGameState->WhiteToMove ^= 1;
			pGameState->Score[pGameState->CurEnd] = GetScore(pGameState);
		}

		// �I������
		DestroyBody(&SubGameState, &world);

		return true;
	}

	// �V?����?�V����?��(�g����)
	//=====================================================================================
	int SimulationEx(GAMESTATE *pGameState, SHOTVEC Shot, float RandX, float RandY, SHOTVEC *lpResShot, float *pLoci, int ResLociSize)
	{
		int iRet = 0;
		SUBGAMESTATE SubGameState;
		b2Vec2 vec;
		bool bFreeGuardRule = false;
		SHOTVEC WorkVec;

		isLegalVec(&Shot);

		// �Q??���̏�����
		b2World world(gravity);
		if (InitSUBGAMESTATE(&SubGameState, pGameState, &world) == false) {
			return false;
		}

		// ���� 0, �W?�΍����X�g?�����a�ŕ��z������
		SHOTPOS TeePos, ShotPos;
		SHOTVEC TeeShot, ShotVec;

		TeePos.x = ShotPos.x = 2.375f;
		TeePos.y = ShotPos.y = 4.88f;
		TeePos.angle = Shot.angle;
		CreateShot(TeePos, &TeeShot);

		// SimulationEx�֐��̗�����GAT�p�ɕύX(2016/1/8)
		if (RandX != 0.0f) {
			std::normal_distribution<float> distX(0, RandX / 2);
			ShotPos.x += distX(dice);
		}
		if (RandY != 0.0f) {
			std::normal_distribution<float> distY(0, RandY * 2);
			ShotPos.y += distY(dice);
		}
		ShotPos.angle = Shot.angle;
		CreateShot(ShotPos, &ShotVec);

		// �V���b�g�̃Z�b�g
		if (SetShot(Shot, &SubGameState, &world, TeeShot.x - ShotVec.x, TeeShot.y - ShotVec.y, &WorkVec) == false) {
			return false;
		}

		// 
		if (lpResShot != nullptr) {
			memcpy(lpResShot, &WorkVec, sizeof(SHOTVEC));
		}

		// �������Z
		if ((iRet = MainLoop(&SubGameState, &world, g_timeStep, pLoci, ResLociSize, -1)) == -1) {
			return false;
		}

		// �t��?�K?�h??���̓K�p
		if (SubGameState.ShotNum < 5) {
			for (int i = (pGameState->WhiteToMove ^ 1); i < pGameState->ShotNum; i += 2) {
				if (GetStoneState(pGameState->body[i][0], pGameState->body[i][1]) & STATE_FREEGUARD) {
					if (SubGameState.body[i] == nullptr) {
						bFreeGuardRule = true;
						break;
					}
					else {
						vec = SubGameState.body[i]->GetPosition();
						if (!(GetStoneState(vec.x, vec.y) & STATE_PLAYAREA)) {
							bFreeGuardRule = true;
							break;
						}
					}
				}
			}
		}

		// �V?����?�V�������ʂ�߂�
		if (bFreeGuardRule == true) {
			pGameState->body[pGameState->ShotNum][0] = 0.0f;
			pGameState->body[pGameState->ShotNum][1] = 0.0f;
		}
		else {
			for (int i = 0; i < 16; i++) {
				if (SubGameState.body[i] != nullptr) {
					vec = SubGameState.body[i]->GetPosition();
				}
				else {
					vec = b2Vec2(0.0f, 0.0f);
				}
				pGameState->body[i][0] = vec.x;
				pGameState->body[i][1] = vec.y;
			}
		}

		// �V���b�g���Ǝ�Ԃ̍X�V
		if (SubGameState.ShotNum == 16) {
			pGameState->Score[pGameState->CurEnd] = GetScore(pGameState);

			if (pGameState->Score[pGameState->CurEnd] > 0) {
				pGameState->WhiteToMove = 0;
			}
			else if (pGameState->Score[pGameState->CurEnd] < 0) {
				pGameState->WhiteToMove = 1;
			}
			else {
				pGameState->WhiteToMove ^= 1;
			}

			pGameState->ShotNum = 0;
			pGameState->CurEnd++;
		}
		else {
			pGameState->ShotNum = SubGameState.ShotNum;
			pGameState->WhiteToMove ^= 1;
			pGameState->Score[pGameState->CurEnd] = GetScore(pGameState);
		}

		// �I������
		DestroyBody(&SubGameState, &world);

		return iRet;
	}

} // namesapce curling_simulator