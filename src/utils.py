import numpy as np

from config import Config 

STEP_H_TO_X = {
    "PlayGround" :Config.play_ground.PLAYAREA_HEIGHT / (Config.network.INPUT_IMAGE_HEIGHT),
    "StrongPolicy" : Config.policy.STRONG_HEIGHT / (Config.network.OUTPUT_IMAGE_HEIGHT),
    "WeakPolicy" : Config.policy.WEAK_HEIGHT / (Config.network.OUTPUT_IMAGE_HEIGHT),
    }

STEP_W_TO_Y = {
    "PlayGround": Config.play_ground.PLAYAREA_WIDTH / (Config.network.INPUT_IMAGE_WIDTH),
    "Policy": Config.policy.WIDTH / (Config.network.OUTPUT_IMAGE_WIDTH),
    }

class ValueUtils(object): 
    @staticmethod
    def score_to_idx(score):
        assert(-8 <= score and score <= 8)
        return int(score + 8)

    @staticmethod
    def idx_to_score(idx):
        assert(0 <= idx and idx <= Config.value.N_SCORE-1)
        return idx - 8

    @staticmethod
    def dist_v_to_exp_v(dist_v): # histotram of v to expectation value
        score_array = np.zeros(Config.value.N_SCORE)
        for i in range(Config.value.N_SCORE):
            score_array[i] = ValueUtils.idx_to_score(i)
        exp_v = np.sum(score_array * dist_v)
        return exp_v

    @staticmethod
    def flip_dist_v(dist_v):
        flipped_dist_v = np.zeros(Config.value.N_SCORE)
        for i in range(8):
            flipped_dist_v[i] = dist_v[Config.value.N_SCORE - 1 - i]
            flipped_dist_v[Config.value.N_SCORE - 1 - i] = dist_v[i]
        return flipped_dist_v

class PlayGroundUtils(object):    
    @staticmethod
    def hw_to_xy(h, w):
        assert(0 <= h and h <= Config.network.INPUT_IMAGE_HEIGHT - 1)
        assert(0 <= w and w <= Config.network.INPUT_IMAGE_WIDTH - 1)
        x = Config.play_ground.X_PLAYAREA_MIN + h * STEP_H_TO_X["PlayGround"] + 0.5 * STEP_H_TO_X["PlayGround"]
        y = Config.play_ground.Y_PLAYAREA_MIN + w * STEP_W_TO_Y["PlayGround"] + 0.5 * STEP_W_TO_Y["PlayGround"]
        return x, y

    @staticmethod
    def xy_to_hw(x, y):
        assert(Config.play_ground.X_PLAYAREA_MIN <= x and x <= Config.play_ground.X_PLAYAREA_MAX)
        assert(Config.play_ground.Y_PLAYAREA_MIN - Config.play_ground.STONE_RADIUS <= y and y <= Config.play_ground.Y_PLAYAREA_MAX)
        if x == Config.play_ground.X_PLAYAREA_MAX:
            h = Config.network.INPUT_IMAGE_HEIGHT - 1
        else:
            h = int((x - Config.play_ground.X_PLAYAREA_MIN) / STEP_H_TO_X["PlayGround"]) 
        if y == Config.play_ground.Y_PLAYAREA_MAX:
            w = Config.network.INPUT_IMAGE_WIDTH - 1
        else:
            w = int((y - Config.play_ground.Y_PLAYAREA_MIN) / STEP_W_TO_Y["PlayGround"])
        return h, w

    @staticmethod
    def is_in_playarea(x, y):
        return (Config.play_ground.X_PLAYAREA_MIN + Config.play_ground.STONE_RADIUS < x) and (Config.play_ground.X_PLAYAREA_MAX - Config.play_ground.STONE_RADIUS > x) \
            and (Config.play_ground.Y_PLAYAREA_MIN - Config.play_ground.STONE_RADIUS < y) and (Config.play_ground.Y_PLAYAREA_MAX - Config.play_ground.STONE_RADIUS > y)

    @staticmethod
    def is_in_house(x, y):
        rx = x - Config.play_ground.TEE_X
        ry = y - Config.play_ground.TEE_Y
        R = np.sqrt(rx**2 + ry**2) 
        return R < Config.play_ground.HOUSE_RADIUS + Config.play_ground.STONE_RADIUS

    @staticmethod
    def is_in_red_circle(x, y):
        rx = x - Config.play_ground.TEE_X
        ry = y - Config.play_ground.TEE_Y
        R = np.sqrt(rx**2 + ry**2) 
        return R < 0.333 * Config.play_ground.HOUSE_RADIUS + Config.play_ground.STONE_RADIUS

    @staticmethod
    def is_in_guardzone(x, y):
        if not PlayGroundUtils.is_in_playarea(x, y):
            return False
        if PlayGroundUtils.is_in_house(x, y):
            return False
        if y < Config.play_ground.TEE_Y + Config.play_ground.STONE_RADIUS:
            return False
        if x < Config.play_ground.TEE_X - 0.5 * Config.play_ground.HOUSE_RADIUS - Config.play_ground.STONE_RADIUS \
            or x > Config.play_ground.TEE_X + 0.5 * Config.play_ground.HOUSE_RADIUS + Config.play_ground.STONE_RADIUS:
            return False
        return True


