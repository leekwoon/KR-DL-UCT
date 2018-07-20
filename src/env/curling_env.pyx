from libc.stdlib cimport malloc
from libcpp cimport bool as bool_t

include "utils.pyx"

from config import Config

cdef extern from "CurlingSimulator.h" namespace "curling_simulator":
    ctypedef struct GAMESTATE:
        int ShotNum
        int CurEnd
        int LastEnd
        int Score[10]
        bool_t WhiteToMove # white is the team who shot hammer-stone at the first end. 
        float body[16][2]

    ctypedef struct SHOTPOS:
        float x
        float y
        bool_t angle
    
    ctypedef struct SHOTVEC:
        float x
        float y
        bool_t angle
    
    int Simulation(GAMESTATE *pGameState, SHOTVEC, float, SHOTVEC *lpResShot, int) nogil except +
    int SimulationEx(GAMESTATE *pGameState, SHOTVEC, float, float, SHOTVEC *lpResShot, float *pLoci, int) nogil except +
    int GetScore(GAMESTATE *pGameState) nogil except +

cdef class _CurlingEnv(object):
    cdef GAMESTATE game_state

    def __init__(self):
        self.reset()

    def reset(self):
        self.game_state.ShotNum = 0
        self.game_state.CurEnd = 0
        self.game_state.LastEnd = 8 # assume 8-end game
        self.game_state.Score = [0, 0, 0, 0, 0, 0, 0, 0, 0 ,0]
        self.game_state.WhiteToMove = False
        self.game_state.body = [[0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0],
                                [0, 0]]

    # return noise added shot vector
    def simulation(self, SHOTVEC shot_vec, float rand):
        cdef SHOTVEC res_shot_vec
        with nogil:
            Simulation(&self.game_state, shot_vec, rand, &res_shot_vec, -1)
        return [res_shot_vec.x, res_shot_vec.y, int(res_shot_vec.angle)]

    def simulationEx(self, SHOTVEC shot_vec, float rand):
        """
            dynamic allocation
            see: http://cython.readthedocs.io/en/latest/src/tutorial/memory_allocation.html
        """
        cdef SHOTVEC res_shot_vec
        cdef float *trj = <float *>malloc(10000 * 16 * 12 * sizeof(float))
        cdef int ResLociSize = 1000000
        SimulationEx(&self.game_state, shot_vec, rand*0.5, rand*2, &res_shot_vec, trj, ResLociSize)

        trajectories = []
        for i in range(10000):
            body = []
            for s in range(16):
                body.append([trj[32*i + s*2], trj[32*i + s*2 + 1]])
            if not trajectories:
                trajectories.append(body)
            else:
                change = False
                for s in range(16):
                    if abs(body[s][0] - trajectories[-1][s][0]) > 0.0001:
                        change = True
                    if abs(body[s][1] - trajectories[-1][s][1]) > 0.0001:
                        change = True
                if change:
                    trajectories.append(body)
                else:
                    break
        return [res_shot_vec.x, res_shot_vec.y, int(res_shot_vec.angle)], trajectories

    property game_state:
        def __get__(self):
            return self.game_state
        def __set__(self, game_state):
            self.game_state = game_state

class CurlingEnv(object):
    def __init__(self):
        self.simulator = _CurlingEnv() 

    def __copy__(self):
        res = CurlingEnv()
        res.simulator.game_state = self.simulator.game_state
        return res

    @property
    def game_state(self):
        return self.simulator.game_state

    def render(self):
        pass

    def reset(self):
        self.simulator.reset()

    def step(self, x, y, spin):
        vec = pos_to_vec(x, y, spin)
        shot_vec = {"x": vec[0], "y": vec[1], "angle": vec[2]}
        noise_added_vec = self.simulator.simulation(shot_vec, rand=Config.play_ground.RAND)
        return noise_added_vec

    def step_without_rand(self, x, y, spin):
        vec = pos_to_vec(x, y, spin)
        shot_vec = {"x": vec[0], "y": vec[1], "angle": vec[2]}
        noise_added_vec = self.simulator.simulation(shot_vec, rand=0)
        return noise_added_vec

    def step_and_get_trajectories(self, x, y, spin):
        vec = pos_to_vec(x, y, spin)
        shot_vec = {"x": vec[0], "y": vec[1], "angle": vec[2]}
        noise_added_vec, trajectories = self.simulator.simulationEx(shot_vec, rand=Config.play_ground.RAND)
        return noise_added_vec, trajectories

    def step_without_rand_and_get_trajectories(self, x, y, spin):
        vec = pos_to_vec(x, y, spin)
        shot_vec = {"x": vec[0], "y": vec[1], "angle": vec[2]}
        noise_added_vec, trajectories = self.simulator.simulationEx(shot_vec, rand=0)
        return noise_added_vec, trajectories

    def set_game_state(self, new_game_state):
        self.simulator.game_state = new_game_state

    @staticmethod
    def pos_to_vec(x, y, spin):
        return pos_to_vec(x, y, spin)

    @staticmethod
    def vec_to_pos(vx, vy, spin):
        return vec_to_pos(vx, vy, spin)


