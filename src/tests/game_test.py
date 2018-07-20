import numpy as np

from env.curling_env import CurlingEnv

from kr_dl_uct.player import Player 
from lib.dcl_utils import DclParser
from lib.pipe_generator import PipeGenerator
from network.pv_network import NetworkPV
from config import Config

def play_game(black, white):
    env = CurlingEnv()
    game_record = []
    turn_record = {}

    last_end = 8
    new_game_state = env.game_state
    new_game_state["LastEnd"] = last_end
    env.set_game_state(new_game_state)

    for end in range(0, last_end):
        for turn in range(0, 16):
            if env.game_state["WhiteToMove"]:
                best_xys = white.think(
                    root_env=env,
                    num_playout=1000
                )
            else:
                best_xys = black.think(
                    root_env=env,
                    num_playout=1000
                )

            print(end, turn, best_xys)
            turn_record = env.game_state
            if turn_record["ShotNum"] == 0:
                turn_record["body"] = np.zeros((16, 2))
            
            turn_record["BestShot"] = CurlingEnv.pos_to_vec(*best_xys)    
            run_vec = env.step(best_xys[0], best_xys[1], best_xys[2])
            turn_record["RunShot"] = run_vec

            game_record.append(turn_record)
            turn_record = {}
        # add end reuslt
        turn_record['CurEnd'] = game_record[-1]['CurEnd']
        turn_record['body'] = game_record[-1]['body']
        turn_record['Score'] = env.game_state['Score']
        game_record.append(turn_record)
        turn_record = {}
    return game_record 

if __name__=="__main__":
    network = NetworkPV("test", "gpu:0")
    network.saver.restore(network.sess, './model/checkpoints/ex/checkpoint-760000')
    pipe_generator = PipeGenerator(network)
    pipe = pipe_generator.get_pipe()

    simple_player = Player(
        pipe=pipe,
        ucb_const=0.03,
        pw_const=0.1,
        init_temperature=1.0,
        out_temperature=0.0,
        num_init=20,
        num_sample=3,
        l=1.0,
        max_depth=2,
        verbose=False
    )

    game_record = play_game(simple_player, simple_player)
    names = ["simple_player", "simple_player"]
    dcl_file_name = DclParser.game_record_to_dcl(Config.resource.DATA_DIR, names, game_record)