class PolicyUtils(object):
    @staticmethod
    def hw_to_xy(h, w):
        assert(0 <= h and h <= Config.network.OUTPUT_IMAGE_HEIGHT - 1)
        assert(0 <= w and w <= Config.network.OUTPUT_IMAGE_WIDTH - 1)
        y = Config.policy.STRONG_Y_MIN + w * STEP_W_TO_Y["Policy"] + 0.5 * STEP_W_TO_Y["Policy"]
        if y > Config.policy.STRONG_Y_MAX: 
            y += Config.policy.WEAK_Y_MIN - Config.policy.STRONG_Y_MAX

        if y > Config.policy.STRONG_Y_MAX: # weak shot
            x = Config.policy.WEAK_X_MIN + h * STEP_H_TO_X["WeakPolicy"] + 0.5 * STEP_H_TO_X["WeakPolicy"]
        else:
            x = Config.policy.STRONG_X_MIN + h * STEP_H_TO_X["StrongPolicy"] + 0.5 * STEP_H_TO_X["StrongPolicy"]
        return x, y

    @staticmethod
    def xy_to_hw(x, y):
        if y > Config.policy.STRONG_Y_MAX: # weak shot
            assert(Config.policy.WEAK_X_MIN <= x and x <= Config.policy.WEAK_X_MAX)
            assert(Config.policy.WEAK_Y_MIN <= y and y <= Config.policy.WEAK_Y_MAX)
            if x == Config.policy.WEAK_X_MAX:
                h = Config.network.OUTPUT_IMAGE_HEIGHT - 1
            else:
                h = int((x - Config.policy.WEAK_X_MIN) / STEP_H_TO_X["WeakPolicy"])
            if y == Config.policy.WEAK_Y_MAX:
                w = Config.network.OUTPUT_IMAGE_WIDTH - 1
            else:
                w = int(((y-(Config.policy.WEAK_Y_MIN - Config.policy.STRONG_Y_MAX)) 
                    - Config.policy.STRONG_Y_MIN) / STEP_W_TO_Y["Policy"])
        else: # strong shot
            assert(Config.policy.STRONG_X_MIN <= x and x <= Config.policy.STRONG_X_MAX)
            assert(Config.policy.STRONG_Y_MIN <= y and y <= Config.policy.STRONG_Y_MAX)
            if x == Config.policy.STRONG_X_MAX:
                h = Config.network.OUTPUT_IMAGE_HEIGHT - 1
            else:
                h = int((x - Config.policy.STRONG_X_MIN) / STEP_H_TO_X["StrongPolicy"])

            w = int((y - Config.policy.STRONG_Y_MIN) / STEP_W_TO_Y["Policy"])
        return h, w

    # We assume "NHWC" shape of output at the end of model.
    @staticmethod
    def action_id_to_xys(idx):
        h, w, spin = PolicyUtils.action_id_to_hws(idx)
        x, y = PolicyUtils.hw_to_xy(h, w)
        return x, y, spin

    @staticmethod
    def xys_to_action_id(x, y, spin):
        h, w = PolicyUtils.xy_to_hw(x, y)
        return PolicyUtils.hws_to_action_id(h, w, spin)

    @staticmethod
    def action_id_to_hws(idx):
        spin = idx % 2
        idx = idx/2
        h, w = int((idx)/Config.network.OUTPUT_IMAGE_WIDTH), int(idx % Config.network.OUTPUT_IMAGE_WIDTH)
        return h, w, spin

    @staticmethod
    def hws_to_action_id(h, w, spin):
        idx = 2 * (Config.network.OUTPUT_IMAGE_WIDTH * h + w) + spin
        return int(idx)  

    @staticmethod
    def is_in_policyarea(x, y, s):
        assert(s == 0 or s == 1)
        if ((Config.policy.WEAK_X_MIN <= x \
            and x <= Config.policy.WEAK_X_MAX) \
            and (Config.policy.WEAK_Y_MIN <= y \
            and y <= Config.policy.WEAK_Y_MAX)) \
            or \
            ((Config.policy.STRONG_X_MIN <= x \
            and x <= Config.policy.STRONG_X_MAX) \
            and (Config.policy.STRONG_Y_MIN <= y \
            and y <= Config.policy.STRONG_Y_MAX)):
            return True
        else:
            return False

class Utils(object):
    value = ValueUtils
    policy = PolicyUtils
    play_ground = PlayGroundUtils