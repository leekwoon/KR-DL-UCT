#include <vector> 
#include "setting.h"

class KDE{
public:
    KDE(){};
    ~KDE(){};
    void add_ob(double x, double y);
    static double kernel(double dx, double dy) {
        return exp(-0.5 * ((pow(dx, 2.0)) / var_x + (pow(dy, 2.)) / var_y));
    }
    double eval(double x, double y);
private:
    std::vector<std::pair<double, double>> m_obs;
};