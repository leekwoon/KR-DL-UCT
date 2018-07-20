"""
Extended from code:
https://github.com/Zeta36/chess-alpha-zero/blob/master/src/chess_zero/agent/api_chess.py
"""
from multiprocessing import connection, Pipe
from threading import Thread

import numpy as np
import time

"""
Defines the process which will listen on the pipe for
an observation of the game state and return a prediction from the network.
""" 
class ThreadPredictor:
    def __init__(self, network): 
        self.network = network
        self.pipes = []

    def start(self):
        prediction_worker = Thread(target=self._predict_batch_worker, name="prediction_worker")
        prediction_worker.daemon = True
        prediction_worker.start()

    def create_pipe(self):
        me, you = Pipe()
        self.pipes.append(me)
        return you

    def _predict_batch_worker(self):
        while True:
            ready = connection.wait(self.pipes,timeout=0.001)
            if not ready:
                continue
            planes, result_pipes = [], []
            for pipe in ready:
                while pipe.poll():
                    planes.append(pipe.recv())
                    result_pipes.append(pipe)

            prediction_p, prediction_v = self.network.predict_p_and_v(planes, is_train=False)
            for pipe, p, v in zip(result_pipes, prediction_p, prediction_v):
                pipe.send((p, v))



