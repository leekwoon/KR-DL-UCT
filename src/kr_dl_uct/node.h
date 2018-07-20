// Node-Structure refers the Leela Zero

/*
    This file is part of Leela Zero.
    Copyright (C) 2017 Gian-Carlo Pascutto
    Leela Zero is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    Leela Zero is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with Leela Zero.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <memory> // unique_ptr, make_unique
#include <vector>

#include "kde.h"

class Node {
public:
    using node_ptr_t = std::unique_ptr<Node>;

    explicit Node(double x, double y, double spin, double prob);
    Node() = delete;
    ~Node();

    void sort_children(bool is_white);

    Node* ucb_select(bool is_white, double ucb_const);
    Node* add_node(double x, double y, double spin, double prob);
    void add_init_info(int id, double prob);

    bool is_root() const;
    bool is_first_update() const;
    bool has_children() const;

    double* sample_move(int num_sample, double l);

    const std::vector<node_ptr_t>& get_children() const;
    int get_num_children() const;
    Node* get_parent() const;
    double* get_move();
    std::pair<int, double> get_init_info(int nth);
    double get_visits() const;
    double get_prob() const;
    double get_exp_v(bool is_white);
    double get_eval(bool is_white);
    static double compute_eval(double dist_v[score_dim], bool is_white);

    void kr_update(double v);

private:
    double m_v; 
    double m_visits;

    double m_move[action_dim];
    double m_prob;
    std::vector<node_ptr_t> m_children;
    Node* m_parent;

    KDE* m_kde_0;
    KDE* m_kde_1;

    std::vector<std::pair<int, double>> m_init_infos;
};

class NodeComp : public std::binary_function<Node::node_ptr_t&,
                                             Node::node_ptr_t&, bool> {
public:
    NodeComp(bool is_white){
      m_is_white = is_white;
    } 
    bool operator()(const Node::node_ptr_t& a,
                    const Node::node_ptr_t& b) {
        // if visits are not same, sort on visits
        if (a->get_visits() != b->get_visits()) {
            return a->get_visits() < b->get_visits();
        }
        // both have same non-zero number of visits
        return a->get_eval(m_is_white) < b->get_eval(m_is_white);
    }
private:
    bool m_is_white;
};