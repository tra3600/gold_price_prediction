#ifndef GOLDPRICEPREDICTOR_H
#define GOLDPRICEPREDICTOR_H

#include <string>
#include <cstddef>
#include <vector>

// Régression linéaire y = intercept + slope * x (moindres carrés ordinaires),
// avec x = 0, 1, ..., n-1 (indice du mois).
struct LinearModel {
    double slope = 0.0;
    double intercept = 0.0;
    double r2 = 0.0;      // coefficient de détermination
    double stdError = 0.0; // écart-type résiduel
    std::size_t n = 0;
    double xMean = 0.0;
    double sxx = 0.0;

    static LinearModel fit(const std::vector<double>& y);
    double predict(double x) const { return intercept + slope * x; }
    // Demi-largeur de l'intervalle de prédiction à ~95 % au point x.
    double predictionMargin(double x) const;
};

struct Prediction {
    std::string label; // "YYYY-MM"
    double goldEUR;
    double goldEURLow;
    double goldEURHigh;
    double rate;       // USD pour 1 EUR
    double goldUSD;
    double goldUSDLow;
    double goldUSDHigh;
};

class GoldPricePredictor {
public:
    // Données d'exemple (fictives) sur 24 mois.
    GoldPricePredictor();
    // Données personnalisées : doivent avoir la même taille (>= 3 mois).
    GoldPricePredictor(std::vector<double> goldPricesEUR,
                       std::vector<double> exchangeRates);

    // Charge un CSV "mois,or_eur,taux_usd_eur" (en-tête et lignes vides ignorés).
    // Lève std::runtime_error en cas d'erreur.
    static GoldPricePredictor fromCsv(const std::string& path);

    // Prédit les `months` mois suivants, à partir de "YYYY-MM" (par défaut 2025-01).
    std::vector<Prediction> predict(int months = 12,
                                    const std::string& startMonth = "2025-01") const;

    const LinearModel& goldModel() const { return goldModel_; }
    const LinearModel& rateModel() const { return rateModel_; }

    static void print(const std::vector<Prediction>& predictions);
    static void exportCsv(const std::vector<Prediction>& predictions,
                          const std::string& path);

private:
    std::vector<double> goldPricesEUR_;
    std::vector<double> exchangeRates_;
    LinearModel goldModel_;
    LinearModel rateModel_;
};

#endif // GOLDPRICEPREDICTOR_H
