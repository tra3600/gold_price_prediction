#include "GoldPricePredictor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {

constexpr double kZ95 = 1.96;

double stddevFromSSE(double sse, int dof) {
    return dof > 0 ? std::sqrt(sse / dof) : 0.0;
}

struct HoltFit {
    double alpha = 0.5, beta = 0.1, level = 0.0, trend = 0.0, sse = 0.0;
    int n = 0;
};

// Lissage exponentiel double (Holt) pour un couple (alpha, beta) donné.
HoltFit runHolt(const std::vector<double>& y, double alpha, double beta) {
    HoltFit f;
    f.alpha = alpha;
    f.beta = beta;
    f.level = y[0];
    f.trend = y[1] - y[0];
    for (size_t t = 1; t < y.size(); ++t) {
        double pred = f.level + f.trend;
        double err = y[t] - pred;
        f.sse += err * err;
        double newLevel = alpha * y[t] + (1 - alpha) * pred;
        f.trend = beta * (newLevel - f.level) + (1 - beta) * f.trend;
        f.level = newLevel;
        ++f.n;
    }
    return f;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

Forecast forecastWith(ModelType m, const std::vector<double>& y, int h) {
    return m == ModelType::Holt ? forecastHolt(y, h) : forecastLinear(y, h);
}

} // namespace

// ---------------------------------------------------------------- modèles

Forecast forecastLinear(const std::vector<double>& y, int horizon) {
    const int n = static_cast<int>(y.size());
    if (n < 3) throw std::invalid_argument("Au moins 3 points sont nécessaires");
    double xbar = (n - 1) / 2.0, ybar = 0;
    for (double v : y) ybar += v;
    ybar /= n;

    double sxx = 0, sxy = 0;
    for (int i = 0; i < n; ++i) {
        sxx += (i - xbar) * (i - xbar);
        sxy += (i - xbar) * (y[i] - ybar);
    }
    double slope = sxy / sxx, intercept = ybar - slope * xbar;

    double sse = 0;
    for (int i = 0; i < n; ++i) {
        double r = y[i] - (intercept + slope * i);
        sse += r * r;
    }
    double s = stddevFromSSE(sse, n - 2);

    Forecast f;
    f.model = "regression lineaire (MCO)";
    for (int h = 1; h <= horizon; ++h) {
        double x = n - 1 + h;
        double m = intercept + slope * x;
        double se = s * std::sqrt(1.0 + 1.0 / n + (x - xbar) * (x - xbar) / sxx);
        f.mean.push_back(m);
        f.lower.push_back(m - kZ95 * se);
        f.upper.push_back(m + kZ95 * se);
    }
    return f;
}

Forecast forecastHolt(const std::vector<double>& y, int horizon) {
    const int n = static_cast<int>(y.size());
    if (n < 3) throw std::invalid_argument("Au moins 3 points sont nécessaires");

    // Recherche en grille des paramètres minimisant l'erreur à un pas.
    HoltFit best;
    best.sse = std::numeric_limits<double>::infinity();
    for (double a = 0.05; a <= 0.951; a += 0.05)
        for (double b = 0.05; b <= 0.951; b += 0.05) {
            HoltFit f = runHolt(y, a, b);
            if (f.sse < best.sse) best = f;
        }
    double s = stddevFromSSE(best.sse, std::max(1, best.n - 2));

    Forecast f;
    f.model = "Holt (lissage exponentiel double)";
    for (int h = 1; h <= horizon; ++h) {
        double m = best.level + h * best.trend;
        // l'incertitude croît avec l'horizon
        double se = s * std::sqrt(static_cast<double>(h));
        f.mean.push_back(m);
        f.lower.push_back(m - kZ95 * se);
        f.upper.push_back(m + kZ95 * se);
    }
    return f;
}

