#define _USE_MATH_DEFINES
#include <random>
#include <math.h>
#include <ctime>

namespace EllipseSampleUtils {
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<> dis(0.0, 1.0);
    double rand_zero_to_one() {
        return dis(gen);
    }

    double generate_theta(double r_x, double r_y) {
        double u = rand_zero_to_one() / 4.0;
        double theta = atan(r_y / r_x * tan(2 * M_PI * u));

        double v = rand_zero_to_one();
        if (v < 0.25) {
            return theta;
        } else if (v < 0.5) {
            return M_PI - theta;
        } else if (v < 0.75) {
            return M_PI + theta;
        } else {
            return - theta;
        }
    }

    double radius(double r_x, double r_y, double theta) {
        return r_x * r_y / std::sqrt(pow((r_y * cos(theta)), 2.0) + pow((r_x * sin(theta)), 2.0));
    }

    // generate a random point within an ellipse
    double* uniform_random_point(double r_x, double r_y) {
        double random_theta = generate_theta(r_x, r_y);
        double max_radius = radius(r_x, r_y, random_theta);
        double random_radius = max_radius * std::sqrt(rand_zero_to_one());

        static double random_point[2];
        random_point[0] = random_radius * cos(random_theta);
        random_point[1] = random_radius * sin(random_theta);
        return random_point;
    }
}


