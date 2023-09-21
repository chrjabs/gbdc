/**
 * MIT License
 * Copyright (c) 2025 Christoph Jabs
 */

#include "MCNFBaseFeatures.h"

#include <cassert>
#include <algorithm>

MCNF::BaseFeatures1::BaseFeatures1(const char* filename) : filename_(filename) { 
    hard_clause_sizes.fill(0);
    for (unsigned oidx = 0; oidx < N_OBJ_ANALYZED; oidx++) soft_clause_sizes[oidx].fill(0);
    n_soft_clauses.fill(0);
    weight_sum.fill(0);
    weights.fill({});

    initFeatures({ "h_clauses", "variables" });
    initFeatures({ "h_cls1", "h_cls2", "h_cls3", "h_cls4", "h_cls5", "h_cls6", "h_cls7", "h_cls8", "h_cls9", "h_cls10p" });
    initFeatures({ "h_horn", "h_invhorn", "h_positive", "h_negative" });
    initFeatures({ "h_hornvars_mean", "h_hornvars_variance", "h_hornvars_min", "h_hornvars_max", "h_hornvars_entropy" });
    initFeatures({ "h_invhornvars_mean", "h_invhornvars_variance", "h_invhornvars_min", "h_invhornvars_max", "h_invhornvars_entropy" });
    initFeatures({ "h_balancecls_mean", "h_balancecls_variance", "h_balancecls_min", "h_balancecls_max", "h_balancecls_entropy" });
    initFeatures({ "h_balancevars_mean", "h_balancevars_variance", "h_balancevars_min", "h_balancevars_max", "h_balancevars_entropy" });
    initFeatures({ "objectives" });
    std::vector<std::string> s_features = { "clauses", "weight_sum",
        "cls1", "cls2", "cls3", "cls4", "cls5", "cls6", "cls7", "cls8", "cls9", "cls10p",
        "weight_mean", "weight_variance", "weight_min", "weight_max", "weight_entropy" };
    for (unsigned oidx = 0; oidx < N_OBJ_ANALYZED; oidx++) {
        const auto prefix = "s_" + std::to_string(oidx + 1) + "_";
        for (auto& feat : s_features) {
            setFeature(prefix + feat, 0.0);
        }
    }
}

MCNF::BaseFeatures1::~BaseFeatures1() { }

void MCNF::BaseFeatures1::run() {
    StreamBuffer in(filename_);

    Cl clause;
    uint64_t weight = 0; // if weight is 0, parsing hard clause
    int oidx;
    while (in.skipWhitespace()) {
        if (*in == 'c') {
            if (!in.skipLine()) break;
            continue;
        } else if (*in == 'h') {
            weight = 0;
            
            in.skip();
            in.readClause(clause);
        } else {
            assert(*in == 'o');
            in.skip();
            in.readInteger(&oidx);
            if (oidx > n_objs) n_objs = oidx;
            oidx -= 1;
            in.readUInt64(&weight);
            in.readClause(clause);
        }
        
        for (Lit lit : clause) {
            if (lit.var() > n_vars) {
                n_vars = lit.var();
                variable_horn.resize(n_vars + 1);
                variable_inv_horn.resize(n_vars + 1);
                literal_occurrences.resize(2 * n_vars + 2);
            }
        }

        // record statistics
        if (!weight) {
            ++n_hard_clauses;
            
            if (clause.size() < 10) {
                ++hard_clause_sizes[clause.size()];
            } else {
                ++hard_clause_sizes[10];
            }

            unsigned n_neg = 0;
            for (Lit lit : clause) {
                // count negative literals
                if (lit.sign()) ++n_neg;
                ++literal_occurrences[lit];
            }

            // horn statistics
            unsigned n_pos = clause.size() - n_neg;
            if (n_neg <= 1) {
                if (n_neg == 0) ++positive;
                ++horn;
                for (Lit lit : clause) {
                    ++variable_horn[lit.var()];
                }
            }
            if (n_pos <= 1) {
                if (n_pos == 0) ++negative;
                ++inv_horn;
                for (Lit lit : clause) {
                    ++variable_inv_horn[lit.var()];
                }
            }

            // balance of positive and negative literals per clause
            if (clause.size() > 0) {
                balance_clause.push_back((double)std::min(n_pos, n_neg) / (double)std::max(n_pos, n_neg));
            }
        } else if (oidx < N_OBJ_ANALYZED) {
            ++n_soft_clauses[oidx];
            weight_sum[oidx] += weight;

            if (clause.size() < 10) {
                ++soft_clause_sizes[oidx][clause.size()];
            } else {
                ++soft_clause_sizes[oidx][10];
            }

            weights[oidx].push_back(weight);
        }
    }

    // balance of positive and negative literals per variable
    for (unsigned v = 0; v < n_vars; v++) {
        double pos = (double)literal_occurrences[Lit(v, false)];
        double neg = (double)literal_occurrences[Lit(v, true)];
        if (std::max(pos, neg) > 0) {
            balance_variable.push_back(std::min(pos, neg) / std::max(pos, neg));
        }
    }

    load_feature_record();
}