BacktestScore backtest(const std::vector<double>& y, ModelType model, int holdout) {
    const int n = static_cast<int>(y.size());
    if (holdout < 1 || n - holdout < 3)
        throw std::invalid_argument("Historique trop court pour le backtest");
    std::vector<double> train(y.begin(), y.end() - holdout);
    Forecast f = forecastWith(model, train, holdout);

    BacktestScore sc;
    for (int i = 0; i < holdout; ++i) {
        double actual = y[n - holdout + i];
        double err = f.mean[i] - actual;
        sc.mae += std::fabs(err);
        sc.rmse += err * err;
        sc.mape += std::fabs(err / actual) * 100.0;
    }
    sc.mae /= holdout;
    sc.rmse = std::sqrt(sc.rmse / holdout);
    sc.mape /= holdout;
    return sc;
}

// ---------------------------------------------------------------- classe

GoldPricePredictor::GoldPricePredictor() {
    // Données d'exemple FICTIVES (2024) : remplacez-les via --data fichier.csv
    const double gold[] = {1500, 1520, 1540, 1560, 1580, 1600, 1620, 1640, 1660, 1680, 1700, 1720};
    const double fx[] = {1.1, 1.105, 1.11, 1.115, 1.12, 1.125, 1.13, 1.135, 1.14, 1.145, 1.15, 1.155};
    std::vector<MonthlyData> d;
    for (int i = 0; i < 12; ++i) d.push_back({2024, i + 1, gold[i], fx[i]});
    setData(d);
}

void GoldPricePredictor::setData(const std::vector<MonthlyData>& data) {
    if (data.size() < 6)
        throw std::runtime_error("Il faut au moins 6 mois de données historiques");
    std::vector<MonthlyData> d = data;
    std::sort(d.begin(), d.end(), [](const MonthlyData& a, const MonthlyData& b) {
        return a.year != b.year ? a.year < b.year : a.month < b.month;
    });
    for (size_t i = 0; i < d.size(); ++i) {
        if (d[i].month < 1 || d[i].month > 12)
            throw std::runtime_error("Mois invalide dans les données");
        if (d[i].goldEUR <= 0 || d[i].usdPerEur <= 0)
            throw std::runtime_error("Les prix et taux doivent être > 0");
        if (i > 0 && d[i].year == d[i - 1].year && d[i].month == d[i - 1].month)
            throw std::runtime_error("Mois en double dans les données");
    }
    history_ = d;
    predictions_.clear();
}

void GoldPricePredictor::loadCSV(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Impossible d'ouvrir le fichier : " + path);

    std::vector<MonthlyData> d;
    std::string line;
    int lineNo = 0;
    bool firstRow = true;
    while (std::getline(in, line)) {
        ++lineNo;
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string date, g, r;
        if (!std::getline(ss, date, ',') || !std::getline(ss, g, ',') || !std::getline(ss, r, ','))
            throw std::runtime_error("Ligne " + std::to_string(lineNo) + " : 3 colonnes attendues");
        MonthlyData m;
        if (std::sscanf(trim(date).c_str(), "%d-%d", &m.year, &m.month) != 2) {
            if (firstRow) { firstRow = false; continue; } // en-tête
            throw std::runtime_error("Ligne " + std::to_string(lineNo) + " : date invalide (YYYY-MM)");
        }
        firstRow = false;
        try {
            m.goldEUR = std::stod(g);
            m.usdPerEur = std::stod(r);
        } catch (const std::exception&) {
            throw std::runtime_error("Ligne " + std::to_string(lineNo) + " : nombre invalide");
        }
        d.push_back(m);
    }
    setData(d);
}

