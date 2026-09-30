#ifndef GOLDPRICEPREDICTOR_H
#define GOLDPRICEPREDICTOR_H

#include <string>
#include <vector>

// Un point mensuel de l'historique.
struct MonthlyData {
    int year = 0;
    int month = 0;          // 1..12
    double goldEUR = 0.0;   // prix de l'once d'or en EUR
    double usdPerEur = 0.0; // taux de change USD pour 1 EUR
};

// Résultat de prévision d'une série (moyenne + intervalle de confiance 95 %).
struct Forecast {
    std::string model;
    std::vector<double> mean;
    std::vector<double> lower;
    std::vector<double> upper;
};

// Qualité d'un modèle mesurée par backtest (validation sur les derniers mois).
struct BacktestScore {
    double mae = 0.0;
    double rmse = 0.0;
    double mape = 0.0; // en %
};

enum class ModelType { Linear, Holt, Auto };

// Une ligne de prévision mensuelle finale.
struct PredictionRow {
    int year = 0;
    int month = 0;
    double goldEUR = 0.0, goldEURLow = 0.0, goldEURHigh = 0.0;
    double goldUSD = 0.0, goldUSDLow = 0.0, goldUSDHigh = 0.0;
    double usdPerEur = 0.0;
};

// Fonctions de prévision de séries (exposées pour les tests).
Forecast forecastLinear(const std::vector<double>& y, int horizon);
Forecast forecastHolt(const std::vector<double>& y, int horizon);
BacktestScore backtest(const std::vector<double>& y, ModelType model, int holdout);

class GoldPricePredictor {
public:
    // Charge des données d'exemple (fictives) intégrées.
    GoldPricePredictor();

    // Charge un CSV "date,gold_eur,usd_per_eur" (date = YYYY-MM). Lève std::runtime_error.
    void loadCSV(const std::string& path);
    void setData(const std::vector<MonthlyData>& data); // valide et trie

    // Calcule les prévisions pour `horizon` mois après le dernier point connu.
    void predictPrices(int horizon = 12, ModelType model = ModelType::Auto);

    void printPredictions() const;
    void writeCSV(const std::string& path) const;

    const std::vector<PredictionRow>& predictions() const { return predictions_; }
    const std::string& modelUsed() const { return modelUsed_; }
    const BacktestScore& goldScore() const { return goldScore_; }

private:
    std::vector<MonthlyData> history_;
    std::vector<PredictionRow> predictions_;
    std::string modelUsed_;
    BacktestScore goldScore_;
};

#endif // GOLDPRICEPREDICTOR_H