void MCNF::BaseFeatures1::load_feature_record() {

    setFeature("h_clauses", (double)n_hard_clauses);
    setFeature("variables", (double)n_vars);
    std::vector<double> clause_sizes_double(hard_clause_sizes.begin()+1, hard_clause_sizes.end());
    setFeatures({ "h_cls1", "h_cls2", "h_cls3", "h_cls4", "h_cls5", "h_cls6", "h_cls7", "h_cls8", "h_cls9", "h_cls10p" }, clause_sizes_double.begin(), clause_sizes_double.end());
    setFeature("h_horn", (double)horn);
    setFeature("h_invhorn", (double)inv_horn);
    setFeature("h_positive", (double)positive);
    setFeature("h_negative", (double)negative);
    std::vector<double> stats = getDistributionStats(variable_horn);
    setFeatures({ "h_hornvars_mean", "h_hornvars_variance", "h_hornvars_min", "h_hornvars_max", "h_hornvars_entropy" }, stats.begin(), stats.end());
    stats = getDistributionStats(variable_inv_horn);
    setFeatures({ "h_invhornvars_mean", "h_invhornvars_variance", "h_invhornvars_min", "h_invhornvars_max", "h_invhornvars_entropy" }, stats.begin(), stats.end());
    stats = getDistributionStats(balance_clause);
    setFeatures({ "h_balancecls_mean", "h_balancecls_variance", "h_balancecls_min", "h_balancecls_max", "h_balancecls_entropy" }, stats.begin(), stats.end());
    stats = getDistributionStats(balance_variable);
    setFeatures({ "h_balancevars_mean", "h_balancevars_variance", "h_balancevars_min", "h_balancevars_max", "h_balancevars_entropy" }, stats.begin(), stats.end());
    setFeature("objectives", (double)n_objs);
    for (unsigned oidx = 0; oidx < N_OBJ_ANALYZED; oidx++) {
        const auto prefix = "s_" + std::to_string(oidx + 1) + "_";
        setFeature(prefix + "clauses", (double)n_soft_clauses[oidx]);
        setFeature(prefix + "weight_sum", (double)weight_sum[oidx]);
        clause_sizes_double.clear();
        std::copy(soft_clause_sizes[oidx].begin()+1, soft_clause_sizes[oidx].end(), std::back_inserter(clause_sizes_double));
        setFeatures(
             { prefix + "cls1", prefix + "cls2", prefix + "cls3", prefix + "cls4",
               prefix + "cls5", prefix + "cls6", prefix + "cls7", prefix + "cls8",
               prefix + "cls9", prefix + "cls10p" },
             clause_sizes_double.begin(), clause_sizes_double.end());
        stats = getDistributionStats(weights[oidx]);
        setFeatures({ prefix + "weight_mean", prefix + "weight_variance",
                      prefix + "weight_min", prefix + "weight_max",
                      prefix + "weight_entropy" },
                    stats.begin(), stats.end());
    }
}

MCNF::BaseFeatures2::BaseFeatures2(const char* filename) : filename_(filename) { 
    initFeatures({ "h_vcg_cdegree_mean", "h_vcg_cdegree_variance", "h_vcg_cdegree_min", "h_vcg_cdegree_max", "h_vcg_cdegree_entropy" });
    initFeatures({ "h_vcg_vdegree_mean", "h_vcg_vdegree_variance", "h_vcg_vdegree_min", "h_vcg_vdegree_max", "h_vcg_vdegree_entropy" });
    initFeatures({ "h_vg_degree_mean", "h_vg_degree_variance", "h_vg_degree_min", "h_vg_degree_max", "h_vg_degree_entropy" });
    initFeatures({ "h_cg_degree_mean", "h_cg_degree_variance", "h_cg_degree_min", "h_cg_degree_max", "h_cg_degree_entropy" });
}

