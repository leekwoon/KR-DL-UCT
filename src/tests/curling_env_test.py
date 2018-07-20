import time
from copy import copy
import numpy as np

from env.curling_env import CurlingEnv
from config import Config 
from utils import Utils 

def test_1():
    print("[Test] env")
    env = CurlingEnv()

    print("test pos_to_vec (TEE_X, TEE_Y, spin=0)")
    vec = CurlingEnv.pos_to_vec(Config.play_ground.TEE_X, Config.play_ground.TEE_Y, 0)
    print("vec=", vec)

    print("test pos_to_vec (TEE_X, TEE_Y, spin=1)")
    vec = CurlingEnv.pos_to_vec(Config.play_ground.TEE_X, Config.play_ground.TEE_Y, 1)
    print("vec=", vec)

    print("test vec_to_pos")
    pos = CurlingEnv.vec_to_pos(*vec)
    print("pos=", pos)

    print("do virtual fast simualtion 1000 times")
    st = time.time()
    for _ in range(1):
        virtual_env = copy(env)
        for _ in range(18):
            virtual_env.step(Config.play_ground.TEE_X, Config.play_ground.TEE_Y, 0)
    print("total execution time:", time.time() - st)
    print("average execution time", (time.time() - st)/1000)

    print(virtual_env.game_state)
    print(env.game_state)

if __name__=="__main__":
    test_1()
