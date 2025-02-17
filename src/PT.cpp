#include "PT.h"
#include "util.h"
#include "RNG.h"

#include <cmath>
#include <climits>
#include <random>
#include <iostream>
#include <algorithm>
#include <float.h>


PT::PT(int L, const std::vector<double>& B, unsigned int seed):
eng(seed)
{
    for (auto& b: B) {
        // reps.push_back(new ReplicaType(L, b));
        reps.emplace_back(ReplicaType(L, b));
    }
    for (auto& rep: reps) {
        rep.Init(eng);
    }
}


void PT::Seed(unsigned int seed) {
    eng.seed(seed);
}


std::vector<double> PT::Start(unsigned int n_sweeps) {
    std::vector<unsigned int> count_acc(reps.size()-1, 0.);
    std::vector<double> p(reps.size()-1, 0.);
    h_up = std::vector<unsigned int>(reps.size(), 0);
    h_down = std::vector<unsigned int>(reps.size(), 0);
    long double rt = INFINITY;

    //unsigned int min_sweeps = 10000;
    unsigned int min_sweeps = 1e5;
    unsigned int step_size = 10000;
    unsigned int next_check = min_sweeps + step_size;

    for (unsigned int i = 0; i < n_sweeps; ++i) {
        for (auto& rep: reps) {
            rep.Update(eng);
        }

        reps[0].direction = 1;
        reps[reps.size()-1].direction = -1;

        for (unsigned int j = 0; j < reps.size(); ++j) {
            if (reps[j].direction == 1) h_up[j]++;
            else if (reps[j].direction == -1) h_down[j]++;
        }

        for (unsigned int j = 0; j < reps.size()-1; ++j) {
            double dB = reps[j+1].B - reps[j].B;
            double dE = reps[j+1].cost - reps[j].cost;
            if (dB*dE > 0. || RNG::zero_one_double(eng) < exp(dB*dE)) {
                std::swap(reps[j], reps[j+1]);
                std::swap(reps[j].B, reps[j+1].B);
                ++count_acc[j];
            }
        }

        if (i == next_check) {
            auto temp = p;
            bool stop = true;
            for (unsigned int k = 0; k < p.size(); ++k) {
                p[k] = (long double)count_acc[k]/(i+1);
            }
            for (unsigned int k = 0; k < p.size(); ++k) {
                if ((1./p[k] - 1./temp[k])/(1./p[k]) > 0.0001) stop = false;
            }
            if (stop) {
                return p;
            }
            // long double rt_new = PT::Expected_RT(p);
            // if ((rt-rt_new)/rt_new < 0.00001 || (rt == INFINITY && rt_new == INFINITY)) {
            //     return p;
            // }
            // rt = rt_new;
            step_size = step_size*2;
            next_check = min_sweeps + step_size;
        }
    }

    for (unsigned int k = 0; k < p.size(); ++k) {
        p[k] = (double)count_acc[k]/n_sweeps;
    }
    std::cout << "reached max MC iterations\n";
    return p;
}


long double PT::Expected_RT(const std::vector<double>& p) {
    long double inv_sum = 0.;
    int n = p.size()+1;
    for (auto& i: p) {
        inv_sum += 1./i;
    }
    return n*(n-1)*inv_sum;
}


std::vector<double> PT::GetBetas() {
    std::vector<double> ret;
    for (auto& r: reps) {
        ret.push_back(r.B);
    }
    return ret;
}


void PT::Adjustment(RNGEngine& main_eng) {
    if (reps.size() < 3) return;
    int i = RNG::uniform_int(main_eng)%(reps.size()-2) + 1;
    double l = reps[i-1].B;
    double r = reps[i+1].B;
    double b_new = RNG::zero_one_double(main_eng) * (r-l) + l;
    reps[i].B = b_new;
    return;
}


void PT::Insertion(RNGEngine& main_eng) {
    int i = 1;
    if (reps.size() > 2) i = RNG::uniform_int(main_eng)%(reps.size()-1) + 1;
    double l = reps[i-1].B;
    double r = reps[i].B;
    double b_new = RNG::zero_one_double(main_eng) * (r-l) + l;
    int L = reps[0].L;
    auto r_new = ReplicaType(L, b_new);
    r_new.Init(eng);
    reps.insert(reps.begin()+i, r_new);
    return;
}


void PT::Deletion(RNGEngine& main_eng) {
    if (reps.size() < 3) return;
    int i = RNG::uniform_int(main_eng)%(reps.size()-2) + 1;
    // delete reps[i];
    reps.erase(reps.begin()+i);
    return;
}
    