MCNF::BaseFeatures2::~BaseFeatures2() { }

void MCNF::BaseFeatures2::run() {
    StreamBuffer in(filename_);

    Cl clause;
    bool hard;
    while (in.skipWhitespace()) {
        if (*in == 'c') {
            if (!in.skipLine()) break;
            continue;
        } else if (*in == 'h') {
            in.skip();
            in.readClause(clause);
            hard = true;
        } else {
            assert(*in == 'o');
            in.skip();
            int oidx;
            in.readInteger(&oidx);
            uint64_t weight;
            in.readUInt64(&weight);
            in.readClause(clause);
            hard = false;
            // don't skip soft clause here since we need the true variable count
        }
        
        vcg_cdegree.push_back(clause.size());

        for (Lit lit : clause) {
            // resize vectors if necessary
            if (lit.var() > n_vars) {
                n_vars = lit.var();
                vcg_vdegree.resize(n_vars + 1);
                vg_degree.resize(n_vars + 1);
            }

            // count variable occurrences (only for hard clauses)
            if (hard) {
                ++vcg_vdegree[lit.var()];
                vg_degree[lit.var()] += clause.size();
            }
        }
    }

    // clause graph features
    StreamBuffer in2(filename_);
    while (in2.skipWhitespace()) {
        if (*in2 == 'c' || *in2 == 'p') {
            if (!in2.skipLine()) break;
            continue;
        } else if (*in2 == 'h') {
            in2.skip();
            in2.readClause(clause);
        } else {
            if (!in2.skipLine()) break;
            // skip soft clauses
            continue;
        }

        unsigned degree = 0;
        for (Lit lit : clause) {
            degree += vcg_vdegree[lit.var()];
        }
        clause_degree.push_back(degree);
    }

    load_feature_records();
}

void MCNF::BaseFeatures2::load_feature_records() {
    std::vector<double> stats = getDistributionStats(vcg_cdegree);
    setFeatures({ "h_vcg_cdegree_mean", "h_vcg_cdegree_variance", "h_vcg_cdegree_min", "h_vcg_cdegree_max", "h_vcg_cdegree_entropy" }, stats.begin(), stats.end());
    stats = getDistributionStats(vcg_vdegree);
    setFeatures({ "h_vcg_vdegree_mean", "h_vcg_vdegree_variance", "h_vcg_vdegree_min", "h_vcg_vdegree_max", "h_vcg_vdegree_entropy" }, stats.begin(), stats.end());
    stats = getDistributionStats(vg_degree);
    setFeatures({ "h_vg_degree_mean", "h_vg_degree_variance", "h_vg_degree_min", "h_vg_degree_max", "h_vg_degree_entropy" }, stats.begin(), stats.end());
    stats = getDistributionStats(clause_degree);
    setFeatures({ "h_cg_degree_mean", "h_cg_degree_variance", "h_cg_degree_min", "h_cg_degree_max", "h_cg_degree_entropy" }, stats.begin(), stats.end());
}

MCNF::BaseFeatures::BaseFeatures(const char* filename) : filename_(filename) { 
    BaseFeatures1 baseFeatures1(filename_);
    auto names1 = baseFeatures1.getNames();
    initFeatures(names1.begin(), names1.end());
    BaseFeatures2 baseFeatures2(filename_);
    auto names2 = baseFeatures2.getNames();
    initFeatures(names2.begin(), names2.end());
}

MCNF::BaseFeatures::~BaseFeatures() { }

void MCNF::BaseFeatures::run() {
    extractBaseFeatures1();
    extractBaseFeatures2();
}

void MCNF::BaseFeatures::extractBaseFeatures1() {
    BaseFeatures1 baseFeatures1(filename_);
    baseFeatures1.run();
    for (auto name : baseFeatures1.getNames()) {
        setFeature(name, baseFeatures1.getFeature(name));
    }
}

void MCNF::BaseFeatures::extractBaseFeatures2() {
    BaseFeatures2 baseFeatures2(filename_);
    baseFeatures2.run();
    auto feat = baseFeatures2.getFeatures();
    for (auto name : baseFeatures2.getNames()) {
        setFeature(name, baseFeatures2.getFeature(name));
    }
}
