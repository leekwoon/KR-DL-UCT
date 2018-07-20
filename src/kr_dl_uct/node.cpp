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

#include <limits>
#include <algorithm> // stable_sort
#include <memory> // make_unique
#include <assert.h>

#include "node.h"
#include "utils.h"

Node::Node(double x, double y, double spin, double prob) {
    m_v = 0.0;
    m_visits = 0.0;

    m_move[0] = x;
    m_move[1] = y;
    m_move[2] = spin;
    m_prob = prob;

    m_parent = nullptr;

    m_kde_0 = new KDE();
    m_kde_1 = new KDE();
}

Node::~Node(){
    delete m_kde_0;
    delete m_kde_1;
}

void Node::sort_children(bool is_white) {
    std::stable_sort(rbegin(m_children), rend(m_children), NodeComp(is_white));
}

Node* Node::ucb_select(bool is_white, double ucb_const) {
    Node* best = nullptr;
    double best_value = std::numeric_limits<double>::lowest();

    int num_child = this->get_num_children();
    double num_total_child_visits = 0.0;
    for (const auto& child : m_children) {
        num_total_child_visits += child->get_visits();
    }
    auto numerator = std::log((double)num_total_child_visits);

    for (const auto& child : m_children) {
        double E_v = child->get_eval(is_white);
        double denom = child->get_visits();
        double value = E_v + ucb_const * std::sqrt((numerator / denom));

        if (value > best_value) {
            best_value = value;
            best = child.get();
        }
    }
    assert(best != nullptr);
    return best;
}

Node* Node::add_node(double x, double y, double spin, double prob) {
    Node* added_node;
    m_children.emplace_back(
        std::make_unique<Node>(x, y, spin, prob)
    );
    m_children.reserve(this->get_num_children());
    added_node = m_children.back().get();
    added_node->m_parent = this;
    return added_node;
}

void Node::add_init_info(int id, double prob) {
    m_init_infos.push_back(std::pair<int, double>(id, prob));
}

bool Node::is_root() const {
    if (this->m_parent == nullptr) {
        return true;
    } else {
        return false;
    }
}

bool Node::is_first_update() const {
    if (m_visits == 0) {
        return true;
    } else {
        return false;
    }
}

bool Node::has_children() const {
    if (this->get_num_children() > 0) {
        return true;
    }
    else {
        return false;
    }
}

double* Node::sample_move(int num_sample, double l) {
    static double best_move[action_dim]; 

    double selected_x = this->get_move()[0];
    double selected_y = this->get_move()[1];
    int selected_spin = this->get_move()[2];

    double best_value = std::numeric_limits<double>::max();
    double value;

    for (unsigned int i=0; i < num_sample; i++) {
        double* err_xy = EllipseSampleUtils::uniform_random_point(l * 2 * std_x, l * 2 * std_y);
        double sample_x = selected_x + err_xy[0];
        double sample_y = selected_y + err_xy[1];
        if (selected_spin == 0) {
            value = this->m_kde_0->eval(sample_x, sample_y);
        } else {
            value = this->m_kde_1->eval(sample_x, sample_y);
        }

        if (value < best_value) {
            best_value = value;
            best_move[0] = sample_x;
            best_move[1] = sample_y;
            best_move[2] = selected_spin;
        }
    }
    return best_move;
}

const std::vector<Node::node_ptr_t>& Node::get_children() const {
    return m_children;
}

int Node::get_num_children() const {
    return m_children.size();
}

Node* Node::get_parent() const {
    return m_parent;
}

double* Node::get_move() {
    return m_move;
}

std::pair<int, double> Node::get_init_info(int nth) {
    return m_init_infos[nth];
}

double Node::get_visits() const {
    return m_visits;
}

double Node::get_prob() const {
    return m_prob;
}

double Node::get_eval(bool is_white) {
    double score = m_v / m_visits;
    if (is_white) {
        score = -1 * score;
    }
    return score;
}

// For the sake of efficiency, the kernel density and value estimates are updated incrementally.
void Node::kr_update(double v) {
    bool first_update = is_first_update();
    if (is_root()) {
        this->m_visits += 1;
        this->m_v += v; 
    } else {
        this->m_visits += 1;
        this->m_v += v; 

        double *this_move = this->get_move();

        if (first_update) {
            if (this_move[2] == 0) {
                m_kde_0->add_ob(this_move[0], this_move[1]);
            } else {
                m_kde_1->add_ob(this_move[0], this_move[1]);
            }
        }

        for (const auto& sibling_node : this->get_parent()->get_children()) {
            double *sibling_move = sibling_node->get_move();
            if ((this == sibling_node.get()) || (this_move[2] != sibling_move[2])) { // only consider same spin, sibling node
                continue; 
            }

            double dx = sibling_move[0] - this_move[0];
            double dy = sibling_move[1] - this_move[1];
            double k = KDE::kernel(dx, dy);

            if (first_update) {
                this->m_visits += k;
                this->m_v += k * sibling_node->get_eval(false);

                // add this_move to sibling node
                if (this_move[2] == 0) {
                    sibling_node->m_kde_0->add_ob(this_move[0], this_move[1]);
                } else {
                    sibling_node->m_kde_1->add_ob(this_move[0], this_move[1]);
                }
                // add sibling_move to this node
                if (sibling_move[2] == 0) {
                    this->m_kde_0->add_ob(sibling_move[0], sibling_move[1]);
                } else {
                    this->m_kde_1->add_ob(sibling_move[0], sibling_move[1]);
                }
            }
            sibling_node->m_visits += k;
            sibling_node->m_v += k * v; 
        }
    }
}


