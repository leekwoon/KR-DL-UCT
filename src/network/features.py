import numpy as np

from config import Config
from utils import Utils

def planes(num_planes):
    def deco(f):
        f.planes = num_planes
        return f
    return deco

@planes(3)
def board(game_state):
    planes = np.zeros((Config.network.INPUT_IMAGE_HEIGHT, Config.network.INPUT_IMAGE_WIDTH, 3), dtype=np.float32)
    planes[:, :, 2] = 1.
    for i in range(game_state["ShotNum"] - 2, -1, -2): # my stones
        x, y = game_state["body"][i]
        if Utils.play_ground.is_in_playarea(x, y):
            h, w = Utils.play_ground.xy_to_hw(x, y)
            planes[h][w][0] = 1
            planes[h][w][2] = 0
    for i in range(game_state["ShotNum"] - 1, -1, -2): # opp stones
        x, y = game_state["body"][i]
        if Utils.play_ground.is_in_playarea(x, y):
            h, w = Utils.play_ground.xy_to_hw(x, y)
            planes[h][w][1] = 1
            planes[h][w][2] = 0
    return planes

@planes(1)
def ones(game_state):
    return np.ones((Config.network.INPUT_IMAGE_HEIGHT, Config.network.INPUT_IMAGE_WIDTH, 1))

@planes(8)
def turn_num(game_state):
    # First Team
    #     0: 1st, 2nd shot # freequard zone rule applied
    #     1: 3rd, 4th, 5th, 6th shot
    #     2: 7th shot
    #     3: 8th shot
    # Second Team (who shot hammer shot)
    #     5: 1st shot # freequard zone rule applied
    #     6: 2nd, 3rd, 4th, 5th, 6th shot
    #     7: 7th shot
    #     8: 8th shot
    # E.g.
    #     First player turn : 0 2 / 4 6 8 10 / 12 / 14
    #     Last player turn  : 1 3 / 5 7 9 11 / 13 / 15
    
    planes = np.zeros((Config.network.INPUT_IMAGE_HEIGHT, Config.network.INPUT_IMAGE_WIDTH, 8), dtype=np.float32)
    # first team
    if game_state["ShotNum"] % 2 == 0:
        shot_num = int(game_state["ShotNum"] / 2)
        if shot_num == 0 or shot_num == 1: # free guard zone rule applied
            planes[:, :, 0] = 1
        elif shot_num == 2 or shot_num == 3 or shot_num == 4 or shot_num == 5:
            planes[:, :, 1] = 1
        elif shot_num == 6:
            planes[:, :, 2] = 1
        elif shot_num == 7:
            planes[:, :, 3] = 1
    # for the last team
    elif game_state["ShotNum"] % 2 == 1:
        shot_num = int(game_state["ShotNum"] / 2)
        if shot_num == 0: # free guard zone rule applied
            planes[:, :, 4] = 1
        elif shot_num == 1 or shot_num == 2 or shot_num == 3 or shot_num == 4 or shot_num == 5:
            planes[:, :, 5] = 1
        elif shot_num == 6:
            planes[:, :, 6] = 1
        elif shot_num == 7:
            planes[:, :, 7] = 1
    return planes

@planes(1)
def in_house(game_state):
    planes = np.zeros((Config.network.INPUT_IMAGE_HEIGHT, Config.network.INPUT_IMAGE_WIDTH, 1), dtype=np.float32)
    for i in range(0, game_state["ShotNum"]):
        x, y = game_state["body"][i]
        if Utils.play_ground.is_in_house(x, y):
            h, w = Utils.play_ground.xy_to_hw(x, y)
            planes[h][w][0] = 1
    return planes

@planes(4)
def our_order_to_tee(game_state):
    planes = np.zeros((Config.network.INPUT_IMAGE_HEIGHT, Config.network.INPUT_IMAGE_WIDTH, 4), dtype=np.float32)

    our_stones_in_playground = []
    for i in range(game_state["ShotNum"] - 2, -1, -2): # my stones
        s = game_state["body"][i]
        if Utils.play_ground.is_in_playarea(*s):
            our_stones_in_playground.append(s)

    our_sorted_stones = sorted(our_stones_in_playground, 
       key = lambda s: (s[0] - Config.play_ground.TEE_X)**2 + (s[1] - Config.play_ground.TEE_Y)**2)

    for i, stone in enumerate(our_sorted_stones):
        h, w = Utils.play_ground.xy_to_hw(*stone)
        if i < 4:
            planes[h][w][i] = 1
        else:
            planes[h][w][3] = 1
    return planes

@planes(4)
def opp_order_to_tee(game_state):
    planes = np.zeros((Config.network.INPUT_IMAGE_HEIGHT, Config.network.INPUT_IMAGE_WIDTH, 4), dtype=np.float32)

    opp_stones_in_playground = []
    for i in range(game_state["ShotNum"] - 1, -1, -2): # opp stones
        s = game_state["body"][i]
        if Utils.play_ground.is_in_playarea(*s):
            opp_stones_in_playground.append(s)

    opp_sorted_stones = sorted(opp_stones_in_playground, 
       key = lambda s: (s[0] - Config.play_ground.TEE_X)**2 + (s[1] - Config.play_ground.TEE_Y)**2)

    for i, stone in enumerate(opp_sorted_stones):
        h, w = Utils.play_ground.xy_to_hw(*stone)
        if i < 4:
            planes[h][w][i] = 1
        else:
            planes[h][w][3] = 1
    return planes

@planes(8)
def all_order_to_tee(game_state):
    planes = np.zeros((Config.network.INPUT_IMAGE_HEIGHT, Config.network.INPUT_IMAGE_WIDTH, 8), dtype=np.float32)

    stones_in_playground = [s for s in game_state["body"][:game_state["ShotNum"]]
                    if Utils.play_ground.is_in_playarea(*s)]            
    sorted_stones = sorted(stones_in_playground, 
       key = lambda s: (s[0] - Config.play_ground.TEE_X)**2 + (s[1] - Config.play_ground.TEE_Y)**2)

    for i, stone in enumerate(sorted_stones):
        h, w = Utils.play_ground.xy_to_hw(*stone)
        if i < 8:
            planes[h][w][i] = 1
        else:
            planes[h][w][7] = 1
    return planes

DEFAULT_FEATURES = [
    board, # 3
    ones, # 1
    turn_num, # 8   
    in_house, # 1
    our_order_to_tee, # 4
    opp_order_to_tee, # 4
    all_order_to_tee # 8
]

def extract_planes(game_state, features=DEFAULT_FEATURES):
    return np.concatenate([feature(game_state) for feature in features], axis=2)

