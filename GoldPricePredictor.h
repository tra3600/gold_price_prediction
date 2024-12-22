#ifndef GOLDPRICEPREDICTOR_H
#define GOLDPRICEPREDICTOR_H

#include <vector>

class GoldPricePredictor {
public:
    GoldPricePredictor();
    void predictPrices();
    void printPredictions() const;

private:
    std::vector<double> goldPricesEUR;
    std::vector<double> goldPricesUSD;
    std::vector<double> exchangeRates;
};

#endif // GOLDPRICEPREDICTOR_H