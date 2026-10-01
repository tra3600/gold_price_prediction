#include <string>
#include "GoldPricePredictor.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <utility>
#include <sstream>
#include <stdexcept>

namespace {

constexpr double kZ95 = 1.96;

void parseMonth(const std::string& s, int& year, int& month) {
    if (std::sscanf(s.c_str(), "%d-%d", &year, &month) != 2 || month < 1 || month > 12)
        throw std::invalid_argument("Mois de départ invalide (attendu YYYY-MM) : " + s);
}

std::string monthLabel(int year, int month) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%04d-%02d", year, month);
    return buf;
}

}  // namespace

LinearModel LinearModel::fit(const std::vector<double>& y) {
    LinearModel m;
    m.n = y.size();
    if (m.n < 3) throw std::invalid_argument("Au moins 3 points sont nécessaires");

    const double n = static_cast<double>(m.n);
    m.xMean = (n - 1) / 2.0;
    double yMean = 0.0;
    for (double v : y) yMean += v;
    yMean /= n;

    double sxy = 0.0, syy = 0.0;
    for (std::size_t i = 0; i < m.n; ++i) {
        const double dx = static_cast<double>(i) - m.xMean;
        const double dy = y[i] - yMean;
        m.sxx += dx * dx;
        sxy += dx * dy;
        syy += dy * dy;
    }
    m.slope = sxy / m.sxx;
    m.intercept = yMean - m.slope * m.xMean;

    double sse = 0.0;
    for (std::size_t i = 0; i < m.n; ++i) {
        const double r = y[i] - m.predict(static_cast<double>(i));
        sse += r * r;
    }
    m.r2 = syy > 0.0 ? 1.0 - sse / syy : 1.0;
    m.stdError = std::sqrt(sse / (n - 2.0));
    return m;
}

double LinearModel::predictionMargin(double x) const {
    const double dx = x - xMean;
    return kZ95 * stdError *
           std::sqrt(1.0 + 1.0 / static_cast<double>(n) + dx * dx / sxx);
}

GoldPricePredictor::GoldPricePredictor()
    : GoldPricePredictor(
          // Données FICTIVES : or en EUR/once sur 24 mois
          {1480, 1502, 1495, 1531, 1548, 1530, 1575, 1590, 1583, 1612, 1640, 1628,
           1655, 1672, 1660, 1698, 1715, 1702, 1741, 1760, 1752, 1788, 1805, 1799},
          // Données FICTIVES : taux USD pour 1 EUR
          {1.085, 1.092, 1.088, 1.097, 1.104, 1.099, 1.108, 1.115, 1.110, 1.121,
           1.127, 1.122, 1.130, 1.136, 1.132, 1.141, 1.147, 1.143, 1.152, 1.158,
           1.153, 1.162, 1.168, 1.164}) {}

GoldPricePredictor::GoldPricePredictor(std::vector<double> goldPricesEUR,
                                       std::vector<double> exchangeRates)
    : goldPricesEUR_(std::move(goldPricesEUR)),
      exchangeRates_(std::move(exchangeRates)) {
    if (goldPricesEUR_.size() != exchangeRates_.size())
        throw std::invalid_argument("Les séries or et taux de change doivent avoir la même taille");
    goldModel_ = LinearModel::fit(goldPricesEUR_);
    rateModel_ = LinearModel::fit(exchangeRates_);
}

GoldPricePredictor GoldPricePredictor::fromCsv(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Impossible d'ouvrir " + path);

    std::vector<double> gold, rates;
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (line.empty() || line == "\r") continue;
        std::stringstream ss(line);
        std::string month, g, r;
        if (!std::getline(ss, month, ',') || !std::getline(ss, g, ',') ||
            !std::getline(ss, r, ','))
            throw std::runtime_error("Ligne " + std::to_string(lineNo) + " invalide : " + line);
        try {
            gold.push_back(std::stod(g));
            rates.push_back(std::stod(r));
        } catch (const std::exception&) {
            if (lineNo == 1) continue;  // en-tête
            throw std::runtime_error("Valeur numérique invalide ligne " + std::to_string(lineNo));
        }
    }
    return GoldPricePredictor(std::move(gold), std::move(rates));
}

std::vector<Prediction> GoldPricePredictor::predict(int months,
                                                    const std::string& startMonth) const {
    if (months < 1) throw std::invalid_argument("Le nombre de mois doit être >= 1");
    int year = 0, month = 0;
    parseMonth(startMonth, year, month);

    std::vector<Prediction> out;
    out.reserve(static_cast<std::size_t>(months));
    const double n = static_cast<double>(goldPricesEUR_.size());

    for (int i = 0; i < months; ++i) {
        const double x = n + i;
        Prediction p;
        p.label = monthLabel(year, month);
        p.goldEUR = goldModel_.predict(x);
        const double gm = goldModel_.predictionMargin(x);
        p.goldEURLow = p.goldEUR - gm;
        p.goldEURHigh = p.goldEUR + gm;

        p.rate = rateModel_.predict(x);
        const double rm = rateModel_.predictionMargin(x);

        p.goldUSD = p.goldEUR * p.rate;
        // Bornes conservatrices : combinaison des bornes basses / hautes.
        p.goldUSDLow = p.goldEURLow * (p.rate - rm);
        p.goldUSDHigh = p.goldEURHigh * (p.rate + rm);
        out.push_back(p);

        if (++month > 12) { month = 1; ++year; }
    }
    return out;
}

void GoldPricePredictor::print(const std::vector<Prediction>& predictions) {
    std::printf("%-8s %-30s %-34s %s\n", "Mois", "Or (EUR) [IC 95 %]",
                "Or (USD) [IC 95 %]", "Taux USD/EUR");
    for (const auto& p : predictions) {
        char eur[64], usd[64];
        std::snprintf(eur, sizeof eur, "%.2f [%.0f - %.0f]", p.goldEUR, p.goldEURLow, p.goldEURHigh);
        std::snprintf(usd, sizeof usd, "%.2f [%.0f - %.0f]", p.goldUSD, p.goldUSDLow, p.goldUSDHigh);
        std::printf("%-8s %-30s %-34s %.4f\n", p.label.c_str(), eur, usd, p.rate);
    }
}

void GoldPricePredictor::exportCsv(const std::vector<Prediction>& predictions,
                                   const std::string& path) {
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Impossible d'écrire " + path);
    out << "mois,or_eur,or_eur_bas,or_eur_haut,taux_usd_eur,or_usd,or_usd_bas,or_usd_haut\n";
    for (const auto& p : predictions)
        out << p.label << ',' << p.goldEUR << ',' << p.goldEURLow << ',' << p.goldEURHigh << ','
            << p.rate << ',' << p.goldUSD << ',' << p.goldUSDLow << ',' << p.goldUSDHigh << '\n';
}
