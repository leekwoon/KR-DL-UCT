from libcpp.memory cimport unique_ptr
from libcpp.vector cimport vector
from libcpp.pair cimport pair
from libcpp cimport bool as bool_t

import time
from copy import copy
import numpy as np

from network.features import extract_planes

from config import Config
from utils import Utils

cdef extern from "node.h":
    cdef cppclass Node:
        Node(double x, double y, double spin, double prob) 
        void sort_children(bool_t is_white) 
        Node* ucb_select(bool_t is_white, double ucb_const)
        Node* add_node(double x, double y, double spin, double prob) 
        void add_init_info(int id, double prob)
        bool_t is_root()
        bool_t has_children()
        double* sample_move(int num_sample, double l)
        const vector[unique_ptr[Node]]& get_children() 
        int get_num_children()
        Node* get_parent()
        double* get_move()
        pair[int, double] get_init_info(int nth);
        double get_visits()
        double get_prob()
        double get_eval(bool_t is_white)
        void kr_update(double v) 

cdef class _KrNode:
    cdef Node* obj 
    cdef bool_t alloc

    def __dealloc__(self):
        if self.alloc:
            del self.obj

    @staticmethod 
    cdef create(double x, double y, double spin, double prob): # create new node 
        cdef _KrNode ret = _KrNode() 
        ret.obj = new Node(x, y, spin, prob) 
        ret.alloc = True
        return ret 

    @staticmethod 
    cdef wrap(Node* node): # convert Node* to _KrNode
        cdef _KrNode ret = _KrNode() 
        ret.obj = node 
        ret.alloc = False
        return ret

    def is_root(self):
        return self.obj.is_root()

    def sort_children(self, bool_t is_white):
        self.obj.sort_children(is_white)

    def ucb_select(self, bool_t is_white, double ucb_const):
        return _KrNode.wrap(self.obj.ucb_select(is_white, ucb_const))

    def add_node(self, double x, double y, double spin, double prob):
        return _KrNode.wrap(self.obj.add_node(x, y, spin, prob))

    def add_init_info(self, int id, double prob):
        self.obj.add_init_info(id, prob)

    def has_children(self):
        return self.obj.has_children()

    def sample_move(self, int num_sample, double l):
        cdef double sample_move[3]
        sample_move = self.obj.sample_move(num_sample, l)
        return sample_move

    def get_children(self, int nth):
        return _KrNode.wrap(self.obj.get_children()[nth].get())

    def get_num_children(self):
        return self.obj.get_num_children()

    def get_init_info(self, int nth):
        return self.obj.get_init_info(nth)

    def get_parent(self):
        return _KrNode.wrap(self.obj.get_parent())

    def get_move(self):
        cdef double move[3]
        move = self.obj.get_move()
        return move

    def get_visits(self):
        return self.obj.get_visits()

    def get_prob(self):
        return self.obj.get_prob()

    def get_eval(self, bool_t is_white):
        return self.obj.get_eval(is_white)

    def kr_update(self, double v):
        self.obj.kr_update(v)

class KrNode(object):
    def __init__(self, _kr_node=None):
        if _kr_node is None:
            self.node = _KrNode.create(0.0, 0.0, 0.0, 0.0)
        else:
            self.node = _kr_node

    def is_root(self):
        return self.node.is_root()

    def sort_children(self, is_white):
        self.node.sort_children(is_white)

    def ucb_select(self, is_white, ucb_const):
        return KrNode(self.node.ucb_select(is_white, ucb_const))

    def add_node(self, x, y, spin, prob):
        return KrNode(self.node.add_node(x, y, spin, prob))

    def add_init_info(self, id, prob):
        self.node.add_init_info(id, prob)

    def has_children(self):
        return self.node.has_children()

    def sample_action(self, num_sample, l):
        return self.node.sample_move(num_sample, l)

    @property
    def num_children(self):
        return self.node.get_num_children()

    @property 
    def action(self):
        return self.node.get_move()

    @property
    def kn(self): # visit counts estimated by kernel density estimation 
        return self.node.get_visits()

    @property 
    def prob(self):
        return self.node.get_prob()

    @property 
    def children(self):
        children = [KrNode(self.node.get_children(i)) for i in range(self.num_children)]
        return children 

    @property
    def parent(self):
        return KrNode(self.node.get_parent())

    def get_init_info(self, nth):
        return self.node.get_init_info(nth)

    def get_keval(self, is_white): # eval estimated by kernel regression 
        return self.node.get_eval(is_white)

    def kr_update(self, v):
        self.node.kr_update(v)