void GoldPricePredictor::predictPrices(int horizon, ModelType model) {
    if (horizon < 1 || horizon > 120)
        throw std::invalid_argument("L'horizon doit être entre 1 et 120 mois");

    std::vector<double> gold, fx;
    for (const auto& m : history_) {
        gold.push_back(m.goldEUR);
        fx.push_back(m.usdPerEur);
    }

    const int n = static_cast<int>(gold.size());
    int holdout = std::max(1, std::min(6, n / 4));
    bool canBacktest = n - holdout >= 3;

    ModelType chosen = model;
    if (model == ModelType::Auto) {
        chosen = ModelType::Linear;
        if (canBacktest) {
            double eLin = backtest(gold, ModelType::Linear, holdout).rmse;
            double eHolt = backtest(gold, ModelType::Holt, holdout).rmse;
            if (eHolt < eLin) chosen = ModelType::Holt;
        }
    }
    if (canBacktest) goldScore_ = backtest(gold, chosen, holdout);

    Forecast fg = forecastWith(chosen, gold, horizon);
    Forecast ff = forecastWith(chosen, fx, horizon);
    modelUsed_ = fg.model;

    predictions_.clear();
    int year = history_.back().year, month = history_.back().month;
    for (int h = 0; h < horizon; ++h) {
        if (++month > 12) { month = 1; ++year; }
        PredictionRow r;
        r.year = year;
        r.month = month;
        r.usdPerEur = ff.mean[h];
        r.goldEUR = fg.mean[h];
        r.goldEURLow = fg.lower[h];
        r.goldEURHigh = fg.upper[h];
        // prix USD = prix EUR x taux (USD/EUR)
        r.goldUSD = fg.mean[h] * ff.mean[h];
        r.goldUSDLow = fg.lower[h] * ff.mean[h];
        r.goldUSDHigh = fg.upper[h] * ff.mean[h];
        predictions_.push_back(r);
    }
}

void GoldPricePredictor::printPredictions() const {
    if (predictions_.empty()) {
        std::cout << "Aucune prévision : appelez predictPrices() d'abord.\n";
        return;
    }
    std::cout << "Modele : " << modelUsed_ << "\n"
              << "Historique : " << history_.front().year << "-" << std::setw(2) << std::setfill('0')
              << history_.front().month << " a " << history_.back().year << "-" << std::setw(2)
              << history_.back().month << std::setfill(' ') << " (" << history_.size() << " mois)\n";
    if (goldScore_.rmse > 0 || goldScore_.mae > 0)
        std::cout << std::fixed << std::setprecision(2) << "Backtest (or, EUR) : MAE=" << goldScore_.mae
                  << "  RMSE=" << goldScore_.rmse << "  MAPE=" << goldScore_.mape << "%\n";

    std::cout << "\n" << std::left << std::setw(9) << "Mois" << std::right << std::setw(10) << "Or EUR"
              << std::setw(22) << "IC 95% EUR" << std::setw(10) << "Or USD" << std::setw(22)
              << "IC 95% USD" << std::setw(10) << "USD/EUR" << "\n"
              << std::string(83, '-') << "\n";
    std::cout << std::fixed;
    for (const auto& r : predictions_) {
        char ym[16], ie[32], iu[32];
        std::snprintf(ym, sizeof ym, "%04d-%02d", r.year, r.month);
        std::snprintf(ie, sizeof ie, "[%.0f ; %.0f]", r.goldEURLow, r.goldEURHigh);
        std::snprintf(iu, sizeof iu, "[%.0f ; %.0f]", r.goldUSDLow, r.goldUSDHigh);
        std::cout << std::left << std::setw(9) << ym << std::right << std::setprecision(2)
                  << std::setw(10) << r.goldEUR << std::setw(22) << ie << std::setw(10) << r.goldUSD
                  << std::setw(22) << iu << std::setprecision(4) << std::setw(10) << r.usdPerEur << "\n";
    }
    std::cout << "\nAvertissement : projection statistique, pas un conseil financier.\n";
}

void GoldPricePredictor::writeCSV(const std::string& path) const {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Impossible d'écrire le fichier : " + path);
    out << "date,gold_eur,gold_eur_low,gold_eur_high,gold_usd,gold_usd_low,gold_usd_high,usd_per_eur\n";
    out << std::fixed << std::setprecision(4);
    for (const auto& r : predictions_) {
        char ym[16];
        std::snprintf(ym, sizeof ym, "%04d-%02d", r.year, r.month);
        out << ym << ',' << r.goldEUR << ',' << r.goldEURLow << ',' << r.goldEURHigh << ',' << r.goldUSD
            << ',' << r.goldUSDLow << ',' << r.goldUSDHigh << ',' << r.usdPerEur << "\n";
    }
}
