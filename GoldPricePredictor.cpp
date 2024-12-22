#include "GoldPricePredictor.h"
#include <iostream>

// Constructor initializes the vectors with dummy historical data
GoldPricePredictor::GoldPricePredictor() {
    // Dummy historical data for the gold price in EUR/ounce (simplified)
    goldPricesEUR = {1500, 1520, 1540, 1560, 1580, 1600, 1620, 1640, 1660, 1680, 1700, 1720};
    // Dummy historical data for the exchange rate USD/EUR (simplified)
    exchangeRates = {1.1, 1.105, 1.11, 1.115, 1.12, 1.125, 1.13, 1.135, 1.14, 1.145, 1.15, 1.155};
}

// Simple linear prediction based on historical trends
void GoldPricePredictor::predictPrices() {
    double trendEUR = (goldPricesEUR.back() - goldPricesEUR.front()) / (goldPricesEUR.size() - 1);
    double trendExchangeRate = (exchangeRates.back() - exchangeRates.front()) / (exchangeRates.size() - 1);

    for (int month = 0; month < 12; ++month) {
        double predictedPriceEUR = goldPricesEUR.back() + trendEUR * (month + 1);
        double predictedExchangeRate = exchangeRates.back() + trendExchangeRate * (month + 1);
        double predictedPriceUSD = predictedPriceEUR * predictedExchangeRate;

        goldPricesEUR.push_back(predictedPriceEUR);
        exchangeRates.push_back(predictedExchangeRate);
        goldPricesUSD.push_back(predictedPriceUSD);
    }
}

// Print the predictions
void GoldPricePredictor::printPredictions() const {
    std::cout << "Predictions for 2025:\n";
    std::cout << "Month\tGold Price (EUR)\tGold Price (USD)\tExchange Rate (USD/EUR)\n";
    for (int month = 0; month < 12; ++month) {
        std::cout << month + 1 << "\t"
                  << goldPricesEUR[12 + month] << "\t\t"
                  << goldPricesUSD[month] << "\t\t"
                  << exchangeRates[12 + month] << "\n";
    }
}