class Player(object):
    def __init__(self, pipe, ucb_const, pw_const, init_temperature, out_temperature,
                 num_init, num_sample, l, max_depth, verbose):
        self.root_node = KrNode()
        self.root_env = None
        self.env_dict = None
        self.num_node = None
        self.num_playout = None 

        self.pipe = pipe
        self.ucb_const = ucb_const
        self.pw_const = pw_const 
        self.init_temperature = init_temperature
        self.out_temperature = out_temperature
        self.num_init = num_init 
        self.num_sample = num_sample 
        self.l = l # control the area of sample space
        self.max_depth = max_depth
        self.verbose = verbose

    def reset(self, root_env, num_playout):
        self.root_node = KrNode()
        self.root_env = root_env
        self.env_dict = {"": copy(self.root_env)}
        self.num_node = 0
        self.num_playout = num_playout

    def think(self, root_env, num_playout):
        st = time.time()
        self.reset(root_env, num_playout)
        root_prediction_p, root_leaf_black_eval = self.predict(root_env)

        self.prepare_init_actions(self.root_node, root_prediction_p)
        self.root_node.kr_update(root_leaf_black_eval)

        if self.verbose:
            if root_env.game_state["WhiteToMove"]:
                print("NN eval={}".format(-1 * root_leaf_black_eval))
            else:
                print("NN eval={}".format(root_leaf_black_eval))

        for _ in range(num_playout):
            cur_env = copy(root_env)
            self.play_simulation(0, cur_env, self.root_node)

        thinking_time = time.time() - st

        if self.verbose:
            self.print_stats(thinking_time, self.root_env, self.root_node)

        return self.sample_best_action()

    def play_simulation(self, cur_depth, cur_env, cur_node):
        is_expanded = False
        if self.root_env.game_state["CurEnd"] != cur_env.game_state["CurEnd"] or cur_depth == self.max_depth:
            return None, False
        
        num_child = cur_node.num_children
        num_total_child_visits = cur_node.kn 
        if num_child < self.num_init: # Generate a set of initial actions
            init_action_id, init_action_prob = cur_node.get_init_info(num_child)
            init_action = Utils.policy.action_id_to_xys(init_action_id)
            expanded_node = cur_node.add_node(init_action[0], init_action[1], init_action[2], init_action_prob)
            self.num_node += 1

            next_env = self.get_env(expanded_node)
            prediction_p, leaf_black_eval = self.predict(next_env) 
            if prediction_p is not None:
                self.prepare_init_actions(expanded_node, prediction_p)
            expanded_node.kr_update(leaf_black_eval)
            is_expanded = True
        else:
            selected_node = cur_node.ucb_select(cur_env.game_state["WhiteToMove"], self.ucb_const)
            selected_action = selected_node.action 

            if num_total_child_visits < self.pw_const * (num_child ** 2): # Progressive Widening
                # select
                selected_env = self.get_env(selected_node)
                leaf_black_eval, is_expanded = self.play_simulation(cur_depth + 1, selected_env,  selected_node)
            if not is_expanded:
                # expand with continuous action sample
                sample_action = selected_node.sample_action(self.num_sample, self.l)
                expanded_node = cur_node.add_node(sample_action[0], sample_action[1], sample_action[2], 0)
                self.num_node += 1

                next_env = self.get_env(expanded_node)
                prediction_p, leaf_black_eval = self.predict(next_env)
                if prediction_p is not None:
                    self.prepare_init_actions(expanded_node, prediction_p)
                expanded_node.kr_update(leaf_black_eval)
                is_expanded = True

        cur_node.kr_update(leaf_black_eval)
        return leaf_black_eval, is_expanded

    def apply_temperature(self, distribution, temperature):
        if temperature < 0.1:
            probabilities = np.zeros(distribution.shape[0])
            probabilities[np.argmax(distribution)] = 1.
        else:
            log_probabilities = np.log(distribution)
            log_probabilities = log_probabilities * (1 / temperature)
            # scale probabilities to a more numerically stable range (in log space)
            log_probabilities = log_probabilities - log_probabilities.max()
            # convert back from log space
            probabilities = np.exp(log_probabilities)
            # re-normalize the distribution
            probabilities = probabilities / probabilities.sum()
        return probabilities 

    def sample_best_action(self):
        kns = np.array([c.kn for c in self.root_node.children])
        kns = kns / sum(kns)
        best_id = np.random.choice(np.arange(len(kns)), p=self.apply_temperature(kns, self.out_temperature))
        return self.root_node.children[best_id].action

    def prepare_init_actions(self, node, prediction_p):
        factor = 1.0
        prev_init_action_prob = 0.0

        for _ in range(self.num_init):
            init_action_id = np.random.choice(np.arange(len(prediction_p)), p=self.apply_temperature(prediction_p, self.init_temperature))
            init_action_prob = prediction_p[init_action_id]

            factor = factor * (1.0 - prev_init_action_prob)
            prev_init_action_prob = init_action_prob

            node.add_init_info(init_action_id, init_action_prob * factor)

            prediction_p[init_action_id] = 0.0
            prediction_p = prediction_p + 1.0E-6
            prediction_p = prediction_p / prediction_p.sum() 

    def predict(self, cur_env):
        if self.root_env.game_state["CurEnd"] != cur_env.game_state["CurEnd"]:
            leaf_dist_v = [0] * Config.value.N_SCORE
            leaf_dist_v[Utils.value.score_to_idx(cur_env.game_state["Score"][self.root_env.game_state["CurEnd"]])] = 1.
            leaf_black_eval = Utils.value.dist_v_to_exp_v(leaf_dist_v)
            return None, leaf_black_eval
        else:
            input_planes = extract_planes(cur_env.game_state)   
            self.pipe.send(input_planes)
            prediction_p, prediction_v = self.pipe.recv()
            if cur_env.game_state["WhiteToMove"]: # change value interms of black team
                prediction_v = Utils.value.flip_dist_v(prediction_v)
            leaf_black_eval = Utils.value.dist_v_to_exp_v(prediction_v)
            return prediction_p, leaf_black_eval

    def node_key(self, cur_node):
        key = ""
        while not cur_node.is_root():
            key = "({0:.4f} {1:.4f} {2})->".format(*cur_node.action) + key 
            cur_node = cur_node.parent
        return key

    def get_env(self, cur_node):
        cur_node_key = self.node_key(cur_node)
        if cur_node_key not in self.env_dict.keys():
            prev_env = self.env_dict[self.node_key(cur_node.parent)]
            cur_env = copy(prev_env)
            cur_env.step_without_rand(*cur_node.action)
            self.env_dict[cur_node_key] = cur_env
        return self.env_dict[cur_node_key]

    def print_stats(self, thinking_time, cur_env, cur_node):
        cur_node.sort_children(cur_env.game_state["WhiteToMove"])
        for child_node in cur_node.children :
            child_action = child_node.action 
            next_env = copy(cur_env)
            next_env.step_without_rand(child_action[0], child_action[1], child_action[2])
            pv = "(%.2f %.2f %d) %s" % (child_action[0], child_action[1], child_action[2], self.get_pv(next_env, child_node))
            print("({0:.2f} {1:.2f} {2}) -> {3:.2f} (V: {4:.2f}) (N: {5:.2f}%) PV: {6}".format(
                child_action[0], child_action[1], child_action[2],
                child_node.kn,
                child_node.get_keval(cur_env.game_state["WhiteToMove"]),
                child_node.prob * 100,
                pv
            ))

        print("{0} visits, {1} nodes, {2} playouts, {3:.0f} n/s".format(
            self.root_node.kn,
            self.num_node,
            self.num_playout,
            self.num_playout / thinking_time
        ))

    def get_pv(self, cur_env, cur_node):
        if not cur_node.has_children():
            return ""

        cur_node.sort_children(cur_env.game_state["WhiteToMove"])
        best_child_node = cur_node.children[0]

        best_action = best_child_node.action 
        res = "(%.2f %.2f %d)" % (best_action[0], best_action[1], best_action[2])

        cur_env.step_without_rand(best_action[0], best_action[1], best_action[2])

        next = self.get_pv(cur_env, best_child_node)
        if next:
            res = "%s %s" % (res, next)
        return res