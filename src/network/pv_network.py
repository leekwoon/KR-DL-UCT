import numpy as np
import sys
import os

import tensorflow as tf

from network import features

from config import Config
from utils import Utils 

class NetworkPV(object):
    def __init__(self, model_name, device):
        self.model_name = model_name 
        self.device = device

        self.img_height = Config.network.INPUT_IMAGE_HEIGHT
        self.img_width = Config.network.INPUT_IMAGE_WIDTH
        self.img_channels = sum(f.planes for f in features.DEFAULT_FEATURES)

        # multiply 2 since, curl can be right or left 
        self.num_actions = 2 * Config.network.OUTPUT_IMAGE_HEIGHT * Config.network.OUTPUT_IMAGE_WIDTH 
        self.num_scores = Config.value.N_SCORE

        self.model_dir = self.get_model_dir(["model_name"])
        self.log_dir = self.get_log_dir(["model_name"])

        self.learning_rate = Config.train.LEARNING_RATE

        self.graph = tf.Graph()
        with self.graph.as_default() as g:
            with tf.device(self.device):
                self.create_placeholder()
                self.create_network()
                self.create_train_op()

                vars = tf.global_variables()
                self.saver = tf.train.Saver({var.name: var for var in vars}, max_to_keep=0)
                self.sess = tf.Session(
                        graph=self.graph,
                        config=tf.ConfigProto(
                            allow_soft_placement=True,
                            # log_device_placement=False,
                            gpu_options=tf.GPUOptions(allow_growth=True)
                        ))
                self.sess.run(tf.global_variables_initializer()) 
                self.log_writer = tf.summary.FileWriter(self.log_dir, self.sess.graph)

    def create_placeholder(self):
        self.x = tf.placeholder(
            tf.float32, [None, self.img_height, self.img_width, self.img_channels], name='X')
        self.action_index = tf.placeholder(tf.float32, [None, self.num_actions])
        self.score_index = tf.placeholder(tf.float32, [None, self.num_scores])
        
        self.global_step = tf.Variable(0, trainable=False, name='step')
        self.is_train = tf.placeholder(tf.bool, name='is_train', shape=[])

        self.var_learning_rate = tf.placeholder(tf.float32, name='lr', shape=[])

    def create_network(self):
        with tf.variable_scope('NetworkVP'):
            conv1 = tf.layers.conv2d(self.x, Config.network.NUM_RESIDUAL_FILTERS, [Config.network.FILTER_SIZE, Config.network.FILTER_SIZE], strides=(1, 1), padding='SAME')
            conv1 = tf.nn.relu(tf.contrib.layers.batch_norm(
                                            conv1,
                                            decay=0.9, 
                                            updates_collections=None,
                                            epsilon=1e-5,
                                            scale=True,
                                            center=True,
                                            is_training=self.is_train,
                                            scope='conv1'))
            
        
            # residual block
            current_conv = conv1
            for i in range(Config.network.NUM_RESIDUAL_BLOCKS):
                int_conv = tf.layers.conv2d(current_conv, Config.network.NUM_RESIDUAL_FILTERS, [Config.network.FILTER_SIZE, Config.network.FILTER_SIZE], strides=(1, 1), padding='SAME')
                int_conv = tf.nn.relu(tf.contrib.layers.batch_norm(
                                            int_conv,
                                            decay=0.9, 
                                            updates_collections=None,
                                            epsilon=1e-5,
                                            scale=True,
                                            center=True,
                                            is_training=self.is_train,
                                            scope='int_conv'+ str(i)))
                
                out_conv = tf.layers.conv2d(int_conv, Config.network.NUM_RESIDUAL_FILTERS, [Config.network.FILTER_SIZE, Config.network.FILTER_SIZE], strides=(1, 1), padding='SAME')
                out_conv = tf.contrib.layers.batch_norm(
                                            out_conv,
                                            decay=0.9, 
                                            updates_collections=None,
                                            epsilon=1e-5,
                                            scale=True,
                                            center=True,
                                            is_training=self.is_train,
                                            scope='out_conv'+ str(i))
                # skip connection
                out_conv = tf.nn.relu(current_conv + out_conv)
                current_conv = out_conv

            # policy head
            conv_final_p = tf.layers.conv2d(current_conv, 2, [1, 1], strides=(1, 1), padding='SAME')
            conv_final_p = tf.nn.relu(tf.contrib.layers.batch_norm(
                                conv_final_p,
                                decay=0.9, 
                                updates_collections=None,
                                epsilon=1e-5,
                                scale=True,
                                center=True,
                                is_training=self.is_train,
                                scope='conv_final_p'))

            flat_p = tf.reshape(conv_final_p, [-1, 2 * self.img_height * self.img_width])
            self.logits_p = tf.layers.dense(flat_p, self.num_actions, activation=None, name='logits_p')
            self.softmax_p = tf.nn.softmax(self.logits_p)
            
            # value head
            conv_final_v = tf.layers.conv2d(current_conv, 1, [1, 1], strides=(1, 1), padding='SAME')
            conv_final_v = tf.nn.relu(tf.contrib.layers.batch_norm(
                    conv_final_v,
                    decay=0.9, 
                    updates_collections=None,
                    epsilon=1e-5,
                    scale=True,
                    center=True,
                    is_training=self.is_train,
                    scope='conv_final_v'))
            
            flat_v = tf.reshape(conv_final_v, [-1, self.img_height * self.img_width])
            fc1_v = tf.layers.dense(flat_v, 256, activation=tf.nn.relu, name='fc1_v')
            self.logits_v = tf.layers.dense(fc1_v, Config.value.N_SCORE, activation=None, name='logits_v')
            self.softmax_v = tf.nn.softmax(self.logits_v)

    def create_train_op(self):
        self.opt = tf.train.MomentumOptimizer(
            learning_rate=self.var_learning_rate, momentum=0.9, use_nesterov=True)

        # loss
        self.cost_p = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(logits=self.logits_p, labels=self.action_index))
        self.cost_v = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(logits=self.logits_v, labels=self.score_index))
        # Regularizer
        regularizer = tf.contrib.layers.l2_regularizer(scale=0.0001)
        reg_variables = tf.trainable_variables()
        self.reg_term = tf.contrib.layers.apply_regularization(regularizer, reg_variables)

        self.cost_all = 1.0 * self.cost_p + 1.0 * self.cost_v + self.reg_term

        self.update_ops = tf.get_collection(tf.GraphKeys.UPDATE_OPS)
        with tf.control_dependencies(self.update_ops):
            self.train_op = self.opt.minimize(self.cost_all, global_step=self.global_step)        

    def get_model_dir(self, attr):
        model_dir = os.path.join('./', Config.resource.MODEL_CHECKPOINT_DIR)
        for attr in attr:
            if hasattr(self, attr):
                if attr == "model_name":
                    model_dir += "/%s" % (getattr(self, attr))
                else:
                    model_dir += "/%s=%s" % (attr, getattr(self, attr))
        return model_dir

    def get_log_dir(self, attr):
        model_dir = os.path.join('/', Config.resource.MODEL_LOG_DIR)
        for attr in attr:
            if hasattr(self, attr):
                if attr == "model_name":
                    model_dir += "/%s" % (getattr(self, attr))
                else:
                    model_dir += "/%s=%s" % (attr, getattr(self, attr))
        return model_dir

    def get_base_feed_dict(self):
        return {self.var_learning_rate: self.learning_rate}

    def save(self, directory, step, model_name='checkpoint'):
        print(" [*] Saving checkpoints...")
        save_model_dir = directory
        if not os.path.exists(save_model_dir):
            os.makedirs(save_model_dir)
        self.saver.save(self.sess, os.path.join(save_model_dir, model_name), global_step = step)

    def load(self, load_model_dir):
        print(" [*] Loading checkpoints...")
        ckpt = tf.train.get_checkpoint_state(load_model_dir)
        
        if ckpt and ckpt.model_checkpoint_path:
            ckpt_name = os.path.basename(ckpt.model_checkpoint_path)
            fname = os.path.join(load_model_dir, ckpt_name)
            self.saver.restore(self.sess, fname)
            print(" [*] Load SUCCESS: %s" % fname)
        else:
            print(" [!] Load FAILED: %s" % load_model_dir)

    def predict_p_single(self, x, is_train):
        feed_dict = {self.x: x[None, :], self.is_train: is_train}
        prediction_p = self.sess.run(self.softmax_p,
             feed_dict=feed_dict)
        return prediction_p[0]

    def predict_p(self, x, is_train):
        feed_dict = {self.x: x, self.is_train: is_train}
        prediction_p = self.sess.run(self.softmax_p,
             feed_dict=feed_dict)
        return prediction_p

    def predict_v_single(self, x, is_train):
        feed_dict = {self.x: x[None, :], self.is_train: is_train}
        prediction_v = self.sess.run(self.softmax_v,
             feed_dict=feed_dict)
        return prediction_v[0]

    def predict_v(self, x, is_train):
        feed_dict = {self.x: x, self.is_train: is_train}
        prediction_v = self.sess.run(self.softmax_v,
             feed_dict=feed_dict)
        return prediction_v

    def predict_p_and_v_single(self, x, is_train):
        feed_dict = {self.x: x[None, :], self.is_train: is_train}
        prediction_p, prediction_v = self.sess.run([self.softmax_p, self.softmax_v],
             feed_dict=feed_dict)
        return prediction_p[0], prediction_v[0]

    def predict_p_and_v(self, x, is_train):
        feed_dict = {self.x: x, self.is_train: is_train}
        prediction_p, prediction_v = self.sess.run([self.softmax_p, self.softmax_v],
             feed_dict=feed_dict)
        return prediction_p, prediction_v

    def get_cost_p_and_cost_v(self, x, action_index, score_index, is_train):
        feed_dict = {self.x: x, self.action_index: action_index,
                    self.score_index: score_index, self.is_train: is_train}
        cost_p, cost_v = self.sess.run([self.cost_p, self.cost_v],
             feed_dict=feed_dict)
        return cost_p, cost_v

    def train(self, x, action_index, score_index):
        feed_dict = self.get_base_feed_dict()
        feed_dict.update({
            self.x: x,
            self.action_index: action_index,
            self.score_index: score_index,
            self.is_train: True
        })
        _, cost_p, cost_v, reg_term = \
            self.sess.run([self.train_op, self.cost_p, self.cost_v, self.reg_term],
                     feed_dict=feed_dict)
        return cost_p, cost_v, reg_term

 