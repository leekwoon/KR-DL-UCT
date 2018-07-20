#include <vector> 
#include <math.h>

#include "kde.h"

#include <iostream>

void KDE::add_ob(double x, double y) {
    m_obs.push_back(std::pair<double, double>(x, y));
}

double KDE::eval(double x, double y) {
    double result = 0;

    for (unsigned int i=0; i < m_obs.size(); i++) {
        std::pair<double, double> ob = m_obs[i];
        double dx = ob.first - x; 
        double dy = ob.second - y;

        result += kernel(dx, dy);
    }
    return result;
}
