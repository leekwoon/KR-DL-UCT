# KR-DL-UCT

This repository provides the source codes for the paper.

[Deep Reinforcement Learning in Continuous Action Spaces: a Case Study in the Game of Simulated Curling]() by Kyowoon Lee*, Sol-A Kim*, Jaesik Choi and Seong-Whan Lee in [ICML-2018](https://icml.cc/Conferences/2018)

## Abstract
Many real world applications of reinforcement learning require an agent to select optimal actions from continuous action spaces. Recently, deep neural networks have successfully been applied to games to with discrete actions spaces. However, such deep neural networks for discrete actions are not directly applicable to find strategies for games where a minute change of an action could alter the outcome of the games dramatically. In this paper, we present a new framework which incorporates a deep neural network for learning game strategy with a kernel-based Monte Carlo tree search  for finding actions from continuous space. Without any hand-crafted feature, we train our network in supervised learning manner and then reinforcement learning settings with a high fidelity simulator of the Olympic sport of curling. Recently, Our framework outperforms existing programs equipped with several hand-crafted features for the game of Curling. Our program trained under our framework won an international digital Curling competition; Game AI Tournaments (GAT-2018@UEC).

If you have any question, please email to the authors:

[Kyowoon Lee] (leekwoon@unist.ac.kr)
[Sol-a Kim] (sol-a@unist.ac.kr)
[Jaesik Choi] (jaesik@unist.ac.kr)  
