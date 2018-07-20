import os
import numpy as np
import datetime
from copy import deepcopy

from env.curling_env import CurlingEnv

from config import Config 
from utils import Utils

class DclParser:
    @staticmethod
    def is_dcl(file_name):
        return file_name.strip()[-4:] == ".dcl"

    @staticmethod
    def walk_all_dcls(root_dir):
        """a helper function/generator to get all dcl files in subdirectories of root
        """
        dcls_path = []
        for (dir_path, dir_name, files) in os.walk(root_dir):
                for file_name in files:
                    if DclParser.is_dcl(file_name):
                        # return the full (relative) path to the file
                        dcls_path.append(os.path.join(dir_path, file_name))
        return dcls_path

    @staticmethod
    def read_dcl(dcl_path, read_only=[], read_valid_record=True):
        # Note that Score always recorded in terms of black team.
        game_record = []

        Score = [0, 0, 0, 0, 0, 0 ,0 ,0, 0, 0]
        turn_record = {}

        names = ["", ""]

        for line in open(dcl_path, 'r'):
            data = line.split()
            if 'First=' in data[0]:
                names[0] = data[0][6:]
            elif 'Second=' in data[0]:
                names[1] = data[0][7:]
                if read_only:
                    if not names[0] in read_only and not names[1] in read_only:
                        # only read dcl of read_only 
                        return []
            elif 'POSITION' in data[0]:
                stones = np.zeros((16, 2))
                for i in range(16):
                    stones[i][0] = float(data[1 + i * 2])
                    stones[i][1] = float(data[1 + i * 2 + 1])
                turn_record['body'] = stones
            elif 'SETSTATE' in data[0]:
                turn_record['ShotNum'] = int(data[1])
                turn_record['CurEnd'] = int(data[2])
                turn_record['LastEnd'] = int(data[3])
                turn_record['WhiteToMove'] = int(data[4])
            elif 'BESTSHOT' in data[0]:
                turn_record['BestPos'] = CurlingEnv.vec_to_pos(float(data[1]), float(data[2]), int(data[3]))
            elif 'RUNSHOT' in data[0]:
                # add turn_record to game_record and make empty turn_record
                turn_record['Score'] = deepcopy(Score)

                if read_valid_record:
                    # keep only the shots in the range we use (see PolicyConfig in Config.py)
                    if Utils.policy.is_in_policyarea(*turn_record["BestPos"]):
                        # ignore the state which shot stone meaninglessly (e.g. just throw away stone outside of playground)
                        if len(game_record) > 0 and turn_record['ShotNum'] > 0 and np.all(turn_record['body'] == game_record[-1]['body']):
                            # if new state is same as before ...
                            # ignore last record
                            game_record[-1] = turn_record
                        else:
                            game_record.append(turn_record)
                else:
                    game_record.append(turn_record)
                turn_record = {}
            elif 'TOTALSCORE' in data[0]:
                # Check the final score and give the result to each turn_records...
                if sum(Score) == 0: # draw
                    for g in game_record:
                        g.update({'GameResult': 0})
                elif sum(Score) > 0: # Black team win
                    for g in game_record:
                        if g['WhiteToMove'] == 0:
                            g.update({'GameResult': 1})
                        else:
                            g.update({'GameResult': -1})
                elif sum(Score) < 0: # White team win
                    for g in game_record:
                        if g['WhiteToMove'] == 1:
                            g.update({'GameResult': 1})
                        else:
                            g.update({'GameResult': -1})
                return game_record
            elif 'SCORE=' in data[0]:
                Score[game_record[-1]['CurEnd']] = int(data[1])
                for g in game_record:
                    if g['WhiteToMove'] == 0 and g['CurEnd'] == game_record[-1]['CurEnd']:
                        g.update({'EndScore': Score[game_record[-1]['CurEnd']]})
                    elif g['WhiteToMove'] == 1 and g['CurEnd'] == game_record[-1]['CurEnd']:
                        g.update({'EndScore': -Score[game_record[-1]['CurEnd']]})
        return []

    @staticmethod
    def game_record_to_dcl(dir_path, names, game_record, random=0.145, limit=9999000):    
        current_time = str(datetime.datetime.now())
        current_time = current_time.replace(':', '').replace('.', '')
        file_name = '%s - %s [%s].dcl' % (names[0], names[1], current_time)

        f = open(os.path.join(dir_path, file_name), "w")
        f.write("[GameInfo]\n")
        f.write("First=%s\n"% names[0])
        f.write("FirstRemTime=%d\n"% limit)
        f.write("Second=%s\n"% names[1])
        f.write("SecondRemTime=%d\n"% limit)
        f.write("Random=%.6f"% random)
        
        for i, g in enumerate(game_record):
            if 'ShotNum' in g.keys():
                f.write("\n[%d%d%d%d]"% (g['CurEnd']/10, g['CurEnd']%10, g['ShotNum']/10, g['ShotNum']%10))
                f.write("\nPOSITION=POSITION")
                for pos in g["body"]:
                    if g['ShotNum'] == 0:
                        f.write(" %.6f %.6f"%(0, 0))
                    else:
                        f.write(" %.6f %.6f"%(pos[0], pos[1]))
                f.write("\nSETSTATE=SETSTATE")
                f.write(" %d %d %d %d"% (g['ShotNum'], g['CurEnd'],
                                         g['LastEnd'], g['WhiteToMove']))
                f.write("\nBESTSHOT=BESTSHOT")
                f.write(" %.6f %.6f %d"% (g['BestShot'][0], g['BestShot'][1], g['BestShot'][2]))
                f.write("\nRUNSHOT=RUNSHOT")
                f.write(" %.6f %.6f %d"% (g['RunShot'][0], g['RunShot'][1], g['RunShot'][2]))
            else: # end reuslt
                f.write("\n[%d%d16]"% (g['CurEnd']/10, g['CurEnd']%10))
                f.write("\nPOSITION=POSITION")
                for pos in g["body"]:
                    f.write(" %.6f %.6f"%(pos[0], pos[1]))
                f.write("\nSCORE=SCORE %d"% g['Score'][g['CurEnd']])
                
        if i == len(game_record) - 1:
            score= np.array(g['Score'])
            black_team_score = np.sum(score[score>0])
            white_team_score = -np.sum(score[score<0])
            f.write("\nTOTALSCORE=TOTALSCORE")
            f.write(" %d %d" % (black_team_score, white_team_score))
            f.write("\n[GameOverInfo]")
            f.write("\nENDSTATE=NORMAL")
            if black_team_score > white_team_score:
                f.write("\nWIN=%s" % names[0])
                f.write("\nLOSE=%s" % names[1])
            elif black_team_score < white_team_score:
                f.write("\nWIN=%s" % names[1])
                f.write("\nLOSE=%s" % names[0])
            else:
                f.write("\nWIN=%s" % "DRAW")
                f.write("\nLOSE=%s" % "DRAW")
            f.write("\nTOTALSCORE=TOTALSCORE")
            f.write(" %d %d" % (black_team_score, white_team_score))
            f.write("\n\n")
        f.close()
        return file_name

    @staticmethod
    def winner(dcl_path, read_only=[], read_valid_record=True):
        for line in open(dcl_path, 'r'):
            data = line.split()
            if 'WIN=' in data[0]:
                return data[0][4:]


